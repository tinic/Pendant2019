#ifndef UI_H_
#define UI_H_

#include <cstdint>
#include <string>

#include "./timeline.h"

class UI {
public:
    static UI &instance();

private:

    void enterPrefs(Timeline::Span &parent);
    void enterSendMessage(Timeline::Span &parent);
    void enterChangeMessageColor(Timeline::Span &parent);
    void enterShowHistory(Timeline::Span &parent);
    void enterChangeMessages(Timeline::Span &parent);
    void enterChangeName(Timeline::Span &parent);
    void enterChangeBirdColor(Timeline::Span &parent);
    void enterChangeRingColor(Timeline::Span &parent);
    void enterRangeOthers(Timeline::Span &parent);
    void enterShowStats(Timeline::Span &parent);
    void enterTestDevice(Timeline::Span &parent);
    void enterShowVersion(Timeline::Span &parent);
    void enterDebug(Timeline::Span &parent);
    void enterResetEverything(Timeline::Span &parent);
    
    void init();
    bool initialized = false;
};

#endif  // #ifndef UI_H_
