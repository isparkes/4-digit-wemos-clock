//**********************************************************************************
//* ESP8266 4-Digit Nixie Clock                                                    *
//* Copyright (c) 2018-2026 Ian Sparkes. All rights reserved.                      *
//*                                                                                *
//* Free for non-commercial use. Commercial use needs a licence from the author.   *
//* See the LICENSE file in the repository root for the full terms.                *
//* Contact: nixie@protonmail.ch  |  https://www.nixieclock.biz                    *
//**********************************************************************************

//**********************************************************************************
//* Definitions for the basic functions of the clock                               *
//**********************************************************************************

#ifndef ClockDefs_h
#define ClockDefs_h

// ***** Pin Defintions ****** Pin Defintions ****** Pin Defintions ******

#define inputPin1   0     // D3
#define pirPin      2     // D4 - Also blue LED pin
#define LED_DOUT    3     // RX defined by DMA output of library
#define LDRPin      A0    // ADC input

// Shift register
#define LATCHPin    14    // D5
#define CLOCKPin    12    // D6
#define LEDPin1     15    // D8
#define LEDPin2     16    // D0
#define DATA1Pin    13    // D7

// -------------------------------------------------------------------------------
// Software version shown in config menu
#define SOFTWARE_VERSION      8

// -------------------------------------------------------------------------------
#define MIN_DIM_DEFAULT       4  // The default minimum dim count
#define MIN_DIM_MIN           2  // The minimum dim count
#define MIN_DIM_MAX           20 // The maximum dim count

// -------------------------------------------------------------------------------
#define SENSOR_SENSIT_MIN     100 // Sensor Sensitivity
#define SENSOR_SENSIT_MAX     400
#define SENSOR_SENSIT_DEFAULT 200

#define SENSOR_THRSH_MIN      0   // Bright is when we have LDR value = 0, when we read less than this value, we have full brightness
#define SENSOR_THRSH_MAX      500
#define SENSOR_THRSH_DEFAULT  100

#define SENSOR_SMOOTH_READINGS_MIN     1
#define SENSOR_SMOOTH_READINGS_MAX     500
#define SENSOR_SMOOTH_READINGS_DEFAULT 150  // Speed at which the brighness adapts to changes

// -------------------------------------------------------------------------------
#define BLINK_COUNT_MAX       40   // The number of impressions between blink state toggle

// -------------------------------------------------------------------------------
// How quickly the scroll works
#define SCROLL_STEPS_DEFAULT 8
#define SCROLL_STEPS_MIN     1
#define SCROLL_STEPS_MAX     80

// -------------------------------------------------------------------------------
// The number of dispay impessions we need to fade by default
// 100 is about 1 second
#define FADE_STEPS_DEFAULT 50
#define FADE_STEPS_MIN     20
#define FADE_STEPS_MAX     200

#define SECS_MAX  60
#define MINS_MAX  60
#define HOURS_MAX 24

// -------------------------------------------------------------------------------
#define COLOUR_CNL_MAX                  15
#define COLOUR_RED_CNL_DEFAULT          15
#define COLOUR_GRN_CNL_DEFAULT          0
#define COLOUR_BLU_CNL_DEFAULT          0
#define COLOUR_CNL_MIN                  0

// -------------------------------------------------------------------------------
// Clock modes - normal running is MODE_TIME, other modes accessed by a middle length ( 1S < press < 2S ) button press
#define MODE_MIN                        0
#define MODE_TIME                       0

// -------------------------------------------------------------------------------
// Time setting, need all six digits, so no flashing mode indicator
#define MODE_HOURS_SET                  1
#define MODE_MINS_SET                   2
#define MODE_SECS_SET                   3
#define MODE_DAYS_SET                   4
#define MODE_MONTHS_SET                 5
#define MODE_YEARS_SET                  6

