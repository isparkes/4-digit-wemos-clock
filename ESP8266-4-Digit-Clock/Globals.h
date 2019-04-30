#ifndef __ESP_GLOBALS_H__
#define __ESP_GLOBALS_H__

#include "DisplayDefs.h"
#include "ClockDefs.h"

#ifdef DEBUG
#define debugMsg(x)  Serial.println (x)
#define debugMsgCont(x)  Serial.print (x)
#define setupDebug()  Serial.begin(115200)
#else
#define debugMsg(x)
#define debugMsgCont(x)
#define setupDebug()
#endif

// ------------------------ NTP ------------------------

// NTP
#define NTP_POOL_DEFAULT "pool.ntp.org"
#define NTP_PACKET_SIZE 48
#define TIME_ZONE_STRING_DEFAULT "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_UPDATE_INTERVAL_DEFAULT 7261
#define NTP_UPDATE_INTERVAL_MIN 60
#define NTP_UPDATE_INTERVAL_MAX 86400

String ntpPool = NTP_POOL_DEFAULT;
String tzs = TIME_ZONE_STRING_DEFAULT;
unsigned long lastUpdateFromServer = 0;
signed long nextUpdateFromServer = 0;
String lastTimeFromServer = "";
unsigned int ntpUpdateInterval = NTP_UPDATE_INTERVAL_DEFAULT;
unsigned long ntpStarted = 0;

// ------------------ Display Variables ----------------

byte NumberArray[DIGIT_COUNT]     = {0, 0, 0, 0};
byte currNumberArray[DIGIT_COUNT] = {0, 0, 0, 0};
byte displayType[DIGIT_COUNT]     = {FADE, FADE, FADE, FADE};
byte fadeState[DIGIT_COUNT]       = {0, 0, 0, 0};
byte valueDisplayTime             = 0;
byte valueToShow[DIGIT_COUNT / 2]   = {0, 0};
byte valueDisplayType[DIGIT_COUNT / 2] = {NORMAL | NORMAL >> 4, NORMAL | NORMAL >> 4}; // All normal by default

// ------------------- Config Variables ----------------

int minDim = MIN_DIM_DEFAULT;
// how many fade steps to increment (out of DIGIT_DISPLAY_COUNT) each impression
// 350 is about 1 second
int fadeSteps = FADE_STEPS_DEFAULT;
int digitOffCount = DIGIT_DISPLAY_OFF;
int scrollSteps = SCROLL_STEPS_DEFAULT;
boolean scrollback = true;
boolean fade = true;
byte antiGhost = ANTI_GHOST_DEFAULT;
int dispCount = DIGIT_DISPLAY_COUNT + antiGhost;
float fadeStep = DIGIT_DISPLAY_COUNT / fadeSteps;
byte blankHourStart = 0;
byte blankHourEnd = 0;
byte blankMode = 0;
boolean useLDR = true;
boolean usePIRPullup = true;
// Back light intensities
byte redCnl = COLOUR_RED_CNL_DEFAULT;
byte grnCnl = COLOUR_GRN_CNL_DEFAULT;
byte bluCnl = COLOUR_BLU_CNL_DEFAULT;
byte cycleCount = 0;
byte cycleSpeed = CYCLE_SPEED_DEFAULT;

// **************************** Auth ****************************
#define WEB_USERNAME_DEFAULT "admin";
#define WEB_PASSWORD_DEFAULT "setup";
#define WEB_AUTH_DEFAULT true;
boolean webAuthentication = WEB_AUTH_DEFAULT;
String webUsername = WEB_USERNAME_DEFAULT;
String webPassword = WEB_PASSWORD_DEFAULT;

#endif
