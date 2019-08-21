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
#include "./ui.h"

#include <atmel_start.h>

#include <string>
#include <cmath>
#include <algorithm>

#include "./model.h"
#include "./sdd1306.h"
#include "./bq25895.h"
#include "./timeline.h"
#include "./leds.h"
#include "./commands.h"

static constexpr int32_t version_number = 1;

static constexpr size_t max_string_length = 25;

static constexpr int32_t build_number =
#include "./build_number"
;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"

#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

static void FlipAnimation(Timeline::Span *parent) {
	static Timeline::Span flipSpan;

	if (Timeline::instance().Scheduled(flipSpan)) {
		return;
	}

	flipSpan.type = Timeline::Span::Display;
	flipSpan.time = Model::instance().Time();
	flipSpan.duration = 0.25; // timeout
	flipSpan.startFunc = [=](Timeline::Span &) {
		SDD1306::instance().SetVerticalShift(0);
		SDD1306::instance().SetBootScreen(false, 0);
		SDD1306::instance().Display();
	};
	flipSpan.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
		double now = Model::instance().Time();
		float delta = 1.0f - static_cast<float>( ( (span.time + span.duration) - now ) / span.duration );
		if (delta > 0.5f) {
			below.Calc();
			delta = 1.0f - (2.0f * (delta - 0.5f));
			delta = Cubic::easeOut(delta, 0.0f, 1.0f, 1.0f);
			SDD1306::instance().SetCenterFlip(static_cast<int8_t>(48.0f * delta));
		} else {
			parent->Calc();
			delta = 1.0f - (2.0f * (0.5f - delta));
			delta = Cubic::easeIn(delta, 0.0f, 1.0f, 1.0f);
			SDD1306::instance().SetCenterFlip(static_cast<int8_t>(48.0f * delta));
		}
	};
	flipSpan.commitFunc = [=](Timeline::Span &) {
		SDD1306::instance().Display();
	};
	flipSpan.doneFunc = [=](Timeline::Span &span) {
		SDD1306::instance().SetCenterFlip(0);
		SDD1306::instance().Display();
		Timeline::instance().Remove(span);
	};
    Timeline::instance().Add(flipSpan);
}

UI &UI::instance() {
    static UI ui;
    if (!ui.initialized) {
        ui.initialized = true;
        ui.init();
    }
    return ui;
}

void UI::enterSendMessage(Timeline::Span &parent) {
    static Timeline::Span s;
    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout

    static int32_t currentMessage = 0;

    currentMessage = static_cast<int32_t>(Model::instance().SelectedMessage());

    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        if (currentMessage >= 0) {
            char str[max_string_length];
            SDD1306::instance().PlaceUTF8String(0, 0, Model::instance().Message(size_t(currentMessage)));
            snprintf(str, max_string_length, "\xc2\x88%02d/%02d \xca\xeb\xca\xec\xca\xed\xca\xee\xca\xef", static_cast<int>(currentMessage), static_cast<int>(Model::instance().MessageCount()));
            SDD1306::instance().PlaceUTF8String(0, 1, str);
        } else {
            SDD1306::instance().PlaceUTF8String(0, 0, "            ");
            SDD1306::instance().PlaceUTF8String(0, 1, "  \xca\xc3\xca\xc4\xca\xc5\xca\xc6\xca\xc7\xca\xc8\xca\xc9\xca\xca  ");
        }
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentMessage --;
        if (currentMessage < -1) {
            currentMessage = Model::instance().MessageCount() - 1;
        }
		Model::instance().SetSelectedMessage(static_cast<uint32_t>(currentMessage));
		Model::instance().save();
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentMessage ++;
        if (currentMessage >= static_cast<int32_t>(Model::instance().MessageCount())) {
            currentMessage = -1;
        }
		Model::instance().SetSelectedMessage(static_cast<uint32_t>(currentMessage));
		Model::instance().save();
    };
    s.switch3Func = [=](Timeline::Span &span) {
        Timeline::instance().Remove(span);
        Timeline::instance().ProcessDisplay();
        if (currentMessage >= 0) {
            Commands::instance().SendV3Message(
                Model::instance().Name(),
                Model::instance().Message(size_t(currentMessage)),
                Model::instance().MessageColor()
            );
        }
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}

