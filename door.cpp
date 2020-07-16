#include <Arduino.h>

#include "door.h"
#include "config_io.h"

DOOR doors[MAX_DOORS];

DOOR *get_door(int which_door) {
    int w = which_door - 1; // which_door is 1s based index
    /*
    for (int w; w<MAX_DOORS; w++) {
        Serial.print(w);
        Serial.print(": ");
        Serial.println(doors[w].get_state()); // depends on later, hand-coded references to 1 and 2
    }
    */
    if (0 <= w < MAX_DOORS) {
        return &(doors[w]);
    }
}

void DOOR_setup() {
    for (int i=0; i<MAX_DOORS; i++) {
        doors[i] = DOOR();
    }

    Serial.println("DOOR_setup start");

    doors[0].init(1, DOOR1_SENSOR_PIN, DOOR1_CONTROL_PIN); // sensor, relay
    doors[1].init(2, DOOR2_SENSOR_PIN, DOOR2_CONTROL_PIN); // sensor, relay

    Serial.println("DOOR_setup end");
}

void DOOR_loop() {

    for (int i=0; i<MAX_DOORS; i++) {
        doors[i].loop();
    }

    // Serial.println("DOOR_loop()");
}
