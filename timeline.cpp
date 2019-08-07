#include "./timeline.h"

#include <atmel_start.h>

#include <limits>
#include <array>

#include "./model.h"
#include "./sdd1306.h"

Timeline &Timeline::instance() {
    static Timeline timeline;
    if (!timeline.initialized) {
        timeline.initialized = true;
        timeline.init();
    }
    return timeline;
}

bool Timeline::Scheduled(Timeline::Span &span) {
    for (Span *i = head; i ; i = i->next) {
        if ( i == &span ) {
            return true;
        }
    }
    return false;
}

void Timeline::Add(Timeline::Span &span) {
    for (Span *i = head; i ; i = i->next) {
        if ( i == &span ) {
            return;
        }
    }

    span.next = head;
    head = &span;
}

void Timeline::Remove(Timeline::Span &span) {
    Span *p = 0;
    for (Span *i = head; i ; i = i->next) {
        if ( i == &span ) {
            if (p) {
                p->next = i->next;
            } else {
                head = i->next;
            }
            i->next = 0;
            i->Done();
            return;
        }
        p = i;
    }
}

void Timeline::Process(Span::Type type) {
    static std::array<Span *, 64> collected;
    size_t collected_num = 0;
    double time = Model::instance().Time();
    Span *p = 0;
    for (Span *i = head; i ; i = i->next) {
        if (i->type == type) {
            if ((i->time) >= time && !i->active) {
                i->active = true;
                i->Start();
            }
            if (i->duration != std::numeric_limits<double>::infinity() && ((i->time + i->duration) < time)) {
                if (p) {
                    p->next = i->next;
                } else {
                    head = i->next;
                }
                collected[collected_num++] = i;
            }
        }
        p = i;
    }
    for (size_t c = 0; c < collected_num; c++) {
        collected[c]->next = 0;
        collected[c]->Done();
    }
}

Timeline::Span &Timeline::Top(Span::Type type) const {
    static Timeline::Span empty;
    double time = Model::instance().Time();
    for (Span *i = head; i ; i = i->next) {
        if ((i->type == type) &&
            (i->time <= time) &&
            ( (i->duration == std::numeric_limits<double>::infinity()) || ((i->time + i->duration) > time) ) ) {
            return *i;
        }
    }
    return empty;
}

Timeline::Span &Timeline::Below(Span *context, Span::Type type) const {
    static Timeline::Span empty;
    double time = Model::instance().Time();
    for (Span *i = head; i ; i = i->next) {
        if (i == context) {
            continue;
        }
        if ((i->type == type) &&
            (i->time <= time) &&
            ( (i->duration == std::numeric_limits<double>::infinity()) || ((i->time + i->duration) > time) ) ) {
            return *i;
        }
    }
    return empty;
}

bool Timeline::Span::InBeginPeriod(float &interpolation, float period_length) {
    double now = Model::instance().Time();
    if ( (now - time) < period_length) {
        interpolation = (now - time) * (1.0 / period_length);
        return true;
    }
    return false;
}

bool Timeline::Span::InEndPeriod(float &interpolation, float period_length) {
    double now = Model::instance().Time();
    if ( ((time + duration) - now) < period_length) {
        interpolation = 1.0f - float( ((time + duration) - now) * (1.0 / period_length));
        return true;
    }
    return false;
}

void Timeline::ProcessEffect()
{
    return Process(Span::Effect);
}

void Timeline::ProcessDisplay()
{
    return Process(Span::Display);
}

void Timeline::ProcessMessage()
{
    return Process(Span::Message);
}

void Timeline::ProcessMeasurement()
{
    return Process(Span::Measurement);
}

Timeline::Span &Timeline::TopEffect() const
{
    return Top(Span::Effect);
}

Timeline::Span &Timeline::TopDisplay() const
{
    return Top(Span::Display);
}

Timeline::Span &Timeline::TopMessage() const
{
    return Top(Span::Message);
}

Timeline::Span &Timeline::TopMeasurement() const
{
    return Top(Span::Measurement);
}

void Timeline::init() {
}
