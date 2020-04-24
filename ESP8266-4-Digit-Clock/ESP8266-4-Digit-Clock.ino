//**********************************************************************************
//* Main code for an ESP8266 based Nixie/VFD clock. Features:                      *
//*  - WiFi Clock configuration interface                                          *
//*  - Real Time Clock interface for DS1307+                                       *
//*  - Digit fading with configurable fade length                                  *
//*  - Digit scrollback with configurable scroll speed                             *
//*  - Configuration stored in Flash (JSON)                                        *
//*  - Low hardware component count (as much as possible done in code)             *
//*  - Single button operation with software debounce                              *
//*  - Automatic dimming, using a Light Dependent Resistor                         *
//*  - RGB back light management using individually addressable WS2812B            *
//*  - PIR/motion sensor to turn off display when no one is around                 *
//*                                                                                *
//*   Program with following settings (status line / IDE):                         *
//*                                                                                *
//*    Board: Wemos D1 R2 & Mini / LOLIN (Wemos) D1 R2 & mini                      *
//*    Flash: 80MHz,                                                               *
//*    CPU: 160MHz,                                                                *
//*    Upload speed: 921600,                                                       *
//*    Flash size: 4M (1M SPIFFS)                                                  *
//*    lwIP Variant: v2 Higher Bandwidth (no features)                             *
//*    Erase Flash: Only sketch       ,                                            *
//*                                                                                *
//*  nixie@protonmail.ch                                                           *
//*                                                                                *
//*  board def: http://arduino.esp8266.com/stable/package_esp8266com_index.json    *
//*                                                                                *
//**********************************************************************************
//**********************************************************************************
// Standard Libraries
#include <FS.h>
#include <Wire.h>

// Clock specific libraries, install these with "Sketch -> Include Library -> Add .ZIP library
// using the ZIP files in the "libraries" directory
#include <TimeLib.h>            // http://playground.arduino.cc/code/time (Margolis 1.5.0)
#include <NeoPixelBus.h>        // https://github.com/Makuna/NeoPixelBus (Makuna 2.3.4)
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager  (tzapu 0.14.0)
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson (bblanchon 5.13.2)

// Other parts of the code, broken out for clarity
#include "Globals.h"
#include "ClockButton.h"
#include "Transition.h"
#include "DisplayDefs.h"
#include "ClockUtils.h"
#include "ClockDefs.h"
#include "ESP_DS1307.h"
#include "NtpAsync.h"
#include "HtmlServer.h"

#define DEBUG

NtpAsync ntpAsync;

//**********************************************************************************
//*                               ISR Display Multiplex                            *
//**********************************************************************************
#define INT_MUX_FREQUENCY         900
#define ISR_STEPS                 20

volatile byte digitStep = 0;
volatile byte timerStep = 1;

ICACHE_RAM_ATTR void displayUpdate() {
  
  if (timerStep == switchCount[digitStep])  digitOn (digitStep, currNumberArray[digitStep]);
  if (timerStep == onCount[digitStep])      digitOn (digitStep, NumberArray[digitStep]);
  if (timerStep == offCount[digitStep])     digitOff(digitStep, NumberArray[digitStep]);
  
  timerStep++;
  if (timerStep > ISR_STEPS) {
    timerStep = 1;
    
    digitStep++;
    if (digitStep > 4) {
      digitStep = 0;
    }
  }
}

//**********************************************************************************
//**********************************************************************************
//*                                    Setup                                       *
//**********************************************************************************
//**********************************************************************************
void setup()
{
  int postStep = 0;
  WiFiManager wifiManager;

  // set up the NTP component and wire up the "got time update" callback
  ntpAsync.setUp();
  NewTimeCallback ntcb = newTimeUpdateReceived;
  ntpAsync.setNewTimeCallback(ntcb);
  
#ifdef DEBUG
  setupDebug(true);
  debugMsg("");
  debugMsg("--------------------- START ---------------------");
  wifiManager.setDebugOutput(true);
  
  DebugCallback dbcb = debugMsg;
  ntpAsync.setDebugCallback(dbcb);
  ntpAsync.setDebugOutput(true);
#endif

  loadNumberArrayPOSTMessage(postStep++);

  // Set up the LED output
  leds.Begin();

  loadNumberArrayPOSTMessage(postStep++);

  // Start I2C now so that the update from NTP can be sent to the RTC immediately
  Wire.begin(4, 5); // SDA = D2 = pin 4, SCL = D1 = pin 5
  debugMsg("I2C master started");
  
  loadNumberArrayPOSTMessage(postStep++);

  setPinModes();

  loadNumberArrayPOSTMessage(postStep++);
  debugMsg("Starting display interrupt handler");
  
  timer1_attachInterrupt(displayUpdate); // Add ISR Function
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  /* Dividers:
    TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
    TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
    TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
  Reloads:
    TIM_SINGLE  0 //on interrupt routine you need to write a new value to start the timer again
    TIM_LOOP  1 //on interrupt the counter will start with the same value again
  */
  
  loadNumberArrayPOSTMessage(postStep++);
  timer1_write(INT_MUX_FREQUENCY);
  loadNumberArrayPOSTMessage(postStep++);

  button1.reset();

  setDiagnosticLED(DIAGS_SPIFFS, STATUS_YELLOW);
  loadNumberArrayPOSTMessage(postStep++);
  // delay(1000);

  loadNumberArrayPOSTMessage(postStep++);
  for (int i = 0 ; i < 25 ; i++) {
    button1.checkButton(millis());
  }
  if (button1.isButtonPressedNow()) {
    setDiagnosticLED(DIAGS_SPIFFS, STATUS_BLUE);
    factoryReset();
    delay(1000);
    ESP.restart();
  }

  loadNumberArrayPOSTMessage(postStep++);
  
  debugMsg("Getting SPIFFS");
  if (!getConfigFromSpiffs()) {
    debugMsg("No config found - resetting everything");
    factoryReset();
    setDiagnosticLED(DIAGS_SPIFFS, STATUS_BLUE);
  } else {
    setDiagnosticLED(DIAGS_SPIFFS, STATUS_GREEN);
  }

  // **********************************************************************

  // Clear down any spurious button action
  button1.reset();

  loadNumberArrayPOSTMessage(postStep++);
  testMode = false;
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

  button1.reset();

  // **********************************************************************

  loadNumberArrayPOSTMessage(postStep++);
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
  wifiManager.setAPCallback(configModeCallback);

  setDiagnosticLED(DIAGS_WIFI, STATUS_YELLOW);

  loadNumberArrayPOSTMessage(postStep++);

  boolean connected = false;
  connected = wifiManager.autoConnect(WiFi.hostname().c_str(), "SetMeUp!");

  if (connected) {
    debugMsg("Connected!");
    setDiagnosticLED(DIAGS_WIFI, STATUS_GREEN);
  } else {
    debugMsg("Could NOT Connect");
    setDiagnosticLED(DIAGS_WIFI, STATUS_RED);
  }

  // loadNumberArrayPOSTMessage(postStep++);
  
  IPAddress apIP = WiFi.softAPIP();
  IPAddress myIP = WiFi.localIP();
  debugMsg("AP IP address: " + formatIPAsString(apIP));
  debugMsg("IP address: " + formatIPAsString(myIP));

  // If we lose the connection, we try to recover it
  WiFi.setAutoReconnect(true);

  // See if we can already get the time
  loadNumberArrayPOSTMessage(postStep++);
  ntpAsync.getTimeFromNTP();
  setDiagnosticLED(DIAGS_NTP, STATUS_YELLOW);
  long startSeachMs = millis();
  bool needUpdate = true;
  while(((millis() - startSeachMs) < 10000) && needUpdate) {
    delay(500);
    ntpAsync.getTimeFromNTP();
    if (ntpAsync.ntpTimeValid(millis()) > 0) {
      debugMsg("Got update from NTP, exiting loop");
      needUpdate = false;
    }
  }

  if (needUpdate) {
    setDiagnosticLED(DIAGS_NTP, STATUS_RED);
  } else {
    setDiagnosticLED(DIAGS_NTP, STATUS_GREEN);
  }

  // loadNumberArrayPOSTMessage(postStep++);
  setServerPageHandlers();
  server.begin();
  debugMsg("HTTP server started");

  // Enable OTA updates
  loadNumberArrayPOSTMessage(postStep++);
  httpUpdater.setup(&server, "/update", "admin", "update");

  loadNumberArrayPOSTMessage(postStep++);
  if (mdns.begin(WiFi.hostname(), WiFi.localIP())) {
    debugMsg("MDNS responder started as http://" + WiFi.hostname() + ".local");
    mdns.addService("http", "tcp", 80);
  }

  loadNumberArrayPOSTMessage(postStep++);
  setPIRPullup(usePIRPullup);

  // initialise the internal time (in case we don't find the time provider)
  nowMillis = millis();

  // loadNumberArrayPOSTMessage(postStep++);
  testRTCTimeProvider();
  if (useRTC) {
    getRTCTime(true);
    setDiagnosticLED(DIAGS_RTC, STATUS_GREEN);
  } else {
    setDiagnosticLED(DIAGS_RTC, STATUS_RED);
  }

  // Note that we had an RTC to allow the indicator DP to have meaning
  onceHadAnRTC = useRTC;

  // Show the version for 1 s
  tempDisplayMode = TEMP_MODE_VERSION;
  tempDisplayModeDuration = TEMP_DISPLAY_MODE_DUR_MS;

  // don't blank anything right now
  blanked = false;
  setTubesAndLEDSBlankMode();
  // loadNumberArrayPOSTMessage(postStep++);

  // mark that we have done the EEPROM setup
  testMode = false;
  saveConfigToSpiffs();

  debugMsg("Exit startup in 1 second");
  getStatsFromSpiffs();
  loadNumberArrayPOSTMessage(postStep++);
  delay(1000);
}

