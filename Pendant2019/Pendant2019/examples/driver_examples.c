/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_examples.h"
#include "driver_init.h"
#include "utils.h"

#if 0
/**
 * Example of using DIGITAL_GLUE_LOGIC_0.
 */
void DIGITAL_GLUE_LOGIC_0_example(void)
{
	custom_logic_enable();
	/* Customer logic now works. */
}

static void button_on_PA16_pressed(void)
{
}

static void button_on_PA17_pressed(void)
{
}

static void button_on_PA18_pressed(void)
{
}

static void button_on_PA19_pressed(void)
{
}

static void button_on_PB08_pressed(void)
{
}

static void button_on_PB09_pressed(void)
{
}

/**
 * Example of using EXTERNAL_IRQ_0
 */
void EXTERNAL_IRQ_0_example(void)
{

	ext_irq_register(PIN_PA16, button_on_PA16_pressed);
	ext_irq_register(PIN_PA17, button_on_PA17_pressed);
	ext_irq_register(PIN_PA18, button_on_PA18_pressed);
	ext_irq_register(PIN_PA19, button_on_PA19_pressed);
	ext_irq_register(PIN_PB08, button_on_PB08_pressed);
	ext_irq_register(PIN_PB09, button_on_PB09_pressed);
}

COMPILER_ALIGNED(128)
struct sha_context context;
COMPILER_PACK_RESET();

/**
 * Example of using HASH_ALGORITHM_0 to generate SHA digest message.
 */
void HASH_ALGORITHM_0_example(void)
{
	const uint8_t sha1_digest[]  = {0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E,
                                   0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D};
	uint8_t       sha_output[20] = {0x00};
	int32_t       i;

	sha_sync_enable(&HASH_ALGORITHM_0);
	sha_sync_sha1_compute(&HASH_ALGORITHM_0, &context, (const uint8_t *)"abc", 3, sha_output);
	for (i = 0; i < 20; i++) {
		while (sha_output[i] != sha1_digest[i])
			;
	}
}

static uint8_t src_data[512];
static uint8_t chk_data[512];
/**
 * Example of using FLASH_0 to read and write Flash main array.
 */
void FLASH_0_example(void)
{
	uint32_t page_size;
	uint16_t i;

	/* Init source data */
	page_size = flash_get_page_size(&FLASH_0);

	for (i = 0; i < page_size; i++) {
		src_data[i] = i;
	}

	/* Write data to flash */
	flash_write(&FLASH_0, 0x3200, src_data, page_size);

	/* Read data from flash */
	flash_read(&FLASH_0, 0x3200, chk_data, page_size);
}

/**
 * Example of using QUAD_SPI_0 to get N25Q256A status value,
 * and check bit 0 which indicate embedded operation is busy or not.
 */
void QUAD_SPI_0_example(void)
{
	uint8_t              status = 0xFF;
	struct _qspi_command cmd    = {
        .inst_frame.bits.inst_en  = 1,
        .inst_frame.bits.data_en  = 1,
        .inst_frame.bits.tfr_type = QSPI_READ_ACCESS,
        .instruction              = 0x05,
        .buf_len                  = 1,
        .rx_buf                   = &status,
    };

	qspi_sync_enable(&QUAD_SPI_0);
	while (status & (1 << 0)) {
		qspi_sync_serial_run_command(&QUAD_SPI_0, &cmd);
	}
	qspi_sync_deinit(&QUAD_SPI_0);
}

static struct timer_task TIMER_0_task1, TIMER_0_task2;
/**
 * Example of using TIMER_0.
 */
static void TIMER_0_task1_cb(const struct timer_task *const timer_task)
{
}

static void TIMER_0_task2_cb(const struct timer_task *const timer_task)
{
}

void TIMER_0_example(void)
{
	TIMER_0_task1.interval = 100;
	TIMER_0_task1.cb       = TIMER_0_task1_cb;
	TIMER_0_task1.mode     = TIMER_TASK_REPEAT;
	TIMER_0_task2.interval = 200;
	TIMER_0_task2.cb       = TIMER_0_task2_cb;
	TIMER_0_task2.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&TIMER_0, &TIMER_0_task1);
	timer_add_task(&TIMER_0, &TIMER_0_task2);
	timer_start(&TIMER_0);
}

/**
 * Example of using SPI_0 to write "Hello World" using the IO abstraction.
 */
static uint8_t example_SPI_0[12] = "Hello World!";

void SPI_0_example(void)
{
	struct io_descriptor *io;
	spi_m_sync_get_io_descriptor(&SPI_0, &io);

	spi_m_sync_enable(&SPI_0);
	io_write(io, example_SPI_0, 12);
}

/**
 * Example of using USART_0 to write "Hello World" using the IO abstraction.
 */
void USART_0_example(void)
{
	struct io_descriptor *io;
	usart_sync_get_io_descriptor(&USART_0, &io);
	usart_sync_enable(&USART_0);

	io_write(io, (uint8_t *)"Hello World!", 12);
}

void I2C_0_example(void)
{
	struct io_descriptor *I2C_0_io;

	i2c_m_sync_get_io_descriptor(&I2C_0, &I2C_0_io);
	i2c_m_sync_enable(&I2C_0);
	i2c_m_sync_set_slaveaddr(&I2C_0, 0x12, I2C_M_SEVEN);
	io_write(I2C_0_io, (uint8_t *)"Hello World!", 12);
}

void delay_example(void)
{
	delay_ms(5000);
}

/**
 * Example of using RAND_0 to generate waveform.
 */
void RAND_0_example(void)
{
	uint32_t random_n[4];
	rand_sync_enable(&RAND_0);
	random_n[0] = rand_sync_read32(&RAND_0);
	random_n[1] = rand_sync_read32(&RAND_0);
	rand_sync_read_buf32(&RAND_0, &random_n[2], 2);
	if (random_n[0] == random_n[1]) {
		/* halt */
		while (1)
			;
	}
	if (random_n[2] == random_n[3]) {
		/* halt */
		while (1)
			;
	}
}

/**
 * Example of using WDT_0.
 */
void WDT_0_example(void)
{
	uint32_t clk_rate;
	uint16_t timeout_period;

	clk_rate       = 1000;
	timeout_period = 4096;
	wdt_set_timeout_period(&WDT_0, clk_rate, timeout_period);
	wdt_enable(&WDT_0);
}

#endif  // #if 0
