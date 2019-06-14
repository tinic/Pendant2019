#ifndef MODEL_H_
#define MODEL_H_

#include <cstdint>
#include <cstring>
#include <string>

class Model {
	static constexpr size_t messageCount = 8;
	static constexpr size_t messageLength = 12;
	static constexpr size_t nameLength = 12;

public:
	static Model &instance();
	
	const char *CurrentMessage(size_t index) { index %= messageCount; return (const char *)&current_messages[index][0]; }
	void SetCurrentMessage(size_t index, const char *message) { index %= messageCount; strncpy((char *)&current_messages[index][0], message, messageLength); }
	uint32_t MessageCount() const { return messageCount; }

	uint32_t CurrentMessageColor() const { return current_message_color; }
	void SetCurrentMessageColor(uint32_t color) { current_message_color = color; }

	const char *CurrentName() const { return (const char *)current_name; }
	void SetCurrentName(const char *name) { strncpy((char *)current_name, name, nameLength); }
	
	uint32_t CurrentEffect() { return current_effect; }
	void SetCurrentEffect(uint32_t effect) { current_effect = effect; }
	uint32_t EffectCount() const { return 4; }

	double CurrentTime() const { return current_time; }
	void SetCurrentTime(double time) { current_time = time; }

	uint32_t CurrentBirdColor() const { return current_bird_color; }
	void SetCurrentBirdColor(uint32_t color) { current_bird_color = color; }

	float CurrentBrightness() const { return current_brightness; }
	void SetCurrentBrightness(float brightness) { current_brightness = brightness; }

	float CurrentBatteryVoltage() const  { return current_battery_voltage; }
	void SetCurrentBatteryVoltage(float voltage) { current_battery_voltage = voltage; }
	std::string CurrentBatteryVoltageString();

	float CurrentSystemVoltage() const  { return current_system_voltage; }
	void SetCurrentSystemVoltage(float voltage) { current_system_voltage = voltage; }
	std::string CurrentSystemVoltageString();

	float CurrentVbusVoltage() const  { return current_vbus_voltage; }
	void SetCurrentVbusVoltage(float voltage) { current_vbus_voltage = voltage; }
	std::string CurrentVbusVoltageString();

	float CurrentChargeCurrent() const  { return current_charge_current; }
	void SetCurrentChargeCurrent(float current) { current_charge_current = current; }
	std::string CurrentChargeCurrentString();

	void save();

private:

	void init();
	void load();

	float current_brightness = 1.0f;
	uint32_t current_effect = 3;
	uint32_t current_bird_color = 0x00FFFF00;
	uint32_t current_message_color = 0x00FFFF00;
	uint8_t current_messages[messageCount][messageLength];
	uint8_t current_name[nameLength];

	double current_time = 0;

	float current_battery_voltage = 0;
	float current_system_voltage = 0;
	float current_vbus_voltage = 0;
	float current_charge_current = 0;

	bool initialized = false;
};

#endif /* MODEL_H_ */