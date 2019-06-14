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

	if (SDD1306::instance().DevicePresent()) {
		SDD1306::instance().Clear();
		SDD1306::instance().PlaceAsciiStr(0,0,".Pendant3.0.");
		SDD1306::instance().PlaceAsciiStr(0,1,"============");
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
			// Do V2 messages
			if (size >= 24 && memcmp(payload, "UTC", 3) == 0) {
			   	struct tm tm;
			   	memset(&tm, 0, sizeof(tm));
			   	sscanf((const char *)&payload[3],"%04d-%02d-%02dT%02d:%02d:%02dZ", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
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
		   		tm.tm_min += int(modf(Model::instance().CurrentTimeZoneOffset(), &intPart));
		   		tm.tm_min %= 60;

				tm.tm_hour += 24;
		   		tm.tm_hour += int(intPart);
		   		tm.tm_hour %= 24;

				Model::instance().SetCurrentDateTime((double(tm.tm_hour) * 24.0 * 60.0 + double(tm.tm_min) * 60.0 + double(tm.tm_sec)));
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
					static Timeline::Span span;
					if (!Timeline::instance().Scheduled(span)) {
						span.type = Timeline::Span::Display;
						span.time = Model::instance().CurrentTime();
						span.duration = 8.0f;
						span.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
							char str[64];
							snprintf(str, 64, "%8.8s : %8.8s", &payload[16], &payload[8] );
							const double speed = 128.0;
							float text_walk = float(Model::instance().CurrentTime() - span.time) * speed - 96;
							float interp = 0;
							if (span.InBeginPeriod(interp, 0.5f)) {
								if (interp < 0.5) {
									SDD1306::instance().SetVerticalShift(-int32_t(interp * 2.0 * 16));
									below.Calc();
								} else {
									SDD1306::instance().SetVerticalShift(16-int32_t((interp * 2.0 - 1.0f ) * 16.0));
									SDD1306::instance().SetAsciiScrollMessage(str,text_walk);
								}
							} else 	if (span.InEndPeriod(interp, 0.5f)) {
								if (interp < 0.5) {
									SDD1306::instance().SetVerticalShift(-int32_t(interp * 2.0 * 16.0));
									SDD1306::instance().SetAsciiScrollMessage(str,text_walk);
								} else {
									SDD1306::instance().SetVerticalShift(16-int32_t((interp * 2.0 - 1.0f ) * 16));
									SDD1306::instance().SetAsciiScrollMessage(0,0);
									below.Calc();
								}
							} else {
								SDD1306::instance().SetVerticalShift(0);
								SDD1306::instance().SetAsciiScrollMessage(str,text_walk);
							}
						};
						span.commitFunc = [=](Timeline::Span &) {
							SDD1306::instance().Display();
						};
						span.doneFunc = [=](Timeline::Span &) {
							SDD1306::instance().SetVerticalShift(0);
						};
						Timeline::instance().Add(span);
					}
				}
				led_control::PerformV2MessageEffect(radio_colors[payload[7]]);
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

	timer_start(&TIMER_0);
}

void Commands::StopTimers() {
	timer_remove_task(&TIMER_0, &update_leds_timer_task);
	timer_remove_task(&TIMER_0, &update_oled_timer_task);
	timer_stop(&TIMER_0);
}

void Commands::SendV2Message(const char *name, const char *message, uint8_t color) {
	static uint8_t buf[32];
	memcpy(&buf[0],"DUCK!!",6);
	buf[7] = color;
	memset(&buf[8],0x20,16);
	strncpy(reinterpret_cast<char *>(&buf[8]),name,8);
	strncpy(reinterpret_cast<char *>(&buf[16]),message,8);
	SX1280::instance().TxStart(buf,24);
}

void Commands::SendV3Message(const char *, const char *, uint32_t) {
}

void Commands::SendDateTimeRequest() {
	SX1280::instance().TxStart((const uint8_t *)"PLEASEPLEASEDATETIMENOW!",24);
}

void Commands::OnLEDTimer() {
	Model::instance().SetCurrentTime(system_time());

	Timeline::instance().ProcessEffect();
	if (Timeline::instance().TopEffect().Valid()) {
		Timeline::instance().TopEffect().Calc();
		Timeline::instance().TopEffect().Commit();
	}
}

void Commands::OnOLEDTimer() {
	Model::instance().SetCurrentTime(system_time());

	Timeline::instance().ProcessDisplay();
	if (Timeline::instance().TopDisplay().Valid()) {
		Timeline::instance().TopDisplay().Calc();
		Timeline::instance().TopDisplay().Commit();
	}
}

void Commands::OnADCTimer() {
	Model::instance().SetCurrentTime(system_time());

	Model::instance().SetCurrentBatteryVoltage(BQ25895::instance().BatteryVoltage());
	Model::instance().SetCurrentSystemVoltage(BQ25895::instance().SystemVoltage());
	Model::instance().SetCurrentVbusVoltage(BQ25895::instance().VBUSVoltage());
	Model::instance().SetCurrentChargeCurrent(BQ25895::instance().ChargeCurrent());

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

void Commands::OnLEDTimer_C(const timer_task *) {
	Commands::instance().OnLEDTimer();
}

void Commands::OnOLEDTimer_C(const timer_task *) {
	Commands::instance().OnOLEDTimer();
}

void Commands::OnADCTimer_C(const timer_task *) {
	Commands::instance().OnADCTimer();
}

void Commands::Switch1_Pressed_C() {
	Commands::instance().Switch1_Pressed();
}

void Commands::Switch2_Pressed_C() {
	Commands::instance().Switch2_Pressed();
}

void Commands::Switch3_Pressed_C() {
	Commands::instance().Switch3_Pressed();
}

void Commands::init() {
	ext_irq_register(PIN_PA17, Switch1_Pressed_C);
	ext_irq_register(PIN_PA18, Switch2_Pressed_C);
	ext_irq_register(PIN_PA19, Switch3_Pressed_C);
}
