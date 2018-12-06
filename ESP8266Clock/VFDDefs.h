#ifndef VFDDefs_h
#define VFDDefs_h

// output gpio run by 2 74HC595, driven by DATA1 and DATA2
// Layout
// DATA 1:
//   Segment a
//   Segment b
//   Segment c
//   Segment d
//   Segment e
//   Segment f
//   Segment g
//   Segment dp

// DATA 2:
//   grid Hx10
//   grid Hx1
//   grid Mx10
//   grid Mx1
//   grid Sx10
//   grid Sx1
//   LED 1 (between H and M)
//   LED 2 (between M and S)

// ************************ Display management ************************
#define SEG_A  B00000001
#define SEG_B  B00000010
#define SEG_C  B00000100
#define SEG_D  B00001000
#define SEG_E  B00010000
#define SEG_F  B00100000
#define SEG_G  B01000000
#define SEG_DP B10000000

#define DIGIT_0 SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F
#define DIGIT_1 SEG_B | SEG_C
#define DIGIT_2 SEG_A | SEG_B | SEG_D | SEG_D | SEG_E | SEG_G
#define DIGIT_3 SEG_A | SEG_B | SEG_C | SEG_D | SEG_G
#define DIGIT_4 SEG_B | SEG_C | SEG_F | SEG_G
#define DIGIT_5 SEG_A | SEG_C | SEG_D | SEG_F | SEG_G
#define DIGIT_6 SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G
#define DIGIT_7 SEG_A | SEG_B | SEG_C
#define DIGIT_8 SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G
#define DIGIT_9 SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G
#define DIGIT_MINUS SEG_G

byte digitData[10] = {DIGIT_0, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4, DIGIT_5, DIGIT_6, DIGIT_7, DIGIT_8, DIGIT_9};

#define GRID_1    1
#define GRID_2    2
#define GRID_3    4
#define GRID_4    8
#define GRID_5    16
#define GRID_6    32
#define LED_1     64
#define LED_2     128

byte data1[6] = {GRID_1, GRID_2, GRID_3, GRID_4, GRID_5, GRID_6};

#endif
