#include "door_http_connector.h"
// #include "door.h"
// #include "timers.h"

// -- main program will have DOOR... how to get doors list in here?
String get_door_state(int which_door) {
    Serial.print("get_door_state(");
    Serial.print(which_door);
    Serial.print(")");
    Serial.println();

    DOOR* d = get_door(which_door);
    Serial.println (d->get_state());
    return (String)(d->get_state());
}

void pulse_door_relay(int which_door) {
    Serial.print("pulse_door_relay(");
    Serial.print(which_door);
    Serial.print(")");
    Serial.println();

    DOOR* d = get_door(which_door);
    Serial.print( "Send Pulse");

    d->send_pulse();
}

int get_door_timeout(int which_door) {
    Serial.print("get_door_timeout(");
    Serial.print(which_door);
    Serial.print(")");
    Serial.println();

    DOOR* d = get_door(which_door);
    Serial.println (
        (d->get_door_timeout())
    );
    return d->get_door_timeout();
}

void set_door_timeout(int which_door, int timeout) {
    // this is the 'insecure' timeout allowed.
    Serial.print("set_door_tiemout(");
    Serial.print(which_door);
    Serial.print(", ");
    Serial.print(timeout);
    Serial.print(")");
    Serial.println();

    DOOR* d = get_door(which_door);
    d->set_door_timeout(timeout);
}



/*
do the same with the doors as we did with the timers in terms of an .init and registering them in an array...
but wait, the real trick was being able to reference them because they were made in another scope...

either way, I need to be able to reach in and ...  get_door(which_door).state.... hmmm
*/