void UI::enterChangeMessageColor(Timeline::Span &parent) {
    static Timeline::Span s;

    static int32_t currentSelection = 0;

    currentSelection = 0;

    static colors::hsv currentColor;

    currentColor = colors::hsv(colors::rgb(Model::instance().MessageColor()));
    
    led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)));

    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        char str[max_string_length];
        snprintf(str, max_string_length, " H\xc2\xd1%03d", static_cast<int>(currentColor.h * 360.f));
        SDD1306::instance().PlaceUTF8String(0, 0, str);
        snprintf(str, max_string_length, " S\xc2\xd1%03d", static_cast<int>(currentColor.s * 100.f));
        SDD1306::instance().PlaceUTF8String(6, 0, str);
        snprintf(str, max_string_length, " V\xc2\xd1%03d", static_cast<int>(currentColor.v * 100.f));
        SDD1306::instance().PlaceUTF8String(0, 1, str);
        snprintf(str, max_string_length, " \xca\xdb\xca\xdc\xca\xdd\xca\xde\xca\xdf");
        SDD1306::instance().PlaceUTF8String(6, 1, str);
        switch(currentSelection) {
            case 0: {
                SDD1306::instance().PlaceUTF8String(0, 0, "\xc2\xd0");
            } break;
            case 1: {
                SDD1306::instance().PlaceUTF8String(6, 0, "\xc2\xd0");
            } break;
            case 2: {
                SDD1306::instance().PlaceUTF8String(0, 1, "\xc2\xd0");
            } break;
            case 3: {
                snprintf(str, max_string_length, " \xca\xe1\xca\xe2\xca\xe3\xca\xe4\xca\xe5");
                SDD1306::instance().PlaceUTF8String(6, 1, str);
            } break;
        }       
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
        led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)), true);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection --;
        if (currentSelection < 0) {
            currentSelection = 3;
        }
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection ++;
        if (currentSelection > 3) {
            currentSelection = 0;
        }
    };
    
    s.switch3Func = [=](Timeline::Span &span) {
        const float step_size = 0.05f;
        span.time = Model::instance().Time(); // reset timeout
        switch(currentSelection) {
            case 0: {
                span.time = Model::instance().Time(); // reset timeout
                currentColor.h += step_size / 3.6f;
                if (currentColor.h > 1.01f) {
                    currentColor.h = 0.0f;
                }
                led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)));
            } break;
            case 1: {
                span.time = Model::instance().Time(); // reset timeout
                currentColor.s += step_size;
                if (currentColor.s > 1.01f) {
                    currentColor.s = 0.0f;
                }
                led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)));
            } break;
            case 2: {
                span.time = Model::instance().Time(); // reset timeout
                currentColor.v += step_size ;
                if (currentColor.v >= 1.01f) {
                    currentColor.v = 0.0f;
                }
                led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)));
            } break;
            case 3: {
                Model::instance().SetMessageColor(colors::rgb8(colors::rgb(currentColor)));
                Model::instance().save();
                led_control::PerformMessageColorDisplay(colors::rgb8(colors::rgb(currentColor)), true);
                Timeline::instance().Remove(span);
                Timeline::instance().ProcessDisplay();
            } break;
        }       
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}

