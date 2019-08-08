/*
Copyright 2019 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "./commands.h"

#include <atmel_start.h>

#include <cstring>
#include <cmath>
#include <limits>

#include "./model.h"
#include "./sx1280.h"
#include "./bq25895.h"
#include "./sdd1306.h"
#include "./timeline.h"
#include "./leds.h"
#include "./system_time.h"
#include "./ui.h"

#ifndef EMULATOR
#include "hri_rstc_d51.h"
#endif  // #ifndef EMULATOR

extern "C" {
    extern struct wdt_descriptor WDT_0;
};

Commands::Commands() {
}

Commands &Commands::instance() {
    static Commands duckcommands;
    if (!duckcommands.initialized) {
        duckcommands.initialized = true;
        duckcommands.init();
    }
    return duckcommands;
}

void Commands::Boot() {

#ifndef EMULATOR
    Model::instance().SetResetCause(hri_rstc_read_RCAUSE_reg(RSTC));
#endif  // #ifndef EMULATOR

    if (SDD1306::instance().DevicePresent()) {
        SDD1306::instance().Clear();
        SDD1306::instance().PlaceUTF8String(0,0,".Pendant3.0.");
        SDD1306::instance().PlaceUTF8String(0,1,"============");
        SDD1306::instance().Display();
    }

    if (BQ25895::instance().DevicePresent()) {
        BQ25895::instance().SetBoostVoltage(4550);
        BQ25895::instance().DisableWatchdog();
        BQ25895::instance().DisableOTG();
        BQ25895::instance().OneShotADC();
        BQ25895::instance().SetInputCurrent(500);
    }

    delay_ms(50);

    if (SX1280::instance().DevicePresent()) {
    
        SendDateTimeRequest();

        SX1280::instance().SetTxDoneCallback([=](void) {
        });
        
        SX1280::instance().SetRxDoneCallback([=](const uint8_t *payload, uint8_t size, SX1280::PacketStatus) {
            if (size >= 24 && memcmp(payload, "PLEASEPLEASERANGEMENOW!!", 24) == 0) {
                static Timeline::Span s;
                s.type = Timeline::Span::Measurement;
                s.time = Model::instance().Time();
                s.duration = 60.0; // timeout

                s.startFunc = [=](Timeline::Span &) {
                    SX1280::instance().SetRangingRX();
                };

                s.doneFunc = [=](Timeline::Span &) {
                    SX1280::instance().SetLoraRX();
                };
				Timeline::instance().Add(s);
            } else if (size >= 24 && memcmp(payload, "UTC", 3) == 0) {
                struct tm tm;
                memset(&tm, 0, sizeof(tm));
                sscanf(reinterpret_cast<const char *>(&payload[3]),"%04d-%02d-%02dT%02d:%02d:%02dZ", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
                tm.tm_year -= 1900;

                if (tm.tm_year < 0) {
                    return;
                }
                if (tm.tm_mon  < 0 || tm.tm_mon  > 11) {
                    return;
                }
                if (tm.tm_mday < 0 || tm.tm_mday > 31) {
                    return;
                }
                if (tm.tm_hour < 0 || tm.tm_hour > 23) {
                    return;
                }
                if (tm.tm_min  < 0 || tm.tm_min  > 59) {
                    return;
                }
                if (tm.tm_sec  < 0 || tm.tm_sec  > 61) {
                    return;
                }

                double intPart = 0;

                tm.tm_min += 60;
                tm.tm_min += static_cast<int>(modf(Model::instance().TimeZoneOffset(), &intPart));
                tm.tm_min %= 60;

                tm.tm_hour += 24;
                tm.tm_hour += static_cast<int>(intPart);
                tm.tm_hour %= 24;

                Model::instance().SetDateTime((static_cast<double>(tm.tm_hour) * 24.0 * 60.0 + static_cast<double>(tm.tm_min) * 60.0 + static_cast<double>(tm.tm_sec)));
                
            // Do V2 messages
            } else if (size >= 24 && memcmp(payload, "DUCK!!", 6) == 0) {
                static const uint32_t radio_colors[] = {
                    0x808080UL,
                    0xA00000UL,
                    0x00A000UL,
                    0x0000A0UL,
                    0x909000UL,
                    0x009090UL,
                    0x900090UL,
                    0x906020UL,
                    0x206090UL,
                    0x602090UL,
                    0x902060UL,
                };
                if (SDD1306::instance().DevicePresent()) {
                    static Timeline::Span s;
                    if (!Timeline::instance().Scheduled(s)) {
                        s.type = Timeline::Span::Display;
                        s.time = Model::instance().Time();
                        s.duration = 8.0;
                        s.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
                            char str[64];
                            snprintf(str, 64, "%8.8s : %8.8s", &payload[16], &payload[8] );
                            const float speed = 128.0;
                            int32_t text_walk = static_cast<int32_t>(static_cast<float>(Model::instance().Time() - static_cast<double>(span.time)) * speed - 96.0f);
                            float interp = 0;
                            if (span.InBeginPeriod(interp, 0.5f)) {
                                if (interp < 0.5f) {
                                    SDD1306::instance().SetVerticalShift(-static_cast<int32_t>(interp * 2.0f * 16));
                                    below.Calc();
                                } else {
                                    SDD1306::instance().SetVerticalShift(16-static_cast<int32_t>((interp * 2.0f - 1.0f ) * 16.0f));
                                    SDD1306::instance().SetAsciiScrollMessage(str,text_walk);
                                }
                            } else  if (span.InEndPeriod(interp, 0.5f)) {
                                if (interp < 0.5f) {
                                    SDD1306::instance().SetVerticalShift(-static_cast<int32_t>(interp * 2.0f * 16.0f));
                                    SDD1306::instance().SetAsciiScrollMessage(str,text_walk);
                                } else {
                                    SDD1306::instance().SetVerticalShift(16-static_cast<int32_t>((interp * 2.0f - 1.0f ) * 16.0f));
                                    SDD1306::instance().SetAsciiScrollMessage(0,0);
                                    below.Calc();
                                }
                            } else {
                                SDD1306::instance().SetVerticalShift(0);
                                SDD1306::instance().SetAsciiScrollMessage(str,text_walk);
                            }
                        };
                        s.commitFunc = [=](Timeline::Span &) {
                            SDD1306::instance().Display();
                        };
                        s.doneFunc = [=](Timeline::Span &) {
                            SDD1306::instance().SetVerticalShift(0);
                        };
                        Timeline::instance().Add(s);
                    }
                }
                led_control::PerformV2MessageEffect(radio_colors[payload[7]]);
            } else if (size >= 42 && memcmp(payload, "DUCK", 4) == 0) {
                struct Model::Message msg;
                
                msg.datetime = Model::instance().DateTime();
                
                msg.uid = ( static_cast<uint32_t>(payload[ 4]) << 24 )|
                          ( static_cast<uint32_t>(payload[ 5]) << 16 )|
                          ( static_cast<uint32_t>(payload[ 6]) <<  8 )| 
                          ( static_cast<uint32_t>(payload[ 7]) <<  0 );
                          
                msg.col.r = static_cast<uint32_t>(payload[ 8]);
                msg.col.g = static_cast<uint32_t>(payload[ 9]);
                msg.col.b = static_cast<uint32_t>(payload[10]);
                          
                msg.flg = ( static_cast<uint32_t>(payload[12]) << 24 )|
                          ( static_cast<uint32_t>(payload[13]) << 16 )|
                          ( static_cast<uint32_t>(payload[14]) <<  8 )| 
                          ( static_cast<uint32_t>(payload[15]) <<  0 );
                          
                msg.cnt = ( uint16_t(payload[16]) <<  8 )| 
                          ( uint16_t(payload[17]) <<  0 );
                          
                memcpy(&msg.name[0], &payload[18], 12);
                memcpy(&msg.message[0], &payload[30], 12);

                Model::instance().PushRecvMessage(msg);
            }
        });

        SX1280::instance().SetRxErrorCallback([=](SX1280::IrqErrorCode) {
        });

        SX1280::instance().SetTxTimeoutCallback([=](void) {
        });

        SX1280::instance().SetRxTimeoutCallback([=](void) {
        });

        SX1280::instance().SetRxSyncWordDoneCallback([=](void) {
        });

        SX1280::instance().SetRxHeaderDoneCallback([=](void) {
        });

        SX1280::instance().SetRangingDoneCallback([=](SX1280::IrqRangingCode, float) {
        });

        SX1280::instance().SetCadDoneCallback([=](bool) {
        });
    }

    UI::instance();

    led_control::init();

    delay_ms(50);
}

void Commands::StartTimers() {
    update_leds_timer_task.interval = 10; // 100fps
    update_leds_timer_task.cb = &OnLEDTimer_C;
    update_leds_timer_task.mode = TIMER_TASK_REPEAT;
    timer_add_task(&TIMER_0, &update_leds_timer_task);

    update_oled_timer_task.interval = 16; // 60fps
    update_oled_timer_task.cb = &OnOLEDTimer_C;
    update_oled_timer_task.mode = TIMER_TASK_REPEAT;
    timer_add_task(&TIMER_0, &update_oled_timer_task);

    update_adc_timer_task.interval = 2000; // Every 2 seconds
    update_adc_timer_task.cb = &OnADCTimer_C;
    update_adc_timer_task.mode = TIMER_TASK_REPEAT;
    timer_add_task(&TIMER_0, &update_adc_timer_task);

#ifdef MCP
    update_adc_timer_task.interval = 16; // 60fp
    update_adc_timer_task.cb = &OnMCPTimer_C;
    update_adc_timer_task.mode = TIMER_TASK_REPEAT;
    timer_add_task(&TIMER_0, &update_mcp_timer_task);
#endif  // #ifdef MCP

    timer_start(&TIMER_0);
}

void Commands::StopTimers() {
    timer_remove_task(&TIMER_0, &update_leds_timer_task);
    timer_remove_task(&TIMER_0, &update_oled_timer_task);
    timer_remove_task(&TIMER_0, &update_adc_timer_task);
    
    timer_stop(&TIMER_0);
}

void Commands::SendV2Message(const char *name, const char *message, uint8_t color) {
    static uint8_t buf[32];
    memcpy(&buf[0],"DUCK!!",6);
    buf[7] = color;
    memset(&buf[8],0x20,16);
    strncpy(reinterpret_cast<char *>(&buf[8]),name,8);
    strncpy(reinterpret_cast<char *>(&buf[16]),message,8);
    SX1280::instance().LoraTxStart(buf,24);
}

void Commands::SendV3Message(const char *msg, const char *nam, colors::rgb8 col) {
    uint8_t buf[42];

    memset(&buf[0], 0, sizeof(buf));

    memcpy(&buf[0], "DUCK", 4);
    
    uint32_t uid = Model::instance().UID();
    
    buf[ 4] = (uid >> 24 ) & 0xFF;
    buf[ 5] = (uid >> 16 ) & 0xFF;
    buf[ 6] = (uid >>  8 ) & 0xFF;
    buf[ 7] = (uid >>  0 ) & 0xFF;

    buf[ 8] = (col.r) & 0xFF;
    buf[ 9] = (col.g) & 0xFF;
    buf[10] = (col.b) & 0xFF;
    buf[11] = (0    ) & 0xFF;

    uint32_t flg = 0;
    buf[12] = (flg >> 24 ) & 0xFF;
    buf[13] = (flg >> 16 ) & 0xFF;
    buf[14] = (flg >>  8 ) & 0xFF;
    buf[15] = (flg >>  0 ) & 0xFF;

    uint32_t cnt = Model::instance().MessageCount();
    buf[16] = (cnt >>  8 ) & 0xFF;
    buf[17] = (cnt >>  0 ) & 0xFF;

    strncpy(reinterpret_cast<char *>(&buf[42-12-12]), nam, 12);
    strncpy(reinterpret_cast<char *>(&buf[42-12   ]), msg, 12);

    SX1280::instance().LoraTxStart(buf, 42);
    
    Model::instance().IncSentMessageCount();
}

void Commands::SendDateTimeRequest() {
    SX1280::instance().LoraTxStart(reinterpret_cast<const uint8_t *>("PLEASEPLEASEDATETIMENOW!"),24);
}

void Commands::OnLEDTimer() {
    Model::instance().SetTime(system_time());

    Timeline::instance().ProcessEffect();
    if (Timeline::instance().TopEffect().Valid()) {
        Timeline::instance().TopEffect().Calc();
        Timeline::instance().TopEffect().Commit();
    }
}

void Commands::OnOLEDTimer() {
    Model::instance().SetTime(system_time());

    Timeline::instance().ProcessDisplay();
    if (Timeline::instance().TopDisplay().Valid()) {
        Timeline::instance().TopDisplay().Calc();
        Timeline::instance().TopDisplay().Commit();
    }
    
#ifdef EMULATOR
    display_debug_area(1);
#endif  // #ifdef EMULATOR
}

void Commands::OnADCTimer() {
    Model::instance().SetTime(system_time());

    Model::instance().SetBatteryVoltage(BQ25895::instance().BatteryVoltage());
    Model::instance().SetSystemVoltage(BQ25895::instance().SystemVoltage());
    Model::instance().SetVbusVoltage(BQ25895::instance().VBUSVoltage());
    Model::instance().SetChargeCurrent(BQ25895::instance().ChargeCurrent());

    BQ25895::instance().OneShotADC();
}

void Commands::Switch1_Pressed() {
    Timeline::instance().ProcessDisplay();
    if (Timeline::instance().TopDisplay().Valid()) {
        Timeline::instance().TopDisplay().ProcessSwitch1();
    }
}

void Commands::Switch2_Pressed() {
    Timeline::instance().ProcessDisplay();
    if (Timeline::instance().TopDisplay().Valid()) {
        Timeline::instance().TopDisplay().ProcessSwitch2();
    }
}

void Commands::Switch3_Pressed() {
    Timeline::instance().ProcessDisplay();
    if (Timeline::instance().TopDisplay().Valid()) {
        Timeline::instance().TopDisplay().ProcessSwitch3();
    }
}

#ifdef MCP
void Commands::OnMCPTimer_C(const timer_task *) {
    SX1280::instance().OnMCPTimer();
}
#endif  // #ifdef MCP

void Commands::OnLEDTimer_C(const timer_task *) {
    Commands::instance().OnLEDTimer();
}

void Commands::OnOLEDTimer_C(const timer_task *) {
    Commands::instance().OnOLEDTimer();
}

void Commands::OnADCTimer_C(const timer_task *) {
    Commands::instance().OnADCTimer();
}

void Commands::Switch1_EXT_C() {
    bool level = gpio_get_pin_level(SW_2_UPPER);
    if (level) {
        Commands::instance().Switch1_Pressed();
        Model::instance().SetSwitch1Down(0.0);
    } else {
        Model::instance().SetSwitch1Down(system_time());
    }
}

void Commands::Switch2_EXT_C() {
    bool level = gpio_get_pin_level(SW_2_LOWER);
    if (level) {
        Commands::instance().Switch2_Pressed();
        Model::instance().SetSwitch2Down(0.0);
    } else {
        Model::instance().SetSwitch2Down(system_time());
    }
}

void Commands::Switch3_EXT_C() {
    bool level = gpio_get_pin_level(SW_1_LOWER);
    if (level) {
        Commands::instance().Switch3_Pressed();
        Model::instance().SetSwitch3Down(0.0);
    } else {
        Model::instance().SetSwitch3Down(system_time());
    }
}

void Commands::init() {
    ext_irq_register(PIN_PA17, Switch1_EXT_C);
    ext_irq_register(PIN_PA18, Switch2_EXT_C);
    ext_irq_register(PIN_PA19, Switch3_EXT_C);
}
