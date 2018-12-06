#include "ClockUtils.h"

String formatIPAsString(IPAddress ip) {
  return String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
}

int getIntValue(String data, char separator, int index) {
  String result = getValue(data, separator, index);
  return atoi(result.c_str());
}

/**
   Split a string based on a separator, get the element given by index
*/
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


