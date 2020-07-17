# garage_door_server

## State Transition Diagram:
https://app.diagrams.net/#G141r2ZxTGMS_f6xRk69w9_cTgzJ55bsQ0

## TODO:
#### Hardware
- document hardware / wiring / schematic
    - add fritzing or eagle sketch
    - bill of materials / parts
- upper limit switch

#### Software

- web configurable timeout
    - on_change handler to call set_...

- Security, on the web side, need some security...
    - allow assigning individual users pins from the interface
    - admin password for adding users/generating codes
        - admin can generate code (hash of pin?)
            - sw requires hash generation algorithm
            - local installation key (admin password = salt?)
            - unique url created for supported hashes
                - 404 if hash removed
                - listing of hashes
                - note per hash, (e.g. who it was shared with)
                - logging of who did what when, present on web page as well
- twilio integration
    - build on top of hash/pin system
   
- serial interrogation of ESSID + wifi-password and storage into EEPROM
    - need to tap EEPROM for security codes as well
    
- could name the doors instead of 1, 2, using EEPROM... or just #define, lol

- optimize HTTP/js interaction due to latency...
    - tried to use a single timer for all the data...  problem is the esp8266 isn't quick and every now and then a client take a while to get data...
so the timers bunch up and overlap eventually, bringing us to 100% load.
    - Need to either:
        1) tally up the responses and wait until all of them come in to re-start the timer...
        2) (better) have one timer that gets an aggregate of all state data (e.g. as a json object):
            data = { 1: {"state" : "blah", "timeout": 123456},
                     2: {"state" : "blah", "timeout": 123456} }
            This will reduce the http calls down by a factor of 4 and give us a fighting chance.
        3) consider using ISR and long polling and a list of clients to support more than one client/response for the periodic data
            UDP would be great for spamming status info, so would MQTT, but I want it on a web page -- so TCP might be the only option...
            web sockets?
   
    

