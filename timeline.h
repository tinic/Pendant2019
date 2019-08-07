/*
 * timeline.h
 *
 * Created: 5/10/2019 5:16:31 PM
 *  Author: Tinic Uro
 */ 


#ifndef TIMELINE_H_
#define TIMELINE_H_

#include <cstdint>
#include <functional>

class Timeline {
public:

    class Span {
    public:

        enum Type {
            None,
            Effect,
            Display,
            Message,
            Measurement
        };

        Type type = None;
        double time = 0.0;
        double duration = 0.0;

        std::function<void (Span &span)> startFunc;
        std::function<void (Span &span, Span &below)> calcFunc;
        std::function<void (Span &span)> commitFunc;
        std::function<void (Span &span)> doneFunc;

        std::function<void (Span &span)> switch1Func;
        std::function<void (Span &span)> switch2Func;
        std::function<void (Span &span)> switch3Func;

        void Start() { if (startFunc) startFunc(*this); }
        void Calc() { if (calcFunc) calcFunc(*this, Timeline::instance().Below(this, type)); }
        void Commit() { if (commitFunc) commitFunc(*this); }
        void Done() { if (doneFunc) doneFunc(*this); }
        
        void ProcessSwitch1() { if (switch1Func) switch1Func(*this); }
        void ProcessSwitch2() { if (switch2Func) switch2Func(*this); }
        void ProcessSwitch3() { if (switch3Func) switch3Func(*this); }

        bool Valid() const { return type != None; }

        bool InBeginPeriod(float &interpolation, float period_length = 0.25f);
        bool InEndPeriod(float &interpolation, float period_length = 0.25f);

    private:

        friend class Timeline;
        bool active = false;
        Span *next = 0;
    };

    static Timeline &instance();

    void Add(Timeline::Span &span);
    void Remove(Timeline::Span &span);
    bool Scheduled(Timeline::Span &span);

    void ProcessEffect();
    Span &TopEffect() const;

    void ProcessDisplay();
    Span &TopDisplay() const;

    void ProcessMessage();
    Span &TopMessage() const;

    void ProcessMeasurement();
    Span &TopMeasurement() const;

private:
    void Process(Span::Type type);
    Span &Top(Span::Type type) const;
    Span &Below(Span *context, Span::Type type) const;

    Span *head = 0;

    void init();
    bool initialized = false;
};

#endif /* TIMELINE_H_ */