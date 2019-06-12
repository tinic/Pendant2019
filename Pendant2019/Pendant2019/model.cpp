#include "./model.h"

#include <atmel_start.h>

#include <string>
#include <cmath>
#include <algorithm>
#include <alloca.h>

static const uint32_t marker = 0x99acfc2d;

Model &Model::instance() {
	static Model model;
	if (!model.initialized) {
		model.initialized = true;
		model.init();
	}
	return model;
}

void Model::init() {
	load();
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

void Model::load() {
	uint32_t page_size = flash_get_page_size(&FLASH_0);
	uint32_t model_page = flash_get_total_pages(&FLASH_0) - 1;
	uint8_t *buf = static_cast<uint8_t *>(alloca(page_size));
	size_t buf_pos = 0;
	flash_read(&FLASH_0, model_page * page_size, buf, page_size);

	auto read_uint32 = [](uint8_t *buf, size_t &buf_pos) {
		uint32_t ret = (uint32_t(buf[buf_pos+0]) << 24)|
					   (uint32_t(buf[buf_pos+1]) << 16)|
					   (uint32_t(buf[buf_pos+2]) <<  8)|
					   (uint32_t(buf[buf_pos+3]) <<  0);
		buf_pos += 4;
		return ret;
	};

	auto read_float = [](uint8_t *buf, size_t &buf_pos) {
		uint32_t val = (uint32_t(buf[buf_pos+0]) << 24)|
					   (uint32_t(buf[buf_pos+1]) << 16)|
					   (uint32_t(buf[buf_pos+2]) <<  8)|
					   (uint32_t(buf[buf_pos+3]) <<  0);
		union {
			uint32_t int32;
			float float32;
		} a;
		a.int32 = val;
		buf_pos += 4;
		return a.float32;
	};
	
	auto read_buf = [](uint8_t *dst, size_t len, uint8_t *buf, size_t &buf_pos) {
		for (size_t c=0; c<len; c++) {
			*dst++ = buf[buf_pos++];
		}
	};

	if (read_uint32(buf, buf_pos) == marker) {
		current_bird_color = read_uint32(buf, buf_pos);
		current_effect = read_uint32(buf, buf_pos);
		current_brightness = read_float(buf, buf_pos);
		read_buf(reinterpret_cast<uint8_t *>(current_messages), sizeof(current_messages), buf, buf_pos);
	} else {
		memcpy(
			current_messages,
			"0123456789ABCDEF"
			"0123456789ABCDEF"
			"0123456789ABCDEF"
			"0123456789ABCDEF"
			"0123456789ABCDEF"
			"0123456789ABCDEF"
			"0123456789ABCDEF"
			"0123456789ABCDEF",
			sizeof(current_messages)
		);
	}
}

void Model::save() {
	uint32_t page_size = flash_get_page_size(&FLASH_0);
	uint32_t model_page = flash_get_total_pages(&FLASH_0) - 1;
	uint8_t *buf = static_cast<uint8_t *>(alloca(page_size));
	size_t buf_pos = 0;

	auto write_uint32 = [](uint32_t val, uint8_t *buf, size_t &buf_pos) {
		buf[buf_pos++] = (val >> 24) & 0xFF;
		buf[buf_pos++] = (val >> 16) & 0xFF;
		buf[buf_pos++] = (val >>  8) & 0xFF;
		buf[buf_pos++] = (val >>  0) & 0xFF;
	};

	auto write_float = [](float val, uint8_t *buf, size_t &buf_pos) {
		union {
			uint32_t int32;
			float float32;
		} a;
		a.float32 = val;
		buf[buf_pos++] = (a.int32 >> 24) & 0xFF;
		buf[buf_pos++] = (a.int32 >> 16) & 0xFF;
		buf[buf_pos++] = (a.int32 >>  8) & 0xFF;
		buf[buf_pos++] = (a.int32 >>  0) & 0xFF;
	};

	auto write_buf = [](uint8_t *src, size_t len, uint8_t *buf, size_t &buf_pos) {
		for (size_t c=0; c<len; c++) {
			buf[buf_pos++] = *src++;
		}
	};

	write_uint32(marker, buf, buf_pos);	
	
	write_uint32(current_bird_color, buf, buf_pos);
	write_uint32(current_effect, buf, buf_pos);
	write_float(current_brightness, buf, buf_pos);
	write_buf(reinterpret_cast<uint8_t *>(current_messages), sizeof(current_messages), buf, buf_pos);
	
	flash_write(&FLASH_0, model_page * page_size, buf, page_size);
}
