#include "timers.h"

#define MAX_DOORS 2

// ensure wiring of switch, the input is pulled high and will be grounded (LOW) on connection
#define DOOR_CLOSED LOW
#define DOOR_OPEN HIGH

#define NOT_SET -1

#define OPENING_DELAY   6000
#define CLOSING_DELAY   5000
#define INSECURE_DELAY  1000 * 60 * 60 /* one hour of insecure */
#define AUTO_SHUT_DELAY 30 * 1000      /* 30 seconds to get it closed before declaring an equipment failure */

#ifndef DOOR_H
#define DOOR_H

typedef enum {
    D_AUTO_SHUT=99,
    D_DISABLED=0,
    D_CLOSED=100,
    D_CMD_OPEN,
    D_OPENING=200,
    D_CLOSING,
    D_OPEN=500,
    D_PART_OPEN_UP,
    D_PART_OPEN_DOWN // no comma
} DoorState_Type;

typedef enum {
    NO_CHANGE,
    NOW_CLOSED,
    NOW_OPENED // no comma
} SwitchStateChange_Type;

class DOOR {
    private:
        DoorState_Type last_state = D_DISABLED;
        DoorState_Type state =      D_DISABLED;

        int number;

        int door_relay_pin;
        int limit_switch_pin;

        bool relay_triggered = false;

        TIMER door_moving_timer;
        TIMER door_insecure_timer;
        TIMER door_auto_shut_timer;

        int last_switch_state = DOOR_CLOSED;
        int switch_state =      DOOR_CLOSED;

    protected:
        SwitchStateChange_Type check_door() {
            SwitchStateChange_Type switch_state_change = NO_CHANGE;

            // store the current state and get a new reading.
            last_switch_state = switch_state;

            // TODO: debounce, debounceRead ? :-)
            switch_state = digitalRead(limit_switch_pin);

            // ignore steady state, only react to changes
            if (last_switch_state != switch_state) {
                if (switch_state == DOOR_CLOSED) {
                    // on door close
                    switch_state_change = NOW_CLOSED;
                    // Serial.println("check_door => now CLOSED");
                } else {
                    // on door open
                    switch_state_change = NOW_OPENED;
                    // Serial.println("check_door => now OPEN");
                }
            }
            return switch_state_change;
        }

        void on_relay_pulsed() {
            // relay was pulsed, determine next state
            // depending on the current state and knowing the relay was triggered
            switch (state) {
                case D_CMD_OPEN:  // no change
                case D_CLOSED:    // commanded cloded
                case D_DISABLED:  // commanded closed
                    state = D_CMD_OPEN;
                    break;
                case D_AUTO_SHUT:       // closing
                case D_PART_OPEN_UP:
                case D_OPEN:
                    state = D_CLOSING;
                    break;
                case D_OPENING:
                    state = D_PART_OPEN_UP;
                    break;
                case D_CLOSING:
                    state = D_PART_OPEN_DOWN;
                    break;
                case D_PART_OPEN_DOWN:
                    state = D_OPENING;
                    break;
            }
        }

        void init_state() {
            // it has been determined that `state` needs to be initialized
            Serial.print("Door ");
            Serial.print(number); // info about which door?
            Serial.print(" new state = ");
            Serial.print(get_state());
            Serial.println();
            switch (state) {
                case D_CLOSED:
                    door_moving_timer.clear();
                    door_insecure_timer.clear();
                    door_auto_shut_timer.clear();
                    break;
                case D_CMD_OPEN:
                    door_moving_timer.set(OPENING_DELAY);
                    break;
                case D_OPENING:
                    if (!door_insecure_timer.is_set()) {
                        door_insecure_timer.set(INSECURE_DELAY);
                    }
                    door_moving_timer.set(OPENING_DELAY);
                    break;
                case D_CLOSING:
                    door_moving_timer.set(CLOSING_DELAY);
                    break;
                case D_AUTO_SHUT:
                    // the first time in AUTO_SHUT, it's over
                    if (!door_auto_shut_timer.is_set()) {
                        door_auto_shut_timer.set(AUTO_SHUT_DELAY);
                    }
                    break;
            }
        }

