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
#include "./sdd1306.h"
#include "./emulator.h"

#include <atmel_start.h>

#include <cstring>
#include <algorithm>

#include "font/duck_font.h"

static const uint8_t rev_bits[] =
{
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

static SDD1306 sdd1306;

SDD1306::SDD1306() {
    memset(text_buffer_cache, 0, sizeof(text_buffer_cache));
    memset(text_buffer_screen, 0, sizeof(text_buffer_screen));
    memset(text_attr_cache, 0, sizeof(text_attr_cache));
    memset(text_attr_screen, 0, sizeof(text_attr_screen));
    memset(scroll_message, 0, sizeof(scroll_message));
}
    
SDD1306 &SDD1306::instance() {
    i2c_m_sync_get_io_descriptor(&I2C_0, &sdd1306.I2C_0_io);

    if (!sdd1306.initialized) {
        sdd1306.initialized = true;
        sdd1306.Init();
    }

    return sdd1306;
}


void SDD1306::Clear() {
    memset(text_buffer_cache, 0, sizeof(text_buffer_cache));
    memset(text_buffer_screen, 0, sizeof(text_buffer_screen));
    memset(text_attr_cache, 0, sizeof(text_attr_cache));
    memset(text_attr_screen, 0, sizeof(text_attr_screen));
    center_flip_screen = 0;
    center_flip_cache = 0;
}
    
void SDD1306::ClearAttr() {
    memset(text_attr_cache, 0, sizeof(text_attr_cache));
}
    
void SDD1306::DisplayBootScreen() {
    for (uint32_t c=0; c<12*2; c++) {
        text_buffer_cache[c] = static_cast<uint16_t>(0x80 + c);
    }
}
    
void SDD1306::SetCenterFlip(int8_t progression) {
    center_flip_cache = progression;
}

void SDD1306::PlaceUTF8String(uint32_t x, uint32_t y, const char *s) {
    if (y>1 || x>11) {
    	return;
    }
	
	const uint8_t *str = reinterpret_cast<const uint8_t *>(s);

	static uint8_t trailingBytesForUTF8[256] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
	};

	static const uint32_t offsetsFromUTF8[6] = {
		0x00000000UL, 0x00003080UL, 0x000E2080UL,
		0x03C82080UL, 0xFA082080UL, 0x82082080UL
	};
	
    for (uint32_t c=0;;c++) {
    	if (*str == 0) {
    		return;
    	}
    	if ((x + c) >= 12) {
    		return;
    	}
    	uint8_t nb  = trailingBytesForUTF8[static_cast<uint8_t>(*str)];
        uint32_t ch = 0;
        switch (nb) {
        case 5: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 4: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 3: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 2: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 1: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 0: ch += static_cast<uint32_t>(*str++);
        // fall through
        }
        ch -= offsetsFromUTF8[nb];
        if ((ch < 0x20) || (ch > 0xFFFF)) {
            text_buffer_cache[y*12+x+c] = 0;
        } else {
            text_buffer_cache[y*12+x+c] = static_cast<uint16_t>(ch) - 0x20;
        }
    }
}

void SDD1306::Invert() {
    for (uint32_t c=0; c<12*2; c++) {
        text_attr_cache[c] ^= 1;
    }
}
    
void SDD1306::SetAttr(uint32_t x, uint32_t y, uint8_t attr) {
    if (y>1 || x>11) return;
    text_attr_cache[y*12+x] = attr;
}

void SDD1306::SetBootScreen(bool on, int32_t xpos) {
	display_boot_screen = on;
	boot_screen_offset = xpos;
}
    
void SDD1306::SetAsciiScrollMessage(const char *str, int32_t offset) {
    if (str) {
        size_t len = std::min(strlen(str), sizeof(scroll_message));
        memset(scroll_message, 0, sizeof(scroll_message));
        for (size_t c=0; c<len; c++) {
            uint8_t ch = static_cast<uint8_t>(str[c]);
            if ((ch < 0x20)) {
                scroll_message[c] = 0;
            } else {
                scroll_message[c] = ch - 0x20;
            }
        }
        display_scroll_message = true;
        scroll_message_offset = offset;
        scroll_message_len = static_cast<uint16_t>(strlen(str));
    } else {
        memset(text_buffer_screen, 0, sizeof(text_buffer_screen));
        memset(text_attr_screen, 0, sizeof(text_attr_screen));
        display_scroll_message = false;
    }
}

#ifdef EMULATOR
void SDD1306::DrawBuffer(uint8_t *buf, int32_t x, int32_t y, int32_t len) {
	std::lock_guard<std::recursive_mutex> lock(g_print_mutex);
	for (int32_t py = 0; py < 8; py ++) {
		int32_t real_y  = y * 8 + static_cast<uint32_t>(py) - vertical_shift;
		if (real_y >= 0 && real_y < 16) {
			printf("\x1b[%d;%df",18 + real_y,2+x);
			for (int32_t px = 0; px < (len-1); px ++) {
				if (((buf[1+px] >> py) & 1) != 0) {
					printf("\x1b[30;47m \x1b[0m");
				} else {
					printf(" ");
				}
			}
		}
	}
}
#endif  // #ifdef EMULATOR

