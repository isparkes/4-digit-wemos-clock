//**********************************************************************************
//* ESP8266 4-Digit Nixie Clock                                                    *
//* Copyright (c) 2018-2026 Ian Sparkes. All rights reserved.                      *
//*                                                                                *
//* Free for non-commercial use. Commercial use needs a licence from the author.   *
//* See the LICENSE file in the repository root for the full terms.                *
//* Contact: nixie@protonmail.ch  |  https://www.nixieclock.biz                    *
//**********************************************************************************

#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include <Arduino.h>

// ------------------------ Types ------------------------

// ----------------------------------------------------------------------------------------------------
// ------------------------------------- SPIFFS Clock Component ---------------------------------------
// ----------------------------------------------------------------------------------------------------

class DebugManager
{
  public:
    void setUp(bool newDebug);
    void debugMsg(String message);
    bool getDebug();
  private:
    bool _debug = false;
};

// ----------------- Exported Variables ------------------

// SPIFFS component
static DebugManager debugManager;

#endif
