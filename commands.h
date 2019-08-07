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
#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <cstdint>
#include <atmel_start.h>

#include "./emulator.h"
#include "./leds.h"

class Commands {
public:
    Commands();

    static Commands &instance();

    void Boot();

    void StartTimers();
    void StopTimers();

    void SendV2Message(const char *name, const char *message, uint8_t color = 0);
    void SendV3Message(const char *name, const char *message, colors::rgb8 color = colors::rgb8());
    void SendDateTimeRequest();

private:
    friend int main();

    void OnLEDTimer();
    void OnOLEDTimer();
    void OnADCTimer();

    void Switch1_Pressed();
    void Switch2_Pressed();
    void Switch3_Pressed();


    static void Switch1_EXT_C();
    static void Switch2_EXT_C();
    static void Switch3_EXT_C();

    void init();

    bool initialized = false;

    static void OnLEDTimer_C(const timer_task *);
    static void OnOLEDTimer_C(const timer_task *);
    static void OnADCTimer_C(const timer_task *);
#ifdef MCP
    static void OnMCPTimer_C(const timer_task *);
#endif  // #ifdef MCP

    struct timer_task update_leds_timer_task = {0, 0, 0, 0, TIMER_TASK_REPEAT};
    struct timer_task update_oled_timer_task = {0, 0, 0, 0, TIMER_TASK_REPEAT};
    struct timer_task update_adc_timer_task = {0, 0, 0, 0, TIMER_TASK_REPEAT};

#ifdef MCP
    struct timer_task update_mcp_timer_task = {0, 0, 0, 0, TIMER_TASK_REPEAT};
#endif  // #ifdef MCP
};

#endif /* COMMANDS_H_ */