void SDD1306::Display() {

#ifdef EMULATOR
    std::lock_guard<std::recursive_mutex> lock(g_print_mutex);
    printf("\x1b[17;1f┌────────────────────────────────────────────────────────────────────────────────────────────────┐");
    for (int32_t y=0; y<16; y++) {
        printf("\x1b[%d;%df│",18+y,1);
        printf("\x1b[%d;%df│",18+y,98);
    }
    printf("\x1b[34;1f└────────────────────────────────────────────────────────────────────────────────────────────────┘");
    if (vertical_shift < 0) {
	    for (int32_t y=0; y<-vertical_shift; y++) {
			printf("\x1b[%d;1f│                                                                                                │", 18+y);
		}
    }
    if (vertical_shift > 0) {
	    for (int32_t y=16-vertical_shift; y<16; y++) {
			printf("\x1b[%d;1f│                                                                                                │", 18+y);
		}
    }
#endif  // #ifdef EMULATOR

    bool display_center_flip = false;
    if (center_flip_cache || center_flip_screen) {
        center_flip_screen = center_flip_cache;
        display_center_flip = true;
    }
    for (uint32_t y=0; y<2; y++) {
        if (display_boot_screen) {

            WriteCommand(0xB0+0);
			WriteCommand(0x00);
			WriteCommand(0x10);

            uint8_t buf[97];
            buf[0] = 0x40;
                
            // write first line
            for (int32_t x = 0; x < 96; x++) {
                int32_t rx = (boot_screen_offset + x ) % (27 * 8) ;
                int32_t cx = rx >> 3;
                buf[x+1] = duck_font_raw[0x0EA8 + cx * 8 + (rx & 0x07)];
            }

            io_write(I2C_0_io, buf, 0x61);

#ifdef EMULATOR
			DrawBuffer(buf, 0, 0, 0x61);
#endif  // #ifdef EMULATOR

            WriteCommand(0xB0+1);
			WriteCommand(0x00);
			WriteCommand(0x10);

            // write second line
            for (int32_t x = 0; x < 96; x++) {
                int32_t rx = (boot_screen_offset + x ) % (27 * 8) ;
                int32_t cx = rx >> 3;
                buf[x+1] = duck_font_raw[0x16A8 + cx * 8 + (rx & 0x07)];
            }
                
            io_write(I2C_0_io, buf, 0x61);

#ifdef EMULATOR
			DrawBuffer(buf, 0, 1, 0x61);
#endif  // #ifdef EMULATOR
        
        } else if (display_scroll_message) {
                
            WriteCommand(0xB0+0);
			WriteCommand(0x00);
			WriteCommand(0x10);

            uint8_t buf[97];
            buf[0] = 0x40;
                
            // write first line
            for (int32_t x = 0; x < 96; x++) {
                int32_t rx = (scroll_message_offset + x ) % (scroll_message_len * 16) ;
                if (rx >= 0) {
					int32_t cx = rx >> 4;
					buf[x+1] = duck_font_raw[0x1000 + scroll_message[cx] * 16 + (rx & 0x0F)];
				} else {
					buf[x+1] = 0;
				}
            }

            io_write(I2C_0_io, buf, 0x61);

#ifdef EMULATOR
			DrawBuffer(buf, 0, 0, 0x61);
#endif  // #ifdef EMULATOR

            WriteCommand(0xB0+1);
			WriteCommand(0x00);
			WriteCommand(0x10);

            // write second line
            for (int32_t x = 0; x < 96; x++) {
                int32_t rx = (scroll_message_offset + x ) % (scroll_message_len * 16) ;
                int32_t cx = rx >> 4;
                if (rx >= 0) {
					buf[x+1] = duck_font_raw[0x1800 + scroll_message[cx] * 16 + (rx & 0x0F)];
				} else {
					buf[x+1] = 0;
				}
            }

            io_write(I2C_0_io, buf, 0x61);

#ifdef EMULATOR
			DrawBuffer(buf, 0, 1, 0x61);
#endif  // #ifdef EMULATOR
                
        } else {
            for (uint32_t x=0; x<12; x++) {
                if (text_buffer_cache[y*12+x] != text_buffer_screen[y*12+x] ||
                    text_attr_cache[y*12+x] != text_attr_screen[y*12+x]) {
                    text_buffer_screen[y*12+x] = text_buffer_cache[y*12+x];
                    text_attr_screen[y*12+x] = text_attr_cache[y*12+x];
                    if (!display_center_flip) {
                        DisplayChar(x,y,text_buffer_screen[y*12+x],text_attr_screen[y*12+x]);
                    }
                }
            }
        }
    }
    if (display_center_flip) {
        DisplayCenterFlip();
    }

}
    
void SDD1306::SetVerticalShift(int8_t val) {
	vertical_shift = static_cast<int32_t>(val);
    WriteCommand(0xD3);
    if (val < 0) {
        val = 64+val;
        WriteCommand(val&0x3F);
    } else {
        WriteCommand(val&0x3F);
    }
}

void SDD1306::DisplayOn() {
    WriteCommand(0xAF);
}

