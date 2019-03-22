#ifndef Transition_h
#define Transition_h

#include "Arduino.h"
#include "DisplayDefs.h"

extern byte NumberArray[];
extern byte displayType[];
extern boolean scrollback;

class Transition
{
  public:
    Transition(int, int, int);
    void start(unsigned long);
    boolean isMessageOnDisplay(unsigned long);
    boolean scrollMsg(unsigned long);
    boolean scrambleMsg(unsigned long);
    boolean scrollInScrambleOut(unsigned long);
    void setRegularValues();
    void setAlternateValues();
    void loadRegularValues();
    void loadAlternateValues();
    void saveCurrentDisplayType();
    void restoreCurrentDisplayType();
  private:
    int _effectInDuration;
    int _effectOutDuration;
    int _holdDuration;
    unsigned long _started;
    unsigned long _end;
    byte _regularDisplay[DIGIT_COUNT] = {0, 0, 0, 0};
    byte _alternateDisplay[DIGIT_COUNT] = {0, 0, 0, 0};
    boolean _savedScrollback;
    byte _savedDisplayType[DIGIT_COUNT] = {FADE, FADE, FADE, FADE};
  
    unsigned long getEnd();
    int scroll(int8_t);
    int scramble(int, byte, byte);
    unsigned long hash(unsigned long);
};

#endif