void UI::enterChangeMessages(Timeline::Span &parent) {
    static Timeline::Span s;

    static char currentMessage[Model::MessageLength() + 1];
    
    memset(currentMessage, 0x20, Model::MessageLength() + 1);
    
    static int32_t currentChar = 0;

    currentChar = 0;
    
    static int32_t currentMode = 0;

    currentMode = 0;

    static int32_t selectedMessage = 0;

    selectedMessage = 0;
    
    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        if (currentMode == 0) {
            char str[max_string_length];
            snprintf(str, max_string_length, "%s", Model::instance().Message(static_cast<size_t>(selectedMessage)));
            SDD1306::instance().PlaceUTF8String(0, 0, str);
            snprintf(str, max_string_length, "   \xc2\x88%02d/%02d   ", static_cast<int>(selectedMessage), static_cast<int>(Model::MessageCount()));
            SDD1306::instance().PlaceUTF8String(0, 1, str);
        } else {
            char str[max_string_length];
            snprintf(str, max_string_length, "%s", currentMessage);
            SDD1306::instance().PlaceUTF8String(0, 0, str);
            if (currentChar == Model::MessageLength()) {
                SDD1306::instance().PlaceUTF8String(0, 1, "  \xca\xd3\xca\xd4\xca\xd5\xca\xd6\xca\xd7\xca\xd8\xca\xd9\xca\xda  ");
            } else {
                for (int32_t c=0; c<static_cast<int32_t>(Model::MessageLength()); c++) {
                    if (c == currentChar) {
                        SDD1306::instance().PlaceUTF8String(static_cast<uint32_t>(c), 1, "\xca\xaa");
                    } else {
                        SDD1306::instance().PlaceUTF8String(static_cast<uint32_t>(c), 1, " ");
                    }
                }
            }
        }
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        if (currentMode == 0) {
            selectedMessage --;
            if (selectedMessage < 0) {
                selectedMessage = Model::MessageCount();
            }
        } else {
            currentChar --;
            if (currentChar < 0) {
                currentChar = Model::MessageLength();
            }
        }
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        if (currentMode == 0) {
            selectedMessage ++;
            if (selectedMessage >= static_cast<int32_t>(Model::MessageCount())) {
                selectedMessage = 0;
            }
        } else {
            currentChar ++;
            if (currentChar > static_cast<int32_t>(Model::MessageLength())) {
                currentChar = 0;
            }
        }
    };
    s.switch3Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        if (currentMode == 0) {
            currentMode = 1;
            strncpy(currentMessage, Model::instance().Message(static_cast<size_t>(selectedMessage)), 12);
            for (int32_t c=0; c<12; c++) {
                currentMessage[c] = std::min(static_cast<char>(0x5f), std::max(static_cast<char>(0x20), currentMessage[c])); 
            }
        } else {
            span.time = Model::instance().Time(); // reset timeout
            if (currentChar == Model::MessageLength()) {
                for (int32_t c=11; c>=0; c--) {
                    if (currentMessage[c] == 0x20) {
                        currentMessage[c] = 0;
                    }
                }
                Model::instance().SetMessage(static_cast<size_t>(selectedMessage), currentMessage);
                Model::instance().save();
                Timeline::instance().Remove(span);
                Timeline::instance().ProcessDisplay();
            } else {
                int32_t idx = currentMessage[currentChar];
                idx = std::min(static_cast<int32_t>(0x5f), std::max(static_cast<int32_t>(0x20), idx)); 
                idx -= 0x20;
                idx ++;
                idx %= 0x40;
                idx += 0x20;
                currentMessage[currentChar] = static_cast<char>(idx);
                char str[max_string_length];
                snprintf(str, max_string_length, "%s", currentMessage);
                SDD1306::instance().PlaceUTF8String(0, 0, str);
                SDD1306::instance().Display();
            }
        }
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}

void UI::enterChangeName(Timeline::Span &parent) {
    static Timeline::Span s;

    static char currentName[max_string_length];
    
    memset(currentName, 0x20, max_string_length);
    strncpy(currentName, Model::instance().Name(), 12);
    
    for (int32_t c=0; c<12; c++) {
        currentName[c] = std::min(static_cast<char>(0x5f), std::max(static_cast<char>(0x20), currentName[c])); 
    }
    
    static int32_t currentChar = 0;

    currentChar = 0;

    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        char str[max_string_length];
        snprintf(str, max_string_length, "%s", currentName);
        SDD1306::instance().PlaceUTF8String(0, 0, str);
        if (currentChar == 12) {
            SDD1306::instance().PlaceUTF8String(0, 1, "  \xca\xd3\xca\xd4\xca\xd5\xca\xd6\xca\xd7\xca\xd8\xca\xd9\xca\xda  ");
        } else {
            for (int32_t c=0; c<12; c++) {
                if (c == currentChar) {
                    SDD1306::instance().PlaceUTF8String(static_cast<uint32_t>(c), 1, "\xca\xaa");
                } else {
                    SDD1306::instance().PlaceUTF8String(static_cast<uint32_t>(c), 1, " ");
                }
            }
        }
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentChar --;
        if (currentChar < 0) {
            currentChar = 12;
        }
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentChar ++;
        if (currentChar >= 13) {
            currentChar = 0;
        }
    };
    s.switch3Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        if (currentChar == 12) {
            for (int32_t c=11; c>=0; c--) {
                if (currentName[c] == 0x20) {
                    currentName[c] = 0;
                }
            }
            Model::instance().SetName(currentName);
            Model::instance().save();
            Timeline::instance().Remove(span);
            Timeline::instance().ProcessDisplay();
        } else {
            int32_t idx = currentName[currentChar];
            idx = std::min(static_cast<int32_t>(0x5f), std::max(static_cast<int32_t>(0x20), idx)); 
            idx -= 0x20;
            idx ++;
            idx %= 0x40;
            idx += 0x20;
            currentName[currentChar] = static_cast<char>(idx);
            char str[max_string_length];
            snprintf(str, max_string_length, "%s", currentName);
            SDD1306::instance().PlaceUTF8String(0, 0, str);
            SDD1306::instance().Display();
        }
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}

