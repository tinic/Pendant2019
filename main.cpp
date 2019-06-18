#include "./commands.h"
#include "./model.h"
#include "./sdd1306.h"

#include <atmel_start.h>

#ifdef EMULATOR
#include <unistd.h> 
#include <stdlib.h>
#include <termios.h>
#endif  // #ifdef EMULATOR

extern "C" {

void WDT_Handler() {
	while (1) { }
}

}

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();

#ifndef EMULATOR
	/* Enable I2C bus */
	i2c_m_sync_enable(&I2C_0);
#endif  // #ifndef EMULATOR

	Commands::instance().Boot();

	Commands::instance().StartTimers();

#ifndef EMULATOR
	while (1) {
		__WFI();
	}
#else  // #ifndef EMULATOR
	static struct termios tty_opts_backup, tty_opts_raw;
    // Back up current TTY settings
    tcgetattr(STDIN_FILENO, &tty_opts_backup);

    // Change TTY settings to raw mode
    cfmakeraw(&tty_opts_raw);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty_opts_raw);

	printf("\x1b[2J\x1b[?25l");
	while (1) {
		int key = getc(stdin);
		switch (key) {
			case	0x03:
					printf("\x1b[2J\x1b[?25h\n");
					tcsetattr(STDIN_FILENO, TCSANOW, &tty_opts_backup);
					exit(0);
					break;
			case	0x31:
					Commands::Switch1_Pressed_C();
					break;
			case	0x32:
					Commands::Switch2_Pressed_C();
					break;
			case	0x33:
					Commands::Switch3_Pressed_C();
					break;
		}
	}
#endif  // #ifndef EMULATOR
}
