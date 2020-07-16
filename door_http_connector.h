// Load Wi-Fi library (for String)
#include <ESP8266WiFi.h>

#include "door.h"

String get_door_state(int which_door);
void pulse_door_relay(int which_door);
int get_door_timeout(int which_door);
void set_door_timeout(int which_door, int timeout); // this is the 'insecure' timeout allowed.