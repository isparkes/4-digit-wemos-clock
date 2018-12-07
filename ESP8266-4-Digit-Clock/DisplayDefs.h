#ifndef DisplayDefs_h
#define DisplayDefs_h

// Display handling
#define DIGIT_DISPLAY_START   0                       // Inner loop start point per digit
#define DIGIT_DISPLAY_COUNT   3000                    // The number of times to traverse inner fade loop per digit
#define DIGIT_DISPLAY_ON      99                      // Switch on the digit after the "front porch" by default
#define DIGIT_DISPLAY_OFF     3000                    // Switch off the digit at the end by default
#define DIGIT_DISPLAY_NEVER   -1                      // When we don't want to switch on or off (i.e. blanking)
#define DISPLAY_COUNT_MAX     DIGIT_DISPLAY_COUNT - 1 // Maximum value we can set to
#define DISPLAY_COUNT_MIN     300                     // Minimum value we can set to

// Display mode, set per digit
#define BLANKED  0
#define DIMMED   1
#define FADE     2
#define NORMAL   3
#define BLINK    4
#define SCROLL   5
#define BRIGHT   6

// ************************************************************
// LED brightness correction: The perceived brightness is not linear
// ************************************************************
const byte dim_curve[] = {
  0,   1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,
  4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,
  6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,
  8,   8,   9,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,  11,  11,  11,
  11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  14,  14,  14,  14,  15,
  15,  15,  16,  16,  16,  16,  17,  17,  17,  18,  18,  18,  19,  19,  19,  20,
  20,  20,  21,  21,  22,  22,  22,  23,  23,  24,  24,  25,  25,  25,  26,  26,
  27,  27,  28,  28,  29,  29,  30,  30,  31,  32,  32,  33,  33,  34,  35,  35,
  36,  36,  37,  38,  38,  39,  40,  40,  41,  42,  43,  43,  44,  45,  46,  47,
  48,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,
  63,  64,  65,  66,  68,  69,  70,  71,  73,  74,  75,  76,  78,  79,  81,  82,
  83,  85,  86,  88,  90,  91,  93,  94,  96,  98,  99,  101, 103, 105, 107, 109,
  110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
  146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
  193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 248, 255,
};

const byte rgb_backlight_curve[] = {0, 16, 32, 48, 64, 80, 99, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255};

// Used to define "colourTime" colours
//                             0    1    2    3    4    5    6    7    8    9
const byte colourTimeR[] = { 255, 255, 255, 96,   0,   0,   0,   0, 180,  255};
const byte colourTimeG[] = {   0, 164, 255, 255, 255, 255, 128,   0,   0,   0};
const byte colourTimeB[] = {   0,   0,   0,   0,  32, 190, 255, 255, 255, 255};

#define DIAGS_SPIFFS      0
#define DIAGS_WIFI        1
#define DIAGS_TIMESERVER  2
#define DIAGS_RTC         3

#define STATUS_RED    0
#define STATUS_YELLOW 1
#define STATUS_GREEN  2
#define STATUS_BLUE   3

#define LED_MODE_MIN        0
#define LED_RAILROAD        0
#define LED_BLINK_SLOW      1
#define LED_BLINK_FAST      2
#define LED_BLINK_DBL       3
#define LED_BLINK_DEFAULT   LED_RAILROAD
#define LED_MODE_MAX        3


#endif
