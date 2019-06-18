#ifndef MODEL_H_
#define MODEL_H_

#include "./leds.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <ctime>
#include <array>

class Model {
	static constexpr size_t messageCount = 8;
	static constexpr size_t messageLength = 12;
	static constexpr size_t nameLength = 12;
	static constexpr size_t messageRecvCount = 8;

public:
	static Model &instance();
	
	const char *CurrentMessage(size_t index) { index %= messageCount; return (const char *)&current_messages[index][0]; }
	void SetCurrentMessage(size_t index, const char *message) { index %= messageCount; strncpy((char *)&current_messages[index][0], message, messageLength); }
	uint32_t MessageCount() const { return messageCount; }

	colors::rgb8 CurrentMessageColor() const { return current_message_color; }
	void SetCurrentMessageColor(colors::rgb8 color) { current_message_color = color; }

	const char *CurrentName() const { return (const char *)current_name; }
	void SetCurrentName(const char *name) { strncpy((char *)current_name, name, nameLength); }
	
	uint32_t CurrentEffect() { return current_effect; }
	void SetCurrentEffect(uint32_t effect) { current_effect = effect; }
	uint32_t EffectCount() const { return 4; }

	double CurrentTime() const { return current_time; }
	void SetCurrentTime(double time) { current_time = time; }

	colors::rgb8 CurrentBirdColor() const { return current_bird_color; }
	void SetCurrentBirdColor(colors::rgb8 color) { current_bird_color = color; }

	float CurrentBrightness() const { return current_brightness; }
	void SetCurrentBrightness(float brightness) { current_brightness = brightness; }

	float CurrentBatteryVoltage() const  { return current_battery_voltage; }
	void SetCurrentBatteryVoltage(float voltage) { current_battery_voltage = voltage; }
	std::string CurrentBatteryVoltageString();

	float CurrentSystemVoltage() const  { return current_system_voltage; }
	void SetCurrentSystemVoltage(float voltage) { current_system_voltage = voltage; }
	std::string CurrentSystemVoltageString();

	float MinBatteryVoltage() const { return 3.5; }
	float MaxBatteryVoltage() const { return 4.2; }

	float CurrentVbusVoltage() const  { return current_vbus_voltage; }
	void SetCurrentVbusVoltage(float voltage) { current_vbus_voltage = voltage; }
	std::string CurrentVbusVoltageString();

	float CurrentChargeCurrent() const  { return current_charge_current; }
	void SetCurrentChargeCurrent(float current) { current_charge_current = current; }
	std::string CurrentChargeCurrentString();
	
	double CurrentDateTime() const { if (current_date_time_offset == 0.0) { return -1.0; } return current_time + current_date_time_offset; };
	void SetCurrentDateTime(double date_time) { current_date_time_offset = date_time - current_time; }

	double CurrentTimeZoneOffset() const { return current_time_zone_offset; }
	void SetCurrentTimeZoneOffset(double time_zone_offset) { current_time_zone_offset = time_zone_offset; }

	uint32_t CurrentMessageCount() const { return current_sent_message_count; }
	void IncCurrentMessageCount() { current_sent_message_count++; save(); }
	
	uint32_t UID() const { return uid; }
	
	struct Message {
		double datetime;
		uint32_t uid;
		colors::rgb8 col;
		uint32_t flg;
		uint16_t cnt;
		uint8_t message[messageLength];
		uint8_t name[nameLength];
	};

	const Message &CurrentRecvMessage(size_t index) { index %= messageRecvCount; return current_recv_messages[index]; }
	void PushCurrentRecvMessage(Message &msg) { current_recv_messages[current_revc_messages_pos++] = msg; current_revc_messages_pos %= messageRecvCount; }

	void save();

private:

	void init();
	void load();

	// Settings
	float current_brightness = 1.0f;
	float current_time_zone_offset = -7.0f;

	uint32_t current_effect = 3;
	
	colors::rgb8 current_bird_color = colors::rgb8(0xFF, 0xFF, 0x00);
	colors::rgb8 current_message_color = colors::rgb8(0xFF, 0xFF, 0x00);

	uint8_t current_messages[messageCount][messageLength];
	uint8_t current_name[nameLength];

	// Persistent
	uint32_t current_sent_message_count = 0;
	
	uint32_t current_revc_messages_pos = 0;
	std::array<Message, messageRecvCount> current_recv_messages;
	
	// Volatile
	double current_time = 0;
	double current_date_time_offset = 0;

	float current_battery_voltage = 0;
	float current_system_voltage = 0;
	float current_vbus_voltage = 0;
	float current_charge_current = 0;

	uint32_t uid;

	bool initialized = false;
};

#endif /* MODEL_H_ */