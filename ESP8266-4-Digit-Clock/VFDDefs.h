#ifndef VFDDefs_h
#define VFDDefs_h

// ************************ Display management ************************

// allows the board wiring to be much simpler
// Mapping             2 3 7 6 4 5 1 0 8 9 
byte decodeDigit[10] = {B0010,B0011,B0111,B0110,B0100,B0101,B0001,B0000,B1000,B1001};

#define GRID_1    B00010000
#define GRID_2    B00100000
#define GRID_3    B01000000
#define GRID_4    B10000000

byte anodes[4] = {GRID_1, GRID_2, GRID_3, GRID_4};

#endif