//**********************************************************************************
//**********************************************************************************
//*                              Main loop                                         *
//**********************************************************************************
//**********************************************************************************
void loop()
{
  server.handleClient();
  mdns.update();

  nowMillis = millis();
  static uint16_t lastSec = second();

  // shows us how fast the inner loop is running
  impressionsPerSec++;

  // -------------------------------------------------------------------------------

  if (lastSec != second()) {
    lastSec = second();
    lastCheckMillis = nowMillis;

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

  if (digitalRead(pirPin) == HIGH) {
    pirConsecutiveCounts++;
  }

  delay(10);
  
  outputDisplay();
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

  // Reset PIR debounce
  pirConsecutiveCounts = 0;

  if (ntpAsync.getNextUpdate(nowMillis) < 0) {
    ntpAsync.getTimeFromNTP();
  }

  // caching these makes the WiFi handling a lot smoother
  disp_sec = second();
  if(hourMode) {
    disp_hour = hourFormat12();
  } else {
    disp_hour = hour();
  }
  disp_min = minute();
}

// ************************************************************
// Called once per minute
// ************************************************************
void performOncePerMinuteProcessing() {
  debugMsg("---> OncePerMinuteProcessing");

  debugMsg("nu: " + String(ntpAsync.getNextUpdate(nowMillis)));

  // Set the internal time to the time from the RTC even if we are still in
  // NTP valid time. This is more accurate than using the internal time source
  getRTCTime(true);

  // Usage stats
  uptimeMins++;

  if (!blankTubes) {
    tubeOnTimeMins++;
  }

  ampm = isAM();

  // caching these makes the WiFi handling a lot smoother
  disp_day = day();
  disp_month = month();
}

// ************************************************************
// Called once per hour
// ************************************************************
void performOncePerHourProcessing() {
  debugMsg("---> OncePerHourProcessing");
  reconnectDroppedConnection();
}

// ************************************************************
// Called once per day
// ************************************************************
void performOncePerDayProcessing() {
  debugMsg("---> OncePerDayProcessing");
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

  // Fixed backlight dim setting
  float fixedDim = (float) backlightDimFactor / (float) 100;
  
  // calculate the PWM factor, goes between minDim% and 100%
  float dimFactor = (float) digitOffCount / (float) ISR_STEPS * fixedDim;
  
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
    case LED_ON:
      {
        led1State = true;
        led2State = true;
        break;
      }
    case LED_OFF:
      {
        led1State = false;
        led2State = false;
        break;
      }
    case LED_AM_PM:
      {
        led1State = !ampm; // bottom LED
        led2State = ampm; // top LED
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
        setAllLEDs( getLEDAdjusted(rgb_backlight_curve[redCnl], 1, fixedDim),
                    getLEDAdjusted(rgb_backlight_curve[grnCnl], 1, fixedDim),
                    getLEDAdjusted(rgb_backlight_curve[bluCnl], 1, fixedDim));
        break;
      case BACKLIGHT_PULSE:
        setAllLEDs( getLEDAdjusted(rgb_backlight_curve[redCnl], pwmFactor, fixedDim),
                    getLEDAdjusted(rgb_backlight_curve[grnCnl], pwmFactor, fixedDim),
                    getLEDAdjusted(rgb_backlight_curve[bluCnl], pwmFactor, fixedDim));
        break;
      case BACKLIGHT_CYCLE:
        cycleColours3(colors);
        setAllLEDs( getLEDAdjusted(colors[0], 1, fixedDim),
                    getLEDAdjusted(colors[1], 1, fixedDim),
                    getLEDAdjusted(colors[2], 1, fixedDim));
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
          ledR[DIGIT_COUNT - 1 - i] = getLEDAdjusted(colourTimeR[NumberArray[i]], 1, fixedDim);
          ledG[DIGIT_COUNT - 1 - i] = getLEDAdjusted(colourTimeG[NumberArray[i]], 1, fixedDim);
          ledB[DIGIT_COUNT - 1 - i] = getLEDAdjusted(colourTimeB[NumberArray[i]], 1, fixedDim);
        }
        outputLEDBuffer();
        break;
      case BACKLIGHT_COLOUR_TIME_DIM:
        for (int i = 0 ; i < DIGIT_COUNT ; i++) {
          ledR[DIGIT_COUNT - 1 - i] = getLEDAdjusted(colourTimeR[NumberArray[i]], 1, dimFactor);
          ledG[DIGIT_COUNT - 1 - i] = getLEDAdjusted(colourTimeG[NumberArray[i]], 1, dimFactor);
          ledB[DIGIT_COUNT - 1 - i] = getLEDAdjusted(colourTimeB[NumberArray[i]], 1, dimFactor);
        }
        outputLEDBuffer();
        break;
    }
  }

  // Blinking for
  blinkState = (secsDeltaAbs % 500) > 250;
}

// ************************************************************
// Check the PIR status. If we don't have a PIR installed, we
// don't want to respect the pin value, because it would defeat
// normal day blanking. The first time the PIR takes the pin low
// we mark that we have a PIR and we should start to respect
// the sensor over configured blanking.
// Returns true if PIR sensor is installed and we are blanked
//
// To deal with noise, we use a counter to detect a number of
// consecutive counts before counting the PIR as "detected"
// ************************************************************
boolean checkPIR(unsigned long nowMillis) {
  if (pirConsecutiveCounts > (lastImpressionsPerSec / 2)) {
    pirLastSeen = nowMillis;
    return false;
  } else {
    if (!pirInstalled) debugMsg("Marking PIR as installed");
    pirInstalled = true;
    if (nowMillis > (pirLastSeen + (pirTimeout * 1000))) {
      return true;
    } else {
      return false;
    }
  }
}

