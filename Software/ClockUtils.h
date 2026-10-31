//**********************************************************************************
//* ESP8266 4-Digit Nixie Clock                                                    *
//* Copyright (c) 2018-2026 Ian Sparkes. All rights reserved.                      *
//*                                                                                *
//* Free for non-commercial use. Commercial use needs a licence from the author.   *
//* See the LICENSE file in the repository root for the full terms.                *
//* Contact: nixie@protonmail.ch  |  https://www.nixieclock.biz                    *
//**********************************************************************************

//**********************************************************************************
//* Utilities which can be separated from the main code                            *
//**********************************************************************************

#ifndef ClockUtils_h
#define ClockUtils_h

#include <FS.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include "HtmlServer.h"
#include "ClockDefs.h"

/*
 * A collection of utility functions
 */

String getBool(bool value);
String formatIPAsString(IPAddress ip);
int getIntValue(String data, char separator, int index);
String getValue(String data, char separator, int index);
void grabInts(String s, int *dest, String sep);
unsigned char hex2bcd (unsigned char x);
void setPinModes();

// ----------------------------------------------------------------------------------------------------
// ------------------------------------------- HTML functions -----------------------------------------
// ----------------------------------------------------------------------------------------------------

String getHTMLHead(boolean isConnected);
String getNavBar();
String getTableHead2Col(String tableHeader, String col1Header, String col2Header);
String getTableRow2Col(String col1Val, String col2Val);
String getTableRow2Col(String col1Val, int col2Val);
String getTableFoot();
String getFormHead(String formTitle);
String getFormFoot();
String getHTMLFoot();
String getRadioGroupHeader(String header);
String getRadioButton(String group_name, String text, String value, boolean checked);
String getRadioGroupFooter();
String getCheckBox(String checkbox_name, String value, String text, boolean checked);
String getDropDownHeader(String heading, String group_name, boolean wide);
String getDropDownOption (String value, String text, boolean checked);
String getDropDownFooter();
String getNumberInput(String heading, String input_name, unsigned int minVal, unsigned int maxVal, unsigned int value, boolean disabled);
String getNumberInputWide(String heading, String input_name, byte minVal, byte maxVal, byte value, boolean disabled);
String getTextInput(String heading, String input_name, String value, boolean disabled);
String getTextInputWide(String heading, String input_name, String value, boolean disabled);
String getSubmitButton(String buttonText);
String secsToReadableString(long secsValue);

// ----------------------------------------------------------------------------------------------------
// --------------------------------------------- Debugging --------------------------------------------
// ----------------------------------------------------------------------------------------------------
void setupDebug(boolean setUseDebug);
void debugMsg(String message);
void debugMsgCont(String message);
boolean getDebug();
void toggleDebug();

#endif
