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
			char str[13];
			snprintf(str, 13, "%02d/%02d Send:", int(currentMessage), int(Model::instance().MessageCount()));
			SDD1306::instance().PlaceAsciiStr(0, 0, str);
			SDD1306::instance().PlaceAsciiStr(0, 1, Model::instance().CurrentMessage(size_t(currentMessage)));
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

	static colors::hsv currentColor;

	currentColor = colors::hsv(colors::rgb(Model::instance().CurrentMessageColor()));
	
	led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)));

	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
		char str[13];
		snprintf(str, 13, " H:%03d", int(currentColor.h * 360.f));
		SDD1306::instance().PlaceAsciiStr(0, 0, str);
		snprintf(str, 13, " S:%03d", int(currentColor.s * 100.f));
		SDD1306::instance().PlaceAsciiStr(6, 0, str);
		snprintf(str, 13, " V:%03d", int(currentColor.v * 100.f));
		SDD1306::instance().PlaceAsciiStr(0, 1, str);
		snprintf(str, 13, " SAVE!");
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
		led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)), true);
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
		const float step_size = 0.05f;
		span.time = Model::instance().CurrentTime(); // reset timeout
		switch(currentSelection) {
			case 0: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				currentColor.h += step_size / 3.6f;
				if (currentColor.h > 1.01f) {
					currentColor.h = 0.0f;
				}
				led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)));
			} break;
			case 1: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				currentColor.s += step_size;
				if (currentColor.s > 1.01f) {
					currentColor.s = 0.0f;
				}
				led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)));
			} break;
			case 2: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				currentColor.v += step_size ;
				if (currentColor.v >= 1.01f) {
					currentColor.v = 0.0f;
				}
				led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)));
			} break;
			case 3: {
				Model::instance().SetCurrentMessageColor(colors::rgb8(colors::rgb(currentColor)));
				Model::instance().save();
				led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)), true);
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
		Timeline::instance().Remove(span);
		Timeline::instance().ProcessDisplay();
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
		Timeline::instance().Remove(span);
		Timeline::instance().ProcessDisplay();
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
		Timeline::instance().Remove(span);
		Timeline::instance().ProcessDisplay();
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterChangeColors(Timeline::Span &parent) {
	static Timeline::Span span;

	static int32_t currentSelection = 0;

	currentSelection = 0;

	static colors::hsv currentColor;

	currentColor = colors::hsv(colors::rgb(Model::instance().CurrentMessageColor()));
	
	led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)));

	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
		char str[13];
		snprintf(str, 13, " H:%03d", int(currentColor.h * 360.f));
		SDD1306::instance().PlaceAsciiStr(0, 0, str);
		snprintf(str, 13, " S:%03d", int(currentColor.s * 100.f));
		SDD1306::instance().PlaceAsciiStr(6, 0, str);
		snprintf(str, 13, " V:%03d", int(currentColor.v * 100.f));
		SDD1306::instance().PlaceAsciiStr(0, 1, str);
		snprintf(str, 13, " SAVE!");
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
		led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)), true);
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
		const float step_size = 0.05f;
		span.time = Model::instance().CurrentTime(); // reset timeout
		switch(currentSelection) {
			case 0: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				currentColor.h += step_size / 3.6f;
				if (currentColor.h > 1.01f) {
					currentColor.h = 0.0f;
				}
				led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)));
			} break;
			case 1: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				currentColor.s += step_size;
				if (currentColor.s > 1.01f) {
					currentColor.s = 0.0f;
				}
				led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)));
			} break;
			case 2: {
				span.time = Model::instance().CurrentTime(); // reset timeout
				currentColor.v += step_size;
				if (currentColor.v > 1.01f) {
					currentColor.v = 0.0f;
				}
				led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)));
			} break;
			case 3: {
				Model::instance().SetCurrentBirdColor(colors::rgb8(colors::rgb(currentColor)));
				Model::instance().save();
				led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)), true);
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
		Timeline::instance().Remove(span);
		Timeline::instance().ProcessDisplay();
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
		Timeline::instance().Remove(span);
		Timeline::instance().ProcessDisplay();
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
		Timeline::instance().Remove(span);
		Timeline::instance().ProcessDisplay();
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
		Timeline::instance().Remove(span);
		Timeline::instance().ProcessDisplay();
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterResetEverything(Timeline::Span &parent) {
	static Timeline::Span span;

	static int32_t currentSelection = 0;

	currentSelection = 0;

	span.type = Timeline::Span::Display;
	span.time = Model::instance().CurrentTime();
	span.duration = 10.0f; // timeout
	span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
		char str[13];
		snprintf(str, 13, "Are U Sure? ");
		SDD1306::instance().PlaceAsciiStr(0, 0, str);
		snprintf(str, 13, "  No! ");
		SDD1306::instance().PlaceAsciiStr(0, 1, str);
		snprintf(str, 13, "  Yes ");
		SDD1306::instance().PlaceAsciiStr(6, 1, str);
		switch(currentSelection) {
			case 0: {
				SDD1306::instance().PlaceAsciiStr(0, 1, ">");
			} break;
			case 1: {
				SDD1306::instance().PlaceAsciiStr(6, 1, ">");
			} break;
		}		
	};
	span.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	span.doneFunc = [=](Timeline::Span &) {
	};
	span.switch1Func = [=](Timeline::Span &) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentSelection --;
		if (currentSelection < 0) {
			currentSelection = 1;
		}
	};
	span.switch2Func = [=](Timeline::Span &) {
		span.time = Model::instance().CurrentTime(); // reset timeout
		currentSelection ++;
		if (currentSelection > 1) {
			currentSelection = 0;
		}
	};
	span.switch3Func = [=](Timeline::Span &) {
		Timeline::instance().Remove(span);
		Timeline::instance().ProcessDisplay();
		switch(currentSelection) {
			case 0: {
			} break;
			case 1: {
			} break;
		}
	};
	Timeline::instance().Remove(parent);
	Timeline::instance().Add(span);
}

