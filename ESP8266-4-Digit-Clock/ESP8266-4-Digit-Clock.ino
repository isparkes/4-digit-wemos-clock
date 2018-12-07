//**********************************************************************************
//* Main code for an ESP8266 based Nixie/VFD clock. Features:                      *
//*  - WiFi Clock interface for the WiFiModule  OR                                 *
//*  - Real Time Clock interface for DS1307+                                       *
//*  - Digit fading with configurable fade length                                  *
//*  - Digit scrollback with configurable scroll speed                             *
//*  - Configuration stored in Flash (JSON)                                        *
//*  - Low hardware component count (as much as possible done in code)             *
//*  - Single button operation with software debounce                              *
//*  - Automatic dimming, using a Light Dependent Resistor                         *
//*  - RGB back light management using individually addressable WS2812B            *
//*  - PIR sensor to turn off display when no one is around                        *
//*   Program with following settings (status line / IDE):                         *
//*                                                                                *
//*    Board: Wemos D1 R2 & Mini                                                   *
//*    Crystal Frequency: 26MHz,                                                   *
//*    Flash: 80MHz,                                                               *
//*    CPU: 160MHz,                                                                *
//*    Flash Mode: QIO,                                                            *
//*    Upload speed: 115200,                                                       *
//*    Flash size: 4M (1M SPIFFS),                                                 *
//*    Reset method: ck, Disabled, none                                            *
//*    Erase Flash: All flash contents,                                            *
//*    Builtin LED: 1                                                              *
//*                                                                                *
//*  nixie@protonmail.ch                                                           *
//*                                                                                *
//**********************************************************************************
//**********************************************************************************
// Standard Libraries
#include <FS.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>

// Clock specific libraries, install these with "Sketch -> Include Library -> Add .ZIP library
// using the ZIP files in the "libraries" directory
#include <TimeLib.h>            // http://playground.arduino.cc/code/time (Margolis 1.5.0)
// #include <Timezone.h>           // https://github.com/JChristensen/Timezone (Christensen 1.2.1) for NTP time management
#include <NeoPixelBus.h>        // https://github.com/Makuna/NeoPixelBus (Makuna 2.3.4)
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager  (tzapu 0.14.0)
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson (bblanchon 5.13.2)

// Other parts of the code, broken out for clarity
#include "ClockButton.h"
#include "Transition.h"
#include "DisplayDefs.h"
#include "VFDDefs.h"
#include "ClockUtils.h"
#include "ClockDefs.h"
#include "ESP_DS1307.h"

// Software version shown in config menu
#define SOFTWARE_VERSION    1

#define DEBUG

#ifdef DEBUG
 #define debugMsg(x)  Serial.println (x)
 #define debugMsgCont(x)  Serial.print (x)
 #define setupDebug()  Serial.begin(115200)
#else
 #define debugMsg(x)
 #define setupDebug() 
#endif

// ------------------------ Wifi ------------------------

#define DEFAULT_TIME_SERVER_URL_1 "http://its.internet-box.ch/getTime/Europe/Zurich"
#define DEFAULT_TIME_SERVER_URL_2 "http://time-zone-server.scapp.io/getTime/Europe/Zurich"

// used for flashing the blue LED
int blinkOnTime = 1000;
int blinkTopTime = 2000;
unsigned long lastMillis = 0;

String timeServerURL = DEFAULT_TIME_SERVER_URL_1;

ESP8266WebServer server(80);

//**********************************************************************************
//**********************************************************************************
//*                               Variables                                        *
//**********************************************************************************
//**********************************************************************************

// ***** Pin Defintions ****** Pin Defintions ****** Pin Defintions ******

#define inputPin1   0     // D3
#define pirPin      2     // D4
#define LED_DOUT    3     // RX defined by DMA output of library
#define LDRPin      A0    // ADC input

// Shift register
#define LATCHPin    14    // D5
#define CLOCKPin    12    // D6
#define LEDPin1     15    // D8
#define LEDPin2     16    // D0
#define DATA1Pin    13    // D7 - Grids

//**********************************************************************************
Transition transition(500, 1000, 3000);
//**********************************************************************************

byte NumberArray[DIGIT_COUNT]    = {0, 0, 0, 0};
byte currNumberArray[DIGIT_COUNT] = {0, 0, 0, 0};
byte displayType[DIGIT_COUNT]    = {FADE, FADE, FADE, FADE};
byte fadeState[DIGIT_COUNT]      = {0, 0, 0, 0};
byte valueDisplayTime  = 0;
byte valueToShow[DIGIT_COUNT/2]    = {0, 0};
byte valueDisplayType[DIGIT_COUNT/2] = {0x33, 0x33}; // All normal by default

// how many fade steps to increment (out of DIGIT_DISPLAY_COUNT) each impression
// 100 is about 1 second
int fadeSteps = FADE_STEPS_DEFAULT;
int digitOffCount = DIGIT_DISPLAY_OFF;
int scrollSteps = SCROLL_STEPS_DEFAULT;
boolean scrollback = true;
boolean fade = true;
byte antiGhost = ANTI_GHOST_DEFAULT;
int dispCount = DIGIT_DISPLAY_COUNT + antiGhost;
float fadeStep = DIGIT_DISPLAY_COUNT / fadeSteps;

// For software blinking
int blinkCounter = 0;
boolean blinkState = true;

// leading digit blanking
boolean blankLeading = false;

// Dimming value
const int DIM_VALUE = DIGIT_DISPLAY_COUNT / 5;
int minDim = MIN_DIM_DEFAULT;

unsigned int tempDisplayModeDuration;      // time for the end of the temporary display
int  tempDisplayMode;

byte slotsMode = SLOTS_MODE_DEFAULT;

byte currentMode = MODE_TIME;   // Initial cold start mode
byte nextMode = currentMode;
byte blankHourStart = 0;
byte blankHourEnd = 0;
byte blankMode = 0;
boolean blankTubes = false;
boolean blankLEDs = false;

boolean testMode = false;

// ************************ Ambient light dimming ************************
int dimDark = SENSOR_LOW_DEFAULT;
int dimBright = SENSOR_HIGH_DEFAULT;
double sensorLDRSmoothed = 0;
double sensorFactor = (double)(DIGIT_DISPLAY_OFF) / (double)(dimBright - dimDark);
int sensorSmoothCountLDR = SENSOR_SMOOTH_READINGS_DEFAULT;
int sensorSmoothCountHV = SENSOR_SMOOTH_READINGS_DEFAULT / 8;
boolean useLDR = true;

// ************************ Clock variables ************************
// RTC, uses Analogue pins A4 (SDA) and A5 (SCL)
DS1307 Clock;

// State variables for detecting changes
byte lastSec;
unsigned long nowMillis = 0;
unsigned long lastCheckMillis = 0;

byte dateFormat = DATE_FORMAT_DEFAULT;
byte dayBlanking = DAY_BLANKING_DEFAULT;
boolean blanked = false;
byte blankSuppressStep = 0;    // The press we are on: 1 press = suppress for 1 min, 2 press = 1 hour, 3 = 1 day
unsigned long blankSuppressedMillis = 0;   // The end time of the blanking, 0 if we are not suppressed
unsigned long blankSuppressedSelectionTimoutMillis = 0;   // Used for determining the end of the blanking period selection timeout

boolean hourMode = false; // 24H

// PIR
unsigned long pirTimeout = PIR_TIMEOUT_DEFAULT;
unsigned long pirLastSeen = 0;
boolean usePIRPullup = true;

boolean triggeredThisSec = false;

byte useRTC = false;  // true if we detect an RTC
byte useWiFi = 0; // the number of minutes ago we recevied information from the WiFi module, 0 = don't use WiFi
boolean onceHadAnRTC = false;

// **************************** LED management ***************************
boolean upOrDown;

// Used to control the separator LEDs
boolean led1State;
boolean led2State;

byte ledMode = 0;       // Separator LED mode

// Blinking colons led in settings modes
int ledBlinkCtr = 0;
int ledBlinkNumber = 0;

byte backlightMode = BACKLIGHT_DEFAULT;

// Back light intensities
byte redCnl = COLOUR_RED_CNL_DEFAULT;
byte grnCnl = COLOUR_GRN_CNL_DEFAULT;
byte bluCnl = COLOUR_BLU_CNL_DEFAULT;
byte cycleCount = 0;
byte cycleSpeed = CYCLE_SPEED_DEFAULT;

int colors[3];

// individual channel colours for the LEDs
byte ledR[DIGIT_COUNT];
byte ledG[DIGIT_COUNT];
byte ledB[DIGIT_COUNT];

// set up the NeoPixel library
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> leds(DIGIT_COUNT, LED_DOUT);

// Strategy 3
int changeSteps = 0;
byte currentColour = 0;

int impressionsPerSec = 0;
int lastImpressionsPerSec = 0;

long lastUpdateFromServer = 0;
String lastTimeFromServer = "";

// ********************** Input switch management **********************
ClockButton button1(inputPin1, true);

// **************************** Usage stats ****************************

unsigned long uptimeMins = 0;
unsigned long tubeOnTimeMins = 0;

//**********************************************************************************

//**********************************************************************************
//**********************************************************************************
//*                                    Setup                                       *
//**********************************************************************************
//**********************************************************************************
void setup()
{
  setupDebug();

  // Set up the LED output
  leds.Begin();

  // Set up shift registers
  pinMode(LATCHPin, OUTPUT);
  pinMode(CLOCKPin, OUTPUT);
  pinMode(DATA1Pin, OUTPUT);
  pinMode(LEDPin1, OUTPUT);
  pinMode(LEDPin2, OUTPUT);

  // NOTE: Grounding the input pin causes it to actuate
  pinMode(inputPin1, INPUT_PULLUP);

  // turn off all digits
  digitOff(0,0);

  if(!getConfigFromSpiffs()) {
    debugMsg("No config found - resetting everything");
    factoryReset();
    setDiagnosticLED(DIAGS_SPIFFS,STATUS_BLUE);
  } else {
    setDiagnosticLED(DIAGS_SPIFFS,STATUS_GREEN);
  }

  WiFiManager wifiManager;

  #ifdef DEBUG
  wifiManager.setDebugOutput(true);
  #endif
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
  wifiManager.setAPCallback(configModeCallback);

  setDiagnosticLED(DIAGS_WIFI,STATUS_YELLOW);

  boolean connected = false;
  connected = wifiManager.autoConnect("VFDClock","SetMeUp!");

  if (connected) {
    debugMsg("Connected!");
    setDiagnosticLED(DIAGS_WIFI,STATUS_GREEN);
  } else {
    debugMsg("Could NOT Connect");
    setDiagnosticLED(DIAGS_WIFI,STATUS_RED);
  }

  IPAddress apIP = WiFi.softAPIP();
  IPAddress myIP = WiFi.localIP();
  debugMsg("AP IP address: " + formatIPAsString(apIP));
  debugMsg("IP address: " + formatIPAsString(myIP));

  lastTimeFromServer = getTimeFromTimeZoneServer();
  if (lastTimeFromServer.startsWith("ERROR:")) {
    debugMsg(lastTimeFromServer);
    setDiagnosticLED(DIAGS_TIMESERVER,STATUS_RED);
  } else {
    setDiagnosticLED(DIAGS_TIMESERVER,STATUS_GREEN);
  }

  // If we lose the connection, we try to recover it
  WiFi.setAutoReconnect(true);

  Wire.begin(4, 5); // SDA = D2 = pin 4, SCL = D1 = pin 5
  debugMsg("I2C master started");

  /* Set page handler functions */
  server.on("/",            rootPageHandler);
  server.on("/time",        timeServerPageHandler);
  server.on("/reset",       resetPageHandler);
  server.on("/clockconfig", clockConfigPageHandler);
  server.on("/setvalue",    setDisplayValuePageHandler);
  server.on("/local.css",   localCSSHandler);
  server.onNotFound(handleNotFound);

  server.begin();
  debugMsg("HTTP server started");

  if (usePIRPullup) {
    debugMsg("Setting PIR pullup");
    pinMode(pirPin, INPUT_PULLUP);
  } else {
    pinMode(pirPin, INPUT);
  }

  // **********************************************************************

  // Clear down any spurious button action
  button1.reset();

  if (testMode) {
    debugMsg("Doing test sequence");
    boolean oldUseLDR = useLDR;
    byte oldBacklightMode = backlightMode;

    // reset the EEPROM values
    factoryReset();

    // turn off LDR
    useLDR = false;

    // turn off Scrollback
    scrollback = false;

    // set backlights to change with the displayed digits
    backlightMode = BACKLIGHT_COLOUR_TIME;

    // All the digits on full
    allBright();

    int secCount = 0;
    int lastSecCount = 0;
    lastCheckMillis = millis();

    boolean inLoop = true;

    // We don't want to stay in test mode forever
    long startTestMode = lastCheckMillis;

    while (inLoop) {
      // Otherwise we get SOFT WDT
      yield();

      nowMillis = millis();
      if (nowMillis - lastCheckMillis > 1000) {
        lastCheckMillis = nowMillis;
        secCount++;
        secCount = secCount % 10;
      }

      loadNumberArraySameValue(secCount);

      // debugMsg("Setting display: " + String(secCount));
      outputDisplay();

      // debugMsg("Setting LEDs");
      if (lastSecCount != secCount) {
        setLeds();
        lastSecCount = secCount;
      }

      // debugMsg("Checing test mode exit condition");
      button1.checkButton(nowMillis);
      if (button1.isButtonPressedNow() && (secCount == 8)) {
        inLoop = false;
        testMode = false;
        saveConfigToSpiffs();
      }
    }

    useLDR = oldUseLDR;
    backlightMode = oldBacklightMode;
  }

  // Clear down any spurious button action
  button1.reset();

  // initialise the internal time (in case we don't find the time provider)
  nowMillis = millis();
  setTime(12, 34, 56, 1, 10, 2018);

  testRTCTimeProvider();
  if (useRTC) {
    getRTCTime();
    setDiagnosticLED(DIAGS_RTC,STATUS_GREEN);
  } else {
    setDiagnosticLED(DIAGS_RTC,STATUS_RED);
  }

  // Note that we had an RTC to allow the indicator DP to have meaning
  onceHadAnRTC = useRTC;

  // Show the version for 1 s
  tempDisplayMode = TEMP_MODE_VERSION;
  tempDisplayModeDuration = TEMP_DISPLAY_MODE_DUR_MS;

  // don't blank anything right now
  blanked = false;
  setTubesAndLEDSBlankMode();

  // mark that we have done the EEPROM setup
  testMode = false;
  saveConfigToSpiffs();

  debugMsg("Exit startup in 1 second");
  getStatsFromSpiffs();
  delay(1000);
}

//**********************************************************************************
//**********************************************************************************
//*                              Main loop                                         *
//**********************************************************************************
//**********************************************************************************
boolean millisNotSet = true;
void loop()
{
  server.handleClient();

  nowMillis = millis();

  // shows us how fast the inner loop is running
  impressionsPerSec++;

  // -------------------------------------------------------------------------------

  if (abs(nowMillis - lastCheckMillis) >= 1000) {
    if ((second() == 0) && (!triggeredThisSec)) {
      if ((minute() == 0)) {
        if (hour() == 0) {
          performOncePerDayProcessing();
        }
        performOncePerHourProcessing();
      }
      performOncePerMinuteProcessing();
    }
    performOncePerSecondProcessing();

    // Make sure we don't call multiple times
    triggeredThisSec = true;

    if ((second() > 0) && triggeredThisSec) {
      triggeredThisSec = false;
    }

    lastCheckMillis = nowMillis;
  }

  // Check button, we evaluate below
  button1.checkButton(nowMillis);

  // ******* Preview the next display mode *******
  // What is previewed here will get actioned when
  // the button is released
  if (button1.isButtonPressed2S()) {
    // Just jump back to the start
    nextMode = MODE_MIN;
  } else if (button1.isButtonPressed1S()) {
    nextMode = currentMode + 1;

    if (nextMode > MODE_MAX) {
      nextMode = MODE_MIN;
    }
  }

  // ******* Set the display mode *******
  if (button1.isButtonPressedReleased2S()) {
    currentMode = MODE_MIN;

    // Store the latest config if we exit the config mode
    saveConfigToSpiffs();

    // Preset the display
    allFadeOrNormal(DO_NOT_APPLY_LEAD_0_BLANK);

    nextMode = currentMode;
  } else if (button1.isButtonPressedReleased1S()) {
    currentMode++;

    if (currentMode > MODE_MAX) {
      currentMode = MODE_MIN;

      // Store the latest config if we exit the config mode
      saveConfigToSpiffs();

      // Preset the display
      allFadeOrNormal(DO_NOT_APPLY_LEAD_0_BLANK);
    }

    nextMode = currentMode;
  }

  // ************* Process the modes *************
  if (nextMode != currentMode) {
    setNextMode(nextMode);
  } else {
    processCurrentMode(currentMode);
  }

  // get the LDR ambient light reading
  digitOffCount = getDimmingFromLDR();
  fadeStep = digitOffCount / fadeSteps;

  // Prepare the tick and backlight LEDs
  setLeds();

  outputDisplay();

//  // ******************* Blue LED pulse *******************
//  if ((nowMillis - lastMillis) > blinkTopTime) {
//    lastMillis = nowMillis;
//    digitalWrite(BUILTIN_LED_PIN, false);
//  } else if ((nowMillis - lastMillis) > blinkOnTime) {
//    digitalWrite(BUILTIN_LED_PIN, true);
//  }
}