// ************************************************************
// Check if the PIR sensor is installed
// ************************************************************
boolean checkPIRInstalled() {
  return pirInstalled;
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
        if (ntpAsync.ntpTimeValid(nowMillis)) {
          // skip past the time and date settings
          setNewNextMode(MODE_12_24);
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
        if ((dayBlanking == DAY_BLANKING_NEVER) ||
            (dayBlanking == DAY_BLANKING_WEEKEND) ||
            (dayBlanking == DAY_BLANKING_WEEKDAY) ||
            (dayBlanking == DAY_BLANKING_ALWAYS)) {
          // Skip past the start and end hour if the blanking mode says it is not relevant
          setNewNextMode(MODE_USE_LDR);
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
          setNewNextMode(MODE_CYCLE_SPEED);
        }
        loadNumberArrayConfInt(redCnl, displayMode);
        displayConfig();
        break;
      }
    case MODE_GRN_CNL: {
        loadNumberArrayConfInt(grnCnl, displayMode);
        displayConfig();
        break;
      }
    case MODE_BLU_CNL: {
        loadNumberArrayConfInt(bluCnl, displayMode);
        displayConfig();
        break;
      }
    case MODE_CYCLE_SPEED: {
        if ((backlightMode != BACKLIGHT_CYCLE) && (backlightMode != BACKLIGHT_CYCLE_DIM))  {
          // Show only if we are in cycle mode
          setNewNextMode(MODE_MIN_DIM_UP);
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
            if (getIsConnected()) {
              IPAddress myIP = WiFi.localIP();
              loadNumberArrayIP(myIP[0]);
              showDisplayIP();
            } else {
              // we can't show the IP address if we have the RTC, just skip
              tempDisplayMode++;
            }
          }

          if (tempDisplayMode == TEMP_IP_ADDR2) {
            if (getIsConnected()) {
              IPAddress myIP = WiFi.localIP();
              loadNumberArrayIP(myIP[1]);
              showDisplayIP();
            } else {
              // we can't show the IP address if we have the RTC, just skip
              tempDisplayMode++;
            }
          }

          if (tempDisplayMode == TEMP_IP_ADDR3) {
            if (getIsConnected()) {
              IPAddress myIP = WiFi.localIP();
              loadNumberArrayIP(myIP[2]);
              showDisplayIP();
            } else {
              // we can't show the IP address if we have the RTC, just skip
              tempDisplayMode++;
            }
          }

          if (tempDisplayMode == TEMP_IP_ADDR4) {
            if (getIsConnected()) {
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
                loadNumberArrayConfIntWide(valueToShow);
                loadDisplaySetValueType();
              } else {
                // normal time
                loadNumberArrayTime();
                allFadeOrNormal(APPLY_LEAD_0_BLANK);
              }
            }
          } else {
            // no slots mode, check if we are in valueDisplayMode
            if (valueDisplayTime > 0) {
              loadNumberArrayConfIntWide(valueToShow);
              loadDisplaySetValueType();
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
        fadeStep = ISR_STEPS / fadeSteps;
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
        fadeStep = ISR_STEPS / fadeSteps;
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
          pirTimeout -= 10;
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
          pirTimeout += 10;
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
  NumberArray[3] = disp_min % 10;
  NumberArray[2] = disp_min / 10;
  NumberArray[1] = disp_hour % 10;
  NumberArray[0] = disp_hour / 10;
}

// ************************************************************
// Break the minutes and seconds into displayable digits
// ************************************************************
void loadNumberArrayMinsSecs() {
  NumberArray[3] = disp_sec % 10;
  NumberArray[2] = disp_sec / 10;
  NumberArray[1] = disp_min % 10;
  NumberArray[0] = disp_min / 10;
}

// ************************************************************
// Break the date into displayable digits, respecting date format
// ************************************************************
void loadNumberArrayDayMonth() {
  switch (dateFormat) {
    case DATE_FORMAT_MMDD:
      NumberArray[3] = disp_day % 10;
      NumberArray[2] = disp_day / 10;
      NumberArray[1] = disp_month % 10;
      NumberArray[0] = disp_month / 10;
      break;
    case DATE_FORMAT_DDMM:
      NumberArray[3] = disp_month % 10;
      NumberArray[2] = disp_month / 10;
      NumberArray[1] = disp_day % 10;
      NumberArray[0] = disp_day / 10;
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
// Display preset
// ************************************************************
void loadDisplaySetValueType() {
  displayType[0] = valueDisplayType[0] >> 4;
  displayType[1] = valueDisplayType[0] & 0xf;
  displayType[2] = valueDisplayType[1] >> 4;
  displayType[3] = valueDisplayType[1] & 0xf;
}

// ************************************************************
// Display preset - used for Power On Self Test
// ************************************************************
void loadNumberArrayPOSTMessage(int postValue) {
  loadNumberArrayConfIntWide(postValue);
}

// ************************************************************
// Do a single complete display, including any fading and
// dimming requested. Performs the display loop
// DIGIT_DISPLAY_COUNT times for each digit, with no delays.
// This is the heart of the display processing!
// ************************************************************
void outputDisplay() {
  int ledsOffTime;
  int digitSwitchTime;
  float digitSwitchTimeFloat;
  int tmpDispType;

  // used to blank all leading digits if 0
  boolean leadingZeros = true;

  for ( int i = 0 ; i < DIGIT_COUNT ; i ++ ) {
    if (blankTubes) {
      tmpDispType = BLANKED;
    } else {
      tmpDispType = displayType[i];
    }

    switch (tmpDispType) {
      case BLANKED:
        {
          // Setting off = on means we never turn on
          onCount[i] = 0;
          offCount[i] = 0;
          break;
        }
      case DIMMED:
        {
          onCount[i] = 1;
          offCount[i] = 8;
          break;
        }
      case BRIGHT:
        {
          onCount[i] = 1;
          offCount[i] = ISR_STEPS;
          break;
        }
      case FADE:
      case NORMAL:
      case SCROLL:
        {
          onCount[i] = 1;
          offCount[i] = digitOffCount;
          break;
        }
      case BLINK:
        {
          if (blinkState) {
            onCount[i] = 1;
            offCount[i] = digitOffCount;
          } else {
            onCount[i] = 0;
            offCount[i] = 0;
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

    // manage scrolling, each impression we show 1 fade step less of the old
    // digit and 1 fade step more of the new. We hold the 
    if (tmpDispType == SCROLL) {
      if (NumberArray[i] != currNumberArray[i]) {
        if (fadeState[i] == 0) {
          // Start the fade
          fadeState[i] = scrollSteps;
          switchCount[i] = 1;
        }

        if (fadeState[i] == 1) {
          // finish the fade
          fadeState[i] = 0;
          currNumberArray[i] = currNumberArray[i] - 1;
        } else if (fadeState[i] > 1) {
          // Continue the scroll countdown
          fadeState[i] = fadeState[i] - 1;
          switchCount[i] = 1;
        }
      }
    } else

      // --------------------- Fade ---------------------

      // manage fading, each impression we show 1 fade step less of the old
      // digit and 1 fade step more of the new
      if (tmpDispType == FADE) {
        if (NumberArray[i] != currNumberArray[i]) {
          // Start the fade
          if (fadeState[i] == 0) {
            // Start the fade
            fadeState[i] = fadeSteps;
            switchCount[i] = 1;
          }
        }

        if (fadeState[i] == 1) {
          // finish the fade
          fadeState[i] = 0;
          currNumberArray[i] = NumberArray[i];
          digitSwitchTime = digitOffCount;
        } else if (fadeState[i] > 1) {
          // Continue the fade
          fadeState[i] = fadeState[i] - 1;
          switchCount[i] = switchCount[i] + 1;
        }
      } else

        // --------------------- Normal ---------------------

      {
        digitSwitchTime = 99;
        currNumberArray[i] = NumberArray[i];
      }

    // LEDs off will usually come before the digits off, or be the same 
    ledsOffTime = digitOffCount * (float) separatorDimFactor / (float) 100;
  }
}

// ************************************************************
// Set a digit with the given value, for a given digit
// ************************************************************
ICACHE_RAM_ATTR void digitOn(byte digit, byte value) {
  byte dispValue = (decodeDigit[value] & 0xf) | (anodes[digit] & 0xf0);
  shiftOut(LATCHPin, DATA1Pin, CLOCKPin, dispValue);
}

// ************************************************************
// Turn the anodes off, but leave the cathodes alone
// ************************************************************
ICACHE_RAM_ATTR void digitOff(byte digit, byte value) {
  byte dispValue = (decodeDigit[value] & 0xf);
  shiftOut(LATCHPin, DATA1Pin, CLOCKPin, dispValue);
}

// ************************************************************
// Turn the separator LEDs on
// ************************************************************
void ledsOn() {
  digitalWrite(LEDPin1, led1State);
  digitalWrite(LEDPin2, led2State);
}

// ************************************************************
// Turn the separator LEDs off
// ************************************************************
void ledsOff() {
  digitalWrite(LEDPin1, LOW);
  digitalWrite(LEDPin2, LOW);
}

// ************************************************************
// Perform the shift out to the shift register
// ************************************************************
void ICACHE_RAM_ATTR shiftOut(uint8_t _loadPin,
              uint8_t _dataPin1,
              uint8_t _clockPin,
              uint8_t _val1) {
  uint8_t i;

  digitalWrite(_loadPin, LOW);
  for (i = 0; i < 8; i++) {
    digitalWrite(_dataPin1, !!(_val1 & (1 << (7 - i))));
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
    case DATE_FORMAT_MMDD:
      highlight0and1();
      break;
    case DATE_FORMAT_DDMM:
      highlight2and3();
      break;
  }
}

// ************************************************************
// highlight days taking into account the date format
// ************************************************************
void highlightDaysDateFormat() {
  switch (dateFormat) {
    case DATE_FORMAT_DDMM:
      highlight0and1();
      break;
    case DATE_FORMAT_MMDD:
      highlight2and3();
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
  if (displayType[0] != BLINK)  displayType[0] = BLINK;
  if (displayType[1] != BLINK)  displayType[1] = BLINK;
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
  int intValues[6];
  grabInts(timeString, &intValues[0], ",");
  setTime(intValues[SYNC_HOURS], intValues[SYNC_MINS], intValues[SYNC_SECS], intValues[SYNC_DAY], intValues[SYNC_MONTH], intValues[SYNC_YEAR]);
  debugMsg("Set internal time to NTP time: " + String(year()) + ":" + String(month()) + ":" + String(day()) + " " + String(hour()) + ":" + String(minute()) + ":" + String(second()));
}


// ************************************************************
// Check the blanking, overridden if we have a PIR installed
// ************************************************************
boolean checkBlanking() {
  // If we are running on PIR, never use native blanking
  if (checkPIRInstalled())
    return false;

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
// Get the readable version of the PIR state
// ************************************************************
String getPIRStateDisplay() {
  float lastMotionDetection = (nowMillis - pirLastSeen) / 1000.0;

  String part3 = " ago";
  if (blanked) {
    part3 = " ago (blanked)";
  }
  return "Motion detected " + secsToReadableString(lastMotionDetection) + part3;
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
    switch (blankMode) {
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
    setAllLEDs(255, 0, 0);
  }
  if (random(3) == 0) {
    setAllLEDs(0, 255, 0);
  }
  if (random(3) == 0) {
    setAllLEDs(0, 0, 255);
  }
  delay(delayVal);

  setAllLEDs(0, 0, 0);
  delay(delayVal);
}

// ************************************************************
// Set the diagnostic LED colour:
// state: 0 = red, 1 = orange, 2 = green, 3 = blue
// ************************************************************
void setDiagnosticLED(byte stepNumber, byte state) {
  for (int i = 0 ; i < DIGIT_COUNT ; i++) {
    if (i > stepNumber) {
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

void setPIRPullup(boolean newState) {
  if (newState) {
    debugMsg("Setting PIR pullup");
    pinMode(pirPin, INPUT_PULLUP);
  } else {
    debugMsg("Resetting PIR pullup");
    pinMode(pirPin, INPUT);
  }
}

// ************************************************************
// Jump to a new position in the menu - used to skip unused items
// ************************************************************
void setNewNextMode(int newNextMode) {
  nextMode = newNextMode;
  currentMode = newNextMode - 1;
}

// ************************************************************
// Callback: When the NTP component tells us there is an update
// go and get it
// ************************************************************
void newTimeUpdateReceived() {
  debugMsg("Got a new time update");
  setTimeFromServer(ntpAsync.getLastTimeFromServer());
  setRTCTime();
}

//**********************************************************************************
//**********************************************************************************
//*                         RTC Module Time Provider                               *
//**********************************************************************************
//**********************************************************************************

// ************************************************************
// Get the time from the RTC
// ************************************************************
String getRTCTime(boolean setInternalTime) {
  testRTCTimeProvider();
  if (useRTC) {
    Clock.getTime();
    int years = Clock.year + 2000;
    byte months = Clock.month;
    byte days = Clock.dayOfMonth;
    byte hours = Clock.hour;
    byte mins = Clock.minute;
    byte secs = Clock.second;

    String returnValue = String(years) + ":" + String(months) + ":" + String(days) + " " + String(hours) + ":" + String(mins) + ":" + String(secs);
    debugMsg("Got RTC time: " + returnValue);

    if (setInternalTime) {
      // Set the internal time provider to the value we got
      setTime(hours, mins, secs, days, months, years);
      debugMsg("Set Internal time to: " + returnValue);
    }

    return returnValue;
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
    Clock.fillByYMD(year() % 100, month(), day());
    Clock.fillByHMS(hour(), minute(), second());
    Clock.setTime();

    debugMsg("Set RTC time to internal time: " + String(year()) + ":" + String(month()) + ":" + String(day()) + " " + String(hour()) + ":" + String(minute()) + ":" + String(second()));
  }
}

//**********************************************************************************
//**********************************************************************************
//*                          Light Dependent Resistor                              *
//**********************************************************************************
//**********************************************************************************

// ******************************************************************
// Check the ambient light through the LDR (Light Dependent Resistor)
// Smooths the reading over a number of reads.
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
    int rawSensorVal = 1023 - rawLDR;

    double sensorDiff = rawSensorVal - sensorLDRSmoothed;
    sensorLDRSmoothed += (sensorDiff / (double)sensorSmoothCountLDR);

    // Scaling offset increases the base brightness
    // factor increases the sensitivity
    double offset = 20.0;
    double factor = 120.0 / 5.0;
    
    int returnValue = (sensorLDRSmoothed + offset) / factor;

    if (returnValue < minDim) returnValue = minDim;
    if (returnValue > ISR_STEPS) returnValue = ISR_STEPS;
    return returnValue;
  } else {
    return ISR_STEPS;
  }
}

//**********************************************************************************
//**********************************************************************************
//*                               SPIFFS functions                                 *
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

          String ntpPool = json["ntp_pool"].as<String>();
          debugMsg("Loaded NTP pool: " + ntpPool);
          ntpAsync.setNtpPool(ntpPool);

          int ntpUpdateInterval = json["ntp_update_interval"].as<int>();
          debugMsg("Loaded NTP update interval: " + String(ntpUpdateInterval));
          ntpAsync.setUpdateInterval(ntpUpdateInterval);

          String tzs = json["time_zone_string"].as<String>();
          debugMsg("Loaded time zone string: " + tzs);
          ntpAsync.setTZS(tzs);

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

          dayBlanking = json["dayBlanking"];
          debugMsg("Loaded dayBlanking: " + String(dayBlanking));

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

          boolean webAuthentication = json["webAuthentication"].as<bool>();
          debugMsg("Loaded webAuthentication: " + String(webAuthentication));
          setWebAuthentication(webAuthentication);

          String webUserName = json["webUsername"].as<String>();
          debugMsg("Loaded webUsername: " + webUserName);
          setWebUserName(webUserName);

          String webPassword = json["webPassword"].as<String>();
          debugMsg("Loaded webPassword: " + webPassword);
          setWebPassword(webPassword);

          separatorDimFactor = json["separatorDimFactor"];
          debugMsg("Loaded separatorDimFactor: " + String(separatorDimFactor));

          backlightDimFactor = json["backlightDimFactor"];
          debugMsg("Loaded backlightDimFactor: " + String(backlightDimFactor));

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
    json["ntp_pool"] = ntpAsync.getNtpPool();
    json["ntp_update_interval"] = ntpAsync.getUpdateInterval();
    json["time_zone_string"] = ntpAsync.getTZS();
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
    json["webAuthentication"] = getWebAuthentication();
    json["webUsername"] = getWebUserName();
    json["webPassword"] = getWebPassword();
    json["separatorDimFactor"] = separatorDimFactor;
    json["backlightDimFactor"] = backlightDimFactor;

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
  ntpAsync.resetDefaults();
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
  setWebAuthentication(WEB_AUTH_DEFAULT);
  setWebUserName(WEB_USERNAME_DEFAULT);
  setWebPassword(WEB_PASSWORD_DEFAULT);
  separatorDimFactor = SEPARATOR_DIM_FACTOR_DEFAULT;
  backlightDimFactor = BACKLIGHT_DIM_FACTOR_DEFAULT;

  saveConfigToSpiffs();
}

//**********************************************************************************
//**********************************************************************************
//*                                Page Handlers                                   *
//**********************************************************************************
//**********************************************************************************

void saveToSpiffsIfChanged(boolean changed) {
  if (changed) {
    debugMsg("Config options changed, saving them");
    saveConfigToSpiffs();

    // Save the stats while we are at it
    saveStatsToSpiffs();
  }
}

// ************************************************************
// Summary page
// ************************************************************
void rootPageHandler() {
  debugMsg("Root page in");

  String response_message = getHTMLHead(getIsConnected());
  response_message += getNavBar();

  // Status table
  response_message += getTableHead2Col("Current Status", "Name", "Value");

  if (WiFi.status() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    response_message += getTableRow2Col("WLAN IP", formatIPAsString(ip));
    response_message += getTableRow2Col("WLAN MAC", WiFi.macAddress());
    response_message += getTableRow2Col("WLAN SSID", WiFi.SSID());
    response_message += getTableRow2Col("NTP Pool", ntpAsync.getNtpPool());
    response_message += getTableRow2Col("TZ", ntpAsync.getTZS());
    response_message += getTableRow2Col("Last NTP time", ntpAsync.getLastTimeFromServer());
    response_message += getTableRow2Col("Clock Name", WiFi.hostname());
  }
  else
  {
    IPAddress softapip = WiFi.softAPIP();
    String ipStrAP = String(softapip[0]) + '.' + String(softapip[1]) + '.' + String(softapip[2]) + '.' + String(softapip[3]);
    response_message += getTableRow2Col("AP IP", ipStrAP);
    response_message += getTableRow2Col("AP MAC", WiFi.softAPmacAddress());
  }

  response_message += getTableRow2Col("Uptime", secsToReadableString(millis() / 1000));

  long lastUpdateTimeSecs = ntpAsync.getLastUpdateTimeSecs(millis());
  response_message += getTableRow2Col("Last time update", secsToReadableString(lastUpdateTimeSecs) + " ago");

  signed long absNextUpdate = abs(ntpAsync.getNextUpdate(millis()));
  String overdueInd = "";
  if (absNextUpdate < 0) {
    overdueInd = " overdue";
    absNextUpdate = -absNextUpdate;
  }
  response_message += getTableRow2Col("Time before next update", secsToReadableString(absNextUpdate) + overdueInd);

  response_message += getTableRow2Col("Version", SOFTWARE_VERSION);

  // Scan I2C bus
  for (int idx = 0 ; idx < 128 ; idx++)
  {
    Wire.beginTransmission(idx);
    int error = Wire.endTransmission();
    if (error == 0) {
      String slaveMsg = "Found I2C slave at";
      response_message += getTableRow2Col(slaveMsg, idx);
    }
  }

  response_message += getTableFoot();

  // ******************** Clock Info table ***************************
  float digitBrightness = digitOffCount * 100.0 / (float) ISR_STEPS;
  String motionSensorState = checkPIRInstalled() ? getPIRStateDisplay() : "Not installed";
  String rtcState = useRTC ? "Installed" : "Not installed";
  String timeSource;

  if (ntpAsync.ntpTimeValid(nowMillis)) {
    timeSource = "NTP";
  } else if (useRTC) {
    timeSource = "RTC";
  } else {
    timeSource = "Internal";
  }
  String currentTime = String(year()) + ":" + String(month()) + ":" + String(day()) + " " + String(hour()) + ":" + String(minute()) + ":" + String(second());
  response_message += getTableHead2Col("Clock information", "Name", "Value");

  if (useLDR) {
    response_message += getTableRow2Col("LDR Value", sensorLDRSmoothed);
  } else {
    response_message += getTableRow2Col("LDR Value", "LDR disabled");
  }

  response_message += getTableRow2Col("Digit brightness %", String(digitBrightness, 2));
  response_message += getTableRow2Col("On Count", String(onCount[0]) + "," + String(onCount[1]) + "," + String(onCount[2]) + "," + String(onCount[3]));
  response_message += getTableRow2Col("Off Count", String(offCount[0]) + "," + String(offCount[1]) + "," + String(offCount[2]) + "," + String(offCount[3]));
  response_message += getTableRow2Col("Motion Sensor", motionSensorState);
  response_message += getTableRow2Col("Time Source", timeSource);
  response_message += getTableRow2Col("Display Time", currentTime);
  response_message += getTableRow2Col("Real Time Clock", rtcState);
  if (useRTC) {
    response_message += getTableRow2Col("RTC Time", getRTCTime(false));
  }
  response_message += getTableRow2Col("Impressions/Sec", lastImpressionsPerSec);
  response_message += getTableRow2Col("Total Clock On Hrs", secsToReadableString(uptimeMins * 60));
  response_message += getTableRow2Col("Total Tube On Hrs", secsToReadableString(tubeOnTimeMins * 60));
  response_message += getTableFoot();

  // ******************** ESP8266 Info table ***************************
  response_message += getTableHead2Col("ESP8266 information", "Name", "Value");
  response_message += getTableRow2Col("Sketch size", ESP.getSketchSize());
  response_message += getTableRow2Col("Free sketch size", ESP.getFreeSketchSpace());
  response_message += getTableRow2Col("Sketch hash", ESP.getSketchMD5());
  response_message += getTableRow2Col("Free heap", ESP.getFreeHeap());
  response_message += getTableRow2Col("Boot version", ESP.getBootVersion());
  response_message += getTableRow2Col("CPU Freqency (MHz)", ESP.getCpuFreqMHz());
  response_message += getTableRow2Col("Flash speed (MHz)", ESP.getFlashChipSpeed() / 1000000);
  response_message += getTableRow2Col("SDK version", ESP.getSdkVersion());
  response_message += getTableRow2Col("Chip ID", String(ESP.getChipId(), HEX));
  response_message += getTableRow2Col("Flash Chip ID", String(ESP.getFlashChipId(), HEX));
  response_message += getTableRow2Col("Flash size", ESP.getFlashChipRealSize());
  response_message += getTableFoot();

  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);

  debugMsg("Root page out");
}

// ************************************************************
// Get the local time from the time server, and modify the time server URL if needed
// ************************************************************
void timeServerPageHandler() {
  debugMsg("Time page in");

  boolean changed = false;
  String ntpPool = ntpAsync.getNtpPool();
  String tzs = ntpAsync.getTZS();
  int ntpUpdateInterval = ntpAsync.getUpdateInterval();
  
  checkServerArgString("ntppool", "NTP Pool", changed, ntpPool);
  checkServerArgInt("ntpupdate", "NTP update interval", changed, ntpUpdateInterval);
  checkServerArgString("tzs", "Time Zone String", changed, tzs);
  if (changed) {
    ntpAsync.setTZS(tzs);
    ntpAsync.setNtpPool(ntpPool);
    ntpAsync.setUpdateInterval(ntpUpdateInterval);
  }
  saveToSpiffsIfChanged(changed);

  // -----------------------------------------------------------------------------

  String response_message = getHTMLHead(getIsConnected());
  response_message += getNavBar();

  // form header
  response_message += getFormHead("Select time server");

  debugMsg("ntpPool is: " + ntpAsync.getNtpPool());
  response_message += getTextInputWide("NTP Pool", "ntppool", ntpAsync.getNtpPool(), false);
  response_message += getNumberInput("Update interval:", "ntpupdate", NTP_UPDATE_INTERVAL_MIN, NTP_UPDATE_INTERVAL_MAX, ntpAsync.getUpdateInterval(), false);
  response_message += getTextInputWide("Time Zone String", "tzs", ntpAsync.getTZS(), false);

  response_message += getSubmitButton("Set");

  response_message += getFormFoot();
  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);

  debugMsg("Time page out");
}

// ************************************************************
// Reset the EEPROM and WiFi values, restart
// ************************************************************
void resetPageHandler() {
  debugMsg("Reset page in");

  WiFiManager wifiManager;
  wifiManager.resetSettings();
  factoryReset();

  String response_message = getHTMLHead(getIsConnected());
  response_message += getNavBar();

  response_message += "<div class=\"container\" role=\"main\"><h3 class=\"sub-header\">Send time to I2C right now</h3>";
  response_message += "<div class=\"alert alert-success fade in\"><strong>Success!</strong> Reset done.</div></div>";

  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);

  debugMsg("Reset page out");

  // let the message get written
  delay(500);

  // do the restart
  ESP.restart();
}

// ************************************************************
// Page for the clock configuration
// ************************************************************
void clockConfigPageHandler() {
  debugMsg("Config page in");

  boolean changed = false;

  checkServerArgBoolean("12h24hMode", "12/24h mode", "12h", "24h", changed, hourMode);
  checkServerArgBoolean("blankLeading", "Leading zero", "blank", "show", changed, blankLeading);
  checkServerArgBoolean("scrollback", "Use scrollback", "on", "off", changed, scrollback);
  checkServerArgBoolean("fade", "Use fade", "on", "off", changed, fade);
  checkServerArgBoolean("useLDR", "Use LDR", "on", "off", changed, useLDR);
  checkServerArgByte("dateFormat", "dateFormat", changed, dateFormat);
  checkServerArgByte("dayBlanking", "dayBlanking", changed, dayBlanking);
  checkServerArgByte("blankHourStart", "blankHourStart", changed, blankHourStart);
  checkServerArgByte("blankHourEnd", "blankHourEnd", changed, blankHourEnd);
  checkServerArgInt("fadeSteps", "fadeSteps", changed, fadeSteps);
  checkServerArgInt("scrollSteps", "scrollSteps", changed, scrollSteps);
  checkServerArgByte("backlightMode", "backlightMode", changed, backlightMode);
  checkServerArgByte("redCnl", "redCnl", changed, redCnl);
  checkServerArgByte("grnCnl", "grnCnl", changed, grnCnl);
  checkServerArgByte("bluCnl", "bluCnl", changed, bluCnl);
  checkServerArgByte("backlightDimFactor", "backlightDimFactor", changed, backlightDimFactor);
  checkServerArgByte("cycleSpeed", "cycleSpeed", changed, cycleSpeed);
  checkServerArgByte("blankMode", "blankMode", changed, blankMode);
  checkServerArgByte("slotsMode", "slotsMode", changed, slotsMode);
  checkServerArgInt("pirTimeout", "pirTimeout", changed, pirTimeout);
  checkServerArgBoolean("usePIRPullup", "Use PIR pullup", "on", "off", changed, usePIRPullup);
  checkServerArgInt("minDim", "minDim", changed, minDim);
  checkServerArgByte("ledMode", "ledMode", changed, ledMode);
  checkServerArgByte("separatorDimFactor", "separatorDimFactor", changed, separatorDimFactor);
  boolean localWebAuth = getWebAuthentication();
  String localWebUserName = getWebUserName();
  String localWebPassword = getWebPassword();
  checkServerArgBoolean("webAuthentication", "Web Authentication", "on", "off", changed, localWebAuth);
  checkServerArgString("webUsername", "webUsername", changed, localWebUserName);
  checkServerArgString("webPassword", "webPassword", changed, localWebPassword);
  setWebAuthentication(localWebAuth);
  setWebUserName(localWebUserName);
  setWebPassword(localWebPassword);
  checkServerArgByte("separatorDimFactor", "separatorDimFactor", changed, separatorDimFactor);
  saveToSpiffsIfChanged(changed);

  // -----------------------------------------------------------------------------
  String response_message = getHTMLHead(getIsConnected());
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

  // Scrollback
  response_message += getRadioGroupHeader("Scrollback effect:");
  if (scrollback) {
    response_message += getRadioButton("scrollback", "On", "on", true);
    response_message += getRadioButton("scrollback", "Off", "off", false);
  } else {
    response_message += getRadioButton("scrollback", "On", "on", false);
    response_message += getRadioButton("scrollback", "Off", "off", true);
  }
  response_message += getRadioGroupFooter();

  // Scroll steps
  response_message += getNumberInput("Scroll steps:", "scrollSteps", SCROLL_STEPS_MIN, SCROLL_STEPS_MAX, scrollSteps, !scrollback);

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
  response_message += getNumberInput("Fade steps:", "fadeSteps", FADE_STEPS_MIN, FADE_STEPS_MAX, fadeSteps, !fade);

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
  response_message += getDropDownOption("0", "MM-DD", (dateFormat == 0));
  response_message += getDropDownOption("1", "DD-MM", (dateFormat == 1));
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
  response_message += getDropDownHeader("Back light:", "backlightMode", true);
  response_message += getDropDownOption("0", "Fixed RGB backlight, no dimming", (backlightMode == 0));
  response_message += getDropDownOption("1", "Pulsing RGB backlight, no dimming", (backlightMode == 1));
  response_message += getDropDownOption("2", "Cycling RGB backlight, no dimming", (backlightMode == 2));
  response_message += getDropDownOption("3", "Fixed RGB backlight, dims with ambient light", (backlightMode == 3));
  response_message += getDropDownOption("4", "Pulsing RGB backlight, dims with ambient light", (backlightMode == 4));
  response_message += getDropDownOption("5", "Cycling RGB backlight, dims with ambient light", (backlightMode == 5));
  response_message += getDropDownOption("6", "'Colourtime' backlight", (backlightMode == 6));
  response_message += getDropDownOption("7", "'Colourtime' backlight, dims with ambient light", (backlightMode == 7));
  response_message += getDropDownFooter();

  boolean activeLEDs = (backlightMode == 0) || (backlightMode == 1) || (backlightMode == 3) || (backlightMode == 4);

  // RGB channels
  response_message += getNumberInput("Red intensity:", "redCnl", 0, 15, redCnl, !activeLEDs);
  response_message += getNumberInput("Green intensity:", "grnCnl", 0, 15, grnCnl, !activeLEDs);
  response_message += getNumberInput("Blue intensity:", "bluCnl", 0, 15, bluCnl, !activeLEDs);

  // Separator dim factor
  response_message += getNumberInput("Backlight Dim:", "backlightDimFactor", BACKLIGHT_DIM_FACTOR_MIN, BACKLIGHT_DIM_FACTOR_MAX, backlightDimFactor, false);

  boolean activeCycle = (backlightMode == 2) || (backlightMode == 5);

  // Cycle speed
  response_message += getNumberInput("Backlight Cycle Speed:", "cycleSpeed", CYCLE_SPEED_MIN, CYCLE_SPEED_MAX, cycleSpeed, !activeCycle);

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
  response_message += getDropDownOption("4", "Always On", (ledMode == LED_ON));
  response_message += getDropDownOption("5", "Always Off", (ledMode == LED_OFF));
  response_message += getDropDownOption("6", "AM/PM", (ledMode == LED_AM_PM));
  response_message += getDropDownFooter();

  // Separator dim factor
  response_message += getNumberInput("Separator Dim:", "separatorDimFactor", SEPARATOR_DIM_FACTOR_MIN, SEPARATOR_DIM_FACTOR_MAX, separatorDimFactor, false);

  // PIR timeout
  response_message += getNumberInput("PIR timeout:", "pirTimeout", PIR_TIMEOUT_MIN, PIR_TIMEOUT_MAX, pirTimeout, false);

  // PIR pullup
  response_message += getRadioGroupHeader("Use PIR pullup:");
  if (usePIRPullup) {
    response_message += getRadioButton("usePIRPullup", "On", "on", true);
    response_message += getRadioButton("usePIRPullup", "Off", "off", false);
  } else {
    response_message += getRadioButton("usePIRPullup", "On", "on", false);
    response_message += getRadioButton("usePIRPullup", "Off", "off", true);
  }
  response_message += getRadioGroupFooter();

  // Min dim
  response_message += getNumberInput("Min Dim:", "minDim", MIN_DIM_MIN, MIN_DIM_MAX, minDim, false);

  // web auth mode
  response_message += getRadioGroupHeader("Web Authentication:");
  if (getWebAuthentication()) {
    response_message += getRadioButton("webAuthentication", " On", "on", true);
    response_message += getRadioButton("webAuthentication", " Off", "off", false);
  } else {
    response_message += getRadioButton("webAuthentication", " On", "on", false);
    response_message += getRadioButton("webAuthentication", " Off", "off", true);
  }
  response_message += getRadioGroupFooter();

  response_message += getTextInput("User Name", "webUsername", getWebUserName(), !getWebAuthentication());
  response_message += getTextInput("Password", "webPassword", getWebPassword(), !getWebAuthentication());

  // form footer
  response_message += getSubmitButton("Set");

  response_message += "</form></div>";

  // all done
  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);

  debugMsg("Config page out");
}

// ************************************************************
// Send a value to the clock for display
// ************************************************************
void setDisplayValuePageHandler() {
  byte timeValue = 0xff;
  boolean valueValid = false;
  boolean timeValid = false;
  boolean formatValid = false;

  int valueToDisplay = 0;
  int valueTime = 0;
  int valueFormat = 0;
  
  checkServerArgInt("value", "value", valueValid, valueToDisplay);
  checkServerArgInt("time", "time", timeValid, valueTime);
  checkServerArgInt("format", "format", formatValid, valueFormat);

  if (valueValid) {
    valueToShow = valueToDisplay % 10000;
    valueDisplayTime = 10;
  }
  
  if (timeValid) {
    if (valueTime > 255)
      valueTime = 255;
    valueDisplayTime = valueTime;
  } else {
    valueDisplayTime = 10;
  }

  if (formatValid) {
    byte val1 = hex2bcd(valueFormat % 100);
    valueFormat = valueFormat / 100;
    byte val0 = hex2bcd(valueFormat % 100);
    valueDisplayType[0] = val0;
    valueDisplayType[1] = val1;
  }

  String response_message = getHTMLHead(getIsConnected());
  response_message += getNavBar();

  response_message += "<div class=\"container\" role=\"main\"><h3 class=\"sub-header\">Show a value on the clock</h3>";

  if (valueValid) {
    response_message += "<div class=\"alert alert-success fade in\"><strong>";
    response_message += "display value was set!";
    response_message += "</strong></div></div>";
  } else {
    response_message += "<div class=\"alert alert-error fade in\"><strong>";
    response_message += "You need to set at least the \"value\" parameter! A valid command line is http://clock.ip.address/setvalue?value=1234&time=60&format=3333";
    response_message += "</strong></div></div>";
  }

  response_message += getHTMLFoot();

  server.send(200, "text/html", response_message);
}

// ************************************************************
// Access to utility functions
// ************************************************************
void utilityPageHandler()
{
  debugMsg("Utility page in");
  String response_message = getHTMLHead(getIsConnected());
  response_message += getNavBar();

  response_message += "<hr><li><a href=\"/resetwifi\">Restart WiFi module</a></li>";
  response_message += "<hr><li><a href=\"/reset\">Clear WiFi config and restart module</a></li>";
  response_message += "<hr><li><a href=\"/update\">Update firmware</a></li>";
  response_message += "<hr><li><a href=\"/ntpupdate\">Force update from NTP now</a></li>";
  response_message += "<hr><li><a href=\"/factoryreset\">Perform factory reset</a></li>";

  response_message += getHTMLFoot();
  server.send(200, "text/html", response_message);
  debugMsg("Utility page out");
}

// ************************************************************
// Reset just the wifi
// ************************************************************
void resetWiFiPageHandler() {
  debugMsg("Reset WiFi page in");

  WiFiManager wifiManager;
  wifiManager.resetSettings();

  String response_message = getHTMLHead(getIsConnected());
  response_message += getNavBar();
  response_message += "<div class=\"alert alert-success fade in\"><strong>Attempting a restart.</strong>";
  response_message += "<script> var timer = setTimeout(function() {window.location='/'}, 10000);</script>";
  response_message += getHTMLFoot();
  server.send(200, "text/html", response_message);
  debugMsg("Reset WiFi page out");
  delay(1000);    // wait to deliver response
  ESP.restart();  // should reboot
}

// ************************************************************
// Do an NTP update now
// ************************************************************
void ntpUpdatePageHandler() {
  debugMsg("NTP Update page in");

  ntpAsync.getTimeFromNTP();

  server.sendHeader("Location", "/utility", true);
  server.send(302, "text/plain", "");

  debugMsg("NTP Update page out");
}

// ************************************************************
// Reset all the settings
// ************************************************************
void factoryResetPageHandler() {
  debugMsg("Factory Reset page in");

  factoryReset();

  server.sendHeader("Location", "/utility", true);
  server.send(302, "text/plain", "");

  debugMsg("Factory Reset page out");
}

// ************************************************************
// Called if requested page is not found
// ************************************************************
void handleNotFound() {
  debugMsg("404 page in");

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

  debugMsg("404 page out");
}

// ************************************************************
// Called if we need to have a local CSS
// ************************************************************
void localCSSHandler()
{
  PGM_P cssStr = PSTR(".navbar,.table{margin-bottom:20px}.nav>li,.nav>li>a,article,aside,details,figcaption,figure,footer,header,hgroup,main,menu,nav,section,summary{display:block}.btn,.form-control,.navbar-toggle{background-image:none}.table,label{max-width:100%}.sub-header{padding-bottom:10px;border-bottom:1px solid #eee}.h3,h3{font-size:24px}.table{width:100%}table{background-color:transparent;border-spacing:0;border-collapse:collapse}.table-striped>tbody>tr:nth-of-type(2n+1){background-color:#f9f9f9}.table>caption+thead>tr:first-child>td,.table>caption+thead>tr:first-child>th,.table>colgroup+thead>tr:first-child>td,.table>colgroup+thead>tr:first-child>th,.table>thead:first-child>tr:first-child>td,.table>thead:first-child>tr:first-child>th{border-top:0}.table>thead>tr>th{border-bottom:2px solid #ddd}.table>tbody>tr>td,.table>tbody>tr>th,.table>tfoot>tr>td,.table>tfoot>tr>th,.table>thead>tr>td,.table>thead>tr>th{padding:8px;line-height:1.42857143;vertical-align:top;border-top:1px solid #ddd}th{text-align:left}td,th{padding:0}.navbar>.container .navbar-brand,.navbar>.container-fluid .navbar-brand{margin-left:-15px}.container,.container-fluid{padding-right:15px;padding-left:15px;margin-right:auto;margin-left:auto}.navbar-inverse .navbar-brand{color:#9d9d9d}.navbar-brand{float:left;height:50px;padding:15px;font-size:18px;line-height:20px}a{color:#337ab7;text-decoration:none;background-color:transparent}.navbar-fixed-top{border:0;top:0;border-width:0 0 1px}.navbar-inverse{background-color:#222;border-color:#080808}.navbar-fixed-bottom,.navbar-fixed-top{border-radius:0;position:fixed;right:0;left:0;z-index:1030}.nav>li,.nav>li>a,.navbar,.navbar-toggle{position:relative}.navbar{border-radius:4px;min-height:50px;border:1px solid transparent}.container{width:750px}.navbar-right{float:right!important;margin-right:-15px}.navbar-nav{float:left;margin:7.5px -15px}.nav{padding-left:0;margin-bottom:0;list-style:none}.navbar-nav>li{float:left}.navbar-inverse .navbar-nav>li>a{color:#9d9d9d}.navbar-nav>li>a{padding-top:10px;padding-bottom:10px;line-height:20px}.nav>li>a{padding:10px 15px}.navbar-inverse .navbar-toggle{border-color:#333}.navbar-toggle{display:none;float:right;padding:9px 10px;margin-top:8px;margin-right:15px;margin-bottom:8px;background-color:transparent;border:1px solid transparent;border-radius:4px}button,select{text-transform:none}button{overflow:visible}button,html input[type=button],input[type=reset],input[type=submit]{-webkit-appearance:button;cursor:pointer}.btn-primary{color:#fff;background-color:#337ab7;border-color:#2e6da4}.btn{display:inline-block;padding:6px 12px;margin-bottom:0;font-size:14px;font-weight:400;line-height:1.42857143;text-align:center;white-space:nowrap;vertical-align:middle;-ms-touch-action:manipulation;touch-action:manipulation;cursor:pointer;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;border:1px solid transparent;border-radius:4px}button,input,select,textarea{font-family:inherit;font-size:inherit;line-height:inherit}input{line-height:normal}button,input,optgroup,select,textarea{margin:0;font:inherit;color:inherit}.form-control,body{font-size:14px;line-height:1.42857143}.form-horizontal .form-group{margin-right:-15px;margin-left:-15px}.form-group{margin-bottom:15px}.form-horizontal .control-label{padding-top:7px;margin-bottom:0;text-align:right}.form-control{display:block;width:100%;height:34px;padding:6px 12px;color:#555;background-color:#fff;border:1px solid #ccc;border-radius:4px;-webkit-box-shadow:inset 0 1px 1px rgba(0,0,0,.075);box-shadow:inset 0 1px 1px rgba(0,0,0,.075);-webkit-transition:border-color ease-in-out .15s,-webkit-box-shadow ease-in-out .15s;-o-transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s;transition:border-color ease-in-out .15s,box-shadow ease-in-out .15s}.col-xs-8{width:66.66666667%}.col-xs-3{width:25%}.col-xs-1,.col-xs-10,.col-xs-11,.col-xs-12,.col-xs-2,.col-xs-3,.col-xs-4,.col-xs-5,.col-xs-6,.col-xs-7,.col-xs-8,.col-xs-9{float:left}.col-lg-1,.col-lg-10,.col-lg-11,.col-lg-12,.col-lg-2,.col-lg-3,.col-lg-4,.col-lg-5,.col-lg-6,.col-lg-7,.col-lg-8,.col-lg-9,.col-md-1,.col-md-10,.col-md-11,.col-md-12,.col-md-2,.col-md-3,.col-md-4,.col-md-5,.col-md-6,.col-md-7,.col-md-8,.col-md-9,.col-sm-1,.col-sm-10,.col-sm-11,.col-sm-12,.col-sm-2,.col-sm-3,.col-sm-4,.col-sm-5,.col-sm-6,.col-sm-7,.col-sm-8,.col-sm-9,.col-xs-1,.col-xs-10,.col-xs-11,.col-xs-12,.col-xs-2,.col-xs-3,.col-xs-4,.col-xs-5,.col-xs-6,.col-xs-7,.col-xs-8,.col-xs-9{position:relative;min-height:1px;padding-right:15px;padding-left:15px}label{display:inline-block;margin-bottom:5px;font-weight:700}*{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}body{font-family:\"Helvetica Neue\",Helvetica,Arial,sans-serif;color:#333}html{font-size:10px;font-family:sans-serif;-webkit-text-size-adjust:100%}");
  server.send(200, "text/css", String(cssStr));
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------- Network handling -----------------------------------------
// ----------------------------------------------------------------------------------------------------

// ************************************************************
// What it says on the tin
// ************************************************************
boolean getIsConnected() {
  return WiFi.isConnected();
}

// ************************************************************
// Tell the Diag LED that we have connected
// ************************************************************
void configModeCallback(WiFiManager *myWiFiManager) {
  debugMsg("*** Entered config mode");
  setDiagnosticLED(DIAGS_WIFI, STATUS_BLUE);
}

// ************************************************************
// In the case that we have lost the WiFi connection, try to reconnect
// ************************************************************
void reconnectDroppedConnection() {
  // Try to reconnect if we are  not connected
  if (WiFi.status() != WL_CONNECTED) {
    debugMsg("Attepting to reconnect dropped connection WiFi connection to " + WiFi.SSID());
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();
    WiFi.begin();
  }
}

// ************************************************************
// set up the server page handlers
// ************************************************************
void setServerPageHandlers() {
  server.on("/", rootPageHandler);

  server.on("/time", []() {
    if (getWebAuthentication() && (!server.authenticate(getWebUserName().c_str(), getWebPassword().c_str()))) {
      return server.requestAuthentication();
    }
    return timeServerPageHandler();
  });

  server.on("/reset", []() {
    if (getWebAuthentication() && (!server.authenticate(getWebUserName().c_str(), getWebPassword().c_str()))) {
      return server.requestAuthentication();
    }
    return resetPageHandler();
  });

  server.on("/resetwifi", []() {
    if (getWebAuthentication() && (!server.authenticate(getWebUserName().c_str(), getWebPassword().c_str()))) {
      return server.requestAuthentication();
    }
    return resetWiFiPageHandler();
  });

  server.on("/ntpupdate", []() {
    if (getWebAuthentication() && (!server.authenticate(getWebUserName().c_str(), getWebPassword().c_str()))) {
      return server.requestAuthentication();
    }
    return ntpUpdatePageHandler();
  });

  server.on("/factoryreset", []() {
    if (getWebAuthentication() && (!server.authenticate(getWebUserName().c_str(), getWebPassword().c_str()))) {
      return server.requestAuthentication();
    }
    return factoryResetPageHandler();
  });

  server.on("/clockconfig", []() {
    if (getWebAuthentication() && (!server.authenticate(getWebUserName().c_str(), getWebPassword().c_str()))) {
      return server.requestAuthentication();
    }
    return clockConfigPageHandler();
  });

  server.on("/setvalue", setDisplayValuePageHandler);

  server.on("/utility", []() {
    if (getWebAuthentication() && (!server.authenticate(getWebUserName().c_str(), getWebPassword().c_str()))) {
      return server.requestAuthentication();
    }
    return utilityPageHandler();
  });

  server.on("/local.css",   localCSSHandler);

  server.onNotFound(handleNotFound);
}
