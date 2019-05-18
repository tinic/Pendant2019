#include <atmel_start.h>

#include "model.h"

#include <string>
#include <math.h>
#include <algorithm>

Model &Model::instance() {
	static Model model;
	if (!model.initialized) {
		model.initialized = true;
		model.init();
	}
	return model;
}

void Model::init() {
}

std::string Model::CurrentBatteryVoltageString() {
	char str[8];
	int32_t i = int32_t(CurrentBatteryVoltage());
	int32_t f = int32_t(fmodf(CurrentBatteryVoltage(),1.0f)*100);
	sprintf(str,"%1d.%02d", int(i), int(f));
	return std::string(str);
}

std::string Model::CurrentSystemVoltageString() {
	char str[8];
	int32_t i = int32_t(CurrentSystemVoltage());
	int32_t f = int32_t(fmodf(CurrentSystemVoltage(),1.0f)*100);
	sprintf(str,"%1d.%02d", int(i), int(f));
	return std::string(str);
}

std::string Model::CurrentVbusVoltageString() {
	char str[8];
	int32_t i = int32_t(CurrentVbusVoltage());
	int32_t f = int32_t(fmodf(CurrentVbusVoltage(),1.0f)*100);
	sprintf(str,"%1d.%02d", int(i), int(f));
	return std::string(str);
}

std::string Model::CurrentChargeCurrentString() {
	char str[8];
	int32_t i = int32_t(CurrentChargeCurrent());
	sprintf(str,"%d", int(i));
	return std::string(str);
}