void UI::enterChangeBirdColor(Timeline::Span &parent) {
    static Timeline::Span s;

    static int32_t currentSelection = 0;

    currentSelection = 0;

    static colors::hsv currentColor;

    currentColor = colors::hsv(colors::rgb(Model::instance().BirdColor()));
    
    led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)));

    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        char str[max_string_length];
        snprintf(str, max_string_length, " H\xc2\xd1%03d", static_cast<int>(currentColor.h * 360.f));
        SDD1306::instance().PlaceUTF8String(0, 0, str);
        snprintf(str, max_string_length, " S\xc2\xd1%03d", static_cast<int>(currentColor.s * 100.f));
        SDD1306::instance().PlaceUTF8String(6, 0, str);
        snprintf(str, max_string_length, " V\xc2\xd1%03d", static_cast<int>(currentColor.v * 100.f));
        SDD1306::instance().PlaceUTF8String(0, 1, str);
        snprintf(str, max_string_length, " \xca\xdb\xca\xdc\xca\xdd\xca\xde\xca\xdf");
        SDD1306::instance().PlaceUTF8String(6, 1, str);
        switch(currentSelection) {
            case 0: {
                SDD1306::instance().PlaceUTF8String(0, 0, "\xc2\xd0");
            } break;
            case 1: {
                SDD1306::instance().PlaceUTF8String(6, 0, "\xc2\xd0");
            } break;
            case 2: {
                SDD1306::instance().PlaceUTF8String(0, 1, "\xc2\xd0");
            } break;
            case 3: {
                snprintf(str, max_string_length, " \xca\xe1\xca\xe2\xca\xe3\xca\xe4\xca\xe5");
                SDD1306::instance().PlaceUTF8String(6, 1, str);
            } break;
        }       
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
        led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)), true);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection --;
        if (currentSelection < 0) {
            currentSelection = 3;
        }
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection ++;
        if (currentSelection > 3) {
            currentSelection = 0;
        }
    };
    s.switch3Func = [=](Timeline::Span &span) {
        const float step_size = 0.05f;
        span.time = Model::instance().Time(); // reset timeout
        switch(currentSelection) {
            case 0: {
                span.time = Model::instance().Time(); // reset timeout
                currentColor.h += step_size / 3.6f;
                if (currentColor.h > 1.01f) {
                    currentColor.h = 0.0f;
                }
                led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)));
            } break;
            case 1: {
                span.time = Model::instance().Time(); // reset timeout
                currentColor.s += step_size;
                if (currentColor.s > 1.01f) {
                    currentColor.s = 0.0f;
                }
                led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)));
            } break;
            case 2: {
                span.time = Model::instance().Time(); // reset timeout
                currentColor.v += step_size;
                if (currentColor.v > 1.01f) {
                    currentColor.v = 0.0f;
                }
                led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)));
            } break;
            case 3: {
                Model::instance().SetBirdColor(colors::rgb8(colors::rgb(currentColor)));
                Model::instance().save();
                led_control::PerformColorBirdDisplay(colors::rgb8(colors::rgb(currentColor)), true);
                Timeline::instance().Remove(span);
                Timeline::instance().ProcessDisplay();
            } break;
        }       
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}

