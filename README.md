# garage_door_server

I should really tag the version that supports only one door
this version as it stands right now has a bug:

BUG:
if the door sensor is broken and claiming the door is open, it will continually trigger the auto-close logic and burn out the motor... yes, it happened.

---
TODO: 
X - Fix bug, test & tag
X - Add a second door
- add fritzing or eagle sketch
X make circuit boards for hardware
- document parts
- incorporate reed switch instead of magnetic sensor
- upper limit switch
- web configurable timeout
    - on_change handler to call set_...

- on the web side, need some security...
   - allow assigning individual users pins from the interface
   - admin password
   
   
   in the arduino programmer there is a menu drop down selection that indicates that there is a flash space for the micro controller and a separate space for the wifi credentials and/or config data (mac address? dunno)
   look into it!:w!
   
   
   found an example for storing sstuff in eeprom
   
   also store names of doors in eeprom or a config...
   e.g. DOOR1_NAME, DOOR2_NAME
   

