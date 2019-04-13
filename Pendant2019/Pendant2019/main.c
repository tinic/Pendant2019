#include <atmel_start.h>

#define LEDS_RAILS 4
#define LEDS_TOTAL 17
#define LEDS_COMPONENTS 3
#define LEDS_WS2812_LEAD_TIME 50

static uint8_t led_data[LEDS_TOTAL*LEDS_COMPONENTS*LEDS_RAILS];

static void led_set_color(uint8_t rail, uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
	led_data[rail*LEDS_TOTAL*LEDS_COMPONENTS+index*3+0] = r;
	led_data[rail*LEDS_TOTAL*LEDS_COMPONENTS+index*3+1] = g;
	led_data[rail*LEDS_TOTAL*LEDS_COMPONENTS+index*3+2] = b;
}

static void update_leds()
{
	static const uint8_t conv_lookup[16][4] = {
		{   0b11111111,	0b00000000, 0b00000000, 0b00000000 }, // 0b0000
		{   0b11111111,	0b00010001, 0b00010000, 0b00000000 }, // 0b0001
		{   0b11111111,	0b00100010, 0b00100000, 0b00000000 }, // 0b0010
		{   0b11111111,	0b00110011, 0b00110000, 0b00000000 }, // 0b0011
		{   0b11111111,	0b01000100, 0b01000000, 0b00000000 }, // 0b0100
		{   0b11111111,	0b01010101, 0b01010000, 0b00000000 }, // 0b0101
		{   0b11111111,	0b01101100, 0b01100000, 0b00000000 }, // 0b0110
		{   0b11111111,	0b01110111, 0b01110000, 0b00000000 }, // 0b0111
		{   0b11111111,	0b10001000, 0b10000000, 0b00000000 }, // 0b1000
		{   0b11111111,	0b10011001, 0b10010000, 0b00000000 }, // 0b1001
		{   0b11111111,	0b10101010, 0b10100000, 0b00000000 }, // 0b1010
		{   0b11111111,	0b10111011, 0b10110000, 0b00000000 }, // 0b1011
		{   0b11111111,	0b11001100, 0b11000000, 0b00000000 }, // 0b1100
		{   0b11111111,	0b11011101, 0b11010000, 0b00000000 }, // 0b1101
		{   0b11111111,	0b11101110, 0b11100000, 0b00000000 }, // 0b1110
		{   0b11111111,	0b11111111, 0b11110000, 0b00000000 }, // 0b1111
	};

	struct _qspi_command cmd = {
		.inst_frame.bits.width				= QSPI_INST4_ADDR4_DATA4,
		.inst_frame.bits.inst_en			= 0,
		.inst_frame.bits.addr_en			= 0,
		.inst_frame.bits.opt_en				= 0,
		.inst_frame.bits.data_en			= 1,

		.inst_frame.bits.opt_len			= 0,
		.inst_frame.bits.addr_len			= 0,
		.inst_frame.bits.continues_read		= 0,
		.inst_frame.bits.ddr_enable			= 0,
		.inst_frame.bits.dummy_cycles		= 0,

		.inst_frame.bits.tfr_type			= QSPI_WRITE_ACCESS,
		.buf_len							= 0,
		.tx_buf								= 0,
	};
	
	// Need to do this manually as HAL will set data lines to high when done
	hri_qspi_write_INSTRFRAME_reg(QSPI, cmd.inst_frame.word);
	uint8_t *qspi_mem = (uint8_t *)QSPI_AHB;
	for (size_t c = 0; c<LEDS_WS2812_LEAD_TIME*LEDS_RAILS; c++) {
		*qspi_mem++ = 0;
	}
	for (int32_t c = 0; c < LEDS_TOTAL*LEDS_COMPONENTS; c++) {
		uint8_t p0 = led_data[LEDS_TOTAL*LEDS_COMPONENTS*0+c];
		uint8_t p1 = led_data[LEDS_TOTAL*LEDS_COMPONENTS*1+c];
		uint8_t p2 = led_data[LEDS_TOTAL*LEDS_COMPONENTS*2+c];
		uint8_t p3 = led_data[LEDS_TOTAL*LEDS_COMPONENTS*3+c];
		for(int32_t d = 7; d >=0; d--) {
			const uint8_t *src = conv_lookup[
			((p0&(1<<d))?0x8:0x0)|
			((p1&(1<<d))?0x4:0x0)|
			((p2&(1<<d))?0x2:0x0)|
			((p3&(1<<d))?0x1:0x0)];
			*qspi_mem++ = *src++;
			*qspi_mem++ = *src++;
			*qspi_mem++ = *src++;
			*qspi_mem++ = *src++;
		}
	}
	for (size_t c = 0; c<LEDS_WS2812_LEAD_TIME*LEDS_RAILS; c++) {
		*qspi_mem++ = 0;
	}
	__DSB();
	__ISB();
}

static void TIMER_0_update_leds_cb(const struct timer_task *const timer_task)
{
	static uint32_t e = 0;
	e++;
	for (uint32_t c=0; c<17; c++) {
		uint32_t d = ( c + e ) % 17;
		led_set_color(0,c,d*2+2,d*2+2,d*2+2);
	}
	for (uint32_t c=0; c<17; c++) {
		uint32_t d = ( c + e ) % 17;
		led_set_color(1,c,d*2+2,d*2+2,d*2+2);
	}
	for (uint32_t c=0; c<17; c++) {
		uint32_t d = ( c + e ) % 17;
		led_set_color(2,c,d*2+2,d*2+2,d*2+2);
	}
	for (uint32_t c=0; c<17; c++) {
		uint32_t d = ( c + e ) % 17;
		led_set_color(3,c,d*2+2,d*2+2,d*2+2);
	}
	//memset(led_data, 0x0, sizeof(led_data));
	update_leds();
}

static void bq24295_int(void)
{
	for(volatile int32_t c=0; c<1024; c++) {}
}

static void sw2_upper_pressed(void)
{
	for(volatile int32_t c=0; c<1024; c++) {}
}

static void sw2_lower_pressed(void)
{
	for(volatile int32_t c=0; c<1024; c++) {}
}

static void sw1_lower_pressed(void)
{
	for(volatile int32_t c=0; c<1024; c++) {}
}

static void sx1280_busy(void)
{
	for(volatile int32_t c=0; c<1024; c++) {}
}

static void sx1280_dio1(void)
{
	for(volatile int32_t c=0; c<1024; c++) {}
}

static void init_pendant()
{
	qspi_sync_enable(&QUAD_SPI_0);

	// Enable LEDs
	gpio_set_pin_level(ENABLE_E, true);
	gpio_set_pin_level(ENABLE_O, true);

	static struct timer_task TIMER_0_update_leds = {
		.interval	= 16,
		.cb			= TIMER_0_update_leds_cb,
		.mode		= TIMER_TASK_REPEAT
	};

	timer_add_task(&TIMER_0, &TIMER_0_update_leds);
	timer_start(&TIMER_0);

	ext_irq_register(PIN_PA16, bq24295_int);
	ext_irq_register(PIN_PA17, sw2_upper_pressed);
	ext_irq_register(PIN_PA18, sw2_lower_pressed);
	ext_irq_register(PIN_PA19, sw1_lower_pressed);
	ext_irq_register(PIN_PB08, sx1280_busy);
	ext_irq_register(PIN_PB09, sx1280_dio1);
}

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	
	init_pendant();
	
	/* Replace with your application code */
	while (1) {
		__WFI();
	}
}