// ************************************************************
// Called once per second
// ************************************************************
void performOncePerSecondProcessing() {  
  // Store the current value and reset
  lastImpressionsPerSec = impressionsPerSec;
  impressionsPerSec = 0;

  // Change the direction of the pulse
  upOrDown = !upOrDown;

  // If we are in temp display mode, decrement the count
  if (tempDisplayModeDuration > 0) {
    if (tempDisplayModeDuration > 1000) {
      tempDisplayModeDuration -= 1000;
    } else {
      tempDisplayModeDuration = 0;
    }
  }

  // decrement the blanking supression counter
  if (blankSuppressedMillis > 0) {
    if (blankSuppressedMillis > 1000) {
      blankSuppressedMillis -= 1000;
    } else {
      blankSuppressedMillis = 0;
    }
  }

  // Get the blanking status, this may be overridden by blanking suppression
  // Only blank if we are in TIME mode
  if (currentMode == MODE_TIME) {
    boolean pirBlanked = checkPIR(nowMillis);
    boolean nativeBlanked = checkBlanking();

    // Check if we are in blanking suppression mode
    blanked = (nativeBlanked || pirBlanked) && (blankSuppressedMillis == 0);

    if (blanked) {
      debugMsg("blanked");
      if (pirBlanked)
        debugMsg(" -> PIR");
      if (nativeBlanked)
        debugMsg(" -> Native");
    }

    // reset the blanking period selection timer
    if (nowMillis > blankSuppressedSelectionTimoutMillis) {
      blankSuppressedSelectionTimoutMillis = 0;
      blankSuppressStep = 0;
    }
  } else {
    blanked = false;
  }

  setTubesAndLEDSBlankMode();

  // Decrement the value display counter
  // 0xff means "forever", so don't decrement it
  if ((valueDisplayTime > 0) && (valueDisplayTime < 0xff)) {
    valueDisplayTime--;
  }
}

// ************************************************************
// Called once per minute
// ************************************************************
void performOncePerMinuteProcessing() {
  // Try to reconnect if we are  not connected
   if (WiFi.status() != WL_CONNECTED) {
      debugMsg("Attepting to reconnect dropped connection WiFi connection to " + WiFi.SSID());
      ETS_UART_INTR_DISABLE();
      wifi_station_disconnect();
      ETS_UART_INTR_ENABLE();

      WiFi.begin();
   }

  // Try to recover the current time
  lastTimeFromServer = getTimeFromTimeZoneServer();

  if (!lastTimeFromServer.startsWith("ERROR:")) {
    setTimeFromServer(lastTimeFromServer);

    setBlinkMode(1);

    // all OK, flash 10 millisecond per second
    debugMsg("Update from time server OK");

    lastUpdateFromServer = nowMillis;
  } else {
    setBlinkMode(2);
    debugMsg("No time server update");
  }

  if (useWiFi > 0) {
    if (useWiFi == MAX_WIFI_TIME) {
      // We recently got an update, send to the RTC (if installed)
      setRTCTime();
      debugMsg("-> Set RTC time to Net time");
      getRTCTime();
    } else {
      debugMsg("Net time is not right up to date: " + String(MAX_WIFI_TIME - useWiFi));
    }
    useWiFi--;
  } else {
    getRTCTime();
  }

  if (useRTC) {
    debugMsg("RTC OK"); 
  } else {
    debugMsg("RTC not found"); 
  }

  // Usage stats
  uptimeMins++;

  if (!blankTubes) {
    tubeOnTimeMins++;
  }
}

// ************************************************************
// Called once per hour
// ************************************************************
void performOncePerHourProcessing() {
}

// ************************************************************
// Called once per day
// ************************************************************
void performOncePerDayProcessing() {
  saveStatsToSpiffs();
}

// ************************************************************
// Set the seconds tick led(s) and the back lights
// ************************************************************
void setLeds()
{
  int secsDelta;
  int secsDeltaAbs = (nowMillis - lastCheckMillis);
  if (upOrDown) {
    secsDelta = (nowMillis - lastCheckMillis);
  } else {
    secsDelta = 1000 - (nowMillis - lastCheckMillis);
  }

  // calculate the PWM factor, goes between minDim% and 100%
  float dimFactor = (float) digitOffCount / (float) DIGIT_DISPLAY_OFF;

  // Calculate the brightness factor based on the "pulse"
  float pwmFactor = (float) secsDelta / (float) 1000.0;

  switch (ledMode) {
    case LED_RAILROAD:
    {
      if (upOrDown) {
        led1State = true;
        led2State = false;
      } else {
        led1State = false;
        led2State = true;
      }
      break;
    }
    case LED_BLINK_SLOW:
    {
      if (upOrDown) {
        led1State = true;
        led2State = true;
      } else {
        led1State = false;
        led2State = false;
      }
      break;
    }
    case LED_BLINK_FAST:
    {
      if (secsDeltaAbs < 500) {
        led1State = true;
        led2State = true;
      } else {
        led1State = false;
        led2State = false;
      }
      break;
    }
    case LED_BLINK_DBL:
    {
      if ((secsDeltaAbs < 100) || ((secsDeltaAbs > 200) && (secsDeltaAbs < 300))) {
        led1State = true;
        led2State = true;
      } else {
        led1State = false;
        led2State = false;
      }
      break;
    }
  }

  if (blankLEDs) {
          setAllLEDs( getLEDAdjusted(0, 1, 1), 
                      getLEDAdjusted(0, 1, 1), 
                      getLEDAdjusted(0, 1, 1));
  } else {
    // RGB Backlight PWM led output
    switch (backlightMode) {
      case BACKLIGHT_FIXED:
        setAllLEDs( getLEDAdjusted(rgb_backlight_curve[redCnl], 1, 1), 
                    getLEDAdjusted(rgb_backlight_curve[grnCnl], 1, 1), 
                    getLEDAdjusted(rgb_backlight_curve[bluCnl], 1, 1));
        break;
      case BACKLIGHT_PULSE:
        setAllLEDs( getLEDAdjusted(rgb_backlight_curve[redCnl], pwmFactor, 1),
                    getLEDAdjusted(rgb_backlight_curve[grnCnl], pwmFactor, 1),
                    getLEDAdjusted(rgb_backlight_curve[bluCnl], pwmFactor, 1));
        break;
      case BACKLIGHT_CYCLE:
        cycleColours3(colors);
        setAllLEDs( getLEDAdjusted(colors[0], 1, 1),
                    getLEDAdjusted(colors[1], 1, 1),
                    getLEDAdjusted(colors[2], 1, 1));
        break;
      case BACKLIGHT_FIXED_DIM:
        setAllLEDs( getLEDAdjusted(rgb_backlight_curve[redCnl], 1, dimFactor), 
                    getLEDAdjusted(rgb_backlight_curve[grnCnl], 1, dimFactor), 
                    getLEDAdjusted(rgb_backlight_curve[bluCnl], 1, dimFactor));
        break;
      case BACKLIGHT_PULSE_DIM:
        setAllLEDs( getLEDAdjusted(rgb_backlight_curve[redCnl], pwmFactor, dimFactor),
                    getLEDAdjusted(rgb_backlight_curve[grnCnl], pwmFactor, dimFactor),
                    getLEDAdjusted(rgb_backlight_curve[bluCnl], pwmFactor, dimFactor));
        break;
      case BACKLIGHT_CYCLE_DIM:
        cycleColours3(colors);
        setAllLEDs( getLEDAdjusted(colors[0], 1, dimFactor),
                    getLEDAdjusted(colors[1], 1, dimFactor),
                    getLEDAdjusted(colors[2], 1, dimFactor));
        break;
      case BACKLIGHT_COLOUR_TIME:
          for (int i = 0 ; i < DIGIT_COUNT ; i++) {
            ledR[DIGIT_COUNT-1-i] = getLEDAdjusted(colourTimeR[NumberArray[i]],1,1);
            ledG[DIGIT_COUNT-1-i] = getLEDAdjusted(colourTimeG[NumberArray[i]],1,1);
            ledB[DIGIT_COUNT-1-i] = getLEDAdjusted(colourTimeB[NumberArray[i]],1,1);
          }
          outputLEDBuffer();
        break;
      case BACKLIGHT_COLOUR_TIME_DIM:
          for (int i = 0 ; i < DIGIT_COUNT ; i++) {
            ledR[DIGIT_COUNT-1-i] = getLEDAdjusted(colourTimeR[NumberArray[i]],1,dimFactor);
            ledG[DIGIT_COUNT-1-i] = getLEDAdjusted(colourTimeG[NumberArray[i]],1,dimFactor);
            ledB[DIGIT_COUNT-1-i] = getLEDAdjusted(colourTimeB[NumberArray[i]],1,dimFactor);
          }
          outputLEDBuffer();
        break;
    }
  }
}

// ************************************************************
// Check the PIR status. If we don't have a PIR installed, we
// don't want to respect the pin value, because it would defeat
// normal day blanking. The first time the PIR takes the pin low
// we mark that we have a PIR and we should start to respect
// the sensor.
// Returns true if PIR sensor is installed and we are blanked
// ************************************************************
boolean checkPIR(unsigned long nowMillis) {
  boolean pirvalue = (digitalRead(pirPin) == HIGH);

  if (pirvalue) {
    pirLastSeen = nowMillis;
    return false;
  } else {
    if (nowMillis > (pirLastSeen + (pirTimeout * 1000))) {
      return true;
    } else {
      return false;
    }
  }
}

// ************************************************************
// Set back light LEDs to the same colour
// ************************************************************
void setAllLEDs(byte red, byte green, byte blue) {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    ledR[i] = red;
    ledG[i] = green;
    ledB[i] = blue;
  }
  outputLEDBuffer();
}

// ************************************************************
// Put the led buffers out
// ************************************************************
void outputLEDBuffer() {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    RgbColor color(ledR[i], ledG[i], ledB[i]);
    leds.SetPixelColor(i, color);
  }
  leds.Show();
}

