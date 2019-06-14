#include "./ui.h"

#include <atmel_start.h>

#include <string>
#include <cmath>
#include <algorithm>

#include "./model.h"
#include "./sdd1306.h"
#include "./timeline.h"
#include "./leds.h"
#include "./commands.h"

UI &UI::instance() {
	static UI ui;
	if (!ui.initialized) {
		ui.initialized = true;
		ui.init();
	}
	return ui;
}

void UI::enterSendMessage(Timeline::Span &parent) {
	static Timeline::Span span;
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout

	static int32_t currentMessage = 0;

	currentMessage = 0;

	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
		if (currentMessage >= 0) {
			SDD1306::instance().PlaceAsciiStr(0, 0, Model::instance().CurrentMessage(size_t(currentMessage)));
			SDD1306::instance().PlaceAsciiStr(0, 1, "->[ Send ]<-");
		} else {
			SDD1306::instance().PlaceAsciiStr(0, 0, "            ");
			SDD1306::instance().PlaceAsciiStr(0, 1, "->[Cancel]<-");
		}
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentMessage --;
		if (currentMessage < -1) {
			currentMessage = Model::instance().MessageCount() - 1;
		}
	};
	span.switch2Func = [=](Timeline::Span &) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentMessage ++;
		if (currentMessage >= int32_t(Model::instance().MessageCount())) {
			currentMessage = -1;
		}
	};
	span.switch3Func = [=](Timeline::Span &span) {
		Timeline::instance().Remove(span);
		Timeline::instance().ProcessDisplay();
		if (currentMessage >= 0) {
			Commands::instance().SendV3Message(
				Model::instance().CurrentName(),
				Model::instance().CurrentMessage(size_t(currentMessage)),
				Model::instance().CurrentMessageColor()
			);
		}
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterMessageColor(Timeline::Span &parent) {
	static Timeline::Span span;

	static int32_t currentSelection = 0;

	currentSelection = 0;

	static uint32_t currentColor = 0;

	currentColor = Model::instance().CurrentMessageColor();
	
	led_control::PerformMessageColorDisplay(currentColor);

	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
		char str[12];
		snprintf(str, 12, " R:%03d", int((Model::instance().CurrentMessageColor() >> 16) & 0xFF));
		SDD1306::instance().PlaceAsciiStr(0, 0, str);
		snprintf(str, 12, " G:%03d", int((Model::instance().CurrentMessageColor() >>  8) & 0xFF));
		SDD1306::instance().PlaceAsciiStr(6, 0, str);
		snprintf(str, 12, " B:%03d", int((Model::instance().CurrentMessageColor() >>  0) & 0xFF));
		SDD1306::instance().PlaceAsciiStr(0, 1, str);
		snprintf(str, 12, " SAVE!");
		SDD1306::instance().PlaceAsciiStr(6, 1, str);
		switch(currentSelection) {
			case 0: {
				SDD1306::instance().PlaceAsciiStr(0, 0, ">");
			} break;
			case 1: {
				SDD1306::instance().PlaceAsciiStr(6, 0, ">");
			} break;
			case 2: {
				SDD1306::instance().PlaceAsciiStr(0, 1, ">");
			} break;
			case 3: {
				SDD1306::instance().PlaceAsciiStr(6, 1, ">");
			} break;
		}		
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
		led_control::PerformMessageColorDisplay(currentColor, true);
	};
	span.switch1Func = [=](Timeline::Span &) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentSelection --;
		if (currentSelection < 0) {
			currentSelection = 3;
		}
	};
	span.switch2Func = [=](Timeline::Span &) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentSelection ++;
		if (currentSelection > 3) {
			currentSelection = 0;
		}
	};
	span.switch3Func = [=](Timeline::Span &span) {
		switch(currentSelection) {
			case 0: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				uint8_t comp = (currentColor >> 16) & 0xFF;
				comp += 16;
				currentColor = (currentColor & 0x00FFFF) | uint32_t(comp);
				led_control::PerformMessageColorDisplay(currentColor);
			} break;
			case 1: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				uint8_t comp = (currentColor >>  8) & 0xFF;
				comp += 16;
				currentColor = (currentColor & 0xFF00FF) | uint32_t(comp);
				led_control::PerformMessageColorDisplay(currentColor);
			} break;
			case 2: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				uint8_t comp = (currentColor >>  0) & 0xFF;
				comp += 16;
				currentColor = (currentColor & 0xFFFF00) | uint32_t(comp);
				led_control::PerformMessageColorDisplay(currentColor);
			} break;
			case 3: {
				Model::instance().SetCurrentMessageColor(currentColor);
				Model::instance().save();
				led_control::PerformMessageColorDisplay(currentColor, true);
				Timeline::instance().Remove(span);
				Timeline::instance().ProcessDisplay();
			} break;
		}		
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterShowHistory(Timeline::Span &parent) {
	static Timeline::Span span;
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
	};
	span.switch2Func = [=](Timeline::Span &) {
	};
	span.switch3Func = [=](Timeline::Span &) {
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterChangeMessages(Timeline::Span &parent) {
	static Timeline::Span span;
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
	};
	span.switch2Func = [=](Timeline::Span &) {
	};
	span.switch3Func = [=](Timeline::Span &) {
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterChangeName(Timeline::Span &parent) {
	static Timeline::Span span;
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
	};
	span.switch2Func = [=](Timeline::Span &) {
	};
	span.switch3Func = [=](Timeline::Span &) {
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterChangeColors(Timeline::Span &parent) {
	static Timeline::Span span;

	static int32_t currentSelection = 0;

	currentSelection = 0;

	static uint32_t currentColor = 0;

	currentColor = Model::instance().CurrentBirdColor();
	
	led_control::PerformColorBirdDisplay(currentColor);

	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
		char str[12];
		snprintf(str, 12, " R:%03d", int((Model::instance().CurrentBirdColor() >> 16) & 0xFF));
		SDD1306::instance().PlaceAsciiStr(0, 0, str);
		snprintf(str, 12, " G:%03d", int((Model::instance().CurrentBirdColor() >>  8) & 0xFF));
		SDD1306::instance().PlaceAsciiStr(6, 0, str);
		snprintf(str, 12, " B:%03d", int((Model::instance().CurrentBirdColor() >>  0) & 0xFF));
		SDD1306::instance().PlaceAsciiStr(0, 1, str);
		snprintf(str, 12, " SAVE!");
		SDD1306::instance().PlaceAsciiStr(6, 1, str);
		switch(currentSelection) {
			case 0: {
				SDD1306::instance().PlaceAsciiStr(0, 0, ">");
			} break;
			case 1: {
				SDD1306::instance().PlaceAsciiStr(6, 0, ">");
			} break;
			case 2: {
				SDD1306::instance().PlaceAsciiStr(0, 1, ">");
			} break;
			case 3: {
				SDD1306::instance().PlaceAsciiStr(6, 1, ">");
			} break;
		}		
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
		led_control::PerformColorBirdDisplay(currentColor, true);
	};
	span.switch1Func = [=](Timeline::Span &) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentSelection --;
		if (currentSelection < 0) {
			currentSelection = 3;
		}
	};
	span.switch2Func = [=](Timeline::Span &) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentSelection ++;
		if (currentSelection > 3) {
			currentSelection = 0;
		}
	};
	span.switch3Func = [=](Timeline::Span &span) {
		switch(currentSelection) {
			case 0: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				uint8_t comp = (currentColor >> 16) & 0xFF;
				comp += 16;
				currentColor = (currentColor & 0x00FFFF) | uint32_t(comp);
				led_control::PerformColorBirdDisplay(currentColor);
			} break;
			case 1: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				uint8_t comp = (currentColor >>  8) & 0xFF;
				comp += 16;
				currentColor = (currentColor & 0xFF00FF) | uint32_t(comp);
				led_control::PerformColorBirdDisplay(currentColor);
			} break;
			case 2: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				uint8_t comp = (currentColor >>  0) & 0xFF;
				comp += 16;
				currentColor = (currentColor & 0xFFFF00) | uint32_t(comp);
				led_control::PerformColorBirdDisplay(currentColor);
			} break;
			case 3: {
				Model::instance().SetCurrentBirdColor(currentColor);
				Model::instance().save();
				led_control::PerformColorBirdDisplay(currentColor, true);
				Timeline::instance().Remove(span);
				Timeline::instance().ProcessDisplay();
			} break;
		}		
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterRangeOthers(Timeline::Span &parent) {
	static Timeline::Span span;
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
	};
	span.switch2Func = [=](Timeline::Span &) {
	};
	span.switch3Func = [=](Timeline::Span &) {
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterShowStats(Timeline::Span &parent) {
	static Timeline::Span span;
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
	};
	span.switch2Func = [=](Timeline::Span &) {
	};
	span.switch3Func = [=](Timeline::Span &) {
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterTestDevice(Timeline::Span &parent) {
	static Timeline::Span span;
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
	};
	span.switch2Func = [=](Timeline::Span &) {
	};
	span.switch3Func = [=](Timeline::Span &) {
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterShowVersion(Timeline::Span &parent) {
	static Timeline::Span span;
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
	};
	span.switch2Func = [=](Timeline::Span &) {
	};
	span.switch3Func = [=](Timeline::Span &) {
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterResetEverything(Timeline::Span &parent) {
	static Timeline::Span span;
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
	};
	span.switch2Func = [=](Timeline::Span &) {
	};
	span.switch3Func = [=](Timeline::Span &) {
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterPrefs(Timeline::Span &) {
	static Timeline::Span span;

	static int32_t currentPage = 0;
	
	const int32_t maxPage = 11;
	
	const char *pageText[] = {
		"    Send    "		// 1
		"  Message!  ",

		"   Change   "		// 2
		"Message Col.",

		"   Change   "		// 3
		"    Name    ",

		"   Change   "		// 4
		"  Messages  ",

		"   Change   "		// 5
		" Bird Color ",

		"    Show    "		// 6
		"   History  ",

		"   Range    "		// 7
		"   Others   ",

		"    Show    "		// 8
		" Statistics ",

		"    Test    "		// 9
		"   Device   ",

		"    Show    "		// 10
		"  Version   ",

		"   Reset    "		// 11
		" Everything "
	};

	currentPage = 0;
	
	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
		SDD1306::instance().PlaceAsciiStr(0, 0, &pageText[currentPage][0]);
		SDD1306::instance().PlaceAsciiStr(0, 1, &pageText[currentPage][12]);
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &span) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentPage --;
		if (currentPage < 0) {
			currentPage = maxPage - 1;
		}
	};
	span.switch2Func = [=](Timeline::Span &span) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentPage ++;
		if (currentPage >= maxPage) {
			currentPage = 0;
		}
	};
	span.switch3Func = [=](Timeline::Span &span) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		switch (currentPage) {
			case 0: {
				enterSendMessage(span);
			} break;
			case 1: {
				enterMessageColor(span);
			} break;
			case 2: {
				enterChangeName(span);
			} break;
			case 3: {
				enterChangeMessages(span);
			} break;
			case 4: {
				enterChangeColors(span);
			} break;
			case 5: {
				enterShowHistory(span);
			} break;
			case 6: {
				enterRangeOthers(span);
			} break;
			case 7: {
				enterShowStats(span);
			} break;
			case 8: {
				enterTestDevice(span);
			} break;
			case 9: {
				enterShowVersion(span);
			} break;
			case 10: {
				enterResetEverything(span);
			} break;
		}
	};
	Timeline::instance().Add(span);
}

void UI::init() {
	if (SDD1306::instance().DevicePresent()) {
		static Timeline::Span span;
		span.type = Timeline::Span::Display;
		span.time = Model::instance().CurrentTime();
		span.duration = std::numeric_limits<double>::infinity();
		span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
			char str[12];
			snprintf(str, 12, "#%02d/%02d", int(Model::instance().CurrentEffect()), int(Model::instance().EffectCount()));
			SDD1306::instance().PlaceAsciiStr(0, 0, str);
			if (Model::instance().CurrentDateTime() >= 0.0) {
				// display time
				int64_t dateTime = Model::instance().CurrentDateTime();
				int32_t hrs = ( ( dateTime / 1000 ) / 60 ) % 24;
				int32_t min = ( ( dateTime / 1000 )      ) % 60;
				snprintf(str, 12, "O%02d:%02d", int(hrs), int(min));
			} else {
				snprintf(str, 12, "DUCKPD");
			}
			SDD1306::instance().PlaceAsciiStr(6, 0, str);
			auto gc = [](int32_t ch_index, float val) {
				val *= 6.0f;
				switch (ch_index) {
					case	0: {
						if (val >= 1.0f) {
							return '=';
						} else {
							return ' ';
						}
					} break;
					case	1: {
						if (val >= 2.0f) {
							return '=';
						} else {
							return ' ';
						}
					} break;
					case	2: {
						if (val >= 3.0f) {
							return '=';
						} else {
							return ' ';
						}
					} break;
					case	3: {
						if (val >= 4.0f) {
							return '=';
						} else {
							return ' ';
						}
					} break;
					case	4: {
						if (val >= 5.0f) {
							return '=';
						} else {
							return ' ';
						}
					} break;
				}
				return ' ';
			};
			float b = Model::instance().CurrentBrightness();
			snprintf(str, 12, "*%c%c%c%c%c", gc(0,b), gc(1,b), gc(2,b), gc(3,b), gc(4,b));
			SDD1306::instance().PlaceAsciiStr(0, 1, str);
			float l = ( Model::instance().CurrentBatteryVoltage()    - Model::instance().MinBatteryVoltage() ) / 
						Model::instance().MaxBatteryVoltage() - Model::instance().MinBatteryVoltage();
			l = std::max(0.0f, std::min(1.0f, l));
			snprintf(str, 12, "%%%c%c%c%c%c", gc(0,l), gc(1,l), gc(2,l), gc(3,l), gc(4,l));
			SDD1306::instance().PlaceAsciiStr(6, 1, str);
		};
		span.commitFunc = [=](Timeline::Span &) {
			SDD1306::instance().Display();
		};
		span.doneFunc = [=](Timeline::Span &) {
		};
		span.switch1Func = [=](Timeline::Span &) {
			Model::instance().SetCurrentEffect((Model::instance().CurrentEffect() + 1) % Model::instance().EffectCount());
			Model::instance().save();
		};
		span.switch2Func = [=](Timeline::Span &) {
			float newBrightness = Model::instance().CurrentBrightness() + 0.1f;
			if (newBrightness > 1.0f) {
				newBrightness = 0.0f;
			}
			Model::instance().SetCurrentBrightness(newBrightness);
			Model::instance().save();
		};
		span.switch3Func = [=](Timeline::Span &span) {
			enterPrefs(span);
		};
		Timeline::instance().Add(span);
	}
}
