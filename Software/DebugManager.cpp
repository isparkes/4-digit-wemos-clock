//**********************************************************************************
//* ESP8266 4-Digit Nixie Clock                                                    *
//* Copyright (c) 2018-2026 Ian Sparkes. All rights reserved.                      *
//*                                                                                *
//* Free for non-commercial use. Commercial use needs a licence from the author.   *
//* See the LICENSE file in the repository root for the full terms.                *
//* Contact: nixie@protonmail.ch  |  https://www.nixieclock.biz                    *
//**********************************************************************************

#include "DebugManager.h"

void DebugManager::setUp(bool newDebug) {
  if (!_debug && newDebug) {
    Serial.begin(115200);
    debugMsg("");
    debugMsg("--------------------- START ---------------------");
  }
  
  if (_debug && !newDebug) {
    debugMsg("");
    debugMsg("---------------------- END ----------------------");
    Serial.end();
  }
  
  _debug = newDebug;
}

void DebugManager::debugMsg(String message) {
  if (_debug) {
    Serial.println(message);
    Serial.flush();
  }
}

bool DebugManager::getDebug() {
  return _debug;
}