// ************************************************************
// Show the preview of the next mode - we stay in this mode until the 
// button is released
// ************************************************************
void setNextMode(int displayMode) {
  // turn off blanking
  blanked = false;
  setTubesAndLEDSBlankMode();

  switch (displayMode) {
    case MODE_TIME: {
        loadNumberArrayTime();
        allFadeOrNormal(APPLY_LEAD_0_BLANK);
        break;
      }
    case MODE_HOURS_SET: {
        if (useWiFi > 0) {
          // skip past the time and date settings
          nextMode += 6;
          currentMode += 6;
        }
        loadNumberArrayTime();
        highlight0and1();
        break;
      }
    case MODE_MINS_SET: {
        loadNumberArrayTime();
        highlight2and3();
        break;
      }
    case MODE_SECS_SET: {
        loadNumberArrayMinsSecs();
        highlight2and3();
        break;
      }
    case MODE_DAYS_SET: {
        loadNumberArrayDayMonth();
        highlightDaysDateFormat();
        break;
      }
    case MODE_MONTHS_SET: {
        loadNumberArrayDayMonth();
        highlightMonthsDateFormat();
        break;
      }
    case MODE_YEARS_SET: {
        loadNumberArrayYear();
        highlight2and3();
        break;
      }
    case MODE_12_24: {
        loadNumberArrayConfBool(hourMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_LEAD_BLANK: {
        loadNumberArrayConfBool(blankLeading, displayMode);
        displayConfig();
        break;
      }
    case MODE_SCROLLBACK: {
        loadNumberArrayConfBool(scrollback, displayMode);
        displayConfig();
        break;
      }
    case MODE_FADE: {
        loadNumberArrayConfBool(fade, displayMode);
        displayConfig();
        break;
      }
    case MODE_DATE_FORMAT: {
        loadNumberArrayConfInt(dateFormat, displayMode);
        displayConfig();
        break;
      }
    case MODE_DAY_BLANKING: {
        loadNumberArrayConfInt(dayBlanking, displayMode);
        displayConfig();
        break;
      }
    case MODE_HR_BLNK_START: {
        if (dayBlanking < DAY_BLANKING_HOURS) {
          // Skip past the start and end hour if the blanking mode says it is not relevant
          nextMode++;
          currentMode++;
          nextMode++;
          currentMode++;
        }

        loadNumberArrayConfInt(blankHourStart, displayMode);
        displayConfig();
        break;
      }
    case MODE_HR_BLNK_END: {
        loadNumberArrayConfInt(blankHourEnd, displayMode);
        displayConfig();
        break;
      }
    case MODE_BLANK_MODE: {
        loadNumberArrayConfInt(blankMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_USE_LDR: {
        loadNumberArrayConfBool(useLDR, displayMode);
        displayConfig();
        break;
      }
    case MODE_FADE_STEPS_UP:
    case MODE_FADE_STEPS_DOWN: {
        loadNumberArrayConfInt(fadeSteps, displayMode);
        displayConfig();
        break;
      }
    case MODE_DISPLAY_SCROLL_STEPS_UP:
    case MODE_DISPLAY_SCROLL_STEPS_DOWN: {
        loadNumberArrayConfInt(scrollSteps, displayMode);
        displayConfig();
        break;
      }
    case MODE_PIR_TIMEOUT_UP:
    case MODE_PIR_TIMEOUT_DOWN: {
        loadNumberArrayConfMode(displayMode);
        displayConfig();
        break;
      }
    case MODE_SLOTS_MODE: {
        loadNumberArrayConfInt(slotsMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_BACKLIGHT_MODE: {
        loadNumberArrayConfInt(backlightMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_RED_CNL: {
        if ((backlightMode == BACKLIGHT_CYCLE) || (backlightMode == BACKLIGHT_CYCLE_DIM) || (backlightMode == BACKLIGHT_COLOUR_TIME) || (backlightMode == BACKLIGHT_COLOUR_TIME_DIM))  {
          // Skip if we are in cycle mode
          nextMode++;
          currentMode++;
        }
        loadNumberArrayConfInt(redCnl, displayMode);
        displayConfig();
        break;
      }
    case MODE_GRN_CNL: {
        if ((backlightMode == BACKLIGHT_CYCLE) || (backlightMode == BACKLIGHT_CYCLE_DIM) || (backlightMode == BACKLIGHT_COLOUR_TIME) || (backlightMode == BACKLIGHT_COLOUR_TIME_DIM))  {
          // Skip if we are in cycle mode
          nextMode++;
          currentMode++;
        }
        loadNumberArrayConfInt(grnCnl, displayMode);
        displayConfig();
        break;
      }
    case MODE_BLU_CNL: {
        if ((backlightMode == BACKLIGHT_CYCLE) || (backlightMode == BACKLIGHT_CYCLE_DIM) || (backlightMode == BACKLIGHT_COLOUR_TIME) || (backlightMode == BACKLIGHT_COLOUR_TIME_DIM))  {
          // Skip if we are in cycle mode
          nextMode++;
          currentMode++;
        }
        loadNumberArrayConfInt(bluCnl, displayMode);
        displayConfig();
        break;
      }
    case MODE_CYCLE_SPEED: {
        if ((backlightMode == BACKLIGHT_CYCLE) || (backlightMode == BACKLIGHT_CYCLE_DIM) || (backlightMode == BACKLIGHT_COLOUR_TIME) || (backlightMode == BACKLIGHT_COLOUR_TIME_DIM))  {
          // Skip if we are in cycle mode
          nextMode++;
          currentMode++;
        }
        loadNumberArrayConfInt(cycleSpeed, displayMode);
        displayConfig();
        break;
      }
    case MODE_MIN_DIM_UP:
    case MODE_MIN_DIM_DOWN: {
        loadNumberArrayConfMode(displayMode);
        displayConfig();
        break;
      }
    case MODE_USE_PIR_PULLUP: {
        loadNumberArrayConfBool(usePIRPullup, displayMode);
        displayConfig();
        break;
      }
    case MODE_LED_BLINK: {
        loadNumberArrayConfInt(ledMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_VERSION: {
        loadNumberArrayConfInt(SOFTWARE_VERSION, displayMode);
        displayConfig();
        break;
      }
    case MODE_TUBE_TEST: {
        loadNumberArrayTestDigits();
        allNormal();
        break;
      }
  }
}

// ************************************************************
// Show the next mode - once the button is released
// ************************************************************
void processCurrentMode(int displayMode) {
  switch (displayMode) {
    case MODE_TIME: {
        if (button1.isButtonPressedAndReleased()) {
          // Deal with blanking first
          if ((nowMillis < blankSuppressedSelectionTimoutMillis) || blanked) {
            if (blankSuppressedSelectionTimoutMillis == 0) {
              // Apply 5 sec tineout for setting the suppression time
              blankSuppressedSelectionTimoutMillis = nowMillis + TEMP_DISPLAY_MODE_DUR_MS;
            }

            blankSuppressStep++;
            if (blankSuppressStep > 3) {
              blankSuppressStep = 3;
            }

            if (blankSuppressStep == 1) {
              blankSuppressedMillis = 10000;
            } else if (blankSuppressStep == 2) {
              blankSuppressedMillis = 3600000;
            } else if (blankSuppressStep == 3) {
              blankSuppressedMillis = 3600000 * 4;
            }
            blanked = false;
            setTubesAndLEDSBlankMode();
          } else {
            // Always start from the first mode, or increment the temp mode if we are already in a display
            if (tempDisplayModeDuration > 0) {
              tempDisplayModeDuration = TEMP_DISPLAY_MODE_DUR_MS;
              tempDisplayMode++;
            } else {
              tempDisplayMode = TEMP_MODE_MIN;
            }

            if (tempDisplayMode > TEMP_MODE_MAX) {
              tempDisplayMode = TEMP_MODE_MIN;
            }

            tempDisplayModeDuration = TEMP_DISPLAY_MODE_DUR_MS;
          }
        }

        if (tempDisplayModeDuration > 0) {
          blanked = false;
          setTubesAndLEDSBlankMode();
          
          if (tempDisplayMode == TEMP_MODE_SECS) {
            loadNumberArrayMinsSecs();
          }

          if (tempDisplayMode == TEMP_MODE_DATE) {
            loadNumberArrayDayMonth();
          }

          if (tempDisplayMode == TEMP_MODE_LDR) {
            loadNumberArrayLDR();
          }

          if (tempDisplayMode == TEMP_MODE_VERSION) {
            loadNumberArrayConfInt(SOFTWARE_VERSION, 0);
          }

          if (tempDisplayMode == TEMP_IP_ADDR1) {
            if (useWiFi > 0) {
              IPAddress myIP = WiFi.localIP();
              loadNumberArrayIP(myIP[0]);
              showDisplayIP();
            } else {
              // we can't show the IP address if we have the RTC, just skip
              tempDisplayMode++;
            }
          }

          if (tempDisplayMode == TEMP_IP_ADDR2) {
            if (useWiFi > 0) {
              IPAddress myIP = WiFi.localIP();
              loadNumberArrayIP(myIP[1]);
              showDisplayIP();
            } else {
              // we can't show the IP address if we have the RTC, just skip
              tempDisplayMode++;
            }
          }

          if (tempDisplayMode == TEMP_IP_ADDR3) {
            if (useWiFi > 0) {
              IPAddress myIP = WiFi.localIP();
              loadNumberArrayIP(myIP[2]);
              showDisplayIP();
            } else {
              // we can't show the IP address if we have the RTC, just skip
              tempDisplayMode++;
            }
          }

          if (tempDisplayMode == TEMP_IP_ADDR4) {
            if (useWiFi > 0) {
              IPAddress myIP = WiFi.localIP();
              loadNumberArrayIP(myIP[3]);
              showDisplayIP();
            } else {
              // we can't show the IP address if we have the RTC, just skip
              tempDisplayMode++;
            }
          }

          if (tempDisplayMode == TEMP_MODE_IMPR) {
            loadNumberArrayConfIntWide(lastImpressionsPerSec);
          }

          allFadeOrNormal(DO_NOT_APPLY_LEAD_0_BLANK);

        } else {
          if (slotsMode > SLOTS_MODE_MIN) {
            if (second() == 50) {

              // initialise the slots values
              loadNumberArrayDayMonth();
              transition.setAlternateValues();
              loadNumberArrayTime();
              transition.setRegularValues();
              allFadeOrNormal(DO_NOT_APPLY_LEAD_0_BLANK);

              transition.start(nowMillis);
            }

            // initialise the slots mode
            boolean msgDisplaying;
            switch (slotsMode) {
              case SLOTS_MODE_1M_SCR_SCR:
              {
                msgDisplaying = transition.scrollInScrambleOut(nowMillis);
                break;
              }
            }

            // Continue slots
            if (!msgDisplaying) {
              // no slots mode, check if we are in valueDisplayMode
              if (valueDisplayTime > 0) {
                loadNumberArrayValue();
                loadDisplayConfigValue();
              } else {
                // normal time
                loadNumberArrayTime();
                allFadeOrNormal(APPLY_LEAD_0_BLANK);
              }
            }
          } else {
            // no slots mode, check if we are in valueDisplayMode
            if (valueDisplayTime > 0) {
              loadNumberArrayValue();
              loadDisplayConfigValue();
            } else {
              // normal time
              loadNumberArrayTime();
              allFadeOrNormal(APPLY_LEAD_0_BLANK);
            }
          }
        }
        break;
      }
    case MODE_HOURS_SET: {
        if (useWiFi > 0) {
          // skip past the time and date settings
          nextMode += 6;
          currentMode += 6;
        }
        if (button1.isButtonPressedAndReleased()) {
          incHours();
        }
        loadNumberArrayTime();
        highlight0and1();
        break;
      }
    case MODE_MINS_SET: {
        if (button1.isButtonPressedAndReleased()) {
          incMins();
        }
        loadNumberArrayTime();
        highlight2and3();
        break;
      }
    case MODE_SECS_SET: {
        if (button1.isButtonPressedAndReleased()) {
          resetSecond();
        }
        loadNumberArrayMinsSecs();
        highlight2and3();
        break;
      }
    case MODE_DAYS_SET: {
        if (button1.isButtonPressedAndReleased()) {
          incDays();
        }
        loadNumberArrayDayMonth();
        highlightDaysDateFormat();
        break;
      }
    case MODE_MONTHS_SET: {
        if (button1.isButtonPressedAndReleased()) {
          incMonths();
        }
        loadNumberArrayDayMonth();
        highlightMonthsDateFormat();
        break;
      }
    case MODE_YEARS_SET: {
        if (button1.isButtonPressedAndReleased()) {
          incYears();
        }
        loadNumberArrayYear();
        highlight2and3();
        break;
      }
    case MODE_12_24: {
        if (button1.isButtonPressedAndReleased()) {
          hourMode = ! hourMode;
        }
        loadNumberArrayConfBool(hourMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_LEAD_BLANK: {
        if (button1.isButtonPressedAndReleased()) {
          blankLeading = !blankLeading;
        }
        loadNumberArrayConfBool(blankLeading, displayMode);
        displayConfig();
        break;
      }
    case MODE_SCROLLBACK: {
        if (button1.isButtonPressedAndReleased()) {
          scrollback = !scrollback;
        }
        loadNumberArrayConfBool(scrollback, displayMode);
        displayConfig();
        break;
      }
    case MODE_FADE: {
        if (button1.isButtonPressedAndReleased()) {
          fade = !fade;
        }
        loadNumberArrayConfBool(fade, displayMode);
        displayConfig();
        break;
      }
    case MODE_DATE_FORMAT: {
        if (button1.isButtonPressedAndReleased()) {
          dateFormat++;
          if (dateFormat > DATE_FORMAT_MAX) {
            dateFormat = DATE_FORMAT_MIN;
          }
        }
        loadNumberArrayConfInt(dateFormat, displayMode);
        displayConfig();
        break;
      }
    case MODE_DAY_BLANKING: {
        if (button1.isButtonPressedAndReleased()) {
          dayBlanking++;
          if (dayBlanking > DAY_BLANKING_MAX) {
            dayBlanking = DAY_BLANKING_MIN;
          }
        }
        loadNumberArrayConfInt(dayBlanking, displayMode);
        displayConfig();
        break;
      }
    case MODE_HR_BLNK_START: {
        if (button1.isButtonPressedAndReleased()) {
          blankHourStart++;
          if (blankHourStart > HOURS_MAX) {
            blankHourStart = 0;
          }
        }
        loadNumberArrayConfInt(blankHourStart, displayMode);
        displayConfig();
        break;
      }
    case MODE_HR_BLNK_END: {
        if (button1.isButtonPressedAndReleased()) {
          blankHourEnd++;
          if (blankHourEnd > HOURS_MAX) {
            blankHourEnd = 0;
          }
        }
        loadNumberArrayConfInt(blankHourEnd, displayMode);
        displayConfig();
        break;
      }
    case MODE_BLANK_MODE: {
        if (button1.isButtonPressedAndReleased()) {
          blankMode++;
          if (blankMode > BLANK_MODE_MAX) {
            blankMode = BLANK_MODE_MIN;
          }
        }
        loadNumberArrayConfInt(blankMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_USE_LDR: {
        if (button1.isButtonPressedAndReleased()) {
          useLDR = !useLDR;
        }
        loadNumberArrayConfBool(useLDR, displayMode);
        displayConfig();
        break;
      }
    case MODE_FADE_STEPS_UP: {
        if (button1.isButtonPressedAndReleased()) {
          fadeSteps++;
          if (fadeSteps > FADE_STEPS_MAX) {
            fadeSteps = FADE_STEPS_MIN;
          }
        }
        loadNumberArrayConfInt(fadeSteps, displayMode);
        displayConfig();
        fadeStep = DIGIT_DISPLAY_COUNT / fadeSteps;
        break;
      }
    case MODE_FADE_STEPS_DOWN: {
        if (button1.isButtonPressedAndReleased()) {
          fadeSteps--;
          if (fadeSteps < FADE_STEPS_MIN) {
            fadeSteps = FADE_STEPS_MAX;
          }
        }
        loadNumberArrayConfInt(fadeSteps, displayMode);
        displayConfig();
        fadeStep = DIGIT_DISPLAY_COUNT / fadeSteps;
        break;
      }
    case MODE_DISPLAY_SCROLL_STEPS_DOWN: {
        if (button1.isButtonPressedAndReleased()) {
          scrollSteps--;
          if (scrollSteps < SCROLL_STEPS_MIN) {
            scrollSteps = SCROLL_STEPS_MAX;
          }
        }
        loadNumberArrayConfInt(scrollSteps, displayMode);
        displayConfig();
        break;
      }
    case MODE_DISPLAY_SCROLL_STEPS_UP: {
        if (button1.isButtonPressedAndReleased()) {
          scrollSteps++;
          if (scrollSteps > SCROLL_STEPS_MAX) {
            scrollSteps = SCROLL_STEPS_MIN;
          }
        }
        loadNumberArrayConfInt(scrollSteps, displayMode);
        displayConfig();
        break;
      }
    case MODE_PIR_TIMEOUT_DOWN: {
        if (button1.isButtonPressedAndReleased()) {
          pirTimeout-=10;
          if (pirTimeout < PIR_TIMEOUT_MIN) {
            pirTimeout = PIR_TIMEOUT_MAX;
          }
        }
        loadNumberArrayConfIntWide(pirTimeout);
        displayConfig();
        break;
      }
    case MODE_PIR_TIMEOUT_UP: {
        if (button1.isButtonPressedAndReleased()) {
          pirTimeout+=10;
          if (pirTimeout > PIR_TIMEOUT_MAX) {
            pirTimeout = PIR_TIMEOUT_MIN;
          }
        }
        loadNumberArrayConfIntWide(pirTimeout);
        displayConfig();
        break;
      }
    case MODE_SLOTS_MODE: {
        if (button1.isButtonPressedAndReleased()) {
          slotsMode++;
          if (slotsMode > SLOTS_MODE_MAX) {
            slotsMode = SLOTS_MODE_MIN;
          }
        }
        loadNumberArrayConfInt(slotsMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_BACKLIGHT_MODE: {
        if (button1.isButtonPressedAndReleased()) {
          backlightMode++;
          if (backlightMode > BACKLIGHT_MAX) {
            backlightMode = BACKLIGHT_MIN;
          }
        }
        loadNumberArrayConfInt(backlightMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_RED_CNL: {
        if ((backlightMode == BACKLIGHT_CYCLE) || (backlightMode == BACKLIGHT_CYCLE_DIM) || (backlightMode == BACKLIGHT_COLOUR_TIME) || (backlightMode == BACKLIGHT_COLOUR_TIME_DIM))  {
          // Skip if we are in cycle mode
          nextMode++;
          currentMode++;
        }

        if (button1.isButtonPressedAndReleased()) {
          redCnl++;
          if (redCnl > COLOUR_CNL_MAX) {
            redCnl = COLOUR_CNL_MIN;
          }
        }
        loadNumberArrayConfInt(redCnl, displayMode);
        displayConfig();
        break;
      }
    case MODE_GRN_CNL: {
        if ((backlightMode == BACKLIGHT_CYCLE) || (backlightMode == BACKLIGHT_CYCLE_DIM) || (backlightMode == BACKLIGHT_COLOUR_TIME) || (backlightMode == BACKLIGHT_COLOUR_TIME_DIM))  {
          // Skip if we are in cycle mode
          nextMode++;
          currentMode++;
        }

        if (button1.isButtonPressedAndReleased()) {
          grnCnl++;
          if (grnCnl > COLOUR_CNL_MAX) {
            grnCnl = COLOUR_CNL_MIN;
          }
        }
        loadNumberArrayConfInt(grnCnl, displayMode);
        displayConfig();
        break;
      }
    case MODE_BLU_CNL: {
        if ((backlightMode == BACKLIGHT_CYCLE) || (backlightMode == BACKLIGHT_CYCLE_DIM) || (backlightMode == BACKLIGHT_COLOUR_TIME) || (backlightMode == BACKLIGHT_COLOUR_TIME_DIM))  {
          // Skip if we are in cycle mode
          nextMode++;
          currentMode++;
        }

        if (button1.isButtonPressedAndReleased()) {
          bluCnl++;
          if (bluCnl > COLOUR_CNL_MAX) {
            bluCnl = COLOUR_CNL_MIN;
          }
        }
        loadNumberArrayConfInt(bluCnl, displayMode);
        displayConfig();
        break;
      }
    case MODE_CYCLE_SPEED: {
        if ((backlightMode == BACKLIGHT_CYCLE) || (backlightMode == BACKLIGHT_CYCLE_DIM) || (backlightMode == BACKLIGHT_COLOUR_TIME) || (backlightMode == BACKLIGHT_COLOUR_TIME_DIM))  {
          // Skip if we are in cycle mode
          nextMode++;
          currentMode++;
        }

        if (button1.isButtonPressedAndReleased()) {
          cycleSpeed = cycleSpeed + 2;
          if (cycleSpeed > CYCLE_SPEED_MAX) {
            cycleSpeed = CYCLE_SPEED_MIN;
          }
        }
        loadNumberArrayConfInt(cycleSpeed, displayMode);
        displayConfig();
        break;
      }
    case MODE_MIN_DIM_UP: {
        if (button1.isButtonPressedAndReleased()) {
          minDim += 10;
          if (minDim > MIN_DIM_MAX) {
            minDim = MIN_DIM_MAX;
          }
        }
        loadNumberArrayConfIntWide(minDim);
        displayConfig();
        break;
      }
    case MODE_MIN_DIM_DOWN: {
        if (button1.isButtonPressedAndReleased()) {
          minDim -= 10;
          if (minDim < MIN_DIM_MIN) {
            minDim = MIN_DIM_MIN;
          }
        }
        loadNumberArrayConfIntWide(minDim);
        displayConfig();
        break;
      }
    case MODE_USE_PIR_PULLUP: {
        if (button1.isButtonPressedAndReleased()) {
          usePIRPullup = !usePIRPullup;
        }
        loadNumberArrayConfBool(usePIRPullup, displayMode);
        digitalWrite(pirPin, usePIRPullup);
        displayConfig();
        break;
      }
    case MODE_LED_BLINK: {
        if (button1.isButtonPressedAndReleased()) {
          ledMode += 1;
          if (ledMode > LED_MODE_MAX) {
            ledMode = LED_MODE_MIN;
          }
        }
        loadNumberArrayConfInt(ledMode, displayMode);
        displayConfig();
        break;
      }
    case MODE_VERSION: {
        loadNumberArrayConfInt(SOFTWARE_VERSION, displayMode);
        displayConfig();
        break;
      }
    case MODE_TUBE_TEST: {
        allNormal();
        loadNumberArrayTestDigits();
        break;
      }
  }
}

// ************************************************************
// output a PWM LED channel, adjusting for dimming and PWM
// brightness:
// rawValue: The raw brightness value between 0 - 255
// ledPWMVal: The pwm factor between 0 - 1
// dimFactor: The dimming value between 0 - 1
// ************************************************************
byte getLEDAdjusted(float rawValue, float ledPWMVal, float dimFactor) {
  byte dimmedPWMVal = (byte)(rawValue * ledPWMVal * dimFactor);
  return dim_curve[dimmedPWMVal];
}

// ************************************************************
// Colour cycling 3: one colour dominates
// ************************************************************
void cycleColours3(int colors[3]) {
  cycleCount++;
  if (cycleCount > cycleSpeed) {
    cycleCount = 0;

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

//**********************************************************************************
//**********************************************************************************
//*                             Utility functions                                  *
//**********************************************************************************
//**********************************************************************************

// ************************************************************
// Break the time into displayable digits
// ************************************************************
void loadNumberArrayTime() {
  NumberArray[3] = minute() % 10;
  NumberArray[2] = minute() / 10;
  if (hourMode) {
    NumberArray[1] = hourFormat12() % 10;
    NumberArray[0] = hourFormat12() / 10;
  } else {
    NumberArray[1] = hour() % 10;
    NumberArray[0] = hour() / 10;
  }
}

void loadNumberArrayMinsSecs() {
  NumberArray[3] = second() % 10;
  NumberArray[2] = second() / 10;
  NumberArray[1] = minute() % 10;
  NumberArray[0] = minute() / 10;
}

// ************************************************************
// Break the date into displayable digits, respecting date format
// ************************************************************
void loadNumberArrayDayMonth() {
  switch (dateFormat) {
    case DATE_FORMAT_YYMMDD:
    case DATE_FORMAT_MMDDYY:
      NumberArray[3] = day() % 10;
      NumberArray[2] = day() / 10;
      NumberArray[1] = month() % 10;
      NumberArray[0] = month() / 10;
      break;
    case DATE_FORMAT_DDMMYY:
      NumberArray[3] = month() % 10;
      NumberArray[2] = month() / 10;
      NumberArray[1] = day() % 10;
      NumberArray[0] = day() / 10;
      break;
  }
}

// ************************************************************
// Break the year into displayable digits
// ************************************************************
void loadNumberArrayYear() {
  NumberArray[3] = (year() / 1) % 10;
  NumberArray[2] = (year() / 10) % 10;
  NumberArray[1] = (year() / 100) % 10;
  NumberArray[0] = (year() / 1000) % 10;
}

// ************************************************************
// Break the LDR reading into displayable digits
// ************************************************************
void loadNumberArrayLDR() {
  NumberArray[3] = (digitOffCount / 1) % 10;
  NumberArray[2] = (digitOffCount / 10) % 10;
  NumberArray[1] = (digitOffCount / 100) % 10;
  NumberArray[0] = (digitOffCount / 1000) % 10;
}

// ************************************************************
// Test digits
// ************************************************************
void loadNumberArrayTestDigits() {
  NumberArray[3] = second() % 10;
  NumberArray[2] = second() % 10;
  NumberArray[1] = second() % 10;
  NumberArray[0] = second() % 10;
}

// ************************************************************
// Show an integer configuration value
// ************************************************************
void loadNumberArrayConfInt(int confValue, byte mode) {
  NumberArray[3] = (confValue / 1) % 10;
  NumberArray[2] = (confValue / 10) % 10;
  NumberArray[1] = mode % 10;
  NumberArray[0] = mode / 10;
}

// ************************************************************
// Show an integer configuration value, use all 4 digits
// ************************************************************
void loadNumberArrayConfIntWide(int confValue) {
  NumberArray[3] = (confValue / 1) % 10;
  NumberArray[2] = (confValue / 10) % 10;
  NumberArray[1] = (confValue / 100) % 10;
  NumberArray[0] = (confValue / 1000) % 10;
}

// ************************************************************
// Show an configuration mode value
// ************************************************************
void loadNumberArrayConfMode(byte mode) {
  NumberArray[3] = (mode / 1) % 10;
  NumberArray[2] = (mode / 10) % 10;
  NumberArray[1] = 0;
  NumberArray[0] = 0;
}

// ************************************************************
// Show a boolean configuration value
// ************************************************************
void loadNumberArrayConfBool(boolean confValue, byte mode) {
  int boolInt;
  if (confValue) {
    boolInt = 1;
  } else {
    boolInt = 0;
  }
  NumberArray[3] = boolInt;
  NumberArray[2] = 0;
  NumberArray[1] = mode % 10;
  NumberArray[0] = mode / 10;
}

// ************************************************************
// Show an 1/4 IP configuration value
// ************************************************************
void loadNumberArrayIP(byte byte1) {
  NumberArray[3] = (byte1) % 10;
  NumberArray[2] = (byte1 / 10) % 10;
  NumberArray[1] = (byte1 / 100) % 10;
  NumberArray[0] = 0;
}

// ************************************************************
// Show the same value on all digits
// ************************************************************
void loadNumberArraySameValue(byte val) {
  NumberArray[3] = val;
  NumberArray[2] = val;
  NumberArray[1] = val;
  NumberArray[0] = val;
}

// ************************************************************
// Show the value we got over I2C
// ************************************************************
void loadNumberArrayValue() {
  NumberArray[3] = valueToShow[1] & 0xf;
  NumberArray[2] = valueToShow[1] >> 4;
  NumberArray[1] = valueToShow[0] & 0xf;
  NumberArray[0] = valueToShow[0] >> 4;
}

// ************************************************************
// Display preset
// ************************************************************
void loadDisplayConfigValue() {
  displayType[0] = valueDisplayType[0] >> 4;
  displayType[1] = valueDisplayType[0] & 0xf;
  displayType[2] = valueDisplayType[1] >> 4;
  displayType[3] = valueDisplayType[1] & 0xf;
}

// ************************************************************
// Do a single complete display, including any fading and
// dimming requested. Performs the display loop
// DIGIT_DISPLAY_COUNT times for each digit, with no delays.
// This is the heart of the display processing!
// ************************************************************
void outputDisplay()
{
  int digitOnTime;
  int digitOffTime;
  int digitSwitchTime;
  float digitSwitchTimeFloat;
  int tmpDispType;

  // used to blank all leading digits if 0
  boolean leadingZeros = true;

  for ( int i = 0 ; i < DIGIT_COUNT ; i ++ )
  {
    if (blankTubes) {
      tmpDispType = BLANKED;
    } else {
      tmpDispType = displayType[i];
    }

    switch (tmpDispType) {
      case BLANKED:
        {
          // Setting off = on means we never turn on
          digitOnTime = DIGIT_DISPLAY_ON;
          digitOffTime = DIGIT_DISPLAY_ON;
          break;
        }
      case DIMMED:
        {
          digitOnTime = DIGIT_DISPLAY_ON;
          digitOffTime = DIM_VALUE;
          break;
        }
      case BRIGHT:
        {
          digitOnTime = DIGIT_DISPLAY_ON;
          digitOffTime = DIGIT_DISPLAY_OFF;
          break;
        }
      case FADE:
      case NORMAL:
      case SCROLL:
        {
          digitOnTime = DIGIT_DISPLAY_ON;
          digitOffTime = digitOffCount;
          break;
        }
      case BLINK:
        {
          if (blinkState) {
            digitOnTime = DIGIT_DISPLAY_ON;
            digitOffTime = digitOffCount;
          } else {
            digitOnTime = DIGIT_DISPLAY_NEVER;
            digitOffTime = DIGIT_DISPLAY_OFF;
          }
          break;
        }
    }

    // Do scrollback when we are going to 0
    if ((NumberArray[i] != currNumberArray[i]) &&
        (NumberArray[i] == 0) &&
        scrollback) {
      tmpDispType = SCROLL;
    }

    // --------------------- Scroll ---------------------

    // manage fading, each impression we show 1 fade step less of the old
    // digit and 1 fade step more of the new
    // manage fading, each impression we show 1 fade step less of the old
    // digit and 1 fade step more of the new
    if (tmpDispType == SCROLL) {
      digitSwitchTime = digitOffTime;
      if (NumberArray[i] != currNumberArray[i]) {
        if (fadeState[i] == 0) {
          // Start the fade
          fadeState[i] = scrollSteps;
        }

        if (fadeState[i] == 1) {
          // finish the fade
          fadeState[i] = 0;
          currNumberArray[i] = currNumberArray[i] - 1;
        } else if (fadeState[i] > 1) {
          // Continue the scroll countdown
          fadeState[i] = fadeState[i] - 1;
        }
      }
    } else 

    // --------------------- Fade ---------------------

    if (tmpDispType == FADE) {
      float countsPerStep = (digitOffTime - digitOnTime) / fadeSteps;

      if (NumberArray[i] != currNumberArray[i]) {
        // Start the fade
        if (fadeState[i] == 0) {
          // Start the fade
          fadeState[i] = fadeSteps;
          digitSwitchTime = digitOnTime + (fadeState[i] * countsPerStep);
        }
      }

      if (fadeState[i] == 1) {
        // finish the fade
        fadeState[i] = 0;
        currNumberArray[i] = NumberArray[i];
        digitSwitchTime = digitOffTime;
      } else if (fadeState[i] > 1) {
        // Continue the fade
        fadeState[i] = fadeState[i] - 1;
        digitSwitchTime = digitOnTime + (fadeState[i] * countsPerStep);
      }
    } else 

    // --------------------- Normal ---------------------

    {
      digitSwitchTime = DIGIT_DISPLAY_COUNT;
      currNumberArray[i] = NumberArray[i];
    }

    // Main display loop - everything up to now was preparation
    for (int timer = DIGIT_DISPLAY_START ; timer < DIGIT_DISPLAY_COUNT ; timer++) {
      if (timer < digitOnTime) {
        // Give the poor ESP time to do other things while we are in the porch
        yield();
      } else if (timer == digitOffTime) {
        digitOff(i, NumberArray[i]);
        ledsOff();
      } else if (timer == digitOnTime) {
        digitOn(i, currNumberArray[i]);
        ledsOn();
      } else if (timer == digitSwitchTime) {
        digitOn(i, NumberArray[i]);
      }
    }
  }

  // Deal with blink, calculate if we are on or off
  blinkCounter++;
  if (blinkCounter == BLINK_COUNT_MAX) {
    blinkCounter = 0;
    blinkState = !blinkState;
  }
}

// ************************************************************
// Set a digit with the given value and turn the HVGen on
// Assumes that all digits have previously been turned off
// by a call to "digitOff"
// ************************************************************
void digitOn(byte digit, byte value) {
  byte dispValue = (anodes[digit] & 0xf0);
  shiftOut(LATCHPin, DATA1Pin, CLOCKPin, MSBFIRST, dispValue);
  dispValue = (decodeDigit[value] & 0xf) | (anodes[digit] & 0xf0);
  shiftOut(LATCHPin, DATA1Pin, CLOCKPin, MSBFIRST, dispValue);
}

// ************************************************************
// Finish displaying a digit and turn the HVGen off
// ************************************************************
void digitOff(byte digit, byte value) {
  byte dispValue = (decodeDigit[value] & 0xf);
  shiftOut(LATCHPin, DATA1Pin, CLOCKPin, MSBFIRST, dispValue);
}

void ledsOn() {
  digitalWrite(LEDPin1, led1State);
  digitalWrite(LEDPin2, led2State);
}

void ledsOff() {
  digitalWrite(LEDPin1, LOW);
  digitalWrite(LEDPin2, LOW);
}

// ************************************************************
// Perform the parallel shift out to the registers
// ************************************************************
void shiftOut(uint8_t _loadPin,
                  uint8_t _dataPin1,
                  uint8_t _clockPin,
                  uint8_t _bitOrder,
                  uint8_t _val1) {
  uint8_t i;

  digitalWrite(_loadPin, LOW);
  for (i = 0; i < 8; i++) {
    if (_bitOrder == LSBFIRST) {
      digitalWrite(_dataPin1, !!(_val1 & (1 << i)));
    }
    else {
      digitalWrite(_dataPin1, !!(_val1 & (1 << (7 - i))));
    }
    digitalWrite(_clockPin, HIGH);
    digitalWrite(_clockPin, LOW);
  }
  digitalWrite(_loadPin, HIGH);
}

// ************************************************************
// Display preset - apply leading zero blanking
// ************************************************************
void applyBlanking() {
  // If we are not blanking, just get out
  if (blankLeading == false) {
    return;
  }

  // We only want to blank the hours tens digit
  if (NumberArray[0] == 0) {
    if (displayType[0] != BLANKED) {
      displayType[0] = BLANKED;
    }
  }
}

// ************************************************************
// Display preset
// ************************************************************
void allFadeOrNormal(boolean blanking) {
  if (fade) {
    allFade();
  } else {
    allNormal();
  }

  if (blanking) {
    applyBlanking();
  }
}

// ************************************************************
// Display preset
// ************************************************************
void allFade() {
  if (displayType[0] != FADE) displayType[0] = FADE;
  if (displayType[1] != FADE) displayType[1] = FADE;
  if (displayType[2] != FADE) displayType[2] = FADE;
  if (displayType[3] != FADE) displayType[3] = FADE;
}

// ************************************************************
// Display preset
// ************************************************************
void allBright() {
  if (displayType[0] != BRIGHT) displayType[0] = BRIGHT;
  if (displayType[1] != BRIGHT) displayType[1] = BRIGHT;
  if (displayType[2] != BRIGHT) displayType[2] = BRIGHT;
  if (displayType[3] != BRIGHT) displayType[3] = BRIGHT;
}

// ************************************************************
// highlight months taking into account the date format
// ************************************************************
void highlightMonthsDateFormat() {
  switch (dateFormat) {
    case DATE_FORMAT_YYMMDD:
      highlight2and3();
      break;
    case DATE_FORMAT_MMDDYY:
      highlight0and1();
      break;
    case DATE_FORMAT_DDMMYY:
      highlight2and3();
      break;
  }
}

// ************************************************************
// highlight days taking into account the date format
// ************************************************************
void highlightDaysDateFormat() {
  switch (dateFormat) {
    case DATE_FORMAT_YYMMDD:
      highlight0and1();
      break;
    case DATE_FORMAT_MMDDYY:
      highlight2and3();
      break;
    case DATE_FORMAT_DDMMYY:
      highlight0and1();
      break;
  }
}

// ************************************************************
// Display preset, highlight digits 0 and 1
// ************************************************************
void highlight0and1() {
  if (displayType[0] != BRIGHT) displayType[0] = BRIGHT;
  if (displayType[1] != BRIGHT) displayType[1] = BRIGHT;
  if (displayType[2] != DIMMED) displayType[2] = DIMMED;
  if (displayType[3] != DIMMED) displayType[3] = DIMMED;
}

// ************************************************************
// Display preset, highlight digits 2 and 3
// ************************************************************
void highlight2and3() {
  if (displayType[0] != DIMMED) displayType[0] = DIMMED;
  if (displayType[1] != DIMMED) displayType[1] = DIMMED;
  if (displayType[2] != BRIGHT) displayType[2] = BRIGHT;
  if (displayType[3] != BRIGHT) displayType[3] = BRIGHT;
}

// ************************************************************
// Display preset
// ************************************************************
void allNormal() {
  if (displayType[0] != NORMAL) displayType[0] = NORMAL;
  if (displayType[1] != NORMAL) displayType[1] = NORMAL;
  if (displayType[2] != NORMAL) displayType[2] = NORMAL;
  if (displayType[3] != NORMAL) displayType[3] = NORMAL;
}

// ************************************************************
// Display preset
// ************************************************************
void displayConfig() {
  if (displayType[0] != BRIGHT) displayType[0] = BRIGHT;
  if (displayType[1] != BRIGHT) displayType[1] = BRIGHT;
  if (displayType[2] != BRIGHT) displayType[2] = BRIGHT;
  if (displayType[3] != BRIGHT) displayType[3] = BRIGHT;
}

// ************************************************************
// Display preset
// ************************************************************
void displayConfig3() {
  if (displayType[0] != BLANKED) displayType[0] = BLANKED;
  if (displayType[1] != NORMAL) displayType[1] = BRIGHT;
  if (displayType[2] != NORMAL) displayType[2] = BRIGHT;
  if (displayType[3] != NORMAL) displayType[3] = BRIGHT;
}

// ************************************************************
// Display preset
// ************************************************************
void displayConfig2() {
  if (displayType[0] != BLANKED) displayType[0] = BLANKED;
  if (displayType[1] != BLANKED) displayType[1] = BLANKED;
  if (displayType[2] != NORMAL) displayType[2] = BRIGHT;
  if (displayType[3] != NORMAL) displayType[3] = BRIGHT;
}

// ************************************************************
// Display preset
// ************************************************************
void allBlanked() {
  if (displayType[0] != BLANKED) displayType[0] = BLANKED;
  if (displayType[1] != BLANKED) displayType[1] = BLANKED;
  if (displayType[2] != BLANKED) displayType[2] = BLANKED;
  if (displayType[3] != BLANKED) displayType[3] = BLANKED;
}

// ************************************************************
// Display preset
// ************************************************************
void confBlink() {
  if (displayType[0] != BLINK) displayType[0] = BLINK;
  if (displayType[1] != BLINK) displayType[1] = BLINK;
  if (displayType[2] != BLINK) displayType[2] = BLINK;
  if (displayType[3] != BLINK) displayType[3] = BLINK;
}

// ************************************************************
// Display preset
// ************************************************************
void showDisplayIP() {
  if (displayType[0] != BLANKED) displayType[0] = BLANKED;
  if (displayType[1] != FADE) displayType[1] = FADE;
  if (displayType[2] != FADE) displayType[2] = FADE;
  if (displayType[3] != FADE) displayType[3] = FADE;
}

// ************************************************************
// Reset the seconds to 00
// ************************************************************
void resetSecond() {
  byte tmpSecs = 0;
  setTime(hour(), minute(), tmpSecs, day(), month(), year());
  setRTCTime();
}

  // ************************************************************
// increment the time by 1 Sec
// ************************************************************
void incSecond() {
  byte tmpSecs = second();
  tmpSecs++;
  if (tmpSecs >= SECS_MAX) {
    tmpSecs = 0;
  }
  setTime(hour(), minute(), tmpSecs, day(), month(), year());
  setRTCTime();
}

// ************************************************************
// increment the time by 1 min
// ************************************************************
void incMins() {
  byte tmpMins = minute();
  tmpMins++;
  if (tmpMins >= MINS_MAX) {
    tmpMins = 0;
  }
  setTime(hour(), tmpMins, 0, day(), month(), year());
  setRTCTime();
}

// ************************************************************
// increment the time by 1 hour
// ************************************************************
void incHours() {
  byte tmpHours = hour();
  tmpHours++;

  if (tmpHours >= HOURS_MAX) {
    tmpHours = 0;
  }
  setTime(tmpHours, minute(), second(), day(), month(), year());
  setRTCTime();
}

// ************************************************************
// increment the date by 1 day
// ************************************************************
void incDays() {
  byte tmpDays = day();
  tmpDays++;

  int maxDays;
  switch (month())
  {
    case 4:
    case 6:
    case 9:
    case 11:
      {
        maxDays = 31;
        break;
      }
    case 2:
      {
        // we won't worry about leap years!!!
        maxDays = 28;
        break;
      }
    default:
      {
        maxDays = 31;
      }
  }

  if (tmpDays > maxDays) {
    tmpDays = 1;
  }
  setTime(hour(), minute(), second(), tmpDays, month(), year());
  setRTCTime();
}

// ************************************************************
// increment the month by 1 month
// ************************************************************
void incMonths() {
  byte tmpMonths = month();
  tmpMonths++;

  if (tmpMonths > 12) {
    tmpMonths = 1;
  }
  setTime(hour(), minute(), second(), day(), tmpMonths, year());
  setRTCTime();
}

// ************************************************************
// increment the year by 1 year
// ************************************************************
void incYears() {
  byte tmpYears = year() % 100;
  tmpYears++;

  if (tmpYears > 50) {
    tmpYears = 15;
  }
  setTime(hour(), minute(), second(), day(), month(), 2000 + tmpYears);
  setRTCTime();
}

// ************************************************************
// Set the time from the value we get back from the timer server
// ************************************************************
void setTimeFromServer(String timeString) {
  int years = getIntValue(timeString, ',', 0);
  byte months = getIntValue(timeString, ',', 1);
  byte days = getIntValue(timeString, ',', 2);
  byte hours = getIntValue(timeString, ',', 3);
  byte minutes = getIntValue(timeString, ',', 4);
  byte seconds = getIntValue(timeString, ',', 5);
  setTime(hours, minutes, seconds, days, months, years);
}


// ************************************************************
// Check the blanking
// ************************************************************
boolean checkBlanking() {
  // Check day blanking, but only when we are in
  // normal time mode
  if (currentMode == MODE_TIME) {
    switch (dayBlanking) {
      case DAY_BLANKING_NEVER:
        return false;
      case DAY_BLANKING_HOURS:
        return getHoursBlanked();
      case DAY_BLANKING_WEEKEND:
        return ((weekday() == 1) || (weekday() == 7));
      case DAY_BLANKING_WEEKEND_OR_HOURS:
        return ((weekday() == 1) || (weekday() == 7)) || getHoursBlanked();
      case DAY_BLANKING_WEEKEND_AND_HOURS:
        return ((weekday() == 1) || (weekday() == 7)) && getHoursBlanked();
      case DAY_BLANKING_WEEKDAY:
        return ((weekday() > 1) && (weekday() < 7));
      case DAY_BLANKING_WEEKDAY_OR_HOURS:
        return ((weekday() > 1) && (weekday() < 7)) || getHoursBlanked();
      case DAY_BLANKING_WEEKDAY_AND_HOURS:
        return ((weekday() > 1) && (weekday() < 7)) && getHoursBlanked();
      case DAY_BLANKING_ALWAYS:
        return true;
    }
  }
}

// ************************************************************
// If we are currently blanked based on hours
// ************************************************************
boolean getHoursBlanked() {
  if (blankHourStart > blankHourEnd) {
    // blanking before midnight
    return ((hour() >= blankHourStart) || (hour() < blankHourEnd));
  } else if (blankHourStart < blankHourEnd) {
    // dim at or after midnight
    return ((hour() >= blankHourStart) && (hour() < blankHourEnd));
  } else {
    // no dimming if Start = End
    return false;
  }
}

// ************************************************************
// Set the tubes and LEDs blanking variables based on blanking mode and 
// blank mode settings
// ************************************************************
void setTubesAndLEDSBlankMode() {
  if (blanked) {
    switch(blankMode) {
      case BLANK_MODE_TUBES:
      {
        blankTubes = true;
        blankLEDs = false;
        break;
      }
      case BLANK_MODE_LEDS:
      {
        blankTubes = false;
        blankLEDs = true;
        break;
      }
      case BLANK_MODE_BOTH:
      {
        blankTubes = true;
        blankLEDs = true;
        break;
      }
    }
  } else {
    blankTubes = false;
    blankLEDs = false;
  }
}

// ************************************************************
// Random colour flash from the back lights
// ************************************************************
void randomRGBFlash(int delayVal) {
  // Set up the PRNG with something so that it looks random
  randomSeed(analogRead(LDRPin));

  if (random(3) == 0) {
    setAllLEDs(255,0,0);
  }
  if (random(3) == 0) {
    setAllLEDs(0,255,0);
  }
  if (random(3) == 0) {
    setAllLEDs(0,0,255);
  }
  delay(delayVal);
  
  setAllLEDs(0,0,0);
  delay(delayVal);
}

// ************************************************************
// Tell the Diag LED that we have connected
// ************************************************************
void configModeCallback (WiFiManager *myWiFiManager) {
  debugMsg("*** Entered config mode");
  setDiagnosticLED(DIAGS_WIFI,STATUS_BLUE);
}

//**********************************************************************************
//**********************************************************************************
//*                         RTC Module Time Provider                               *
//**********************************************************************************
//**********************************************************************************

// ************************************************************
// Get the time from the RTC
// ************************************************************
void getRTCTime() {
  testRTCTimeProvider();
  if (useRTC) {
    Clock.getTime();
    int years = Clock.year + 2000;
    byte months = Clock.month;
    byte days = Clock.dayOfMonth;
    byte hours = Clock.hour;
    byte mins = Clock.minute;
    byte secs = Clock.second;

    debugMsg("Got RTC time: " + String(years) + ":" + String(months) + ":" + String(days) + " " + String(hours) + ":" + String(mins) + ":" + String(secs));

    // Set the internal time provider to the value we got
    setTime(hours, mins, secs, days, months, years);
  }
}

// ************************************************************
// Check that we still have access to the time from the RTC
// ************************************************************
void testRTCTimeProvider() {
  // Set up the time provider
  // first try to find the RTC, if not available, go into slave mode
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  useRTC = (Wire.endTransmission() == 0);
}

// ************************************************************
// Set the date/time in the RTC from the internal time
// Always hold the time in 24 format, we convert to 12 in the
// display.
// ************************************************************
void setRTCTime() {
  testRTCTimeProvider();
  if (useRTC) {
    Clock.fillByYMD(year()%100,month(),day());
    Clock.fillByHMS(hour(),minute(),second());
    Clock.setTime();

    debugMsg("Set RTC time to Net time: " + String(year()) + ":" + String(month()) + ":" + String(day()) + " " + String(hour()) + ":" + String(minute()) + ":" + String(second()));
  }
}

//**********************************************************************************
//**********************************************************************************
//*                          Light Dependent Resistor                              *
//**********************************************************************************
//**********************************************************************************

// ******************************************************************
// Check the ambient light through the LDR (Light Dependent Resistor)
// Smooths the reading over several reads.
//
// The LDR in bright light gives reading of around 50, the reading in
// total darkness is around 900.
//
// The return value is the dimming count we are using. 999 is full
// brightness, 100 is very dim.
//
// Because the floating point calculation may return more than the
// maximum value, we have to clamp it as the final step
// ******************************************************************
int getDimmingFromLDR() {
  if (useLDR) {
    int rawLDR = analogRead(LDRPin);
    if (rawLDR < 100) rawLDR = 100;
    if (rawLDR > 1000) rawLDR = 1000;
    int rawSensorVal = 1100 - rawLDR;

    double sensorDiff = rawSensorVal - sensorLDRSmoothed;
    sensorLDRSmoothed += (sensorDiff / sensorSmoothCountLDR);

    double sensorSmoothedResult = sensorLDRSmoothed - dimDark;
    if (sensorSmoothedResult < dimDark) sensorSmoothedResult = dimDark;
    if (sensorSmoothedResult > dimBright) sensorSmoothedResult = dimBright;
    sensorSmoothedResult = (sensorSmoothedResult - dimDark) * sensorFactor;

    int returnValue = sensorSmoothedResult * 2;

    if (returnValue < minDim) returnValue = minDim;
    if (returnValue > DIGIT_DISPLAY_OFF) returnValue = DIGIT_DISPLAY_OFF;
    return returnValue;
  } else {
    return DIGIT_DISPLAY_OFF;  
  }
}

//**********************************************************************************
//**********************************************************************************
//*                               JSON functions                                   *
//**********************************************************************************
//**********************************************************************************

boolean getConfigFromSpiffs() {
  boolean loaded = false;
  if (SPIFFS.begin()) {
    debugMsg("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      debugMsg("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        debugMsg("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        debugMsg("\n");

        if (json.success()) {
          debugMsg("parsed json");

          char tzsTmp[256];
          strcpy(tzsTmp, json["time_zone"]);
          timeServerURL = String(tzsTmp);
          debugMsg("Loaded time server URL: " + timeServerURL);

          hourMode = json["hourMode"].as<bool>();
          debugMsg("Loaded 12/24H mode: " + String(hourMode));

          blankLeading = json["blankLeading"].as<bool>();
          debugMsg("Loaded lead zero blanking: " + String(blankLeading));

          scrollback = json["scrollback"].as<bool>();
          debugMsg("Loaded ScrollBack mode: " + String(scrollback));

          fade = json["fade"].as<bool>();
          debugMsg("Loaded Fade mode: " + String(fade));

          fadeSteps = json["fadeSteps"];
          debugMsg("Loaded Fade Steps mode: " + String(fadeSteps));

          dateFormat = json["dateFormat"];
          debugMsg("Loaded date format: " + String(dateFormat));

          dimDark = json["dimDark"];
          debugMsg("Loaded dimDark: " + String(dimDark));

          scrollSteps = json["scrollSteps"];
          debugMsg("Loaded scroll steps: " + String(scrollSteps));

          dimBright = json["dimBright"];
          debugMsg("Loaded dimBright: " + String(dimBright));

          sensorSmoothCountLDR = json["sensorSmoothCountLDR"];
          debugMsg("Loaded sensorSmoothCountLDR: " + String(sensorSmoothCountLDR));

          backlightMode = json["backlightMode"];
          debugMsg("Loaded backlight mode: " + String(backlightMode));

          redCnl = json["redCnl"];
          debugMsg("Loaded redCnl: " + String(redCnl));

          grnCnl = json["grnCnl"];
          debugMsg("Loaded grnCnl: " + String(grnCnl));

          bluCnl = json["bluCnl"];
          debugMsg("Loaded bluCnl: " + String(bluCnl));

          blankHourStart = json["blankHourStart"];
          debugMsg("Loaded blankHourStart: " + String(blankHourStart));

          blankHourEnd = json["blankHourEnd"];
          debugMsg("Loaded blankHourEnd: " + String(blankHourEnd));

          cycleSpeed = json["cycleSpeed"];
          debugMsg("Loaded cycleSpeed: " + String(cycleSpeed));

          minDim = json["minDim"];
          debugMsg("Loaded minDim: " + String(minDim));

          antiGhost = json["antiGhost"];
          debugMsg("Loaded antiGhost: " + String(antiGhost));

          pirTimeout = json["pirTimeout"];
          debugMsg("Loaded pirTimeout: " + String(pirTimeout));

          useLDR = json["useLDR"];
          debugMsg("Loaded useLDR: " + String(useLDR));

          blankMode = json["blankMode"];
          debugMsg("Loaded blankMode: " + String(blankMode));

          slotsMode = json["slotsMode"];
          debugMsg("Loaded slotsMode: " + String(slotsMode));

          usePIRPullup = json["usePIRPullup"];
          debugMsg("Loaded usePIRPullup: " + String(usePIRPullup));

          testMode = json["testMode"].as<bool>();
          debugMsg("Loaded testMode: " + String(testMode));

          ledMode = json["ledMode"];
          debugMsg("Loaded ledMode: " + String(ledMode));

          loaded = true;
        } else {
          debugMsg("failed to load json config");
        }
        debugMsg("Closing config file");
        configFile.close();
      }
    }
  } else {
    debugMsg("failed to mount FS");
  }

  SPIFFS.end();
  return loaded;
}

void saveConfigToSpiffs() {
  if (SPIFFS.begin()) {
    debugMsg("mounted file system");
    debugMsg("saving config");

    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["time_zone"] = timeServerURL;
    json["hourMode"] = hourMode;
    json["blankLeading"] = blankLeading;
    json["scrollback"] = scrollback;
    json["fade"] = fade;
    json["fadeSteps"] = fadeSteps;
    json["dateFormat"] = dateFormat;
    json["dayBlanking"] = dayBlanking;
    json["dimDark"] = dimDark;
    json["scrollSteps"] = scrollSteps;
    json["dimBright"] = dimBright;
    json["sensorSmoothCountLDR"] = sensorSmoothCountLDR;
    json["backlightMode"] = backlightMode;
    json["redCnl"] = redCnl;
    json["grnCnl"] = grnCnl;
    json["bluCnl"] = bluCnl;
    json["blankHourStart"] = blankHourStart;
    json["blankHourEnd"] = blankHourEnd;
    json["cycleSpeed"] = cycleSpeed;
    json["minDim"] = minDim;
    json["antiGhost"] = antiGhost;
    json["pirTimeout"] = pirTimeout;
    json["useLDR"] = useLDR;
    json["blankMode"] = blankMode;
    json["slotsMode"] = slotsMode;
    json["usePIRPullup"] = usePIRPullup;
    json["testMode"] = testMode;
    json["ledMode"] = ledMode;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      debugMsg("failed to open config file for writing");
      configFile.close();
      return;
    }

    json.printTo(Serial);
    debugMsg("\n");

    json.printTo(configFile);
    configFile.close();
    debugMsg("Saved config");
    //end save
  } else {
    debugMsg("failed to mount FS");
  }
  SPIFFS.end();
}

boolean getStatsFromSpiffs() {
  boolean loaded = false;
  if (SPIFFS.begin()) {
    debugMsg("mounted file system");
    if (SPIFFS.exists("/stats.json")) {
      //file exists, reading and loading
      debugMsg("reading stats file");
      File statsFile = SPIFFS.open("/stats.json", "r");
      if (statsFile) {
        debugMsg("opened stats file");
        size_t size = statsFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        statsFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        debugMsg("\n");

        if (json.success()) {
          debugMsg("parsed stats json");

          uptimeMins = json.get<unsigned long>("uptime");
          debugMsg("Loaded uptime: " + String(uptimeMins));

          tubeOnTimeMins = json.get<unsigned long>("tubeontime");
          debugMsg("Loaded tubeontime: " + String(tubeOnTimeMins));

          loaded = true;
        } else {
          debugMsg("failed to load json config");
        }
        debugMsg("Closing stats file");
        statsFile.close();
      }
    }
  } else {
    debugMsg("failed to mount FS");
  }

  SPIFFS.end();
  return loaded;
}

void saveStatsToSpiffs() {
  if (SPIFFS.begin()) {
    debugMsg("mounted file system");
    debugMsg("saving stats");

    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json.set("uptime", uptimeMins);
    json.set("tubeontime", tubeOnTimeMins);

    File statsFile = SPIFFS.open("/stats.json", "w");
    if (!statsFile) {
      debugMsg("failed to open stats file for writing");
      statsFile.close();
      return;
    }

    json.printTo(Serial);
    debugMsg("\n");

    json.printTo(statsFile);
    statsFile.close();
    debugMsg("Saved stats");
    //end save
  } else {
    debugMsg("failed to mount FS");
  }
  SPIFFS.end();
}

// ************************************************************
// Reset configuration values back to what they once were
// ************************************************************
void factoryReset() {
  hourMode = HOUR_MODE_DEFAULT;
  blankLeading = LEAD_BLANK_DEFAULT;
  scrollback = SCROLLBACK_DEFAULT;
  fade = FADE_DEFAULT;
  fadeSteps = FADE_STEPS_DEFAULT;
  dateFormat = DATE_FORMAT_DEFAULT;
  dayBlanking = DAY_BLANKING_DEFAULT;
  dimDark = SENSOR_LOW_DEFAULT;
  scrollSteps = SCROLL_STEPS_DEFAULT;
  dimBright = SENSOR_HIGH_DEFAULT;
  sensorSmoothCountLDR = SENSOR_SMOOTH_READINGS_DEFAULT;
  dateFormat = DATE_FORMAT_DEFAULT;
  dayBlanking = DAY_BLANKING_DEFAULT;
  backlightMode = BACKLIGHT_DEFAULT;
  redCnl = COLOUR_RED_CNL_DEFAULT;
  grnCnl = COLOUR_GRN_CNL_DEFAULT;
  bluCnl = COLOUR_BLU_CNL_DEFAULT;
  blankHourStart = 0;
  blankHourEnd = 7;
  cycleSpeed = CYCLE_SPEED_DEFAULT;
  minDim = MIN_DIM_DEFAULT;
  antiGhost = ANTI_GHOST_DEFAULT;
  pirTimeout = PIR_TIMEOUT_DEFAULT;
  useLDR = USE_LDR_DEFAULT;
  blankMode = BLANK_MODE_DEFAULT;
  slotsMode = SLOTS_MODE_DEFAULT;
  usePIRPullup = USE_PIR_PULLUP_DEFAULT;
  ledMode = LED_BLINK_DEFAULT;
  testMode = true;

  saveConfigToSpiffs();
}

//**********************************************************************************
//**********************************************************************************
//*                                Page Handlers                                   *
//**********************************************************************************
//**********************************************************************************

/**
   Root page for the webserver
*/
void rootPageHandler() {
  debugMsg("Root page in");
  digitalWrite(BUILTIN_LED_PIN, true);
  
  String response_message = getHTMLHead();
  response_message += getNavBar();

  // Status table
  response_message += getTableHead2Col("Current Status", "Name", "Value");

  if (WiFi.status() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    response_message += getTableRow2Col("WLAN IP", formatIPAsString(ip));
    response_message += getTableRow2Col("WLAN MAC", WiFi.macAddress());
    response_message += getTableRow2Col("WLAN SSID", WiFi.SSID());
    response_message += getTableRow2Col("Time server URL", timeServerURL);
    lastTimeFromServer = getTimeFromTimeZoneServer();
    response_message += getTableRow2Col("Time according to server", lastTimeFromServer);
  }
  else
  {
    IPAddress softapip = WiFi.softAPIP();
    String ipStrAP = String(softapip[0]) + '.' + String(softapip[1]) + '.' + String(softapip[2]) + '.' + String(softapip[3]);
    response_message += getTableRow2Col("AP IP", ipStrAP);
    response_message += getTableRow2Col("AP MAC", WiFi.softAPmacAddress());
  }

  // Make the uptime readable
  long upSecs = millis() / 1000;
  long upDays = upSecs / 86400;
  long upHours = (upSecs - (upDays * 86400)) / 3600;
  long upMins = (upSecs - (upDays * 86400) - (upHours * 3600)) / 60;
  upSecs = upSecs - (upDays * 86400) - (upHours * 3600) - (upMins * 60);
  String uptimeString = ""; uptimeString += upDays; uptimeString += " days, "; uptimeString += upHours, uptimeString += " hours, "; uptimeString += upMins; uptimeString += " mins, "; uptimeString += upSecs; uptimeString += " secs";

  response_message += getTableRow2Col("Uptime", uptimeString);

  String lastUpdateString = ""; lastUpdateString += (nowMillis - lastUpdateFromServer);
  response_message += getTableRow2Col("Time since last update", lastUpdateString);

  response_message += getTableRow2Col("Version", SOFTWARE_VERSION);

  // Scan I2C bus
  for (int idx = 0 ; idx < 128 ; idx++)
  {
    Wire.beginTransmission(idx);
    int error = Wire.endTransmission();
    if (error == 0) {
      String slaveMsg = "Found I2C slave at";
      response_message += getTableRow2Col(slaveMsg,idx);
    }
  }

  response_message += getTableFoot();

  // ESP8266 Info table
  double onTimeHrs = uptimeMins/60.0;
  double tubeOnTimeHrs = tubeOnTimeMins/60.0;
  float digitBrightness = digitOffCount * 100.0 / (float) DIGIT_DISPLAY_OFF;
  response_message += getTableHead2Col("ESP8266 information", "Name", "Value");
  response_message += getTableRow2Col("Sketch size", ESP.getSketchSize());
  response_message += getTableRow2Col("Free sketch size", ESP.getFreeSketchSpace());
  response_message += getTableRow2Col("Free heap", ESP.getFreeHeap());
  response_message += getTableRow2Col("Boot version", ESP.getBootVersion());
  response_message += getTableRow2Col("CPU Freqency (MHz)", ESP.getCpuFreqMHz());
  response_message += getTableRow2Col("SDK version", ESP.getSdkVersion());
  response_message += getTableRow2Col("Chip ID", ESP.getChipId());
  response_message += getTableRow2Col("Flash Chip ID", ESP.getFlashChipId());
  response_message += getTableRow2Col("Flash size", ESP.getFlashChipRealSize());
  response_message += getTableRow2Col("LDR Value", sensorLDRSmoothed);
  response_message += getTableRow2Col("Digit brightness %", String(digitBrightness,2));
  response_message += getTableRow2Col("Impressions/Sec", lastImpressionsPerSec);
  response_message += getTableRow2Col("Total Clock On Hrs", String(onTimeHrs,2));
  response_message += getTableRow2Col("Total Tube On Hrs", String(tubeOnTimeHrs,2));
  response_message += getTableFoot();

  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);
  
  digitalWrite(BUILTIN_LED_PIN, false);
  debugMsg("Root page out");
}

// ===================================================================================================================
// ===================================================================================================================

/**
   Get the local time from the time server, and modify the time server URL if needed
*/
void timeServerPageHandler() {
  debugMsg("Time page in");
  digitalWrite(BUILTIN_LED_PIN, true);
  
  // Check if there are any GET parameters, if there are, we are configuring
  if (server.hasArg("timeserverurl"))
  {
    if (strlen(server.arg("timeserverurl").c_str()) > 4) {
      timeServerURL = server.arg("timeserverurl").c_str();
    }
  }

  String response_message = getHTMLHead();
  response_message += getNavBar();

  // form header
  response_message += getFormHead("Select time server");

  // only fill in the value we have if it looks realistic
  if ((timeServerURL.length() < 10) || (timeServerURL.length() > 250)) {
    debugMsg("Got a bad URL for the TZS, resetting");
    timeServerURL = DEFAULT_TIME_SERVER_URL_1;
  } else {
    // save the timeserver URL
    saveConfigToSpiffs();
  }

  response_message += getTextInputWide("URL", "timeserverurl", timeServerURL, false);
  response_message += getSubmitButton("Set");

  response_message += getFormFoot();
  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);
  
  digitalWrite(BUILTIN_LED_PIN, false);
  debugMsg("Time page out");
}

// ===================================================================================================================
// ===================================================================================================================

// ===================================================================================================================
// ===================================================================================================================

/**
   Reset the EEPROM and stored values
*/
void resetPageHandler() {
  debugMsg("Reset page in");
  digitalWrite(BUILTIN_LED_PIN, true);

  WiFiManager wifiManager;
  wifiManager.resetSettings();
  factoryReset();

  String response_message = getHTMLHead();
  response_message += getNavBar();

  response_message += "<div class=\"container\" role=\"main\"><h3 class=\"sub-header\">Send time to I2C right now</h3>";
  response_message += "<div class=\"alert alert-success fade in\"><strong>Success!</strong> Reset done.</div></div>";

  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);

  digitalWrite(BUILTIN_LED_PIN, false);
  debugMsg("Reset page out");

  // do the restart
  ESP.restart();
  
}

// ===================================================================================================================
// ===================================================================================================================

/**
   Page for the clock configuration.
*/
void clockConfigPageHandler() {
  debugMsg("Config page in");
  digitalWrite(BUILTIN_LED_PIN, true);

  boolean changed = false;
  
  if (server.hasArg("12h24hMode"))
  {
    debugMsg("Got 24h mode param: " + server.arg("12h24hMode"));
    if ((server.arg("12h24hMode") == "24h") && (hourMode)) {
      debugMsg("-> Set 24h mode");
      changed = true;
      hourMode = false;
    }

    if ((server.arg("12h24hMode") == "12h") && (!hourMode)) {
      debugMsg("-> Set 12h mode");
      changed = true;
      hourMode = true;
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("blankLeading"))
  {
    debugMsg("Got blankLeading param: " + server.arg("blankLeading"));
    if ((server.arg("blankLeading") == "blank") && (!blankLeading)) {
      debugMsg("-> Set blank leading zero");
      changed = true;
      blankLeading = true;
    }

    if ((server.arg("blankLeading") == "show") && (blankLeading)) {
      debugMsg("-> Set show leading zero");
      changed = true;
      blankLeading = false;
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("useScrollback"))
  {
    debugMsg("Got useScrollback param: " + server.arg("useScrollback"));
    if ((server.arg("useScrollback") == "on") && (!scrollback)) {
      debugMsg("-> Set scrollback on");
      changed = true;
      scrollback = true;
    }

    if ((server.arg("useScrollback") == "off") && (scrollback)) {
      debugMsg("-> Set scrollback off");
      changed = true;
      scrollback = false;
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("fade"))
  {
    debugMsg("Got fade param: " + server.arg("fade"));
    if ((server.arg("fade") == "on") && (!fade)) {
      debugMsg("-> Set fade on");
      changed = true;
      fade = true;
    }

    if ((server.arg("fade") == "off") && (fade)) {
      debugMsg("-> Set fade off");
      changed = true;
      fade = false;
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("useLDR"))
  {
    debugMsg("Got useLDR param: " + server.arg("useLDR"));
    if ((server.arg("useLDR") == "on") && (!useLDR)) {
      debugMsg("-> Set useLDR on");
      changed = true;
      useLDR = true;
    }

    if ((server.arg("useLDR") == "off") && (useLDR)) {
      debugMsg("-> Set useLDR off");
      changed = true;
      useLDR = false;
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("dateFormat")) {
    debugMsg("Got dateFormat param: " + server.arg("dateFormat"));
    byte newDateFormat = atoi(server.arg("dateFormat").c_str());
    if (newDateFormat != dateFormat) {
      changed = true;
      dateFormat = newDateFormat;
      debugMsg("-> Set dateFormat: " + String(newDateFormat));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("dayBlanking")) {
    debugMsg("Got dayBlanking param: " + server.arg("dayBlanking"));
    byte newDayBlanking = atoi(server.arg("dayBlanking").c_str());
    if (newDayBlanking != dayBlanking) {
      changed = true;
      dayBlanking = newDayBlanking;
      debugMsg("-> Set dayBlanking: " + String(newDayBlanking));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("blankHourStart")) {
    debugMsg("Got blankHourStart param: " + server.arg("blankHourStart"));
    byte newBlankFrom = atoi(server.arg("blankHourStart").c_str());
    if (newBlankFrom != blankHourStart) {
      changed = true;
      blankHourStart = newBlankFrom;
      debugMsg("-> Set blankHourStart: " + String(newBlankFrom));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("blankHourEnd")) {
    debugMsg("Got blankHourEnd param: " + server.arg("blankHourEnd"));
    byte newBlankTo = atoi(server.arg("blankHourEnd").c_str());
    if (newBlankTo != blankHourEnd) {
      changed = true;
      blankHourEnd = newBlankTo;
      debugMsg("-> Set blankHourEnd: " + String(newBlankTo));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("fadeSteps")) {
    debugMsg("Got fadeSteps param: " + server.arg("fadeSteps"));
    byte newFadeSteps = atoi(server.arg("fadeSteps").c_str());
    if (newFadeSteps != fadeSteps) {
      changed = true;
      fadeSteps = newFadeSteps;
      debugMsg("-> Set fadeSteps: " + String(newFadeSteps));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("scrollSteps")) {
    debugMsg("Got scrollSteps param: " + server.arg("scrollSteps"));
    byte newScrollSteps = atoi(server.arg("scrollSteps").c_str());
    if (newScrollSteps != scrollSteps) {
      changed = true;
      scrollSteps = newScrollSteps;
      debugMsg("-> Set fadeSteps: " + String(newScrollSteps));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("backLight")) {
    debugMsg("Got backLight param: " + server.arg("backLight"));
    byte newBacklight = atoi(server.arg("backLight").c_str());
    if (newBacklight != backlightMode) {
      changed = true;
      backlightMode = newBacklight;
      debugMsg("-> Set backLight: " + String(newBacklight));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("redCnl")) {
    debugMsg("Got redCnl param: " + server.arg("redCnl"));
    byte newRedCnl = atoi(server.arg("redCnl").c_str());
    if (newRedCnl != redCnl) {
      changed = true;
      redCnl = newRedCnl;
      debugMsg("-> Set redCnl: " + String(newRedCnl));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("grnCnl")) {
    debugMsg("Got grnCnl param: " + server.arg("grnCnl"));
    byte newGreenCnl = atoi(server.arg("grnCnl").c_str());
    if (newGreenCnl != grnCnl) {
      changed = true;
      grnCnl = newGreenCnl;
      debugMsg("-> Set grnCnl: " + String(newGreenCnl));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("bluCnl")) {
    debugMsg("Got bluCnl param: " + server.arg("bluCnl"));
    byte newBlueCnl = atoi(server.arg("bluCnl").c_str());
    if (newBlueCnl != bluCnl) {
      changed = true;
      bluCnl = newBlueCnl;
      debugMsg("-> Set bluCnl: " + String(newBlueCnl));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("cycleSpeed")) {
    debugMsg("Got cycleSpeed param: " + server.arg("cycleSpeed"));
    byte newCycleSpeed = atoi(server.arg("cycleSpeed").c_str());
    if (newCycleSpeed != cycleSpeed) {
      changed = true;
      cycleSpeed = newCycleSpeed;
      debugMsg("-> Set cycleSpeed: " + String(newCycleSpeed));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("blankMode")) {
    debugMsg("Got blankMode param: " + server.arg("blankMode"));
    byte newBlankMode = atoi(server.arg("blankMode").c_str());
    if (newBlankMode != blankMode) {
      changed = true;
      blankMode = newBlankMode;
      debugMsg("-> Set blankMode: " + String(newBlankMode));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("slotsMode")) {
    debugMsg("Got slotsMode param: " + server.arg("slotsMode"));
    byte newSlotsMode = atoi(server.arg("slotsMode").c_str());
    if (newSlotsMode != slotsMode) {
      changed = true;
      slotsMode = newSlotsMode;
      debugMsg("-> Set slotsMode: " + String(newSlotsMode));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("pirTimeout")) {
    debugMsg("Got pirTimeout param: " + server.arg("pirTimeout"));
    int newPirTimeOut = atoi(server.arg("pirTimeout").c_str());
    if (newPirTimeOut != pirTimeout) {
      changed = true;
      pirTimeout = newPirTimeOut;
      debugMsg("-> Set pirTimeout: " + String(newPirTimeOut));
    }
  }

  // -----------------------------------------------------------------------------
  
  if (server.hasArg("usePirPullup"))
  {
    debugMsg("Got usePirPullup param: " + server.arg("usePirPullup"));
    if ((server.arg("usePirPullup") == "on") && (!usePIRPullup)) {
      changed = true;
      debugMsg("-> Set usePirPullup on");
      usePIRPullup = true;
    }

    if ((server.arg("usePirPullup") == "off") && (usePIRPullup)) {
      changed = true;
      debugMsg("-> Set usePirPullup off");
      usePIRPullup = false;
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("minDim")) {
    debugMsg("Got minDim param: " + server.arg("minDim"));
    int newMinDim = atoi(server.arg("minDim").c_str());
    if (newMinDim != minDim) {
      changed = true;
      minDim = newMinDim;
      debugMsg("-> Set minDim: " + String(newMinDim));
    }
  }

  // -----------------------------------------------------------------------------

  if (server.hasArg("ledMode")) {
    debugMsg("Got ledMode param: " + server.arg("ledMode"));
    byte newLedMode = atoi(server.arg("ledMode").c_str());
    if (newLedMode != ledMode) {
      changed = true;
      ledMode = newLedMode;
      debugMsg("-> Set ledMode: " + String(newLedMode));
    }
  }
  // -----------------------------------------------------------------------------

  // Save the options
  if (changed) {
    debugMsg("Config options changed, saving them");
    saveConfigToSpiffs();
  }

  String response_message = getHTMLHead();
  response_message += getNavBar();

  // form header
  response_message += getFormHead("Set Configuration");

  // 12/24 mode
  response_message += getRadioGroupHeader("12H/24H mode:");
  if (hourMode) {
    response_message += getRadioButton("12h24hMode", " 12H", "12h", true);
    response_message += getRadioButton("12h24hMode", " 24H", "24h", false);
  } else {
    response_message += getRadioButton("12h24hMode", " 12H", "12h", false);
    response_message += getRadioButton("12h24hMode", " 24H", "24h", true);
  }
  response_message += getRadioGroupFooter();

  // blank leading
  response_message += getRadioGroupHeader("Blank leading zero:");
  if (blankLeading) {
    response_message += getRadioButton("blankLeading", "Blank", "blank", true);
    response_message += getRadioButton("blankLeading", "Show", "show", false);
  } else {
    response_message += getRadioButton("blankLeading", "Blank", "blank", false);
    response_message += getRadioButton("blankLeading", "Show", "show", true);
  }
  response_message += getRadioGroupFooter();
  //response_message += getCheckBox("blankLeadingZero", "on", "Blank leading zero on hours", (configBlankLead == 1));

  // Scrollback
  response_message += getRadioGroupHeader("Scrollback effect:");
  if (scrollback) {
    response_message += getRadioButton("useScrollback", "On", "on", true);
    response_message += getRadioButton("useScrollback", "Off", "off", false);
  } else {
    response_message += getRadioButton("useScrollback", "On", "on", false);
    response_message += getRadioButton("useScrollback", "Off", "off", true);
  }
  response_message += getRadioGroupFooter();

  // Scroll steps
  response_message += getNumberInput("Scroll steps:", "scrollSteps", SCROLL_STEPS_MIN, SCROLL_STEPS_MAX, scrollSteps, false);

  // fade
  response_message += getRadioGroupHeader("Fade effect:");
  if (fade) {
    response_message += getRadioButton("fade", "On", "on", true);
    response_message += getRadioButton("fade", "Off", "off", false);
  } else {
    response_message += getRadioButton("fade", "On", "on", false);
    response_message += getRadioButton("fade", "Off", "off", true);
  }
  response_message += getRadioGroupFooter();

  // Fade steps
  response_message += getNumberInput("Fade steps:", "fadeSteps", FADE_STEPS_MIN, FADE_STEPS_MAX, fadeSteps, false);

  // LDR
  response_message += getRadioGroupHeader("Use LDR:");
  if (useLDR) {
    response_message += getRadioButton("useLDR", "On", "on", true);
    response_message += getRadioButton("useLDR", "Off", "off", false);
  } else {
    response_message += getRadioButton("useLDR", "On", "on", false);
    response_message += getRadioButton("useLDR", "Off", "off", true);
  }
  response_message += getRadioGroupFooter();

  // Date format
  response_message += getDropDownHeader("Date format:", "dateFormat", false);
  response_message += getDropDownOption("0", "YY-MM-DD", (dateFormat == 0));
  response_message += getDropDownOption("1", "MM-DD-YY", (dateFormat == 1));
  response_message += getDropDownOption("2", "DD-MM-YY", (dateFormat == 2));
  response_message += getDropDownFooter();

  // Day blanking
  response_message += getDropDownHeader("Day blanking:", "dayBlanking", true);
  response_message += getDropDownOption("0", "Never blank", (dayBlanking == 0));
  response_message += getDropDownOption("1", "Blank all day on weekends", (dayBlanking == 1));
  response_message += getDropDownOption("2", "Blank all day on week days", (dayBlanking == 2));
  response_message += getDropDownOption("3", "Blank always", (dayBlanking == 3));
  response_message += getDropDownOption("4", "Blank during selected hours every day", (dayBlanking == 4));
  response_message += getDropDownOption("5", "Blank during selected hours on week days and all day on weekends", (dayBlanking == 5));
  response_message += getDropDownOption("6", "Blank during selected hours on weekends and all day on week days", (dayBlanking == 6));
  response_message += getDropDownOption("7", "Blank during selected hours on weekends only", (dayBlanking == 7));
  response_message += getDropDownOption("8", "Blank during selected hours on week days only", (dayBlanking == 8));
  response_message += getDropDownFooter();

  // Blank Mode
  response_message += getDropDownHeader("Blank Mode:", "blankMode", true);
  response_message += getDropDownOption("0", "Blank tubes only", (blankMode == 0));
  response_message += getDropDownOption("1", "Blank LEDs only", (blankMode == 1));
  response_message += getDropDownOption("2", "Blank tubes and LEDs", (blankMode == 2));
  response_message += getDropDownFooter();
  
  boolean hoursDisabled = (dayBlanking < 4);

  // Blank hours from
  response_message += getNumberInput("Blank from:", "blankHourStart", 0, 23, blankHourStart, hoursDisabled);

  // Blank hours to
  response_message += getNumberInput("Blank to:", "blankHourEnd", 0, 23, blankHourEnd, hoursDisabled);

  // Back light
  response_message += getDropDownHeader("Back light:", "backLight", true);
  response_message += getDropDownOption("0", "Fixed RGB backlight, no dimming", (backlightMode == 0));
  response_message += getDropDownOption("1", "Pulsing RGB backlight, no dimming", (backlightMode == 1));
  response_message += getDropDownOption("2", "Cycling RGB backlight, no dimming", (backlightMode == 2));
  response_message += getDropDownOption("3", "Fixed RGB backlight, dims with ambient light", (backlightMode == 3));
  response_message += getDropDownOption("4", "Pulsing RGB backlight, dims with ambient light", (backlightMode == 4));
  response_message += getDropDownOption("5", "Cycling RGB backlight, dims with ambient light", (backlightMode == 5));
  response_message += getDropDownOption("6", "'Colourtime' backlight", (backlightMode == 6));
  response_message += getDropDownOption("7", "'Colourtime' backlight, dims with ambient light", (backlightMode == 7));
  response_message += getDropDownFooter();

  // RGB channels
  response_message += getNumberInput("Red intensity:", "redCnl", 0, 15, redCnl, false);
  response_message += getNumberInput("Green intensity:", "grnCnl", 0, 15, grnCnl, false);
  response_message += getNumberInput("Blue intensity:", "bluCnl", 0, 15, bluCnl, false);

  // Cycle speed
  response_message += getNumberInput("Backlight Cycle Speed:", "cycleSpeed", CYCLE_SPEED_MIN, CYCLE_SPEED_MAX, cycleSpeed, false);

  // Slots Mode
  response_message += getDropDownHeader("Date Slots:", "slotsMode", true);
  response_message += getDropDownOption("0", "Don't use slots mode", (slotsMode == 0));
  response_message += getDropDownOption("1", "Scroll In, Scramble Out", (slotsMode == 1));
  response_message += getDropDownFooter();

  // LED mode
  response_message += getDropDownHeader("LED Mode:", "ledMode", true);
  response_message += getDropDownOption("0", "RailRoad", (ledMode == LED_RAILROAD));
  response_message += getDropDownOption("1", "Slow flash", (ledMode == LED_BLINK_SLOW));
  response_message += getDropDownOption("2", "Fast flash", (ledMode == LED_BLINK_FAST));
  response_message += getDropDownOption("3", "Double flash", (ledMode == LED_BLINK_DBL));
  response_message += getDropDownFooter();
  
  // PIR timeout
  response_message += getNumberInput("PIR timeout:", "pirTimeout", PIR_TIMEOUT_MIN, PIR_TIMEOUT_MAX, pirTimeout, false);

  // PIR pullup
  response_message += getRadioGroupHeader("Use PIR pullup:");
  if (usePIRPullup) {
    response_message += getRadioButton("usePirPullup", "On", "on", true);
    response_message += getRadioButton("usePirPullup", "Off", "off", false);
  } else {
    response_message += getRadioButton("usePirPullup", "On", "on", false);
    response_message += getRadioButton("usePirPullup", "Off", "off", true);
  }
  response_message += getRadioGroupFooter();

  // Min dim
  response_message += getNumberInput("Min Dim:", "minDim", MIN_DIM_MIN, MIN_DIM_MAX, minDim, false);

  // form footer
  response_message += getSubmitButton("Set");

  response_message += "</form></div>";

  // all done
  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);

  digitalWrite(BUILTIN_LED_PIN, false);
  debugMsg("Config page out");
}

// ===================================================================================================================
// ===================================================================================================================

/**
   Send a value to the clock for display
*/
void setDisplayValuePageHandler() {
  String resultMessage;
  byte timeValue = 0xff;
  boolean argsValid = false;

  if (server.hasArg("value"))
  {
    debugMsg("Got server arg value: " + server.arg("value"));
    long valueToSend = atol(server.arg("value").c_str());
    byte val1 = hex2bcd(valueToSend % 100);
    valueToSend = valueToSend / 100;
    byte val0 = hex2bcd(valueToSend % 100);

    valueToShow[0] = val0;
    valueToShow[1] = val1;

    resultMessage += "Set Value  ";
    argsValid = true;
  }

  if (server.hasArg("time"))
  {
    debugMsg("Got time");
    debugMsg(server.arg("time"));
    unsigned int timeToSend = atoi(server.arg("time").c_str());
    if (timeToSend > 255) 
    {
      valueDisplayTime = 0xff;
    } else {
      valueDisplayTime = timeToSend;
      resultMessage += "Set Time  ";
    }
  };

  if (server.hasArg("format"))
  {
    debugMsg("Got format");
    debugMsg(server.arg("format"));

    long formatToSend = atol(server.arg("format").c_str());
    byte val2 = hex2bcd(formatToSend % 100);
    formatToSend = formatToSend / 100;
    byte val1 = hex2bcd(formatToSend % 100);
    formatToSend = formatToSend / 100;
    byte val0 = hex2bcd(formatToSend % 100);
    valueDisplayType[0] = val0;
    valueDisplayType[1] = val1;
    valueDisplayType[2] = val2;

    resultMessage += "Set Format  ";
  };

  String response_message = getHTMLHead();
  response_message += getNavBar();

  response_message += "<div class=\"container\" role=\"main\"><h3 class=\"sub-header\">Show a value on the clock</h3>";

  if (argsValid) {
    response_message += "<div class=\"alert alert-success fade in\"><strong>";
    response_message += resultMessage;
    response_message += "</strong></div></div>";
  } else {
    response_message += "<div class=\"alert alert-error fade in\"><strong>";
    response_message += "You need to set at least the \"Time\" parameter! A valid command line is http://<clock.ip.addres>/setvalue?value=123456&time=60&format=333333";
    response_message += "</strong></div></div>";
  }

  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);
}

unsigned char hex2bcd (unsigned char x)
{
    unsigned char y;
    y = (x / 10) << 4;
    y = y | (x % 10);
    return (y);
}

// ===================================================================================================================
// ===================================================================================================================

/* Called if requested page is not found */
void handleNotFound() {
  debugMsg("404 page in");
  digitalWrite(BUILTIN_LED_PIN, true);

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);

  digitalWrite(BUILTIN_LED_PIN, false);
  debugMsg("404 page out");
}

// ===================================================================================================================
// ===================================================================================================================

/* Called if we need to have a local CSS */
void localCSSHandler()
{
  PGM_P cssStr = PSTR(".navbar,.table{margin-bottom:20px}.nav>li,.nav>li>a,article,aside,details,figcaption,figure,footer,header,hgroup,main,menu,nav,section,summary{display:block}.btn,.form-control,.navbar-toggle{background-image:none}.table,label{max-width:100%}.sub-header{padding-bottom:10px;border-bottom:1px solid #eee}.h3,h3{font-size:24px}.table{width:100%}table{background-color:transparent;border-spacing:0;border-collapse:collapse}.table-striped>tbody>tr:nth-of-type(2n+1){background-color:#f9f9f9}.table>caption+thead>tr:first-child>td,.table>caption+thead>tr:first-child>th,.table>colgroup+thead>tr:first-child>td,.table>colgroup+thead>tr:first-child>th,.table>thead:first-child>tr:first-child>td,.table>thead:first-child>tr:first-child>th{border-top:0}.table>thead>tr>th{border-bottom:2px solid #ddd}.table>tbody>tr>td,.table>tbody>tr>th,.table>tfoot>tr>td,.table>tfoot>tr>th,.table>thead>tr>td,.table>thead>tr>th{padding:8px;line-height:1.42857143;vertical-align:top;border-top:1px solid #ddd}th{text-align:left}td,th{padding:0}.navbar>.container .navbar-brand,.navbar>.container-fluid .navbar-brand{margin-left:-15px}.container,.container-fluid{padding-right:15px;padding-left:15px;margin-right:auto;margin-left:auto}.navbar-inverse .navbar-brand{color:#9d9d9d}.navbar-brand{float:left;height:50px;padding:15px;font-size:18px;line-height:20px}a{color:#337ab7;text-decoration:none;background-color:transparent}.navbar-fixed-top{border:0;top:0;border-width:0 0 1px}.navbar-inverse{background-color:#222;border-color:#080808}.navbar-fixed-bottom,.navbar-fixed-top{border-radius:0;position:fixed;right:0;left:0;z-index:1030}.nav>li,.nav>li>a,.navbar,.navbar-toggle{position:relative}.navbar{border-radius:4px;min-height:50px;border:1px solid transparent}.container{width:750px}.navbar-right{float:right!important;margin-right:-15px}.navbar-nav{float:left;margin:7.5px -15px}.nav{padding-left:0;margin-bottom:0;list-style:none}.navbar-nav>li{float:left}.navbar-inverse .navbar-nav>li>a{color:#9d9d9d}.navbar-nav>li>a{padding-top:10px;padding-bottom:10px;line-height:20px}.nav>li>a{padding:10px 15px}.navbar-inverse .navbar-toggle{border-color:#333}.navbar-toggle{display:none;float:right;padding:9px 10px;margin-top:8px;margin-right:15px;margin-bottom:8px;background-color:transparent;border:1px solid transparent;border-radius:4px}button,select{text-transform:none}button{overflow:visible}button,html input[type=button],input[type=reset],input[type=submit]{-webkit-appearance:button;cursor:pointer}.btn-primary{color:#fff;background-color:#337ab7;border-color:#2e6da4}.btn{display:inline-block;padding:6px 12px;margin-bottom:0;font-size:14px;font-weight:400;line-height:1.42857143;text-align:center;white-space:nowrap;vertical-align:middle;-ms-touch-action:manipulation;touch-action:manipulation;cursor:pointer;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;border:1px solid transparent;border-radius:4px}button,input,select,textarea{font-family:inherit;font-size:inherit;line-height:inherit}input{line-height:normal}button,input,optgroup,select,textarea{margin:0;font:inherit;color:inherit}.form-control,body{font-size:14px;line-height:1.42857143}.form-horizontal .form-group{margin-right:-15px;margin-left:-15px}.form-group{margin-bottom:15px}.form-horizontal .control-label{padding-top:7px;margin-bottom:0;text-align:right}.form-control{display:block;width:100%;height:34px;padding:6px 12px;color:#555;background-color:#fff;border:1px solid #ccc;border-radius:4px;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.075);box-shadow:inset 0 1px 1px rgba(0,0,0,.075);-webkit-transition:border-color ease-in-out .15s,-webkit-box-shadow ease-in-out .15s;-o-transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s;transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s}.col-xs-8{width:66.66666667%}.col-xs-3{width:25%}.col-xs-1,.col-xs-10,.col-xs-11,.col-xs-12,.col-xs-2,.col-xs-3,.col-xs-4,.col-xs-5,.col-xs-6,.col-xs-7,.col-xs-8,.col-xs-9{float:left}.col-lg-1,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9,.col-md-1,.col-md-10,.col-md-11,.col-md-12,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9,.col-sm-1,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9,.col-xs-1,.col-xs-10,.col-xs-11,.col-xs-12,.col-xs-2,.col-xs-3,.col-xs-4,.col-xs-5,.col-xs-6,.col-xs-7,.col-xs-8,.col-xs-9{position:relative;min-height:1px;padding-right:15px;padding-left:15px}label{display:inline-block;margin-bottom:5px;font-weight:700}*{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}body{font-family:\"Helvetica Neue\",Helvetica,Arial,sans-serif;color:#333}html{font-size:10px;font-family:sans-serif;-webkit-text-size-adjust:100%}");

  server.send(200, "text/css", String(cssStr));
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------- Network handling -----------------------------------------
// ----------------------------------------------------------------------------------------------------

/**
   Get the local time from the time zone server. Return the error description prefixed by "ERROR:" if something went wrong.
   Uses the global variable timeServerURL.
*/
String getTimeFromTimeZoneServer() {
  debugMsg("Get Time in");
  digitalWrite(BUILTIN_LED_PIN, true);
  
  HTTPClient http;
  String payload;

  http.begin(timeServerURL);
  String espId = "";espId += ESP.getChipId();
  http.addHeader("ESP",espId);

  int httpCode = http.GET();

  // file found at server
  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
    useWiFi = MAX_WIFI_TIME;
  } else {
    debugMsg("[HTTP] GET... failed, error: " + http.errorToString(httpCode));
    if (httpCode > 0) {
      // RFC error codes don't have a string mapping
      payload = "ERROR: " + String(httpCode);
    } else {
      // ESP error codes have a string mapping
      payload = "ERROR: " + String(httpCode) + " ("+ http.errorToString(httpCode) + ")";
    }
  }

  http.end();

  digitalWrite(BUILTIN_LED_PIN, false);
  debugMsg("Get Time out");

  return payload;
}

// ----------------------------------------------------------------------------------------------------
// ------------------------------------------- HTML functions -----------------------------------------
// ----------------------------------------------------------------------------------------------------

String getHTMLHead() {
  String header = "<!DOCTYPE html><html><head>";

  if (WiFi.status() == WL_CONNECTED) {
    header += "<link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-1q8mTJOASx8j1Au+a5WDVnPi2lkFfwwEAa8hDDdjZlpLegxhjVME1fgjWPGmkzs7\" crossorigin=\"anonymous\">";
    header += "<link href=\"http://www.open-rate.com/wl.css\" rel=\"stylesheet\" type=\"text/css\">";
    header += "<script src=\"http://code.jquery.com/jquery-1.12.3.min.js\" integrity=\"sha256-aaODHAgvwQW1bFOGXMeX+pC4PZIPsvn2h1sArYOhgXQ=\" crossorigin=\"anonymous\"></script>";
    header += "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\" integrity=\"sha384-0mSbJDEHialfmuBBQP6A4Qrprq5OVfW37PRR3j5ELqxss1yVqOtnepnHVP9aJ7xS\" crossorigin=\"anonymous\"></script>";
  } else {
    header += "<link href=\"local.css\" rel=\"stylesheet\">";
  }
  header += "<title>Arduino Nixie Clock Time Module</title></head>";
  header += "<body>";
  return header;
}

/**
   Get the bootstrap top row navbar, including the Bootstrap links
*/
String getNavBar() {
  String navbar = "<nav class=\"navbar navbar-inverse navbar-fixed-top\">";
  navbar += "<div class=\"container-fluid\"><div class=\"navbar-header\"><button type=\"button\" class=\"navbar-toggle collapsed\" data-toggle=\"collapse\" data-target=\"#navbar\" aria-expanded=\"false\" aria-controls=\"navbar\">";
  navbar += "<span class=\"sr-only\">Toggle navigation</span><span class=\"icon-bar\"></span><span class=\"icon-bar\"></span><span class=\"icon-bar\"></span></button>";
  navbar += "<a class=\"navbar-brand\" href=\"#\">Arduino Nixie Clock Time Module</a></div><div id=\"navbar\" class=\"navbar-collapse collapse\"><ul class=\"nav navbar-nav navbar-right\">";
  navbar += "<li><a href=\"/\">Summary</a></li><li><a href=\"/time\">Configure Time Server</a></li><li><a href=\"/clockconfig\">Configure clock settings</a></li></ul></div></div></nav>";
  return navbar;
} 

/**
   Get the header for a 2 column table
*/
String getTableHead2Col(String tableHeader, String col1Header, String col2Header) {
  String tableHead = "<div class=\"container\" role=\"main\"><h3 class=\"sub-header\">";
  tableHead += tableHeader;
  tableHead += "</h3><div class=\"table-responsive\"><table class=\"table table-striped\"><thead><tr><th>";
  tableHead += col1Header;
  tableHead += "</th><th>";
  tableHead += col2Header;
  tableHead += "</th></tr></thead><tbody>";

  return tableHead;
}

String getTableRow2Col(String col1Val, String col2Val) {
  String tableRow = "<tr><td>";
  tableRow += col1Val;
  tableRow += "</td><td>";
  tableRow += col2Val;
  tableRow += "</td></tr>";

  return tableRow;
}

String getTableRow2Col(String col1Val, int col2Val) {
  String tableRow = "<tr><td>";
  tableRow += col1Val;
  tableRow += "</td><td>";
  tableRow += col2Val;
  tableRow += "</td></tr>";

  return tableRow;
}

String getTableFoot() {
  return "</tbody></table></div></div>";
}

/**
   Get the header for an input form
*/
String getFormHead(String formTitle) {
  String tableHead = "<div class=\"container\" role=\"main\"><h3 class=\"sub-header\">";
  tableHead += formTitle;
  tableHead += "</h3><form class=\"form-horizontal\">";

  return tableHead;
}

/**
   Get the header for an input form
*/
String getFormFoot() {
  return "</form></div>";
}

String getHTMLFoot() {
  return "</body></html>";
}

String getRadioGroupHeader(String header) {
  String result = "<div class=\"form-group\"><label class=\"control-label col-xs-3\">";
  result += header;
  result += "</label>";
  return result;
}

String getRadioButton(String group_name, String text, String value, boolean checked) {
  String result = "<div class=\"col-xs-1\">";
  if (checked) {
    result += "<label class=\"radio-inline\"><input checked type=\"radio\" name=\"";
  } else {
    result += "<label class=\"radio-inline\"><input type=\"radio\" name=\"";
  }
  result += group_name;
  result += "\" value=\"";
  result += value;
  result += "\"> ";
  result += text;
  result += "</label></div>";
  return result;
}

String getRadioGroupFooter() {
  String result = "</div>";
  return result;
}

String getCheckBox(String checkbox_name, String value, String text, boolean checked) {
  String result = "<div class=\"form-group\"><div class=\"col-xs-offset-3 col-xs-9\"><label class=\"checkbox-inline\">";
  if (checked) {
    result += "<input checked type=\"checkbox\" name=\"";
  } else {
    result += "<input type=\"checkbox\" name=\"";
  }

  result += checkbox_name;
  result += "\" value=\"";
  result += value;
  result += "\"> ";
  result += text;
  result += "</label></div></div>";

  return result;
}

String getDropDownHeader(String heading, String group_name, boolean wide) {
  String result = "<div class=\"form-group\"><label class=\"control-label col-xs-3\">";
  result += heading;
  if (wide) {
    result += "</label><div class=\"col-xs-8\"><select class=\"form-control\" name=\"";
  } else {
    result += "</label><div class=\"col-xs-2\"><select class=\"form-control\" name=\"";
  }
  result += group_name;
  result += "\">";
  return result;
}

String getDropDownOption (String value, String text, boolean checked) {
  String result = "";
  if (checked) {
    result += "<option selected value=\"";
  } else {
    result += "<option value=\"";
  }
  result += value;
  result += "\">";
  result += text;
  result += "</option>";
  return result;
}

String getDropDownFooter() {
  return "</select></div></div>";
}

String getNumberInput(String heading, String input_name, unsigned int minVal, unsigned int maxVal, unsigned int value, boolean disabled) {
  String result = "<div class=\"form-group\"><label class=\"control-label col-xs-3\" for=\"";
  result += input_name;
  result += "\">";
  result += heading;
  result += "</label><div class=\"col-xs-2\"><input type=\"number\" class=\"form-control\" name=\"";
  result += input_name;
  result += "\" id=\"";
  result += input_name;
  result += "\" min=\"";
  result += minVal;
  result += "\" max=\"";
  result += maxVal;
  result += "\" value=\"";
  result += value;
  if (disabled) {
    result += " disabled";
  }
  result += "\"></div></div>";

  return result;
}

String getNumberInputWide(String heading, String input_name, byte minVal, byte maxVal, byte value, boolean disabled) {
  String result = "<div class=\"form-group\"><label class=\"control-label col-xs-8\" for=\"";
  result += input_name;
  result += "\">";
  result += heading;
  result += "</label><div class=\"col-xs-2\"><input type=\"number\" class=\"form-control\" name=\"";
  result += input_name;
  result += "\" id=\"";
  result += input_name;
  result += "\" min=\"";
  result += minVal;
  result += "\" max=\"";
  result += maxVal;
  result += "\" value=\"";
  result += value;
  if (disabled) {
    result += " disabled";
  }
  result += "\"></div></div>";

  return result;
}

String getTextInput(String heading, String input_name, String value, boolean disabled) {
  String result = "<div class=\"form-group\"><label class=\"control-label col-xs-3\" for=\"";
  result += input_name;
  result += "\">";
  result += heading;
  result += "</label><div class=\"col-xs-2\"><input type=\"text\" class=\"form-control\" name=\"";
  result += input_name;
  result += "\" id=\"";
  result += input_name;
  result += "\" value=\"";
  result += value;
  if (disabled) {
    result += " disabled";
  }
  result += "\"></div></div>";

  return result;
}

String getTextInputWide(String heading, String input_name, String value, boolean disabled) {
  String result = "<div class=\"form-group\"><label class=\"control-label col-xs-3\" for=\"";
  result += input_name;
  result += "\">";
  result += heading;
  result += "</label><div class=\"col-xs-8\"><input type=\"text\" class=\"form-control\" name=\"";
  result += input_name;
  result += "\" id=\"";
  result += input_name;
  result += "\" value=\"";
  result += value;
  if (disabled) {
    result += " disabled";
  }
  result += "\"></div></div>";

  return result;
}

String getSubmitButton(String buttonText) {
  String result = "<div class=\"form-group\"><div class=\"col-xs-offset-3 col-xs-9\"><input type=\"submit\" class=\"btn btn-primary\" value=\"";
  result += buttonText;
  result += "\"></div></div>";
  return result;
}

/**
 * Set the diagnostic LED colour:
 * stepNumber 0..5
 * state: 0 = red, 1 = orange, 2 = green, 3 = blue
 */
void setDiagnosticLED(byte stepNumber, byte state) {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    if(i > stepNumber) {
      ledR[i] = ledG[i] = ledB[i] = 0;
    } else if (i == stepNumber) {
      if (state == STATUS_RED) {
        ledR[i] = 0xff;
        ledG[i] = 0;
        ledB[i] = 0;
      } else if (state == STATUS_YELLOW) {
        ledR[i] = 0xff;
        ledG[i] = 0x7f;
        ledB[i] = 0x0f;
      } else if (state == STATUS_GREEN) {
        ledR[i] = 0;
        ledG[i] = 0xff;
        ledB[i] = 0;
      } else if (state == STATUS_BLUE) {
        ledR[i] = 0;
        ledG[i] = 0;
        ledB[i] = 0xff;
      }
    }
  }
  outputLEDBuffer();

  // A little pause for effect
  delay(500);
}

// 0 = Connected, short flash, 1 = connected, update blink, 2 = connected, no time server
void setBlinkMode(byte newMode) {
  switch (newMode) {
    case 0:
      {
        blinkOnTime = 5;
        blinkTopTime = 1000;
        break;
      }
    case 1:
      {
        blinkOnTime = 250;
        blinkTopTime = 1000;
        break;
      }
    case 2:
      {
        blinkOnTime = 250;
        blinkTopTime = 500;
        break;
      }
    case 3:
      {
        blinkOnTime = 100;
        blinkTopTime = 200;
        break;
      }
  }
}
