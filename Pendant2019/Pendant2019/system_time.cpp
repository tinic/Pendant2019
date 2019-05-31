
#include <atmel_start.h>

#include "system_time.h"

// Generate system time based on 32-bit cycle count
static uint64_t large_dwt_cyccnt() {
	volatile uint32_t *DWT_CYCCNT  = (volatile uint32_t *)0xE0001004;
	volatile uint32_t *DWT_CONTROL = (volatile uint32_t *)0xE0001000;
	volatile uint32_t *SCB_DEMCR   = (volatile uint32_t *)0xE000EDFC;

	static uint32_t PREV_DWT_CYCCNT = 0;
	static uint64_t LARGE_DWT_CYCCNT = 0;

	static bool init = false;
	if (!init) {

		init = true;
		*SCB_DEMCR   = *SCB_DEMCR | 0x01000000;
		*DWT_CYCCNT  = 0; // reset the counter
		*DWT_CONTROL = 0; 
	    *DWT_CONTROL = *DWT_CONTROL | 1 ; // enable the counter

	}

	uint32_t CURRENT_DWT_CYCCNT = *DWT_CYCCNT;

	// wrap around
	if (PREV_DWT_CYCCNT > CURRENT_DWT_CYCCNT) {
		LARGE_DWT_CYCCNT += 0x100000000UL;
	}

	PREV_DWT_CYCCNT = CURRENT_DWT_CYCCNT;

	return LARGE_DWT_CYCCNT + CURRENT_DWT_CYCCNT;
}

double system_time() {
	return double(large_dwt_cyccnt() / 65536) * (1.0f / ( 60000000.0 / 65536.0 ) );
}
