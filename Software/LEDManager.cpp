//**********************************************************************************
//* ESP8266 4-Digit Nixie Clock                                                    *
//* Copyright (c) 2018-2026 Ian Sparkes. All rights reserved.                      *
//*                                                                                *
//* Free for non-commercial use. Commercial use needs a licence from the author.   *
//* See the LICENSE file in the repository root for the full terms.                *
//* Contact: nixie@protonmail.ch  |  https://www.nixieclock.biz                    *
//**********************************************************************************

#include "LEDManager.h"

// DIGIT_COUNT Backlights and DIGIT_COUNT underlights
static NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> leds(DIGIT_COUNT * 2, LED_DOUT);

void LEDManager::setUp()
{
  // Set up the LED output
  leds.Begin();
}

// ************************************************************
// recalculate "slow moving" parameters
// ************************************************************
void LEDManager::recalculateVariables() {
  _backlightDim = (float) cc->backlightDimFactor / (float) 100;
#ifdef FEATURE_UNDERLIGHTS
  _underlightDim = (float) cc->extDimFactor / (float) 100;
#endif
}

// ************************************************************
// Set the LDR dimming value
// ************************************************************
void LEDManager::setLDRValue(unsigned int ldrValue)
{
  if (cc->useBLDim) {
    // calculate the PWM factor, goes between current_config.minDim% and 100%
    _ldrDimFactor = (float) ldrValue / _ldrRange;
  }
}

// ************************************************************
// Set the LDR dimming value
// ************************************************************
void LEDManager::setLDRRange(unsigned int ldrRange)
{
    _ldrRange = (float) ldrRange;
}

// ************************************************************
// Set the pulse current value
// ************************************************************
void LEDManager::setPulseValue(unsigned int secsDelta)
{
  if (cc->useBLPulse) {
    // Calculate the brightness factor based on the "pulse"
    _pwmFactor = (float) secsDelta / (float) 1000.0;
  }
}

// ************************************************************
// Set blank status
// ************************************************************
void LEDManager::setBlanked(boolean blanked)
{
  _blanked = blanked;
}

// ************************************************************
// Set back light LEDs to the same colour
// ************************************************************
void LEDManager::setBacklightLEDs(byte red, byte green, byte blue) {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    ledRb[i] = red;
    ledGb[i] = green;
    ledBb[i] = blue;
  }
}

#ifdef FEATURE_UNDERLIGHTS
// ************************************************************
// Set under light LEDs to the same colour
// ************************************************************
void LEDManager::setUnderlightLEDs(byte red, byte green, byte blue) {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    ledRu[i] = red;
    ledGu[i] = green;
    ledBu[i] = blue;
  }
}
#endif

// ************************************************************
// Set day of week for the 'day of week' backlight mode
// ************************************************************
void LEDManager::setDayOfWeek(byte dow) {
  _dow = dow-1;
}

// ************************************************************
// Put the led buffers out
// ************************************************************
void LEDManager::outputLEDBuffer() {
#ifdef FEATURE_REVERSE_BACKLIGHTS
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    RgbColor color(ledRb[DIGIT_COUNT - 1 - i], ledGb[DIGIT_COUNT - 1 - i], ledBb[DIGIT_COUNT - 1 - i]);
    leds.SetPixelColor(i, color);
  }
#else
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    RgbColor color(ledRb[i], ledGb[i], ledBb[i]);
    leds.SetPixelColor(i, color);
  }
#endif
#ifdef FEATURE_UNDERLIGHTS
#ifdef FEATURE_REVERSE_BACKLIGHTS
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    RgbColor color(ledRu[DIGIT_COUNT - 1 - i], ledGu[DIGIT_COUNT - 1 - i], ledBu[DIGIT_COUNT - 1 - i]);
    leds.SetPixelColor(i + DIGIT_COUNT, color);
  }
#else
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    RgbColor color(ledRu[i], ledGu[i], ledBu[i]);
    leds.SetPixelColor(i + DIGIT_COUNT, color);
  }
#endif
#endif
  leds.Show();
}

