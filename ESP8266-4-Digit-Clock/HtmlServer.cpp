#include "HtmlServer.h"

ESP8266WebServer server(80);            // port 80
ESP8266HTTPUpdateServer httpUpdater;    // ESP8266HTTPUpdateServer.h
MDNSResponder mdns;

// ----------------------- Auth ------------------------

boolean webAuthentication = WEB_AUTH_DEFAULT;
String webUsername = WEB_USERNAME_DEFAULT;
String webPassword = WEB_PASSWORD_DEFAULT;

// ************************************************************
// Check server args for a Boolean value and note if it has changed
// ************************************************************
void checkServerArgBoolean(String argument, String argumentName, String trueLiteral, String falseLiteral, boolean &changed, boolean &value) {
  if (server.hasArg(argument)) {
    debugMsg("Got " + argumentName + " : " + server.arg(argument));
    if ((server.arg(argument) == falseLiteral) && (value)) {
      debugMsg("-> Set " + falseLiteral + " mode");
      changed = true;
      value = false;
    }

    if ((server.arg(argument) == trueLiteral) && (!value)) {
      debugMsg("-> Set " + trueLiteral + " mode");
      changed = true;
      value = true;
    }
  }
}

// ************************************************************
// Check server args for an int value and note if it has changed
// ************************************************************
void checkServerArgInt(String argument, String argumentName, boolean &changed, int &value) {
  if (server.hasArg(argument)) {
    debugMsg("Got " + argumentName + " : " + server.arg(argument));
    int newValue = atoi(server.arg(argument).c_str());
    if (newValue != value) {
      changed = true;
      value = newValue;
      debugMsg("-> Set " + argumentName + ": " + String(value));
    }
  }
}

// ************************************************************
// Check server args for a byte value and note if it has changed
// ************************************************************
void checkServerArgByte(String argument, String argumentName, boolean &changed, byte &value) {
  if (server.hasArg(argument)) {
    debugMsg("Got " + argumentName + " : " + server.arg(argument));
    byte newValue = atoi(server.arg(argument).c_str());
    if (newValue != value) {
      changed = true;
      value = newValue;
      debugMsg("-> Set " + argumentName + ": " + String(value));
    }
  }
}

// ************************************************************
// Check server args for a string value and note if it has changed
// ************************************************************
void checkServerArgString(String argument, String argumentName, boolean &changed, String &value) {
  if (server.hasArg(argument)) {
    debugMsg("Got " + argumentName + " : " + server.arg(argument));
    String newValue = server.arg(argument);
    if (value != newValue) {
      value = newValue;
      debugMsg("-> Set " + argumentName + " = " + value);
      changed = true;
    }
  }
}

boolean getWebAuthentication() {
  return webAuthentication;
}

void setWebAuthentication(boolean newValue) {
  webAuthentication = newValue;
}

String getWebUserName() {
  return webUsername;
}

void setWebUserName(String newValue) {
  webUsername = newValue;
}

String getWebPassword() {
  return webPassword;
}

void setWebPassword(String newValue) {
  webPassword = newValue;
}
