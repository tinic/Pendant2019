#include <atmel_start.h>

#include "commands.h"
#include "model.h"
#include "sdd1306.h"

extern "C" {

void WDT_Handler() {
	while (1) { }
}

}

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();

	/* Enable I2C bus */
	i2c_m_sync_enable(&I2C_0);

	Commands::instance().Boot();

	Commands::instance().StartTimers();

	while (1) {
		__WFI();
	}
}
