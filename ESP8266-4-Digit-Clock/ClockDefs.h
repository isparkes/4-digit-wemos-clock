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
#define SOFTWARE_VERSION      4

// -------------------------------------------------------------------------------
#define MIN_DIM_DEFAULT       500  // The default minimum dim count
#define MIN_DIM_MIN           100  // The minimum dim count
#define MIN_DIM_MAX           1000 // The maximum dim count

// -------------------------------------------------------------------------------
#define SENSOR_LOW_MIN        0
#define SENSOR_LOW_MAX        900
#define SENSOR_LOW_DEFAULT    100  // Dark
#define SENSOR_HIGH_MIN       0
#define SENSOR_HIGH_MAX       900
#define SENSOR_HIGH_DEFAULT   700 // Bright

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
#define MODE_12_24                      7  // Mode "00" 0 = 24, 1 = 12
#define HOUR_MODE_DEFAULT               false
#define MODE_LEAD_BLANK                 8  // Mode "01" 1 = blanked
#define LEAD_BLANK_DEFAULT              false
#define MODE_SCROLLBACK                 9  // Mode "02" 1 = use scrollback
#define SCROLLBACK_DEFAULT              false
#define MODE_FADE                       10 // Mode "03" 1 = use fade
#define FADE_DEFAULT                    false
#define MODE_DATE_FORMAT                11 // Mode "04"
#define MODE_DAY_BLANKING               12 // Mode "05"
#define MODE_HR_BLNK_START              13 // Mode "06" - skipped if not using hour blanking
#define MODE_HR_BLNK_END                14 // Mode "07" - skipped if not using hour blanking

#define MODE_USE_LDR                    15 // Mode "08" 1 = use LDR, 0 = don't (and have 100% brightness)
#define MODE_USE_LDR_DEFAULT            true

#define MODE_BLANK_MODE                 16 // Mode "09" 
#define MODE_BLANK_MODE_DEFAULT         BLANK_MODE_BOTH

// Display tricks
#define MODE_FADE_STEPS_UP              17 // Mode "10"
#define MODE_FADE_STEPS_DOWN            18 // Mode "11"
#define MODE_DISPLAY_SCROLL_STEPS_UP    19 // Mode "12"
#define MODE_DISPLAY_SCROLL_STEPS_DOWN  20 // Mode "13"

#define MODE_SLOTS_MODE                 21 // Mode "14"

// Separator LED mode
#define MODE_LED_BLINK                  22 // Mode "15"

// PIR
#define MODE_PIR_TIMEOUT_UP             23 // Mode "16"
#define MODE_PIR_TIMEOUT_DOWN           24 // Mode "17"

// Back light
#define MODE_BACKLIGHT_MODE             25 // Mode "18"
#define MODE_RED_CNL                    26 // Mode "19"
#define MODE_GRN_CNL                    27 // Mode "20"
#define MODE_BLU_CNL                    28 // Mode "21"
#define MODE_CYCLE_SPEED                29 // Mode "22"

// Minimum dimming value for old tubes
#define MODE_MIN_DIM_UP                 30 // Mode "23"
#define MODE_MIN_DIM_DOWN               31 // Mode "24"

// Use the PIR pin pullup resistor
#define MODE_USE_PIR_PULLUP             32 // Mode "25" 1 = use Pullup
#define USE_PIR_PULLUP_DEFAULT          true

// Software Version
#define MODE_VERSION                    33 // Mode "25"

// Tube test - all six digits, so no flashing mode indicator
#define MODE_TUBE_TEST                  34

#define MODE_MAX                        34

// -------------------------------------------------------------------------------
// Temporary display modes - accessed by a short press ( < 1S ) on the button when in MODE_TIME

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
#define BLANK_MODE_MAX                  2
#define BLANK_MODE_DEFAULT              2

// -------------------------------------------------------------------------------
#define BACKLIGHT_MIN                   0
#define BACKLIGHT_FIXED                 0   // Just define a colour and stick to it
#define BACKLIGHT_PULSE                 1   // pulse the defined colour
#define BACKLIGHT_CYCLE                 2   // cycle through random colours
#define BACKLIGHT_FIXED_DIM             3   // A defined colour, but dims with bulb dimming
#define BACKLIGHT_PULSE_DIM             4   // pulse the defined colour, dims with bulb dimming
#define BACKLIGHT_CYCLE_DIM             5   // cycle through random colours, dims with bulb dimming
#define BACKLIGHT_COLOUR_TIME           6   // use "ColourTime" - different colours for each digit value
#define BACKLIGHT_COLOUR_TIME_DIM       7   // use "ColourTime" - dims with bulb dimming
#define BACKLIGHT_MAX                   7
#define BACKLIGHT_DEFAULT               4

// -------------------------------------------------------------------------------
#define CYCLE_SPEED_MIN                 4
#define CYCLE_SPEED_MAX                 64
#define CYCLE_SPEED_DEFAULT             10

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
#define SEPARATOR_DIM_FACTOR_MIN        10
#define SEPARATOR_DIM_FACTOR_MAX        100
#define SEPARATOR_DIM_FACTOR_DEFAULT    100

// -------------------------------------------------------------------------------
#define BACKLIGHT_DIM_FACTOR_MIN        10
#define BACKLIGHT_DIM_FACTOR_MAX        100
#define BACKLIGHT_DIM_FACTOR_DEFAULT    100

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

void setPinModes() {
  // Set up shift registers
  pinMode(LATCHPin, OUTPUT);
  pinMode(CLOCKPin, OUTPUT);
  pinMode(DATA1Pin, OUTPUT);
  pinMode(LEDPin1, OUTPUT);
  pinMode(LEDPin2, OUTPUT);
  pinMode(LEDPin2, OUTPUT);

  // NOTE: Grounding the input pin causes it to actuate
  pinMode(inputPin1, INPUT_PULLUP);
}

#endif
