#ifndef ClockUtils_h
#define ClockUtils_h

#include <FS.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>

/*
 * A collection of utility functions
 */

 String getBool(bool value);
 String formatIPAsString(IPAddress ip);
 int getIntValue(String data, char separator, int index);
 String getValue(String data, char separator, int index);

#endif
