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
	
	const char *Message(size_t index) { index %= messageCount; return (const char *)&messages[index][0]; }
	void SetMessage(size_t index, const char *message) { index %= messageCount; strncpy((char *)&messages[index][0], message, messageLength); }
	static constexpr size_t MessageCount() { return messageCount; }
	static constexpr size_t MessageLength() { return messageLength; }

	colors::rgb8 MessageColor() const { return message_color; }
	void SetMessageColor(colors::rgb8 color) { message_color = color; }

	const char *Name() const { return (const char *)name; }
	void SetName(const char *newname) { strncpy((char *)name, newname, nameLength); }
	static constexpr size_t NameLength() { return nameLength; }
	
	uint32_t Effect() { return effect; }
	void SetEffect(uint32_t neweffect) { effect = neweffect; }
	static constexpr uint32_t EffectCount() { return 9; }

	double Time() const { return time; }
	void SetTime(double current_time) { time = current_time; }

	colors::rgb8 BirdColor() const { return bird_color; }
	void SetBirdColor(colors::rgb8 color) { bird_color = color; }

	float Brightness() const { return brightness; }
	void SetBrightness(float newbrightness) { brightness = newbrightness; }

	float BatteryVoltage() const  { return battery_voltage; }
	void SetBatteryVoltage(float voltage) { battery_voltage = voltage; }
	std::string BatteryVoltageString();

	float SystemVoltage() const  { return system_voltage; }
	void SetSystemVoltage(float voltage) { system_voltage = voltage; }
	std::string SystemVoltageString();

	static constexpr float MinBatteryVoltage() { return 3.5f; }
	static constexpr float MaxBatteryVoltage() { return 4.2f; }

	float VbusVoltage() const  { return vbus_voltage; }
	void SetVbusVoltage(float voltage) { vbus_voltage = voltage; }
	std::string VbusVoltageString();

	float ChargeCurrent() const  { return charge_current; }
	void SetChargeCurrent(float current) { charge_current = current; }
	std::string ChargeCurrentString();
	
	double DateTime() const { if (date_time_offset == 0.0) { return -1.0; } return time + date_time_offset; };
	void SetDateTime(double date_time) { date_time_offset = date_time - time; }

	double TimeZoneOffset() const { return time_zone_offset; }
	void SetTimeZoneOffset(double new_time_zone_offset) { time_zone_offset = new_time_zone_offset; }

	uint32_t SentMessageCount() const { return sent_message_count; }
	void IncSentMessageCount() { sent_message_count++; save(); }
	
	uint32_t UID() const { return uid; }
	uint32_t RandomUInt32();
	
	bool IsSwitch1Down(double &timestamp) const { 
		if (switch_1_down != 0.0) {
			timestamp = switch_1_down;
			return true;
		} 
		else { 
			return false; 
		}
	}
	
	bool IsSwitch2Down(double &timestamp) const {
		if (switch_2_down != 0.0) {
			timestamp = switch_1_down;
			return true;
		} 
		else { 
			return false; 
		}
	}

	bool IsSwitch3Down(double &timestamp) const {
		if (switch_3_down != 0.0) {
			timestamp = switch_1_down;
			return true;
		} 
		else { 
			return false; 
		}
	}
	
	struct Message {
		double datetime;
		uint32_t uid;
		colors::rgb8 col;
		uint32_t flg;
		uint16_t cnt;
		uint8_t message[messageLength];
		uint8_t name[nameLength];
	};

	const struct Message &RecvMessage(size_t index) { index %= messageRecvCount; return recv_messages[index]; }
	void PushRecvMessage(struct Message &msg) { recv_messages[revc_messages_pos++] = msg; revc_messages_pos %= messageRecvCount; }

	void save();

private:
	friend class Commands;

	void SetSwitch1Down(double timestamp) { switch_1_down = timestamp; }
	void SetSwitch2Down(double timestamp) { switch_2_down = timestamp; }
	void SetSwitch3Down(double timestamp) { switch_3_down = timestamp; }

private:

	void init();
	void load();

	// Settings
	float brightness = 1.0f;
	float time_zone_offset = -7.0f;

	uint32_t effect = 8;
	
	colors::rgb8 bird_color = colors::rgb8(0xFF, 0xFF, 0x00);
	colors::rgb8 message_color = colors::rgb8(0xFF, 0xFF, 0x00);

	uint8_t messages[messageCount][messageLength];
	uint8_t name[nameLength];

	// Persistent
	uint32_t sent_message_count = 0;
	
	uint32_t revc_messages_pos = 0;
	std::array<struct Message, messageRecvCount> recv_messages;
	
	// Volatile
	double time = 0;
	double date_time_offset = 0;

	float battery_voltage = 0;
	float system_voltage = 0;
	float vbus_voltage = 0;
	float charge_current = 0;
	
	double switch_1_down = 0.0;
	double switch_2_down = 0.0;
	double switch_3_down = 0.0;

	uint32_t uid;

	bool initialized = false;
};

#endif /* MODEL_H_ */
