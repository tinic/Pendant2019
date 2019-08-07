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
    rand_sync_enable(&RAND_0);

    uid128[0] = *((uint32_t *)(0x008061FC));
    uid128[1] = *((uint32_t *)(0x00806010));
    uid128[2] = *((uint32_t *)(0x00806014));
    uid128[3] = *((uint32_t *)(0x00806018));
#else  // #ifndef EMULATOR
    memset(uid128, 0xCC, sizeof(uid128));
#endif  // #ifndef EMULATOR

    uid = MurmurHash3_32(uid128, sizeof(uid128), 0x5cfed374);
}

uint32_t Model::RandomUInt32() {
#ifndef EMULATOR
    return (rand_sync_read32(&RAND_0) << 24)|
           (rand_sync_read32(&RAND_0) << 16)|
           (rand_sync_read32(&RAND_0) <<  8)|
           (rand_sync_read32(&RAND_0) <<  0);
#else  // #ifndef EMULATOR
    return 0;
#endif  // #ifndef EMULATOR
}

std::string Model::BatteryVoltageString() {
    char str[8];
    int32_t i = int32_t(BatteryVoltage());
    int32_t f = int32_t(fmodf(BatteryVoltage(),1.0f)*100);
    sprintf(str,"%1d.%02d", int(i), int(f));
    return std::string(str);
}

std::string Model::SystemVoltageString() {
    char str[8];
    int32_t i = int32_t(SystemVoltage());
    int32_t f = int32_t(fmodf(SystemVoltage(),1.0f)*100);
    sprintf(str,"%1d.%02d", int(i), int(f));
    return std::string(str);
}

std::string Model::VbusVoltageString() {
    char str[8];
    int32_t i = int32_t(VbusVoltage());
    int32_t f = int32_t(fmodf(VbusVoltage(),1.0f)*100);
    sprintf(str,"%1d.%02d", int(i), int(f));
    return std::string(str);
}

std::string Model::ChargeCurrentString() {
    char str[8];
    int32_t i = int32_t(ChargeCurrent());
    sprintf(str,"%d", int(i));
    return std::string(str);
}

void Model::load() {
    uint32_t page_size = flash_get_page_size(&FLASH_0);
    uint32_t model_page = flash_get_total_pages(&FLASH_0) - 1;
    uint8_t *buf = static_cast<uint8_t *>(alloca(page_size));
    size_t buf_pos = 0;
    flash_read(&FLASH_0, model_page * page_size, buf, page_size);

    auto read_uint32 = [](uint8_t *b, size_t &bp) {
        uint32_t ret = (uint32_t(b[bp+0]) << 24)|
                       (uint32_t(b[bp+1]) << 16)|
                       (uint32_t(b[bp+2]) <<  8)|
                       (uint32_t(b[bp+3]) <<  0);
        bp += 4;
        return ret;
    };

    auto read_uint16 = [](uint8_t *b, size_t &bp) {
        uint16_t ret = (uint32_t(b[bp+0]) <<  8)|
                       (uint32_t(b[bp+1]) <<  0);
        bp += 2;
        return ret;
    };

    auto read_float = [](uint8_t *b, size_t &bp) {
        uint32_t val = (uint32_t(b[bp+0]) << 24)|
                       (uint32_t(b[bp+1]) << 16)|
                       (uint32_t(b[bp+2]) <<  8)|
                       (uint32_t(b[bp+3]) <<  0);
        union {
            uint32_t int32;
            float float32;
        } a;
        a.int32 = val;
        bp += 4;
        return a.float32;
    };

    auto read_double = [](uint8_t *b, size_t &bp) {
        uint64_t val = (uint64_t(b[bp+0]) << 56)|
                       (uint64_t(b[bp+1]) << 48)|
                       (uint64_t(b[bp+2]) << 40)|
                       (uint64_t(b[bp+3]) << 32)|
                       (uint64_t(b[bp+4]) << 24)|
                       (uint64_t(b[bp+5]) << 16)|
                       (uint64_t(b[bp+6]) <<  8)|
                       (uint64_t(b[bp+7]) <<  0);
        union {
            uint64_t int64;
            double float64;
        } a;
        a.int64 = val;
        bp += 8;
        return a.float64;
    };
    
    auto read_buf = [](uint8_t *dst, size_t len, uint8_t *b, size_t &bp) {
        for (size_t c=0; c<len; c++) {
            *dst++ = b[bp++];
        }
    };

    if (read_uint32(buf, buf_pos) == marker) {
        bird_color.rgbx = read_uint32(buf, buf_pos);
        ring_color.rgbx = read_uint32(buf, buf_pos);
        message_color.rgbx = read_uint32(buf, buf_pos);
        effect = read_uint32(buf, buf_pos);
        sent_message_count = read_uint32(buf, buf_pos);

        brightness = read_float(buf, buf_pos);
        time_zone_offset = read_double(buf, buf_pos);

        read_buf(reinterpret_cast<uint8_t *>(messages), sizeof(messages), buf, buf_pos);
        read_buf(reinterpret_cast<uint8_t *>(name), sizeof(name), buf, buf_pos);

        revc_messages_pos = read_uint32(buf, buf_pos);
        
        for (size_t c=0; c<messageRecvCount; c++) {
            recv_messages[c].datetime = read_double(buf, buf_pos);

            recv_messages[c].uid = read_uint32(buf, buf_pos);
            recv_messages[c].col.rgbx = read_uint32(buf, buf_pos);
            recv_messages[c].flg = read_uint32(buf, buf_pos);
            recv_messages[c].cnt = read_uint16(buf, buf_pos);

            read_buf(reinterpret_cast<uint8_t *>(recv_messages[c].name), sizeof(recv_messages[c].name), buf, buf_pos);
            read_buf(reinterpret_cast<uint8_t *>(recv_messages[c].message), sizeof(recv_messages[c].message), buf, buf_pos);
        }
    } else {
        memcpy(
            messages,
            "QUACK!QUACK!"
            "GOING HOME!!"
            " I AM OUT!! "
            ".DON'T WAIT."
            "LOOK THERE!!"
            "GIMME MARLOT"
            "SAFETY THIRD"
            "DUCKING DUCK",
            sizeof(messages)
        );
        memcpy(name, "DUCKLING\0\0\0\0", nameLength);
    }
}