void SDD1306::DisplayOff() {
    WriteCommand(0xAE);
}

void SDD1306::Init() {
    // Reset OLED screen
    gpio_set_pin_level(OLED_RESET, true);
    delay_ms(1);
    gpio_set_pin_level(OLED_RESET, false);
    delay_ms(10);
    gpio_set_pin_level(OLED_RESET, true);
    delay_ms(20);

    i2c_m_sync_set_slaveaddr(&I2C_0, i2caddr, I2C_M_SEVEN);

    uint8_t status = 0x8;
    uint8_t value = 0x0;

    if ((io_write(I2C_0_io, &status, 1)) == 1 &&
        (io_read(I2C_0_io, &value, 1)) == 1) {
        devicePresent = true;
    } else {
        return;
    }

    static uint8_t startup_sequence[] = {
        0xAE,           // Display off
        0xD5, 0x80,     // Set Display Clock Divide Ratio
        0xA8, 0x0F,     // Set Multiplex Ratio
        0xD3, 0x00,     // Set Display Offset
        0x8D, 0x14,     // Enable Charge Pump
        0x40,           // Set Display RAM start
        0xA6,           // Set to normal display (0xA7 == inverse)
        0xA4,           // Force Display From RAM On
        0xA1,           // Set Segment Re-map
        0xC8,           // Set COM Output Scan Direction (flipped)
        0xDA, 0x02,     // Set Pins configuration
        0x81, 0x80,     // Set Contrast (0x00-0xFF)
        0xD9, 0x10,     // Set Pre-Charge period
        0xDB, 0x40,     // Adjust Vcomm regulator output
        0xAF            // Display on
    };

    for (size_t c = 0; c < sizeof(startup_sequence); c++) {
        WriteCommand(startup_sequence[c]);
    }
}

void SDD1306::DisplayCenterFlip() {
    uint8_t buf[0x61];
    buf[0] = 0x40;
    for (uint32_t y=0; y<2; y++) {
        WriteCommand(static_cast<uint8_t>(0xB0+y));
        WriteCommand(0x00);
        WriteCommand(0x10);
        for (uint32_t x = 0; x < 96; x++) {
            if (center_flip_screen == 48) {
                buf[x+1] = 0x00;
            } else {
                int32_t rx = ( ( ( static_cast<int32_t>(x) - 48 ) * 48 ) / static_cast<int32_t>(48 - center_flip_screen) ) + 48;
                if (rx < 0 || rx > 95) {
                    buf[x+1] = 0x00;
                } else {
                    uint8_t a = text_attr_screen[y*12+static_cast<uint32_t>(rx/8)];
                    uint8_t r = (a & 4) ? (7-(rx&7)) : (rx&7);
                    uint8_t v = duck_font_raw[text_buffer_screen[y*12+static_cast<uint32_t>(rx/8)]*8+r];
                    if (a & 1) {
                        v = ~v;
                    }
                    if (a & 2) {
                        v = rev_bits[v];
                    }
                    buf[x+1] = v;
                }
            }
        }

        io_write(I2C_0_io, buf, 0x61);

#ifdef EMULATOR
		DrawBuffer(buf, 0, y, 0x61);
#endif  // #ifdef EMULATOR
    }
}
    
void SDD1306::DisplayChar(uint32_t x, uint32_t y, uint16_t ch, uint8_t attr) {

    x = x * 8;

    WriteCommand(static_cast<uint8_t>(0xB0 + y));
    WriteCommand(static_cast<uint8_t>(0x0f&(x   )));
    WriteCommand(static_cast<uint8_t>(0x10|(x>>4)));
        
    uint8_t buf[9];
    buf[0] = 0x40;
    if ((attr & 4)) {
        if ((attr & 1)) {
            if ((attr & 2)) {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] = ~rev_bits[duck_font_raw[ch*8+7-c]];
                }
            } else {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] = ~duck_font_raw[ch*8+7-c];
                }
            }
        } else {
            if ((attr & 2)) {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] =  rev_bits[duck_font_raw[ch*8+7-c]];
                }
                } else {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] =  duck_font_raw[ch*8+7-c];
                }
            }
        }
    } else {
        if ((attr & 1)) {
            if ((attr & 2)) {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] = ~rev_bits[duck_font_raw[ch*8+c]];
                }
                } else {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] = ~duck_font_raw[ch*8+c];
                }
            }
        } else {
            if ((attr & 2)) {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] =  rev_bits[duck_font_raw[ch*8+c]];
                }
                } else {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] =  duck_font_raw[ch*8+c];
                }
            }
        }
    }

    io_write(I2C_0_io, buf, 9);

#ifdef EMULATOR
	DrawBuffer(buf, x, y, 0x09);
#endif  // #ifdef EMULATOR
}

void SDD1306::WriteCommand(uint8_t cmd_val) const {
    uint8_t cmd[2];
    cmd[0] = 0;
    cmd[1] = cmd_val;
    i2c_m_sync_set_slaveaddr(&I2C_0, i2caddr, I2C_M_SEVEN);
    io_write(I2C_0_io, cmd, sizeof(cmd));
}