void UI::enterChangeRingColor(Timeline::Span &parent) {
    static Timeline::Span s;

    static int32_t currentSelection = 0;

    currentSelection = 0;

    static colors::hsv currentColor;

    currentColor = colors::hsv(colors::rgb(Model::instance().RingColor()));
    
    led_control::PerformColorRingDisplay(colors::rgb8(colors::rgb(currentColor)));

    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        char str[max_string_length];
        snprintf(str, max_string_length, " H\xc2\xd1%03d", static_cast<int>(currentColor.h * 360.f));
        SDD1306::instance().PlaceUTF8String(0, 0, str);
        snprintf(str, max_string_length, " S\xc2\xd1%03d", static_cast<int>(currentColor.s * 100.f));
        SDD1306::instance().PlaceUTF8String(6, 0, str);
        snprintf(str, max_string_length, " V\xc2\xd1%03d", static_cast<int>(currentColor.v * 100.f));
        SDD1306::instance().PlaceUTF8String(0, 1, str);
        snprintf(str, max_string_length, " \xca\xdb\xca\xdc\xca\xdd\xca\xde\xca\xdf");
        SDD1306::instance().PlaceUTF8String(6, 1, str);
        switch(currentSelection) {
            case 0: {
                SDD1306::instance().PlaceUTF8String(0, 0, "\xc2\xd0");
            } break;
            case 1: {
                SDD1306::instance().PlaceUTF8String(6, 0, "\xc2\xd0");
            } break;
            case 2: {
                SDD1306::instance().PlaceUTF8String(0, 1, "\xc2\xd0");
            } break;
            case 3: {
                snprintf(str, max_string_length, " \xca\xe1\xca\xe2\xca\xe3\xca\xe4\xca\xe5");
                SDD1306::instance().PlaceUTF8String(6, 1, str);
            } break;
        }       
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
        led_control::PerformColorRingDisplay(colors::rgb8(colors::rgb(currentColor)), true);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection --;
        if (currentSelection < 0) {
            currentSelection = 3;
        }
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection ++;
        if (currentSelection > 3) {
            currentSelection = 0;
        }
    };
    s.switch3Func = [=](Timeline::Span &span) {
        const float step_size = 0.05f;
        span.time = Model::instance().Time(); // reset timeout
        switch(currentSelection) {
            case 0: {
                span.time = Model::instance().Time(); // reset timeout
                currentColor.h += step_size / 3.6f;
                if (currentColor.h > 1.01f) {
                    currentColor.h = 0.0f;
                }
                led_control::PerformColorRingDisplay(colors::rgb8(colors::rgb(currentColor)));
            } break;
            case 1: {
                span.time = Model::instance().Time(); // reset timeout
                currentColor.s += step_size;
                if (currentColor.s > 1.01f) {
                    currentColor.s = 0.0f;
                }
                led_control::PerformColorRingDisplay(colors::rgb8(colors::rgb(currentColor)));
            } break;
            case 2: {
                span.time = Model::instance().Time(); // reset timeout
                currentColor.v += step_size;
                if (currentColor.v > 1.01f) {
                    currentColor.v = 0.0f;
                }
                led_control::PerformColorRingDisplay(colors::rgb8(colors::rgb(currentColor)));
            } break;
            case 3: {
                Model::instance().SetRingColor(colors::rgb8(colors::rgb(currentColor)));
                Model::instance().save();
                led_control::PerformColorRingDisplay(colors::rgb8(colors::rgb(currentColor)), true);
                Timeline::instance().Remove(span);
                Timeline::instance().ProcessDisplay();
            } break;
        }       
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}

void UI::enterRadioOnOff(Timeline::Span &parent) {
    static Timeline::Span s;
    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout


    static int32_t currentSelection = 0;
    currentSelection = Model::instance().RadioOn() ? 0 : 1;

    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        SDD1306::instance().PlaceUTF8String(0, 0, "   Radio    ");
        if (currentSelection == 0) {
			SDD1306::instance().PlaceUTF8String(0, 1, "  \xc6\x8c\xc6\x8d\xc6\x8e\xc6\x8f\xc6\x90\xc6\x91\xc6\x92\xc6\x93  ");
        } else {
			SDD1306::instance().PlaceUTF8String(0, 1, "  \xc6\x93\xc6\x94\xc6\x95\xc6\x96\xc6\x97\xc6\x98\xc6\x99\xc6\x9a  ");
        }
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection --;
        if (currentSelection < 0) {
            currentSelection = 1;
        }
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection ++;
        if (currentSelection > 1) {
            currentSelection = 0;
        }
    };
    s.switch3Func = [=](Timeline::Span &span) {
    	Model::instance().SetRadioOn(currentSelection == 0 ? true : false);
    	Model::instance().save();
        Timeline::instance().Remove(span);
        Timeline::instance().ProcessDisplay();
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}

