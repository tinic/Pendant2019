#include "./model.h"
#include "./emulator.h"

#include <atmel_start.h>

#include <string>
#include <cmath>
#include <algorithm>
#include <alloca.h>

#include "./murmur_hash3.h"

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

	
	uint32_t uid128[4];
	
#ifndef EMULATOR
	uid128[0] = *((uint32_t *)(0x008061FC));
	uid128[1] = *((uint32_t *)(0x00806010));
	uid128[2] = *((uint32_t *)(0x00806014));
	uid128[3] = *((uint32_t *)(0x00806018));
#else  // #ifndef EMULATOR
	memset(uid128, 0xCC, sizeof(uid128));
#endif  // #ifndef EMULATOR

	uid = MurmurHash3_32(uid128, sizeof(uid128), 0x5cfed374);
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
#ifndef EMULATOR
	uint32_t page_size = flash_get_page_size(&FLASH_0);
	uint32_t model_page = flash_get_total_pages(&FLASH_0) - 1;
#else  // #ifndef EMULATOR
	uint32_t page_size = 512;
	uint32_t model_page = 0;
#endif  // #ifndef EMULATOR
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

	auto read_uint16 = [](uint8_t *buf, size_t &buf_pos) {
		uint16_t ret = (uint32_t(buf[buf_pos+0]) <<  8)|
					   (uint32_t(buf[buf_pos+1]) <<  0);
		buf_pos += 2;
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

	auto read_double = [](uint8_t *buf, size_t &buf_pos) {
		uint64_t val = (uint64_t(buf[buf_pos+0]) << 56)|
					   (uint64_t(buf[buf_pos+0]) << 48)|
					   (uint64_t(buf[buf_pos+0]) << 40)|
					   (uint64_t(buf[buf_pos+0]) << 32)|
					   (uint64_t(buf[buf_pos+0]) << 24)|
					   (uint64_t(buf[buf_pos+1]) << 16)|
					   (uint64_t(buf[buf_pos+2]) <<  8)|
					   (uint64_t(buf[buf_pos+3]) <<  0);
		union {
			uint64_t int64;
			double float64;
		} a;
		a.int64 = val;
		buf_pos += 8;
		return a.float64;
	};
	
	auto read_buf = [](uint8_t *dst, size_t len, uint8_t *buf, size_t &buf_pos) {
		for (size_t c=0; c<len; c++) {
			*dst++ = buf[buf_pos++];
		}
	};

	if (read_uint32(buf, buf_pos) == marker) {
		current_bird_color = read_uint32(buf, buf_pos);
		current_message_color = read_uint32(buf, buf_pos);
		current_effect = read_uint32(buf, buf_pos);
		current_sent_message_count = read_uint32(buf, buf_pos);

		current_brightness = read_float(buf, buf_pos);
		current_time_zone_offset = read_float(buf, buf_pos);

		read_buf(reinterpret_cast<uint8_t *>(current_messages), sizeof(current_messages), buf, buf_pos);
		read_buf(reinterpret_cast<uint8_t *>(current_name), sizeof(current_name), buf, buf_pos);

		current_revc_messages_pos = read_uint32(buf, buf_pos);
		
		for (size_t c=0; c<messageRecvCount; c++) {
			current_recv_messages[c].datetime = read_double(buf, buf_pos);

			current_recv_messages[c].uid = read_uint32(buf, buf_pos);
			current_recv_messages[c].col = read_uint32(buf, buf_pos);
			current_recv_messages[c].flg = read_uint32(buf, buf_pos);
			current_recv_messages[c].cnt = read_uint16(buf, buf_pos);

			read_buf(reinterpret_cast<uint8_t *>(current_recv_messages[c].name), sizeof(current_recv_messages[c].name), buf, buf_pos);
			read_buf(reinterpret_cast<uint8_t *>(current_recv_messages[c].message), sizeof(current_recv_messages[c].message), buf, buf_pos);
		}
	} else {
		memcpy(
			current_messages,
			"0123456789AB"
			"0123456789AB"
			"0123456789AB"
			"0123456789AB"
			"0123456789AB"
			"0123456789AB"
			"0123456789AB"
			"0123456789AB",
			sizeof(current_messages)
		);
		memcpy(current_name, "DUCK\0\0\0\0\0\0\0\0", nameLength);
	}
}