void UI::enterPrefs(Timeline::Span &) {
	static Timeline::Span span;

	static int32_t currentPage = 0;
	
	const int32_t maxPage = 11;
	
	const char *pageText[] = {
		"01/11 Send  "		// 1
		"  Message!  ",

		"02/11 Change"		// 2
		"Message Col.",

		"03/11 Change"		// 3
		"    Name    ",

		"04/11 Change"		// 4
		"  Messages  ",

		"05/11 Change"		// 5
		" Bird Color ",

		"06/11  Show "		// 6
		"   History  ",

		"07/11 Range "		// 7
		"   Others   ",

		"08/11 Show  "		// 8
		" Statistics ",

		"09/11 Test  "		// 9
		"   Device   ",

		"10/11 Show  "		// 10
		"  Version   ",

		"11/11 Reset "		// 11
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
			char str[13];
			snprintf(str, 13, "#%02d/%02d", int(Model::instance().CurrentEffect()), int(Model::instance().EffectCount()));
			SDD1306::instance().PlaceAsciiStr(0, 0, str);
			if (Model::instance().CurrentDateTime() >= 0.0) {
				// display time
				int64_t dateTime = Model::instance().CurrentDateTime();
				int32_t hrs = ( ( dateTime / 1000 ) / 60 ) % 24;
				int32_t min = ( ( dateTime / 1000 )      ) % 60;
				snprintf(str, 13, "O%02d:%02d", int(hrs), int(min));
			} else {
				snprintf(str, 13, "$**__*");
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
			snprintf(str, 13, "*%c%c%c%c%c", gc(0,b), gc(1,b), gc(2,b), gc(3,b), gc(4,b));
			SDD1306::instance().PlaceAsciiStr(0, 1, str);
			float l = ( Model::instance().CurrentBatteryVoltage() - Model::instance().MinBatteryVoltage() ) / 
					  ( Model::instance().MaxBatteryVoltage() - Model::instance().MinBatteryVoltage() );
			l = std::max(0.0f, std::min(1.0f, l));
			snprintf(str, 13, "%%%c%c%c%c%c", gc(0,l), gc(1,l), gc(2,l), gc(3,l), gc(4,l));
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