// ************************************************************
// Process the options and create a new buffer
// ************************************************************
void LEDManager::processLedStatus() {
  // -------------------------------- Backlights / Underlights -------------------------------

  if (_blanked) {
    setBacklightLEDs( getLEDAdjustedBL(0),
                      getLEDAdjustedBL(0),
                      getLEDAdjustedBL(0));
#ifdef FEATURE_UNDERLIGHTS
    setUnderlightLEDs(getLEDAdjustedUL(0),
                      getLEDAdjustedUL(0),
                      getLEDAdjustedUL(0));
#endif
  } else {
    switch (cc->backlightMode) {
      case BACKLIGHT_FIXED: {
          setBacklightLEDs( getLEDAdjustedBL(rgb_backlight_curve[cc->redCnl]),
                            getLEDAdjustedBL(rgb_backlight_curve[cc->grnCnl]),
                            getLEDAdjustedBL(rgb_backlight_curve[cc->bluCnl]));
#ifdef FEATURE_UNDERLIGHTS
          setUnderlightLEDs(getLEDAdjustedUL(rgb_backlight_curve[cc->redCnl]),
                            getLEDAdjustedUL(rgb_backlight_curve[cc->grnCnl]),
                            getLEDAdjustedUL(rgb_backlight_curve[cc->bluCnl]));
#endif
          break;
        }
      case BACKLIGHT_CYCLE: {
          cycleColours3(colors);
          setBacklightLEDs( getLEDAdjustedBL(colors[0]),
                            getLEDAdjustedBL(colors[1]),
                            getLEDAdjustedBL(colors[2]));
#ifdef FEATURE_UNDERLIGHTS
          setUnderlightLEDs(getLEDAdjustedUL(colors[0]),
                            getLEDAdjustedUL(colors[1]),
                            getLEDAdjustedUL(colors[2]));
#endif
          break;
        }
      case BACKLIGHT_COLOUR_TIME: {
          if (!_syncColourTime) {
            for (byte i = 0 ; i < DIGIT_COUNT ; i++) {
              ledRb[i] = getLEDAdjustedBL(colourTimeR[numberArray[i]]);
              ledGb[i] = getLEDAdjustedBL(colourTimeG[numberArray[i]]);
              ledBb[i] = getLEDAdjustedBL(colourTimeB[numberArray[i]]);
#ifdef FEATURE_UNDERLIGHTS
              ledRu[i] = getLEDAdjustedUL(colourTimeR[numberArray[i]]);
              ledGu[i] = getLEDAdjustedUL(colourTimeG[numberArray[i]]);
              ledBu[i] = getLEDAdjustedUL(colourTimeB[numberArray[i]]);
#endif
            }
          }
          break;
        }
      case BACKLIGHT_DAY_OF_WEEK: {
          setBacklightLEDs( getLEDAdjustedBL(dayOfWeekR[_dow]),
                            getLEDAdjustedBL(dayOfWeekG[_dow]),
                            getLEDAdjustedBL(dayOfWeekB[_dow]));
#ifdef FEATURE_UNDERLIGHTS
          setUnderlightLEDs(getLEDAdjustedUL(dayOfWeekR[_dow]),
                            getLEDAdjustedUL(dayOfWeekG[_dow]),
                            getLEDAdjustedUL(dayOfWeekB[_dow]));
#endif
          break;
        }
    }
  }

  outputLEDBuffer();
}

// ************************************************************
// output a PWM LED channel, adjusting for dimming, PWM
// and user back light brightness
// ************************************************************
byte LEDManager::getLEDAdjustedBL(byte rawValue) {
  byte dimmedPWMVal;
  if (cc->useBLDim) {
    if (cc->useBLPulse) {
      dimmedPWMVal = (byte)(rawValue * _pwmFactor * _backlightDim * _ldrDimFactor);
    } else {
      dimmedPWMVal = (byte)(rawValue * _backlightDim * _ldrDimFactor);
    }
  } else {
    if (cc->useBLPulse) {
      dimmedPWMVal = (byte)(rawValue * _pwmFactor * _backlightDim);
    } else {
      dimmedPWMVal = (byte)(rawValue * _backlightDim);
    }
  }
  return dim_curve[dimmedPWMVal];
}