// Basic settings
#define MODE_12_24                      7  // 0 = 24, 1 = 12
#define HOUR_MODE_DEFAULT               false
#define MODE_LEAD_BLANK                 8  // 1 = blanked
#define LEAD_BLANK_DEFAULT              false
#define MODE_SCROLLBACK                 9  // 1 = use scrollback
#define SCROLLBACK_DEFAULT              false
#define MODE_DISPLAY_SCROLL_STEPS_UP    10 // See "SCROLL_STEPS_" for values
#define MODE_DISPLAY_SCROLL_STEPS_DOWN  11 // 
#define MODE_FADE                       12 // 1 = use fade
#define FADE_DEFAULT                    false
#define MODE_FADE_STEPS_UP              13 // See "FADE_STEPS_" for values
#define MODE_FADE_STEPS_DOWN            14 // 
#define MODE_DATE_FORMAT                15 // See "DATE_FORMAT_" for values
#define MODE_DAY_BLANKING               16 // See "DAY_BLANKING_" for values
#define MODE_HR_BLNK_START              17 // skipped if not using hour blanking
#define MODE_HR_BLNK_END                18 // skipped if not using hour blanking

#define MODE_USE_LDR                    19 // 1 = use LDR, 0 = don't (and have 100% brightness)
#define MODE_USE_LDR_DEFAULT            true
#define MODE_MIN_DIM_UP                 20 // 
#define MODE_MIN_DIM_DOWN               21 // 

#define MODE_BLANK_MODE                 22 // See "BLANK_MODE_" for values
#define MODE_BLANK_MODE_DEFAULT         BLANK_MODE_BOTH

#define MODE_SLOTS_MODE                 23 // See "SLOTS_MODE_" for values

// Separator LED mode
#define MODE_LED_BLINK                  24 // See "LED_" for values

// PIR
#define MODE_PIR_TIMEOUT_UP             25 // See "PIR_TIMEOUT_" for values
#define MODE_PIR_TIMEOUT_DOWN           26 //
#define MODE_USE_PIR_PULLUP             27 // 1 = use Pullup
#define USE_PIR_PULLUP_DEFAULT          true

// Back light
#define MODE_BACKLIGHT_MODE             28 // See "BACKLIGHT_" (ledManager) for values
#define MODE_RED_CNL                    29 //
#define MODE_GRN_CNL                    30 // >> See "COLOUR_CNL_" for values
#define MODE_BLU_CNL                    31 //
#define MODE_CYCLE_SPEED                32 // See "CYCLE_SPEED_" for values
#define MODE_USE_PULSE                  33 //
#define MODE_USE_DIM                    34 //

// Software Version
#define MODE_VERSION                    35 // Mode "25"

// Tube test - all six digits, so no flashing mode indicator
#define MODE_TUBE_TEST                  36

#define MODE_MAX                        36

// -------------------------------------------------------------------------------
// Temporary display modes - accessed by a short press ( < 1S ) on the button when in MODE_TIME
#define TEMP_MODE_MIN                   0
#define TEMP_MODE_SECS                  0 // Display mins and secs
#define TEMP_MODE_DATE                  1 // Display the date
#define TEMP_MODE_LDR                   2 // Display the normalised LDR reading for 5S, returns a value from 100 (dark) to 999 (bright)
#define TEMP_MODE_VERSION               3 // Display the version
#define TEMP_IP_ADDR1                   4 // IP xxx.yyy.zzz.aaa: xxx
#define TEMP_IP_ADDR2                   5 // IP xxx.yyy.zzz.aaa: yyy
#define TEMP_IP_ADDR3                   6 // IP xxx.yyy.zzz.aaa: zzz
#define TEMP_IP_ADDR4                   7 // IP xxx.yyy.zzz.aaa: aaa
#define TEMP_MODE_IMPR                  8 // number of impressions per second
#define TEMP_MODE_MAX                   8

#define TEMP_DISPLAY_MODE_DUR_MS        5000

// -------------------------------------------------------------------------------
#define DATE_FORMAT_MIN                 0
#define DATE_FORMAT_MMDD                0
#define DATE_FORMAT_DDMM                1
#define DATE_FORMAT_MAX                 1
#define DATE_FORMAT_DEFAULT             1

