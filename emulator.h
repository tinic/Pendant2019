#ifndef EMULATOR_H_
#define EMULATOR_H_

#ifdef EMULATOR

#include <stdint.h>

#define I2C_M_SEVEN 0x0800

#define PIN_PA16 16
#define PIN_PA17 17
#define PIN_PA18 18
#define PIN_PA19 19
#define PIN_PB08 40
#define PIN_PB09 41

enum gpio_port { GPIO_PORTA, GPIO_PORTB, GPIO_PORTC, GPIO_PORTD, GPIO_PORTE };

#define GPIO(port, pin) ((((port)&0x7u) << 5) + ((pin)&0x1Fu))

#ifdef __cplusplus
#include <mutex>
extern std::recursive_mutex g_print_mutex;
#endif  // #ifdef __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

bool gpio_get_pin_level(const uint8_t pin);
void gpio_set_pin_level(const uint8_t pin, const bool level);

struct io_descriptor;

struct _i2c_m_msg {
	uint16_t          addr;
	volatile uint16_t flags;
	int32_t           len;
	uint8_t *         buffer;
};

struct _i2c_m_service {
	struct _i2c_m_msg msg;
	uint16_t          mode;
	uint16_t          trise;
};

struct _i2c_m_sync_device {
	struct _i2c_m_service service;
	void *                hw;
};

typedef int32_t (*io_write_t)(struct io_descriptor *const io_descr, const uint8_t *const buf, const uint16_t length);
typedef int32_t (*io_read_t)(struct io_descriptor *const io_descr, uint8_t *const buf, const uint16_t length);

struct io_descriptor {
	io_write_t write; /*! The write function pointer. */
	io_read_t  read;  /*! The read function pointer. */
};

struct i2c_m_sync_desc {
	struct _i2c_m_sync_device device;
	struct io_descriptor      io;
	uint16_t                  slave_addr;
};

extern struct i2c_m_sync_desc I2C_0;

int32_t i2c_m_sync_enable(struct i2c_m_sync_desc *i2c);

int32_t i2c_m_sync_get_io_descriptor(struct i2c_m_sync_desc *const i2c, struct io_descriptor **io);
int32_t i2c_m_sync_set_slaveaddr(struct i2c_m_sync_desc *i2c, int16_t addr, int32_t addr_len);


int32_t io_write(struct io_descriptor *const io_descr, const uint8_t *const buf, const uint16_t length);
int32_t io_read(struct io_descriptor *const io_descr, uint8_t *const buf, const uint16_t length);

void __disable_irq(void);
void __enable_irq(void);

void delay_ms(int32_t ms);

enum timer_task_mode { 
	TIMER_TASK_ONE_SHOT, 
	TIMER_TASK_REPEAT 
};

struct timer_task;

typedef void (*timer_cb_t)(const struct timer_task *const timer_task);

struct timer_task {
	double					time_dummy;
	double					time_label; /*! Absolute timer start time. */
	uint32_t				interval;   /*! Number of timer ticks before calling the task. */
	timer_cb_t				cb;         /*! Function pointer to the task. */
	enum timer_task_mode	mode;       /*! Task mode: one shot or repeat. */
};

struct timer_descriptor {
	uint32_t dummy;
};

extern struct timer_descriptor TIMER_0;

int32_t timer_add_task(struct timer_descriptor *const descr, struct timer_task *const task);
int32_t timer_start(struct timer_descriptor *const descr);
int32_t timer_remove_task(struct timer_descriptor *const descr, const struct timer_task *const task);
int32_t timer_stop(struct timer_descriptor *const descr);

typedef void (*ext_irq_cb_t)(void);

int32_t ext_irq_register(const uint32_t pin, ext_irq_cb_t cb);

struct _qspi_sync_dev {
	void *prvt;
};

struct qspi_sync_descriptor {
	struct _qspi_sync_dev dev;
};

extern struct qspi_sync_descriptor QUAD_SPI_0;

