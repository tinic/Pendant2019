#include <atmel_start.h>

#include "commands.h"
#include "model.h"
#include "sx1280.h"
#include "bq25895.h"
#include "sdd1306.h"
#include "timeline.h"
#include "leds.h"
#include "system_time.h"

#include <string.h>
#include <limits>

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

		SX1280::instance().SetTxDoneCallback([=](void) {
		});
		
		SX1280::instance().SetRxDoneCallback([=](const uint8_t *payload, uint8_t size, SX1280::PacketStatus packetStatus) {
			// Do V2 messages
			if (memcmp(payload, "DUCK!!", 6) == 0) {
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
							sprintf(str,"%8.8s : %8.8s", &payload[16], &payload[8] );
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
						span.commitFunc = [=](Timeline::Span &span) {
							SDD1306::instance().Display();
						};
						span.doneFunc = [=](Timeline::Span &span) {
							SDD1306::instance().SetVerticalShift(0);
						};
						Timeline::instance().Add(span);
					}
				}
				led_control::PerformV2MessageEffect(radio_colors[payload[7]]);
			}
		});

		SX1280::instance().SetRxErrorCallback([=](SX1280::IrqErrorCode errCode) {
		});

		SX1280::instance().SetTxTimeoutCallback([=](void) {
		});

		SX1280::instance().SetRxTimeoutCallback([=](void) {
		});

		SX1280::instance().SetRxSyncWordDoneCallback([=](void) {
		});

		SX1280::instance().SetRxHeaderDoneCallback([=](void) {
		});

		SX1280::instance().SetRangingDoneCallback([=](SX1280::IrqRangingCode errCode, float value) {
		});

		SX1280::instance().SetCadDoneCallback([=](bool cadFlag) {
		});
	}

	if (SDD1306::instance().DevicePresent()) {
		static Timeline::Span span;
		span.type = Timeline::Span::Display;
		span.time = Model::instance().CurrentTime();
		span.duration = std::numeric_limits<double>::infinity();
		span.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
			char str[64];
			sprintf(str,"%05d %05d ", int(Model::instance().CurrentBatteryVoltage() * 1000), int(Model::instance().CurrentSystemVoltage() * 1000) );
			SDD1306::instance().PlaceAsciiStr(0, 0, str);
			sprintf(str,"%05d %05d ", int(Model::instance().CurrentVbusVoltage() * 1000), int(Model::instance().CurrentChargeCurrent()) );
			SDD1306::instance().PlaceAsciiStr(0, 1, str);
		};
		span.commitFunc = [=](Timeline::Span &span) {
			SDD1306::instance().Display();
		};
		span.doneFunc = [=](Timeline::Span &span) {
			SDD1306::instance().SetAsciiScrollMessage(0,0);
		};
		Timeline::instance().Add(span);
	}
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
	timer_remove_task(&TIMER_0, &update_adc_timer_task);
	timer_stop(&TIMER_0);
}

void Commands::SendV2Message(const char *name, const char *message, uint8_t color) {
	static uint8_t buf[24];
	memcpy(&buf[0],"DUCK!!",6);
	buf[7] = color;
	memset(&buf[8],0x20,16);
	strncpy(reinterpret_cast<char *>(&buf[8]),name,8);
	strncpy(reinterpret_cast<char *>(&buf[16]),message,8);
	SX1280::instance().TxStart(buf,24);
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
	Model::instance().SetCurrentEffect((Model::instance().CurrentEffect() + 1) % Model::instance().EffectCount());
}

void Commands::Switch2_Pressed() {
}

void Commands::Switch3_Pressed() {
	Commands::instance().SendV2Message("DUCK","HELLO!",0);
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