        void check_timers() {
            // based on the sate, check the timer, if fired, blah
            // if auto_close... what's the program?, button button button?  over and over
            switch (state) {
                case D_CMD_OPEN:
                    if (door_moving_timer.time_up()) {
                        // failed to open, we gave it ONE WHOLE TRY :-P
                        state = D_DISABLED;
                    }
                    break;

                case D_OPENING:
                    if (door_moving_timer.time_up()) {
                        // opening for a while, going to assume it stopped
                        state = D_OPEN;
                    }
                    break;

                case D_CLOSING:
                    if (door_moving_timer.time_up()) {
                        // trying to close for a while, probably really still open
                        state = D_OPENING;
                    }
                    break;

                // insecure static states
                case D_PART_OPEN_UP:
                case D_OPEN:
                case D_PART_OPEN_DOWN:
                    if (door_insecure_timer.time_up()) {
                        // trying to close for a while, probably really still open
                        state = D_AUTO_SHUT;
                    }
                    break;

                case D_AUTO_SHUT:
                    if (door_auto_shut_timer.time_up()) {
                        door_auto_shut_timer.clear(); // don't keep firing;
                        state = D_DISABLED; // give up
                    } else {
                        send_pulse(); // and then life goes on... hopefully, else, we'll be back
                    }
                    break;
            }
        }

    public:
        DOOR () {
        }

        void init(int n, int a, int b) {
            number = n;

            limit_switch_pin = a;
            door_relay_pin = b;

            pinMode(limit_switch_pin, INPUT_PULLUP);
            pinMode(door_relay_pin, OUTPUT);
            Serial.print("Pin ");
            Serial.print(limit_switch_pin);
            Serial.println(" => input with pullup");
            Serial.print("Pin ");
            Serial.print(door_relay_pin);
            Serial.println(" => output");

            // Set outputs to LOW (initialize!!!)
            digitalWrite(door_relay_pin, LOW);

            state = D_DISABLED;
            last_state = D_DISABLED;
            Serial.println("DOOR initialized");

            door_moving_timer = TIMER();
            door_insecure_timer = TIMER();
            door_auto_shut_timer = TIMER();

            door_moving_timer.init();
            door_insecure_timer.init();
            door_auto_shut_timer.init();
        }

        void loop() {
            int switch_state_change;

            last_state = state;

//            Serial.print(number);
//            Serial.print(": last_state = ");
//            Serial.print( lookup_state(last_state) );
//            Serial.print("; relay_triggered = ");
//            Serial.println(relay_triggered);

            // update state based on triggers
            switch_state_change = (int)check_door();

            if (switch_state_change == NOW_CLOSED) {
//                Serial.println("\t\tJust closed.");
                state = D_CLOSED; // switch just closed, state will be CLOSED now
            } else if (switch_state_change == NOW_OPENED) {
//                Serial.println("\t\tJust opening.");
                state = D_OPENING; // switch just opened, state will be OPENING now
            } else { // no change in the limit switch value
                if (relay_triggered) { // was the relay just pulsed?
                    on_relay_pulsed();
                    relay_triggered = false;
                    Serial.print(number); Serial.print(": ");
//                    Serial.println("\t\tRELAY_TRIGGERED := FALSE");
                } else {
//                    Serial.println("\t\t Check timers.");
                    check_timers();
                }
            }

            if (last_state != state) {
                init_state();
            }
        }

        void send_pulse() {
            Serial.println("send_pulse()");
            digitalWrite(door_relay_pin, HIGH);
            delay(500); // long enough but not forever...
            digitalWrite(door_relay_pin, LOW);

            relay_triggered = true;
//            Serial.print(number); Serial.print(": ");
//            Serial.println("\t\tRELAY_TRIGGERED := TRUE");
        }

        int get_door_timeout() {
            return door_insecure_timer.value;
        }

        void set_door_timeout(int timeout) {
            Serial.print("Changing door_insecure_timer for door ");
            Serial.print(number);
            Serial.print(" from value ");
            Serial.print(door_insecure_timer.value);
            Serial.print(" to ");
            Serial.print(timeout);
            Serial.print("...");
            door_insecure_timer.set(timeout);
            Serial.println("Done.");
            Serial.print("Timer value is now: ");
            Serial.println(get_door_timeout());
        }

        char* lookup_state(DoorState_Type s) {
            switch (s) {
                case D_CMD_OPEN:
                    return "COMMANDED OPEN";
                case D_CLOSED:
                    return "CLOSED";
                case D_DISABLED:
                    return "DISABLED";
                case D_AUTO_SHUT:
                    return "AUTO_SHUT";
                case D_PART_OPEN_UP:
                    return "PART_OPEN_UP";
                case D_OPEN:
                    return "OPEN";
                case D_OPENING:
                    return "OPENING";
                case D_CLOSING:
                    return "CLOSING";
                case D_PART_OPEN_DOWN:
                    return "PART_OPEN_DOWN";
            }
        }

        char* get_state() {
            return lookup_state(state);
        }

};

DOOR* get_door(int which_door);

void DOOR_setup();
void DOOR_loop();

#endif
