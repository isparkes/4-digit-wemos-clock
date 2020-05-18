/**
 * A class that displays a message by scrolling it into and out of the display
 *
 * Thanks judge!
 */

#include "Arduino.h"
#include "Transition.h"

Transition::Transition(int effectInDuration, int effectOutDuration, int holdDuration) {
  _effectInDuration = effectInDuration;
  _effectOutDuration = effectOutDuration;
  _holdDuration = holdDuration;
  _started = 0;
  _end = 0;
}

void Transition::start(unsigned long now) {
  if (_end < now) {
    _started = now;
    _end = getEnd();
  }
  // else we are already running!
}

boolean Transition::isMessageOnDisplay(unsigned long now)
{
  return (now < _end);
}

boolean Transition::scrollMsg(unsigned long now)
{
  if (now < _end) {
    int msCount = now - _started;
    if (msCount < _effectInDuration) {
      loadRegularValues();
      // Scroll -1 -> -DIGIT_COUNT
      scroll(-(msCount % _effectInDuration) * DIGIT_COUNT / _effectInDuration - 1);
    } else if (msCount < _effectInDuration * 2) {
      loadAlternateValues();
      // Scroll (DIGIT_COUNT-1) -> 0
      scroll((DIGIT_COUNT-1) - (msCount % _effectInDuration) * DIGIT_COUNT / _effectInDuration);
    } else if (msCount < _effectInDuration * 2 + _holdDuration) {
      loadAlternateValues();
    } else if (msCount < _effectInDuration * 2 + _holdDuration + _effectOutDuration) {
      loadAlternateValues();
      // Scroll 1 -> DIGIT_COUNT
      scroll(((msCount - _holdDuration) % _effectOutDuration) * DIGIT_COUNT / _effectOutDuration + 1);
    } else if (msCount < _effectInDuration * 2 + _holdDuration + _effectOutDuration * 2) {
      loadRegularValues();
      // Scroll 0 -> -(DIGIT_COUNT-1)
      scroll(((msCount - _holdDuration) % _effectOutDuration) * DIGIT_COUNT / _effectOutDuration - (DIGIT_COUNT-1));
    } else {
      _end = 0;
    }
     return true;  // we are still running
  }

  return false;   // We aren't running
}

boolean Transition::scrambleMsg(unsigned long now)
{
  if (now < _end) {
    int msCount = now - _started;
    if (msCount < _effectInDuration) {
      loadRegularValues();
      scramble(msCount, (DIGIT_COUNT-1) - (msCount % _effectInDuration) * DIGIT_COUNT / _effectInDuration, DIGIT_COUNT);
    } else if (msCount < _effectInDuration * 2) {
      loadAlternateValues();
      scramble(msCount, 0, (DIGIT_COUNT-1) - (msCount % _effectInDuration) * DIGIT_COUNT / _effectInDuration);
    } else if (msCount < _effectInDuration * 2 + _holdDuration) {
      loadAlternateValues();
    } else if (msCount < _effectInDuration * 2 + _holdDuration + _effectOutDuration) {
      loadAlternateValues();
      scramble(msCount, 0, ((msCount - _holdDuration - _effectInDuration * 2) % _effectOutDuration) * DIGIT_COUNT / _effectOutDuration + 1);
    } else if (msCount < _effectInDuration * 2 + _holdDuration + _effectOutDuration * 2) {
      loadRegularValues();
      scramble(msCount, ((msCount - _holdDuration - _effectInDuration * 2) % _effectOutDuration) * DIGIT_COUNT / _effectOutDuration + 1, DIGIT_COUNT);
    } else {
      _end = 0;
    }
    return true;  // we are still running
  }
  return false;   // We aren't running
}

boolean Transition::scrollInScrambleOut(unsigned long now)
{
  if (now < _end) {
    int msCount = now - _started;
    if (msCount < _effectInDuration) {
      loadRegularValues();
      scroll(-(msCount % _effectInDuration) * DIGIT_COUNT / _effectInDuration - 1);
    } else if (msCount < _effectInDuration * 2) {
      loadAlternateValues();
      scroll((DIGIT_COUNT-1) - (msCount % _effectInDuration) * DIGIT_COUNT / _effectInDuration);
    } else if (msCount < _effectInDuration * 2 + _holdDuration) {
      loadAlternateValues();
    } else if (msCount < _effectInDuration * 2 + _holdDuration + _effectOutDuration) {
      loadAlternateValues();
      scramble(msCount, 0, ((msCount - _holdDuration - _effectInDuration * 2) % _effectOutDuration) * DIGIT_COUNT / _effectOutDuration + 1);
    } else if (msCount < _effectInDuration * 2 + _holdDuration + _effectOutDuration * 2) {
      loadRegularValues();
      scramble(msCount, ((msCount - _holdDuration - _effectInDuration * 2) % _effectOutDuration) * DIGIT_COUNT / _effectOutDuration + 1, DIGIT_COUNT);
    } else {
      _end = 0;
   }
    return true;  // we are still running
  }
  return false;   // We aren't running
}

/**
 * +ve scroll right
 * -ve scroll left
 */
int Transition::scroll(int8_t count) {
  volatile byte copy[DIGIT_COUNT] = {0, 0, 0, 0};
  for (int i = 0 ; i < DIGIT_COUNT ; i++) copy[i] = numberArray[i];
  int8_t offset = 0;
  int8_t slope = 1;
  if (count < 0) {
    count = -count;
    offset = (DIGIT_COUNT-1);
    slope = -1;
  }
  for (byte i=0; i<DIGIT_COUNT; i++) {
    if (i < count) {
      displayType[offset + i * slope] = BLANKED;
    }
    if (i >= count) {
      numberArray[offset + i * slope] = copy[offset + (i-count) * slope];
    }
  }

  return count;
}

unsigned long Transition::hash(unsigned long x) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return x;
}

// In these functions we want something that changes quickly
// hence msCount/20. Plus it needs to be different for different
// indices, hence +i. Plus it needs to be 'random', hence hash function
int Transition::scramble(int msCount, byte start, byte end) {
  for (int i=start; i < end; i++) {
    numberArray[i] = hash(msCount / 20 + i) % 10;
  }

  return start;
}

void Transition::setRegularValues() {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) _regularDisplay[i] = numberArray[i];
}

void Transition::setAlternateValues() {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) _alternateDisplay[i] = numberArray[i];
}

void Transition::loadRegularValues() {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) numberArray[i] = _regularDisplay[i];
}

void Transition::loadAlternateValues() {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) numberArray[i] = _alternateDisplay[i];
}

unsigned long Transition::getEnd() {
  return _started + _effectInDuration * 2 + _holdDuration + _effectOutDuration * 2;
}
