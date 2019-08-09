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
#include "./commands.h"
#include "./model.h"
#include "./sdd1306.h"
#include "./sx1280.h"

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
#ifdef EMULATOR
    static struct termios tty_opts_backup, tty_opts_raw;
    // Back up current TTY settings
    tcgetattr(STDIN_FILENO, &tty_opts_backup);

    // Change TTY settings to raw mode
    cfmakeraw(&tty_opts_raw);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty_opts_raw);

    printf("\x1b[2J\x1b[?25l");
#endif  // #ifdef EMULATOR

    /* Initializes MCU, drivers and middleware */
    atmel_start_init();

    /* Enable I2C bus */
    i2c_m_sync_enable(&I2C_0);

    Commands::instance().Boot();

    Commands::instance().StartTimers();

#ifndef EMULATOR
    while (1) {
        __WFI();
    }
#else  // #ifndef EMULATOR
    while (1) {
        int key = getc(stdin);
        switch (key) {
            case    0x03:
                    printf("\x1b[2J\x1b[?25h\x1b[0;0f\n");
                    tcsetattr(STDIN_FILENO, TCSANOW, &tty_opts_backup);
                    exit(0);
                    break;
            case    0x31:
                    Commands::instance().Switch1_Pressed();
                    break;
            case    0x32:
                    Commands::instance().Switch2_Pressed();
                    break;
            case    0x33:
                    Commands::instance().Switch3_Pressed();
                    break;
            case    0x34:
            		Commands::instance().SendV3Message("DRINK MALORT", "DUCKLING", colors::rgb8(0xFF, 0x80, 0x00));
            		break;
        }
    }
#endif  // #ifndef EMULATOR
}
