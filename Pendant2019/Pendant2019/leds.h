#ifndef LEDS_H_
#define LEDS_H_

#include <cstdint>

class led_control {
public:
	static void init();
	static void PerformV2MessageEffect(uint32_t color, bool remove = false);

	static void PerformMessageColorDisplay(uint32_t color, bool remove = false);
	static void PerformColorBirdDisplay(uint32_t color, bool remove = false);
};

#endif /* LEDS_H_ */
