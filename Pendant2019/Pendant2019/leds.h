#ifndef LEDS_H_
#define LEDS_H_

#include <cstdint>

class led_control {
public:
	static void init();
	static void PerformV2MessageEffect(uint32_t color);
	static void PerformVoltageDisplay();

	static void PerformColorRingDisplay(uint32_t color);
	static void RemoveColorRingDisplay();

	static void PerformColorBirdDisplay(uint32_t color);
	static void RemoveColorBirdDisplay();
};

#endif /* LEDS_H_ */
