#ifndef __HMTL_SERVER_H__
#define __HMTL_SERVER_H__

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include "ClockUtils.h"

// ----------------------- Auth ------------------------

#define WEB_USERNAME_DEFAULT "admin"
#define WEB_PASSWORD_DEFAULT "setup"
#define WEB_AUTH_DEFAULT true

// -----------------------------------------------------

extern ESP8266WebServer server;
extern ESP8266HTTPUpdateServer httpUpdater;
extern MDNSResponder mdns;

boolean getWebAuthentication();
void setWebAuthentication(boolean newValue);
String getWebUserName();
void setWebUserName(String newValue);
String getWebPassword();
void setWebPassword(String newValue);

void setServerPageHandlers();
void checkServerArgBoolean(String argument, String argumentName, String trueLiteral, String falseLiteral, boolean &changed, boolean &value);
void checkServerArgInt(String argument, String argumentName, boolean &changed, int &value);
void checkServerArgByte(String argument, String argumentName, boolean &changed, byte &value);
void checkServerArgString(String argument, String argumentName, boolean &changed, String &value);

#endif
