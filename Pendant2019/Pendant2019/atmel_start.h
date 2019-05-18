#ifndef ATMEL_START_H_INCLUDED
#define ATMEL_START_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define register
#include "driver_init.h"
#include "usb_start.h"
#undef register

/**
 * Initializes MCU, drivers and middleware in the project
 **/
void atmel_start_init(void);

#ifdef __cplusplus
}
#endif
#endif
