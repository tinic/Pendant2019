#include "./emulator.h"

#include "./system_time.h"

#ifdef EMULATOR

#include <list>
#include <thread>
#include <algorithm>

std::recursive_mutex g_print_mutex;

struct flash_descriptor FLASH_0;

static uint8_t flash_memory[512] = { 0 };

int32_t flash_write(struct flash_descriptor *flash, uint32_t dst_addr, uint8_t *buffer, uint32_t length) {
    std::lock_guard<std::recursive_mutex> lock(g_print_mutex);

	for (int32_t y=0; y<32; y++) {
		printf("\x1b[%d;%df",20+1+y,0);
		for (int32_t x=0; x<16; x++) {
			printf("%02x ", buffer[y*16+x]);
		}
	}

	for (int32_t y=0; y<32; y++) {
		printf("\x1b[%d;%df",20+1+y,49);
		for (int32_t x=0; x<16; x++) {
			char c = buffer[y*16+x];
			if ( c < 0x20 || c > 0x7F ) {
				c = '.';
			}
			printf("%c", c);
		}
	}

	memcpy(flash_memory, buffer, std::min(uint32_t(512),length));
	return 0;
}

int32_t flash_read(struct flash_descriptor *flash, uint32_t src_addr, uint8_t *buffer, uint32_t length) {
	memcpy(buffer, flash_memory, std::min(uint32_t(512),length));
	return 0;
}

void delay_ms(int32_t ms) {
}

struct timer_descriptor TIMER_0;

std::list<std::reference_wrapper<struct timer_task>> timer_tasks;

int32_t timer_add_task(struct timer_descriptor *const descr, struct timer_task *const task) {
	timer_tasks.push_back(*task);
	return 0;
}

int32_t timer_start(struct timer_descriptor *const descr) {
    std::thread t([=]() {
    	for (;;) {
    		double now = system_time();
			for (auto it = timer_tasks.cbegin(); it != timer_tasks.cend(); it++) {
	    		if (int((now - (*it).get().time_label) * 1000.f) >= (*it).get().interval) {	
	    			(*it).get().time_label = now;
	    			(*it).get().cb(&(*it).get());
	    		}
			}
	    }
    });
    t.detach();
	return 0;
}

int32_t timer_stop(struct timer_descriptor *const descr) {
	return 0;
}

int32_t timer_remove_task(struct timer_descriptor *const descr, const struct timer_task *const task) {
	return 0;
}

void system_init(void) {
}

#endif  // #ifdef EMULATOR
