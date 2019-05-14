#ifndef MODEL_H_
#define MODEL_H_

class Model {
public:
	static Model &instance();
	 
	uint32_t CurrentEffect() { return current_effect; }
	uint32_t SetCurrentEffect(uint32_t effect) { current_effect = effect; }
	uint32_t EffectCount() const { return 4; }

	double CurrentTime() const { return current_time; }
	void SetCurrentTime(double time) { current_time = time; }

	uint32_t CurrentBirdColor() const { return current_bird_color; }
	void SetCurrentBirdColor(uint32_t color) { current_bird_color = color; }

	float CurrentBrightness() const { return current_brightness; }
	void SetCurrentBrightness(float brightness) { current_brightness = brightness; }

	float CurrentBatteryVoltage() const  { return current_battery_voltage; }
	void SetCurrentBatteryVoltage(float voltage) { current_battery_voltage = voltage; }

	float CurrentSystemVoltage() const  { return current_system_voltage; }
	void SetCurrentSystemVoltage(float voltage) { current_system_voltage = voltage; }

	float CurrentVbusVoltage() const  { return current_vbus_voltage; }
	void SetCurrentVbusVoltage(float voltage) { current_vbus_voltage = voltage; }

	float CurrentChargeCurrent() const  { return current_charge_current; }
	void SetCurrentChargeCurrent(float current) { current_charge_current = current; }

private:
	void init();

	double current_time = 0;
	float current_brightness = 1.0f;
	uint32_t current_effect = 3;
	uint32_t current_bird_color = 0x00FFFF00;
	float current_battery_voltage = 0;
	float current_system_voltage = 0;
	float current_vbus_voltage = 0;
	float current_charge_current = 0;

	bool initialized = false;
};

#endif /* MODEL_H_ */