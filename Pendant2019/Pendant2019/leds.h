#ifndef LEDS_H_
#define LEDS_H_

#include <cstdint>

class led_control {
public:
	static void init();
	static void PerformV2MessageEffect(uint32_t color);
	static void PerformVoltageDisplay();
};

#endif /* LEDS_H_ */