#ifdef FEATURE_UNDERLIGHTS
// ************************************************************
// output a PWM LED channel, adjusting for dimming, PWM
// and user under light brightness
// ************************************************************
byte LEDManager::getLEDAdjustedUL(byte rawValue) {
  byte dimmedPWMVal;
  if (cc->useBLDim) {
    if (cc->useBLPulse) {
      dimmedPWMVal = (byte)(rawValue * _pwmFactor * _underlightDim * _ldrDimFactor);
    } else {
      dimmedPWMVal = (byte)(rawValue * _underlightDim * _ldrDimFactor);
    }
  } else {
    if (cc->useBLPulse) {
      dimmedPWMVal = (byte)(rawValue * _pwmFactor * _underlightDim);
    } else {
      dimmedPWMVal = (byte)(rawValue * _underlightDim);
    }
  }
  return dim_curve[dimmedPWMVal];
}
#endif

// ************************************************************
// Colour cycling 3: one colour dominates
// ************************************************************
void LEDManager::cycleColours3(int colors[3]) {
  _cycleCount++;
  if (_cycleCount > cc->cycleSpeed) {
    _cycleCount = 0;

    if (changeSteps == 0) {
      changeSteps = random(256);
      currentColour = random(3);
    }

    changeSteps--;

    switch (currentColour) {
      case 0:
        if (colors[0] < 255) {
          colors[0]++;
          if (colors[1] > 0) {
            colors[1]--;
          }
          if (colors[2] > 0) {
            colors[2]--;
          }
        } else {
          changeSteps = 0;
        }
        break;
      case 1:
        if (colors[1] < 255) {
          colors[1]++;
          if (colors[0] > 0) {
            colors[0]--;
          }
          if (colors[2] > 0) {
            colors[2]--;
          }
        } else {
          changeSteps = 0;
        }
        break;
      case 2:
        if (colors[2] < 255) {
          colors[2]++;
          if (colors[0] > 0) {
            colors[0]--;
          }
          if (colors[1] > 0) {
            colors[1]--;
          }
        } else {
          changeSteps = 0;
        }
        break;
    }
  }
}

void LEDManager::setSyncColourTime(boolean value) {
  _syncColourTime = value;
}


// ************************************************************
// Set the diagnostic LED colour - progressively setting the
// LEDs to dignostic colours
// ************************************************************
void LEDManager::setDiagnosticLED(byte stepNumber, byte state) {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    if (i > stepNumber) {
      ledRb[i] = ledGb[i] = ledBb[i] = 0x1f;
#ifdef FEATURE_UNDERLIGHTS
      ledRu[i] = ledGu[i] = ledBu[i] = 0x1f;
#endif
    } else if (i == stepNumber) {
      if (state == STATUS_RED) {
        ledRb[i] = 0xff;
        ledGb[i] = 0;
        ledBb[i] = 0;
#ifdef FEATURE_UNDERLIGHTS
        ledRu[i] = 0xff;
        ledGu[i] = 0;
        ledBu[i] = 0;
#endif
      } else if (state == STATUS_YELLOW) {
        ledRb[i] = 0xff;
        ledGb[i] = 0x7f;
        ledBb[i] = 0x0f;
#ifdef FEATURE_UNDERLIGHTS
        ledRu[i] = 0xff;
        ledGu[i] = 0x7f;
        ledBu[i] = 0x0f;
#endif
      } else if (state == STATUS_GREEN) {
        ledRb[i] = 0;
        ledGb[i] = 0xff;
        ledBb[i] = 0;
#ifdef FEATURE_UNDERLIGHTS
        ledRu[i] = 0;
        ledGu[i] = 0xff;
        ledBu[i] = 0;
#endif
      } else if (state == STATUS_BLUE) {
        ledRb[i] = 0;
        ledGb[i] = 0;
        ledBb[i] = 0xff;
#ifdef FEATURE_UNDERLIGHTS
        ledRu[i] = 0;
        ledGu[i] = 0;
        ledBu[i] = 0xff;
#endif
      }
    }
  }
  outputLEDBuffer();
}