void UI::enterFlashlight(Timeline::Span &parent) {
    static Timeline::Span s;
    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = std::numeric_limits<double>::infinity(); // timeout
	led_control::PerformFlashlight(colors::rgb8(255,255,255),false);
 	SDD1306::instance().Invert();
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
		SDD1306::instance().PlaceUTF8String(0, 0, "            ");
		SDD1306::instance().PlaceUTF8String(0, 1, "            ");
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
    };
    s.switch1Func = [=](Timeline::Span &span) {
	 	SDD1306::instance().Clear();
		led_control::PerformFlashlight(colors::rgb8(0,0,0),true);
        Timeline::instance().Remove(span);
        Timeline::instance().ProcessDisplay();
    };
    s.switch2Func = [=](Timeline::Span &span) {
	 	SDD1306::instance().Clear();
		led_control::PerformFlashlight(colors::rgb8(0,0,0),true);
        Timeline::instance().Remove(span);
        Timeline::instance().ProcessDisplay();
    };
    s.switch3Func = [=](Timeline::Span &span) {
	 	SDD1306::instance().Clear();
		led_control::PerformFlashlight(colors::rgb8(0,0,0),true);
        Timeline::instance().Remove(span);
        Timeline::instance().ProcessDisplay();
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}


void UI::enterShowVersion(Timeline::Span &parent) {
    static Timeline::Span s;
    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        char str[max_string_length];
        snprintf(str, max_string_length, "    %01d.%02d    ", static_cast<int>(version_number), static_cast<int>(build_number));
        SDD1306::instance().PlaceUTF8String(0, 0, str);
        snprintf(str, max_string_length, "%s ", __DATE__);
        SDD1306::instance().PlaceUTF8String(0, 1, str);
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        Timeline::instance().Remove(span);
        Timeline::instance().ProcessDisplay();
    };
    s.switch2Func = [=](Timeline::Span &span) {
        Timeline::instance().Remove(span);
        Timeline::instance().ProcessDisplay();
    };
    s.switch3Func = [=](Timeline::Span &span) {
        Timeline::instance().Remove(span);
        Timeline::instance().ProcessDisplay();
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}

void UI::enterDebug(Timeline::Span &parent) {
    static Timeline::Span s;

    static int32_t currentSelection = 0;

    currentSelection = 0;
    
    const int32_t maxSelection = 0x12;

    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        char str[max_string_length];
        snprintf(str, max_string_length, "%02d          ", static_cast<int>(currentSelection));
        SDD1306::instance().PlaceUTF8String(0, 0, str);
        if (currentSelection >= 0 && currentSelection < 0x12) {
            SDD1306::instance().PlaceUTF8String(5, 0, "BQ25895");
            snprintf(str, max_string_length, "%02x b", static_cast<int>(currentSelection));
            SDD1306::instance().PlaceUTF8String(0, 1, str);
            uint8_t val = BQ25895::instance().getRegister(static_cast<uint8_t>(currentSelection));
            snprintf(str, max_string_length, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(val));
            SDD1306::instance().PlaceUTF8String(4, 1, str);
        }
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection --;
        if (currentSelection < 0) {
            currentSelection = maxSelection;
        }
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection ++;
        if (currentSelection >= maxSelection) {
            currentSelection = 0;
        }
    };
    s.switch3Func = [=](Timeline::Span &span) {
        Timeline::instance().Remove(span);
        Timeline::instance().ProcessDisplay();
    };
    Timeline::instance().Remove(parent);
    Timeline::instance().Add(s);
}

