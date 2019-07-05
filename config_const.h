// when south pole of magnet is near will set output of hall effect sensor to 0
// when magnet is not present, will set to pull up (high)
// the door will be CLOSED when the magnet is near and the pin is tus LOW
#define DOOR_CLOSED LOW
#define DOOR_OPEN HIGH

// timeout in seconds where open door will be automatically closed
#define DOOR_TIMEOUT 60*60 // seconds
#define DOOR_TIME_TO_CLOSE 30 // seconds
