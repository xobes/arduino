// when south pole of magnet is near will set output of hall effect sensor to 0
// when magnet is not present, will set to pull up (high)
// the door will be CLOSED when the magnet is near and the pin is tus LOW
#define DOOR_CLOSED LOW
#define DOOR_OPEN HIGH

// door gpio pin
// red led is 0
// blue led is 2 (and if wired will not program *facepalm*)
#define DOOR_SENSOR_PIN 5
#define DOOR_CONTROL_PIN 4