void UI::enterResetEverything(Timeline::Span &parent) {
    static Timeline::Span s;

    static int32_t currentSelection = 0;

    currentSelection = 0;

    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        char str[max_string_length];
        snprintf(str, max_string_length, "Are U Sure? ");
        SDD1306::instance().PlaceUTF8String(0, 0, str);
        snprintf(str, max_string_length, "  No! ");
        SDD1306::instance().PlaceUTF8String(0, 1, str);
        snprintf(str, max_string_length, "  Yes ");
        SDD1306::instance().PlaceUTF8String(6, 1, str);
        switch(currentSelection) {
            case 0: {
                SDD1306::instance().PlaceUTF8String(0, 1, "\xc2\xd0");
            } break;
            case 1: {
                SDD1306::instance().PlaceUTF8String(6, 1, "\xc2\xd0");
            } break;
        }       
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection --;
        if (currentSelection < 0) {
            currentSelection = 1;
        }
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentSelection ++;
        if (currentSelection > 1) {
            currentSelection = 0;
        }
    };
    s.switch3Func = [=](Timeline::Span &span) {
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
    Timeline::instance().Add(s);
}

void UI::enterPrefs(Timeline::Span &) {
    static Timeline::Span s;

    static int32_t currentPage = 0;
    
    const int32_t maxPage = 9;
    
    const char *pageText[] = {
        "01/11 Send  "      // 1
        "  Message!  ",

        "02/11 Change"      // 2
        "Message Col.",

        "03/11 Change"      // 3
        "  Messages  ",

        "04/11 Change"      // 4
        "    Name    ",

        "05/11 Change"      // 5
        " Bird Color ",

        "06/11 Change"      // 6
        " Ring Color ",

        "07/11 Radio "       // 7
        "   On/Off   ",

        "08/11 Enable"      // 8
        " Flashlight ",

        "09/11 Show  "      // 8
        "  Version   ",

        "10/11 Debug "      // 9
        "Information ",

        "11/11 Reset "      // 10
        " Everything "
    };

    currentPage = 0;
    
    s.type = Timeline::Span::Display;
    s.time = Model::instance().Time();
    s.duration = 10.0; // timeout
    s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
        SDD1306::instance().PlaceUTF8String(0, 0, &pageText[currentPage][0]);
        SDD1306::instance().PlaceUTF8String(0, 1, &pageText[currentPage][12]);
    };
    s.commitFunc = [=](Timeline::Span &) {
        SDD1306::instance().Display();
    };
    s.doneFunc = [=](Timeline::Span &) {
		FlipAnimation(&s);
    };
    s.switch1Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentPage --;
        if (currentPage < 0) {
            currentPage = maxPage - 1;
        }
    };
    s.switch2Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        currentPage ++;
        if (currentPage >= maxPage) {
            currentPage = 0;
        }
    };
    s.switch3Func = [=](Timeline::Span &span) {
        span.time = Model::instance().Time(); // reset timeout
        switch (currentPage) {
            case 0: {
                enterSendMessage(span);
            } break;
            case 1: {
                enterChangeMessageColor(span);
            } break;
            case 2: {
                enterChangeMessages(span);
            } break;
            case 3: {
                enterChangeName(span);
            } break;
            case 4: {
                enterChangeBirdColor(span);
            } break;
            case 5: {
                enterChangeRingColor(span);
            } break;
            case 6: {
                enterRadioOnOff(span);
            } break;
            case 7: {
                enterFlashlight(span);
            } break;
            case 8: {
                enterShowVersion(span);
            } break;
            case 9: {
                enterDebug(span);
            } break;
            case 10: {
                enterResetEverything(span);
            } break;
        }
    };
    Timeline::instance().Add(s);
}

