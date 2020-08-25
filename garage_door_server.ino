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
