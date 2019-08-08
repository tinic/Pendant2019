/*
Copyright 2019 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "./emulator.h"

#include "./atmel_start_pins.h"
#include "./system_time.h"
#include "./sx1280.h"

#ifdef EMULATOR

#include <list>
#include <vector>
#include <thread>
#include <algorithm>
#include <memory.h>

std::recursive_mutex g_print_mutex;

struct flash_descriptor FLASH_0;

static uint8_t flash_memory[512] = { 0 };

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"

#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

static uint8_t bq25895_regs[0x13] = {
    0b00001000, // 0x00
    0b00000110, // 0x01
    0b00111111, // 0x02
    0b00111010, // 0x03
    0b00100000, // 0x04
    0b00010011, // 0x05
    0b01011110, // 0x06
    0b10011101, // 0x07
    0b00000011, // 0x08
    0b01000100, // 0x09
    0b10010011, // 0x0A
    0b00000100, // 0x0B
    0b00000000, // 0x0C
    0b00010010, // 0x0D
    0b01010111, // 0x0E
    0b00000000, // 0x0F
    0b00000000, // 0x10
    0b00000000, // 0x11
    0b00000000  // 0x12
};

struct spi_byte {
    uint8_t tx;
    uint8_t rx;
    uint32_t seq;
};

static uint32_t spiSeqIdx = 0;
static uint32_t spiSeqCmd = 0;
static uint32_t spiSeqID = 0;
static std::vector<spi_byte> spiBuf;

void display_debug_area(int32_t area) {
    std::lock_guard<std::recursive_mutex> lock(g_print_mutex);

    const int32_t sy = 35;

    switch (area) {
        case    0: {
            const int32_t y_len = 32;
            const int32_t x_len =  8;
            while (spiBuf.size() < y_len*x_len) {   
                spiBuf.push_back({0,0,0});
            }
            size_t idx = spiBuf.size() - y_len*x_len;
            printf("\x1b[%d;%df SPI", sy, 1);
            for (int32_t y = 0; y < y_len; y++) {
                printf("\x1b[%d;%df", y+sy+1, 1);
                for (int32_t x = 0; x < x_len; x++) {
                    int32_t i = static_cast<int32_t>(spiBuf[idx+static_cast<size_t>(y*x_len+x)].seq & 1);
                    if (i) {
                        printf("\x1b[41m");
                    } else {
                        printf("\x1b[42m");
                    }
                    printf("%02x ", spiBuf[idx+static_cast<size_t>(y*x_len+x)].tx);
                }
                printf("\x1b[0m  ");
                for (int32_t x = 0; x < x_len; x++) {
                    int32_t i = static_cast<int32_t>(spiBuf[idx+static_cast<size_t>(y*x_len+x)].seq & 1);
                    if (i) {
                        printf("\x1b[41m");
                    } else {
                        printf("\x1b[42m");
                    }
                    char c = static_cast<char>(spiBuf[idx+static_cast<size_t>(y*x_len+x)].tx);
                    if ( c < 0x20 || c > 0x7F ) {
                        c = '.';
                    }
                    printf("%c", c);
                }
                printf("\x1b[0m  ");
                for (int32_t x = 0; x < x_len; x++) {
                    int32_t i = static_cast<int32_t>(spiBuf[idx+static_cast<size_t>(y*x_len+x)].seq & 1);
                    if (i) {
                        printf("\x1b[41m");
                    } else {
                        printf("\x1b[42m");
                    }
                    printf("%02x ", spiBuf[idx+static_cast<size_t>(y*x_len+x)].rx);
                }
                printf("\x1b[0m  ");
                for (int32_t x = 0; x < x_len; x++) {
                    int32_t i = spiBuf[idx+static_cast<size_t>(y*x_len+x)].seq & 1;
                    if (i) {
                        printf("\x1b[41m");
                    } else {
                        printf("\x1b[42m");
                    }
                    char c = static_cast<char>(spiBuf[idx+static_cast<size_t>(y*x_len+x)].rx);
                    if ( c < 0x20 || c > 0x7F ) {
                        c = '.';
                    }
                    printf("%c", c);
                }
                printf("\x1b[0m");
            }
        } break;
        case    1: {
            printf("\x1b[%d;%df FLASH", sy, 1);
            for (int32_t y=0; y<32; y++) {
                printf("\x1b[%d;%df",sy+1+y,0);
                for (int32_t x=0; x<16; x++) {
                    printf("%02x ", flash_memory[y*16+x]);
                }
            }

            for (int32_t y=0; y<32; y++) {
                printf("\x1b[%d;%df",sy+1+y,49);
                for (int32_t x=0; x<16; x++) {
                    char c = static_cast<char>(flash_memory[y*16+x]);
                    if ( c < 0x20 || c > 0x7F ) {
                        c = '.';
                    }
                    printf("%c", c);
                }
            }
        } break;
        case    2: {
            printf("\x1b[%d;%df I2C BQ25895", sy, 1);
            for (int32_t c=0; c<0x13; c++) {
                printf("\x1b[%d;%df%02x 0b",sy+1+c,0,c);
                printf(BYTE_TO_BINARY_PATTERN,BYTE_TO_BINARY(bq25895_regs[c]));
            }
        } break;
    }
}


int32_t flash_write(struct flash_descriptor *, uint32_t, uint8_t *buffer, uint32_t length) {
    std::lock_guard<std::recursive_mutex> lock(g_print_mutex);
    memcpy(flash_memory, buffer, std::min(uint32_t(512),length));
    return 0;
}

int32_t flash_read(struct flash_descriptor *, uint32_t, uint8_t *buffer, uint32_t length) {
    memcpy(buffer, flash_memory, std::min(uint32_t(512),length));
    return 0;
}

bool gpio_get_pin_level(const uint8_t) {
    return 0;
}

void gpio_set_pin_level(const uint8_t pin, const bool level) {
    switch(pin) {   
        case    SX1280_SSEL:
                if (!level) {
                    spiSeqID ++;
                    spiSeqIdx = 0;
                }
                break;
    }
}

struct i2c_m_sync_desc I2C_0;

static int32_t i2c_addr = 0;

static int32_t bq25895_read_reg = 0;

int32_t i2c_m_sync_enable(struct i2c_m_sync_desc *) {
    return 0;
}

int32_t i2c_m_sync_get_io_descriptor(struct i2c_m_sync_desc * const , struct io_descriptor **) {
    return 1;
}

int32_t i2c_m_sync_set_slaveaddr(struct i2c_m_sync_desc *, int16_t addr, int32_t) {
    i2c_addr = addr;
    return 1;
}

int32_t io_write(struct io_descriptor * const, const uint8_t *const buf, const uint16_t length) {
    switch (i2c_addr) {
        case 0x6A: {
            if (length == 1) {
                bq25895_read_reg = buf[0];
                return 1;
            } else if (length == 2) {
                bq25895_regs[buf[0]] = buf[1];
                bq25895_read_reg = -1;
                return 2;
            } else {
                bq25895_read_reg = -1;
            }
        } break;
        case 0x3C: {
        }
    }
    return length;
}

int32_t io_read(struct io_descriptor * const, uint8_t *const buf, const uint16_t length) {
    switch (i2c_addr) {
        case 0x6A: {
            if (bq25895_read_reg >= 0 && length == 1) {
                buf[0] = bq25895_regs[bq25895_read_reg];
                bq25895_read_reg = -1;
                return 1;
            }
            bq25895_read_reg = -1;
        } break;
        case 0x3C: {
        }
    }
    return length;
}

void __disable_irq(void) {
}

void __enable_irq(void) {
}

void delay_ms(int32_t) {
}

struct timer_descriptor TIMER_0;

std::list<std::reference_wrapper<struct timer_task>> timer_tasks;

int32_t timer_add_task(struct timer_descriptor * const, struct timer_task *const task) {
    timer_tasks.push_back(*task);
    return 0;
}

int32_t timer_start(struct timer_descriptor * const) {
    std::thread t([=]() {
        for (;;) {
            double now = system_time();
            for (auto it = timer_tasks.cbegin(); it != timer_tasks.cend(); it++) {
                if (uint32_t((now - (*it).get().time_label) * 1000.0) >= (*it).get().interval) { 
                    (*it).get().time_label = now;
                    (*it).get().cb(&(*it).get());
                }
            }
        }
    });
    t.detach();
    return 0;
}

int32_t timer_stop(struct timer_descriptor * const) {
    return 0;
}

int32_t timer_remove_task(struct timer_descriptor * const, const struct timer_task * const) {
    return 0;
}

int32_t ext_irq_register(const uint32_t, ext_irq_cb_t) {
    return 0;
}

struct qspi_sync_descriptor QUAD_SPI_0;

int32_t qspi_sync_enable(struct qspi_sync_descriptor *) {
    return 0;
}

struct spi_m_sync_descriptor SPI_0;

void spi_m_sync_enable(struct spi_m_sync_descriptor *) {
}


int32_t spi_m_sync_transfer(struct spi_m_sync_descriptor *, const struct spi_xfer *xfer) {
    for (size_t c = 0; c < xfer->size; c++) {
        if (spiSeqIdx == 0) {
            spiSeqCmd = xfer->txbuf[c];
        }
        xfer->rxbuf[c] = 0x00;
        switch(spiSeqCmd) {
            case SX1280::RADIO_READ_REGISTER: {
                static uint8_t read_reg[5];
                read_reg[spiSeqIdx] = xfer->txbuf[c];
                if (spiSeqIdx == 4) {
                    switch((read_reg[1]<<8)|(read_reg[2]<<0)) {
                        case SX1280::REG_LR_FIRMWARE_VERSION_MSB: {
                            xfer->rxbuf[c] = 0xa9;
                        } break;
                        case SX1280::REG_LR_FIRMWARE_VERSION_MSB+1: {
                            xfer->rxbuf[c] = 0xb5;
                        } break;
                    }
                }
            } break;
        }
        spiBuf.push_back({xfer->txbuf[c],xfer->rxbuf[c],spiSeqID});
        spiSeqIdx++;
    }
    return 0;
}

uint32_t flash_get_page_size(struct flash_descriptor *) {
    return 512;
}

uint32_t flash_get_total_pages(struct flash_descriptor *) {
    return 1;
}

void system_init(void) {
}

#endif  // #ifdef EMULATOR