void UI::init() {
    if (SDD1306::instance().DevicePresent()) {
        static Timeline::Span s;
        s.type = Timeline::Span::Display;
        s.time = Model::instance().Time();
        s.duration = std::numeric_limits<double>::infinity();
        s.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
            char str[max_string_length];
            snprintf(str, max_string_length, "\xc2\x88%02d/%02d", static_cast<int>(Model::instance().Effect()), static_cast<int>(Model::instance().EffectCount()));
            SDD1306::instance().PlaceUTF8String(0, 0, str);
            if (Model::instance().DateTime() >= 0.0) {
                // display time
                int64_t dateTime = static_cast<int64_t>(Model::instance().DateTime());
                int32_t hrs = ( ( dateTime / 60 ) / 60 ) % 24;
                int32_t min = ( ( dateTime / 60 )      ) % 60;
                snprintf(str, max_string_length, "\xc2\x87%02d:%02d", static_cast<int>(hrs), static_cast<int>(min));
            } else {
				auto gc = [](int32_t ch_index) {
					switch (ch_index) {
						case 0: {
							if (BQ25895::instance().IsInFaultState()) {
								return "\xca\xa9";
							} else {
								return "\xca\xa8";
							}
						} break;
						case 1: {
							if (BQ25895::instance().ADCActive()) {
								return "\xca\xa9";
							} else {
								return "\xca\xa8";
							}
						} break;
						case 2: {
							if ((BQ25895::instance().GetStatus() & 0x18 ) == 0x08 ||
								(BQ25895::instance().GetStatus() & 0x18 ) == 0x10) {
								return "\xca\xa9";
							} else {
								return "\xca\xa8";
							}
						} break;
						case 3: {
							if ((BQ25895::instance().GetStatus() & 0xb0 ) != 0) {
								return "\xca\xa9";
							} else {
								return "\xca\xa8";
							}
						} break;
						case 4: {
							if ((BQ25895::instance().GetStatus() & 0x04) != 0) {
								return "\xca\xa9";
							} else {
								return "\xca\xa8";
							}
						} break;
					}
					return "\xca\x88";
				};
                snprintf(str, max_string_length, "\xc2\x9b%s%s%s%s%s", gc(0), gc(1), gc(2), gc(3), gc(4));
            }
            SDD1306::instance().PlaceUTF8String(6, 0, str);
            auto gc = [](int32_t ch_index, float val) {
                val *= 11.0f;
                switch (ch_index) {
                    case    0: {
                        if (val >= 2.0f) {
                            return "\xc2\x8e";
                        } else if (val >= 1.0f) {
                            return "\xc2\x8d";
                        } else {
                            return "\xc2\x8c";
                        }
                    } break;
                    case    1: {
                        if (val >= 4.0f) {
                            return "\xc2\x91";
                        } else if (val >= 3.0f) {
                            return "\xc2\x90";
                        } else {
                            return "\xc2\x8f";
                        }
                    } break;
                    case    2: {
                        if (val >= 6.0f) {
                            return "\xc2\x91";
                        } else if (val >= 5.0f) {
                            return "\xc2\x90";
                        } else {
                            return "\xc2\x8f";
                        }
                    } break;
                    case    3: {
                        if (val >= 8.0f) {
                            return "\xc2\x91";
                        } else if (val >= 7.0f) {
                            return "\xc2\x90";
                        } else {
                            return "\xc2\x8f";
                        }
                    } break;
                    case    4: {
                        if (val >= 10.0f) {
                            return "\xc2\x94";
                        } else if (val >= 9.0f) {
                            return "\xc2\x93";
                        } else {
                            return "\xc2\x92";
                        }
                    } break;
                }
                return " ";
            };
            float b = Model::instance().Brightness();
            snprintf(str, max_string_length, "\xc2\x9e%s%s%s%s%s", gc(0,b), gc(1,b), gc(2,b), gc(3,b), gc(4,b));
            SDD1306::instance().PlaceUTF8String(0, 1, str);
            float l = ( Model::instance().BatteryVoltage() - Model::instance().MinBatteryVoltage() ) / 
                      ( Model::instance().MaxBatteryVoltage() - Model::instance().MinBatteryVoltage() );
            l = std::max(0.0f, std::min(1.0f, l));
            snprintf(str, max_string_length, "\xc2\x9f%s%s%s%s%s", gc(0,l), gc(1,l), gc(2,l), gc(3,l), gc(4,l));
            SDD1306::instance().PlaceUTF8String(6, 1, str);
        };
        s.commitFunc = [=](Timeline::Span &) {
            SDD1306::instance().Display();
        };
        s.doneFunc = [=](Timeline::Span &) {
        };
        s.switch1Func = [=](Timeline::Span &) {
            Model::instance().SetEffect((Model::instance().Effect() + 1) % Model::instance().EffectCount());
            Model::instance().save();
        };
        s.switch2Func = [=](Timeline::Span &) {
            float newBrightness = Model::instance().Brightness() + 0.1f;
            if (newBrightness > 1.05f) {
                newBrightness = 0.0f;
            }
            Model::instance().SetBrightness(newBrightness);
            Model::instance().save();
        };
        s.switch3Func = [=](Timeline::Span &span) {
            enterPrefs(span);
        };
        Timeline::instance().Add(s);
    }
}
