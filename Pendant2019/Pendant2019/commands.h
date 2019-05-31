#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <cstdint>
#include <atmel_start.h>

class Commands {
public:
	Commands();

	static Commands &instance();

	void Boot();

	void StartTimers();
	void StopTimers();

	void SendV2Message(const char *name, const char *message, uint8_t color = 0);

private:

	void OnLEDTimer();
	void OnOLEDTimer();
	void OnADCTimer();

	void Switch1_Pressed();
	void Switch2_Pressed();
	void Switch3_Pressed();

	static void OnLEDTimer_C(const timer_task *);
	static void OnOLEDTimer_C(const timer_task *);
	static void OnADCTimer_C(const timer_task *);

	static void Switch1_Pressed_C();
	static void Switch2_Pressed_C();
	static void Switch3_Pressed_C();

	void init();

	bool initialized = false;

	struct timer_task update_leds_timer_task = {0, 0, 0, 0, TIMER_TASK_REPEAT};
	struct timer_task update_oled_timer_task = {0, 0, 0, 0, TIMER_TASK_REPEAT};
	struct timer_task update_adc_timer_task = {0, 0, 0, 0, TIMER_TASK_REPEAT};
};

#endif /* COMMANDS_H_ */