void Model::save() {
    uint32_t page_size = flash_get_page_size(&FLASH_0);
    uint32_t model_page = flash_get_total_pages(&FLASH_0) - 1;
    uint8_t *buf = static_cast<uint8_t *>(alloca(page_size));
    size_t buf_pos = 0;

    auto write_uint32 = [](uint32_t val, uint8_t *b, size_t &bp) {
        b[bp++] = (val >> 24) & 0xFF;
        b[bp++] = (val >> 16) & 0xFF;
        b[bp++] = (val >>  8) & 0xFF;
        b[bp++] = (val >>  0) & 0xFF;
    };

    auto write_uint16 = [](uint16_t val, uint8_t *b, size_t &bp) {
        b[bp++] = (val >>  8) & 0xFF;
        b[bp++] = (val >>  0) & 0xFF;
    };

    auto write_float = [](float val, uint8_t *b, size_t &bp) {
        union {
            uint32_t int32;
            float float32;
        } a;
        a.float32 = val;
        b[bp++] = (a.int32 >> 24) & 0xFF;
        b[bp++] = (a.int32 >> 16) & 0xFF;
        b[bp++] = (a.int32 >>  8) & 0xFF;
        b[bp++] = (a.int32 >>  0) & 0xFF;
    };

    auto write_double = [](double val, uint8_t *b, size_t &bp) {
        union {
            uint64_t int64;
            double float64;
        } a;
        a.float64 = val;
        b[bp++] = (a.int64 >> 56) & 0xFF;
        b[bp++] = (a.int64 >> 48) & 0xFF;
        b[bp++] = (a.int64 >> 40) & 0xFF;
        b[bp++] = (a.int64 >> 32) & 0xFF;
        b[bp++] = (a.int64 >> 24) & 0xFF;
        b[bp++] = (a.int64 >> 16) & 0xFF;
        b[bp++] = (a.int64 >>  8) & 0xFF;
        b[bp++] = (a.int64 >>  0) & 0xFF;
    };


    auto write_buf = [](uint8_t *src, size_t len, uint8_t *b, size_t &bp) {
        for (size_t c=0; c<len; c++) {
            b[bp++] = *src++;
        }
    };

    write_uint32(marker, buf, buf_pos); 

    write_uint32(bird_color.rgbx, buf, buf_pos);
    write_uint32(ring_color.rgbx, buf, buf_pos);
    write_uint32(message_color.rgbx, buf, buf_pos);
    write_uint32(effect, buf, buf_pos);
    write_uint32(sent_message_count, buf, buf_pos);

    write_float(brightness, buf, buf_pos);
    write_double(time_zone_offset, buf, buf_pos);

    write_buf(reinterpret_cast<uint8_t *>(messages), sizeof(messages), buf, buf_pos);
    write_buf(reinterpret_cast<uint8_t *>(name), sizeof(name), buf, buf_pos);

    write_uint32(revc_messages_pos, buf, buf_pos);

    for (size_t c=0; c<messageRecvCount; c++) {
        write_double(recv_messages[c].datetime, buf, buf_pos);

        write_uint32(recv_messages[c].uid, buf, buf_pos);
        write_uint32(recv_messages[c].col.rgbx, buf, buf_pos);
        write_uint32(recv_messages[c].flg, buf, buf_pos);
        write_uint16(recv_messages[c].cnt, buf, buf_pos);

        write_buf(reinterpret_cast<uint8_t *>(recv_messages[c].name), sizeof(recv_messages[c].name), buf, buf_pos);
        write_buf(reinterpret_cast<uint8_t *>(recv_messages[c].message), sizeof(recv_messages[c].message), buf, buf_pos);
    }
    
    flash_write(&FLASH_0, model_page * page_size, buf, page_size);
}
