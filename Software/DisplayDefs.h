//**********************************************************************************
//* ESP8266 4-Digit Nixie Clock                                                    *
//* Copyright (c) 2018-2026 Ian Sparkes. All rights reserved.                      *
//*                                                                                *
//* Free for non-commercial use. Commercial use needs a licence from the author.   *
//* See the LICENSE file in the repository root for the full terms.                *
//* Contact: nixie@protonmail.ch  |  https://www.nixieclock.biz                    *
//**********************************************************************************

//**********************************************************************************
//* Definitions display based functions                                            *
//**********************************************************************************

#ifndef DisplayDefs_h
#define DisplayDefs_h

// Display mode, set per digit
#define BLANKED  0
#define DIMMED   1
#define NORMAL   2
#define FADE     3
#define SCROLL   4
#define BLINK    5
#define BRIGHT   6

#define DIAGS_SPIFFS      0
#define DIAGS_WIFI        1
#define DIAGS_NTP         2
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
#define LED_ON              4
#define LED_OFF             5
#define LED_AM_PM           6
#define LED_BLINK_DEFAULT   LED_RAILROAD
#define LED_MODE_MAX        6

#define DIGIT_COUNT         4

#define STATUS_LED_MODE_MIN    0
#define STATUS_LED_MODE_NTP    0
#define STATUS_LED_MODE_ONLINE 1
#define STATUS_LED_MODE_AM_PM  2
#define STATUS_LED_MODE_PIR    3
#define STATUS_LED_MODE_OFF    4
#define STATUS_LED_MODE_ON     5
#define STATUS_LED_MODE_MAX    5

// ************************ Display management ************************

// allows the board wiring to be much simpler
// Mapping             2 3 7 6 4 5 1 0 8 9 
const byte decodeDigit[16] = {B00000010,B00000011,B00000111,B00000110,B00000100,B00000101,B00000001,B00000000,B00001000,B00001001,B00001111,B00001111,B00001111,B00001111,B00001111,B00001111};

#define ANODE_1    B00010000
#define ANODE_2    B00100000
#define ANODE_3    B01000000
#define ANODE_4    B10000000
#define ANODE_NONE B00000000

const byte anodes[4] = {ANODE_1, ANODE_2, ANODE_3, ANODE_4};

#endif