void Model::save() {
#ifndef EMULATOR
	uint32_t page_size = flash_get_page_size(&FLASH_0);
	uint32_t model_page = flash_get_total_pages(&FLASH_0) - 1;
#else  // #ifndef EMULATOR
	uint32_t page_size = 512;
	uint32_t model_page = 0;
#endif  // #ifndef EMULATOR
	uint8_t *buf = static_cast<uint8_t *>(alloca(page_size));
	size_t buf_pos = 0;

	auto write_uint32 = [](uint32_t val, uint8_t *buf, size_t &buf_pos) {
		buf[buf_pos++] = (val >> 24) & 0xFF;
		buf[buf_pos++] = (val >> 16) & 0xFF;
		buf[buf_pos++] = (val >>  8) & 0xFF;
		buf[buf_pos++] = (val >>  0) & 0xFF;
	};

	auto write_uint16 = [](uint16_t val, uint8_t *buf, size_t &buf_pos) {
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

	auto write_double = [](double val, uint8_t *buf, size_t &buf_pos) {
		union {
			uint64_t int64;
			double float64;
		} a;
		a.float64 = val;
		buf[buf_pos++] = (a.int64 >> 56) & 0xFF;
		buf[buf_pos++] = (a.int64 >> 48) & 0xFF;
		buf[buf_pos++] = (a.int64 >> 40) & 0xFF;
		buf[buf_pos++] = (a.int64 >> 32) & 0xFF;
		buf[buf_pos++] = (a.int64 >> 24) & 0xFF;
		buf[buf_pos++] = (a.int64 >> 16) & 0xFF;
		buf[buf_pos++] = (a.int64 >>  8) & 0xFF;
		buf[buf_pos++] = (a.int64 >>  0) & 0xFF;
	};


	auto write_buf = [](uint8_t *src, size_t len, uint8_t *buf, size_t &buf_pos) {
		for (size_t c=0; c<len; c++) {
			buf[buf_pos++] = *src++;
		}
	};

	write_uint32(marker, buf, buf_pos);	

	write_uint32(current_bird_color, buf, buf_pos);
	write_uint32(current_message_color, buf, buf_pos);
	write_uint32(current_effect, buf, buf_pos);
	write_uint32(current_sent_message_count, buf, buf_pos);

	write_float(current_brightness, buf, buf_pos);
	write_float(current_time_zone_offset, buf, buf_pos);

	write_buf(reinterpret_cast<uint8_t *>(current_messages), sizeof(current_messages), buf, buf_pos);
	write_buf(reinterpret_cast<uint8_t *>(current_name), sizeof(current_name), buf, buf_pos);

	write_uint32(current_revc_messages_pos, buf, buf_pos);

	for (size_t c=0; c<messageRecvCount; c++) {
		write_double(current_recv_messages[c].datetime, buf, buf_pos);

		write_uint32(current_recv_messages[c].uid, buf, buf_pos);
		write_uint32(current_recv_messages[c].col, buf, buf_pos);
		write_uint32(current_recv_messages[c].flg, buf, buf_pos);
		write_uint16(current_recv_messages[c].cnt, buf, buf_pos);

		write_buf(reinterpret_cast<uint8_t *>(current_recv_messages[c].name), sizeof(current_recv_messages[c].name), buf, buf_pos);
		write_buf(reinterpret_cast<uint8_t *>(current_recv_messages[c].message), sizeof(current_recv_messages[c].message), buf, buf_pos);
	}
	
	flash_write(&FLASH_0, model_page * page_size, buf, page_size);
}
