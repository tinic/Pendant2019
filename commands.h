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