int32_t qspi_sync_enable(struct qspi_sync_descriptor *qspi);

struct spi_xfer {
	/** Pointer to data buffer to TX */
	uint8_t *txbuf;
	/** Pointer to data buffer to RX */
	uint8_t *rxbuf;
	/** Size of data characters to TX & RX */
	uint32_t size;
};

struct _spi_sync_dev {
	/** Pointer to the hardware base or private data for special device. */
	void *prvt;
	/** Data size, number of bytes for each character */
	uint8_t char_size;
	/** Dummy byte used in master mode when reading the slave */
	uint16_t dummy_byte;
};

struct spi_m_sync_descriptor {
	void *func;
	/** SPI device instance */
	struct _spi_sync_dev dev;
	/** I/O read/write */
	struct io_descriptor io;
	/** Flags for HAL driver */
	uint16_t flags;
};

extern struct spi_m_sync_descriptor SPI_0;

void spi_m_sync_enable(struct spi_m_sync_descriptor *spi);

int32_t spi_m_sync_transfer(struct spi_m_sync_descriptor *spi, const struct spi_xfer *xfer);

struct _qspi_command {
	union {
		struct {
			/* Width of QSPI Addr , inst data */
			uint32_t width : 3;
			/* Reserved */
			uint32_t reserved0 : 1;
			/* Enable Instruction */
			uint32_t inst_en : 1;
			/* Enable Address */
			uint32_t addr_en : 1;
			/* Enable Option */
			uint32_t opt_en : 1;
			/* Enable Data */
			uint32_t data_en : 1;
			/* Option Length */
			uint32_t opt_len : 2;
			/* Address Length */
			uint32_t addr_len : 1;
			/* Option Length */
			uint32_t reserved1 : 1;
			/* Transfer type */
			uint32_t tfr_type : 2;
			/* Continuous read mode */
			uint32_t continues_read : 1;
			/* Enable Double Data Rate */
			uint32_t ddr_enable : 1;
			/* Dummy Cycles Length */
			uint32_t dummy_cycles : 5;
			/* Reserved */
			uint32_t reserved3 : 11;
		} bits;
		uint32_t word;
	} inst_frame;

	uint8_t  instruction;
	uint8_t  option;
	uint32_t address;

	size_t      buf_len;
	const void *tx_buf;
	void *      rx_buf;
};

typedef void (*flash_cb_t)(struct flash_descriptor *const descr);

struct _irq_descriptor {
	void (*handler)(void *parameter);
	void *parameter;
};

struct _flash_device;

struct _flash_callback {
	/** Ready to accept new command handler */
	void (*ready_cb)(struct _flash_device *device);
	/** Error handler */
	void (*error_cb)(struct _flash_device *device);
};

struct _flash_device {
	struct _flash_callback flash_cb; /*!< Interrupt handers  */
	struct _irq_descriptor irq;      /*!< Interrupt descriptor */
	void *                 hw;       /*!< Hardware module instance handler */
};

struct flash_callbacks {
	/** Callback invoked when ready to accept a new command */
	flash_cb_t cb_ready;
	/** Callback invoked when error occurs */
	flash_cb_t cb_error;
};

struct flash_descriptor {
	/** Pointer to FLASH device instance */
	struct _flash_device dev;
	/** Callbacks for asynchronous transfer */
	struct flash_callbacks callbacks;
};

extern struct flash_descriptor FLASH_0;

int32_t flash_write(struct flash_descriptor *flash, uint32_t dst_addr, uint8_t *buffer, uint32_t length);
int32_t flash_read(struct flash_descriptor *flash, uint32_t src_addr, uint8_t *buffer, uint32_t length);

uint32_t flash_get_page_size(struct flash_descriptor *flash);
uint32_t flash_get_total_pages(struct flash_descriptor *flash);

void system_init(void);

void display_debug_area(int32_t area);

#ifdef __cplusplus
};
#endif

#endif  // #ifdef EMULATOR

#endif  // #ifndef EMULATOR_H_
