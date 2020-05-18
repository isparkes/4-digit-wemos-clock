#ifndef __ESP_GLOBALS_H__
#define __ESP_GLOBALS_H__

#include "HtmlServer.h"
#include "DisplayDefs.h"
#include "ClockDefs.h"
#include "Transition.h"
#include "ESP_DS1307.h"
#include "ClockButton.h"

// ----------------------- Components ----------------------------

Transition transition(500, 1000, 3000); // Scroll/Scramble (Transition.h)  
DS1307 Clock;                           // RTC, uses Analogue pins A4 (SDA) and A5 (SCL)
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> leds(DIGIT_COUNT, LED_DOUT);
ClockButton button1(inputPin1, true);   // Clock button debounce and handling

// ------------------------ NTP ------------------------

#define TIME_ZONE_STRING_DEFAULT "CET-1CEST,M3.5.0,M10.5.0/3"

// ------------------ Display Variables ----------------

#define ISR_STEPS   20

byte numberArray[DIGIT_COUNT]          = {0, 0, 0, 0};
byte currNumberArray[DIGIT_COUNT]      = {0, 0, 0, 0};
byte displayType[DIGIT_COUNT]          = {NORMAL, NORMAL, NORMAL, NORMAL};
byte fadeState[DIGIT_COUNT]            = {0, 0, 0, 0};
byte valueDisplayTime                  = 0;
int valueToShow                        = 0;
byte valueDisplayType[DIGIT_COUNT / 2] = {NORMAL | NORMAL << 4, NORMAL | NORMAL << 4}; // All normal by default
byte ledsOffTime;

// ------------------- Config Variables ----------------

boolean hourMode = false;     // 24H
int minDim = MIN_DIM_DEFAULT;

// how many fade steps to increment (out of DIGIT_DISPLAY_COUNT) each impression
// 350 is about 1 second
int fadeSteps = ISR_STEPS;
int digitOffCount = ISR_STEPS;
int scrollSteps = SCROLL_STEPS_DEFAULT;
boolean scrollback = true;
boolean fade = true;
byte antiGhost = ANTI_GHOST_DEFAULT;
float fadeStep = ISR_STEPS / fadeSteps;
byte dayBlanking = DAY_BLANKING_DEFAULT;
byte blankHourStart = 0;
byte blankHourEnd = 0;
byte blankMode = 0;
boolean useLDR = true;
boolean usePIRPullup = true;
byte backlightMode = BACKLIGHT_DEFAULT;
byte redCnl = COLOUR_RED_CNL_DEFAULT;
byte grnCnl = COLOUR_GRN_CNL_DEFAULT;
byte bluCnl = COLOUR_BLU_CNL_DEFAULT;
byte cycleCount = 0;
byte cycleSpeed = CYCLE_SPEED_DEFAULT;
boolean testMode = false;
byte slotsMode = SLOTS_MODE_DEFAULT;
int pirTimeout = PIR_TIMEOUT_DEFAULT;
boolean blankLeading = false;
byte dateFormat = DATE_FORMAT_DEFAULT;
byte ledMode = 0;       // Separator LED mode
byte separatorDimFactor = SEP_DIM_DEFAULT;
byte backlightDimFactor = BACKLIGHT_DIM_FACTOR_DEFAULT;

// Cache values for display smoothness
byte disp_hour;
byte disp_min;
byte disp_sec;
byte disp_day;
byte disp_month;

// ------------- Time management variables -------------

unsigned long nowMillis = 0;
unsigned long lastCheckMillis = 0;

byte currentMode = MODE_TIME;   // Initial cold start mode
byte nextMode = currentMode;
boolean triggeredThisSec = false;

unsigned int tempDisplayModeDuration;      // time for the end of the temporary display
int  tempDisplayMode;

// ---------------- Backlight management ---------------

int colors[3];
byte ledR[DIGIT_COUNT];
byte ledG[DIGIT_COUNT];
byte ledB[DIGIT_COUNT];

// ------------------ Usage statistics -----------------

unsigned long uptimeMins = 0;
unsigned long tubeOnTimeMins = 0;
int impressionsPerSec = 0;
int lastImpressionsPerSec = 0;

// ----------------- Real time clock -------------------

byte useRTC = false;  // true if we detect an RTC
boolean onceHadAnRTC = false;

// ----------------- Motion detector -------------------

unsigned long pirLastSeen = 0;
boolean pirInstalled = false;
int pirConsecutiveCounts = 300;  // set with a high value to stop the PIR being falsely detected the first time round the loop

// --------------------- Blanking ----------------------

boolean blanked = false;
byte blankSuppressStep = 0;    // The press we are on: 1 press = suppress for 1 min, 2 press = 1 hour, 3 = 1 day
unsigned long blankSuppressedMillis = 0;   // The end time of the blanking, 0 if we are not suppressed
unsigned long blankSuppressedSelectionTimoutMillis = 0;   // Used for determining the end of the blanking period selection timeout
boolean blankTubes = false;
boolean blankLEDs = false;
boolean blankSeparators = false;

// --------------- Ambient light dimming ---------------

int dimDark = SENSOR_LOW_DEFAULT;
int dimBright = SENSOR_HIGH_DEFAULT;
double sensorLDRSmoothed = 0;
int sensorSmoothCountLDR = SENSOR_SMOOTH_READINGS_DEFAULT;

boolean blinkState;

// ------------------- LED management ------------------

boolean upOrDown;

// Used to control the separator LEDs
boolean led1State;
boolean led2State;
boolean ampm = false;

// Strategy 3
int changeSteps = 0;
byte currentColour = 0;

#endif