// -------------------------------------------------------------------------------
#define DAY_BLANKING_MIN                0
#define DAY_BLANKING_NEVER              0  // Don't blank ever (default)
#define DAY_BLANKING_WEEKEND            1  // Blank during the weekend
#define DAY_BLANKING_WEEKDAY            2  // Blank during weekdays
#define DAY_BLANKING_ALWAYS             3  // Always blank
#define DAY_BLANKING_HOURS              4  // Blank between start and end hour every day
#define DAY_BLANKING_WEEKEND_OR_HOURS   5  // Blank between start and end hour during the week AND all day on the weekend
#define DAY_BLANKING_WEEKDAY_OR_HOURS   6  // Blank between start and end hour during the weekends AND all day on week days
#define DAY_BLANKING_WEEKEND_AND_HOURS  7  // Blank between start and end hour during the weekend
#define DAY_BLANKING_WEEKDAY_AND_HOURS  8  // Blank between start and end hour during week days
#define DAY_BLANKING_MAX                8
#define DAY_BLANKING_DEFAULT            0

// -------------------------------------------------------------------------------
#define BLANK_MODE_MIN                  0
#define BLANK_MODE_TUBES                0  // Use blanking for tubes only 
#define BLANK_MODE_LEDS                 1  // Use blanking for LEDs only
#define BLANK_MODE_BOTH                 2  // Use blanking for tubes and LEDs
#define BLANK_MODE_ALL                  3  // Use blanking for tubes, LEDs and separators
#define BLANK_MODE_MAX                  3
#define BLANK_MODE_DEFAULT              2

// --------------------------- Strategy Backlights -------------------------------
#define BACKLIGHT_MIN                   0
#define BACKLIGHT_FIXED                 0  // Just define a colour and stick to it
#define BACKLIGHT_CYCLE                 1  // cycle through random colours, strategy 3
#define BACKLIGHT_COLOUR_TIME           2  // use "ColourTime" - different colours for each digit value
#define BACKLIGHT_DAY_OF_WEEK           3  // use "DayOfWeek" - different colours for each day
#define BACKLIGHT_MAX                   3
#define BACKLIGHT_DEFAULT               1

// -------------------------------------------------------------------------------
#define ANTI_GHOST_MIN                  0
#define ANTI_GHOST_MAX                  50
#define ANTI_GHOST_DEFAULT              0

// -------------------------------------------------------------------------------
#define PIR_TIMEOUT_MIN                 60    // 1 minute in seconds
#define PIR_TIMEOUT_MAX                 3600  // 1 hour in seconds
#define PIR_TIMEOUT_DEFAULT             300   // 5 minutes in seconds

// -------------------------------------------------------------------------------
#define USE_LDR_DEFAULT                 true

// -------------------------------------------------------------------------------
#define SLOTS_MODE_MIN                  0
#define SLOTS_MODE_NONE                 0   // Don't use slots effect
#define SLOTS_MODE_1M_SCR_SCR           1   // use slots effect every minute, scroll in, scramble out
#define SLOTS_MODE_MAX                  1
#define SLOTS_MODE_DEFAULT              1

// -------------------------------------------------------------------------------
#define SEP_OFF                         0
#define SEP_DIM                         1
#define SEP_MED                         2
#define SEP_BRIGHT                      3
#define SEP_FULL                        4
#define SEP_DIM_DEFAULT                 SEP_FULL

// -------------------------------------------------------------------------------
#define BACKLIGHT_DIM_FACTOR_MIN        10
#define BACKLIGHT_DIM_FACTOR_MAX        100
#define BACKLIGHT_DIM_FACTOR_DEFAULT    100

// -------------------------------------------------------------------------------
#define EXT_DIM_FACTOR_MIN              10
#define EXT_DIM_FACTOR_MAX              100
#define EXT_DIM_FACTOR_DEFAULT          100

// -------------------------------------------------------------------------------
// RTC address of the onboard DS1307 (if installed)
#define RTC_I2C_ADDRESS                 0x68

// -------------------------------------------------------------------------------
#define DO_NOT_APPLY_LEAD_0_BLANK       false
#define APPLY_LEAD_0_BLANK              true

// -------------------------------------------------------------------------------
#define DIGIT_COUNT                     4

// -------------------------------------------------------------------------------
#define CONFIG_PORTAL_TIMEOUT           60

// -------------------------------------------------------------------------------
#define DIAGS_DELAY_TIME                500

// -------------------------------------------------------------------------------
#define SYNC_HOURS 3
#define SYNC_MINS 4
#define SYNC_SECS 5
#define SYNC_DAY 2
#define SYNC_MONTH 1
#define SYNC_YEAR 0

// -------------------------------------------------------------------------------

#endif
