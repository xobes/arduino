// Adafruit IO IFTTT Door Detector
// 
// Learn Guide: https://learn.adafruit.com/using-ifttt-with-adafruit-io
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2018 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

// edit the config.h tab and enter your configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config_const.h"
#include "config_io.h"
#include "config_wifi.h"

// Load Wi-Fi library
#include <ESP8266WiFi.h>
/************************ Example Starts Here *******************************/

#include "timers.h"
#include "door.h"
#include "wifi_http_server.h"

void setup() {

  // start the serial connection
  Serial.begin(115200);
  while (!Serial);

  // setup_io();
  setup_server();

  setup_timers();
  DOOR_setup();
  Serial.println("done setup");

}


void loop() {
    serve_loop();

    delay(100);
    DOOR_loop();
}
