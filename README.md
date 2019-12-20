# garage_door_server

I should really tag the version that supports only one door
this version as it stands right now has a bug:

BUG:
if the door sensor is broken and claiming the door is open, it will continually trigger the auto-close logic and burn out the motor... yes, it happened.

---
TODO: 
- add friting or eagle sketch
- make circuit boards for hardware
- document parts
- incorporate reed switch instead of magnetic sensor
- upper limit switch
- web configurable timeout

