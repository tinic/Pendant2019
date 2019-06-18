#ifndef EMULATOR_H_
#define EMULATOR_H_

#ifdef EMULATOR

#include <stdint.h>


#ifdef __cplusplus
#include <mutex>
extern std::recursive_mutex g_print_mutex;
#endif  // #ifdef __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

void delay_ms(int32_t ms);

struct flash_descriptor {
	uint32_t dummy;
};

extern struct flash_descriptor FLASH_0;

int32_t flash_write(struct flash_descriptor *flash, uint32_t dst_addr, uint8_t *buffer, uint32_t length);
int32_t flash_read(struct flash_descriptor *flash, uint32_t src_addr, uint8_t *buffer, uint32_t length);

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

void system_init(void);

#ifdef __cplusplus
};
#endif

#endif  // #ifdef EMULATOR

#endif  // #ifndef EMULATOR_H_
