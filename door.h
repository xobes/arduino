#include "timers.h"

#define MAX_DOORS 2

// ensure wiring of switch, the input is pulled high and will be grounded (LOW) on connection
#define DOOR_CLOSED LOW
#define DOOR_OPEN HIGH

#define NOT_SET -1

// All timeouts in milliseconds
#define OPENING_TIMEOUT_TO_OPEN        6000
#define CLOSING_TIMEOUT_TO_FAILURE     5000
#define INSECURE_TIMEOUT_TO_AUTO_SHUT  1000 * 60 * 60 /* one hour of insecure */
#define AUTO_SHUT_TIMEOUT_TO_FAILURE   30 * 1000      /* 30 seconds to get it closed before declaring an equipment failure */

#ifndef DOOR_H
#define DOOR_H

typedef enum {
    D_CLOSED=100,
    D_CMD_OPEN,
    D_INSECURE=199, // if (state >= D_INSECURE)
    D_OPENING=200,
    D_CLOSING,
    D_OPEN=500,
    D_PART_OPEN_UP,
    D_PART_OPEN_DOWN // no comma
} DoorState_Type;

typedef enum {
    DM_AUTO_SHUT,
    DM_DISABLED,
    DM_NORMAL // no comma
} DoorMasterState_Type;

typedef enum {
    NO_CHANGE,
    NOW_CLOSED,
    NOW_OPENED // no comma
} SwitchStateChange_Type;

class DOOR {
    private:
        DoorState_Type last_state = D_OPEN;
        DoorState_Type state =      D_OPEN;

        DoorMasterState_Type last_master_state = DM_NORMAL;
        DoorMasterState_Type master_state =      DM_NORMAL;

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
                case D_CLOSED:    // commanded closed
                    state = D_CMD_OPEN;
                    break;
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
                    if (master_state == DM_AUTO_SHUT) {
                        // resume normal operation, auto-shut did it's job
                        master_state = DM_NORMAL;
                    }
                    break;
                case D_CMD_OPEN:
                    door_moving_timer.set(OPENING_TIMEOUT_TO_OPEN);
                    break;
                case D_OPENING:
                    if (!door_insecure_timer.is_set()) {
                        door_insecure_timer.set(INSECURE_TIMEOUT_TO_AUTO_SHUT);
                    }
                    door_moving_timer.set(OPENING_TIMEOUT_TO_OPEN);
                    break;
                case D_CLOSING:
                    door_moving_timer.set(CLOSING_TIMEOUT_TO_FAILURE);
                    break;
            }
        }

        void init_master_state() {
            // it has been determined that `master_state` needs to be initialized
            Serial.print("Door ");
            Serial.println(number); // info about which door?

            Serial.print("last_master_state = ");
            Serial.print(lookup_master_state(last_master_state));
            Serial.print("; master_state = ");
            Serial.print(lookup_master_state(master_state));
            Serial.println();

            Serial.print(" new master state = ");
            Serial.print(get_master_state());
            Serial.println();
            switch (master_state) {
                case DM_AUTO_SHUT:
                    if (!door_auto_shut_timer.is_set()) {
                        door_auto_shut_timer.set(AUTO_SHUT_TIMEOUT_TO_FAILURE); // after this much time we'll transition to DISABLED
                    }
                    door_insecure_timer.clear();
                    break;
                case DM_DISABLED:
                    door_auto_shut_timer.clear();
                    door_insecure_timer.clear();
                    break;
                case DM_NORMAL:
                    door_auto_shut_timer.clear();
                    if (state >= D_INSECURE) {
                        if (!door_insecure_timer.is_set()) {
                            door_insecure_timer.set(INSECURE_TIMEOUT_TO_AUTO_SHUT);
                        }
                    }
                    break;
            }
        }

        void check_timers() {
            // normal state timers are always in effect, but master mode controls the response
            if ((master_state == DM_AUTO_SHUT) && (door_auto_shut_timer.time_up())) {
                door_auto_shut_timer.clear(); // don't keep firing;
                master_state = DM_DISABLED;   // give up
            }

            // based on the sate, check the timer, if fired, blah
            // if auto_close... what's the program?, button button button?  over and over
            switch (state) {
                case D_CMD_OPEN:
                    if (door_moving_timer.time_up()) {
                        // failed to open, we gave it ONE WHOLE TRY :-P
                        master_state = DM_DISABLED; // door failed to open
                        // reason_disabled = "Failed to open";
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
                        if (master_state == DM_NORMAL) {
                            master_state = DM_AUTO_SHUT;
                        }
                    }
                    if (master_state == DM_AUTO_SHUT) {
                        send_pulse();
                    }
                    break;
            }
        }

    public:
        DOOR () {}

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

            state =      D_OPEN;
            last_state = D_OPEN;
            master_state =      DM_NORMAL;
            last_master_state = DM_NORMAL;
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

            // update state based on triggers
            switch_state_change = (int)check_door();

            if (switch_state_change == NOW_CLOSED) {
                state = D_CLOSED; // switch just closed, state will be CLOSED now
            } else if (switch_state_change == NOW_OPENED) {
                state = D_OPENING; // switch just opened, state will be OPENING now
            } else { // no change in the limit switch value
                if (relay_triggered) { // was the relay just pulsed?
                    on_relay_pulsed();
                    relay_triggered = false;
                    Serial.print(number); Serial.print(": ");
                }
            }

            if (last_state != state) {
                init_state();
            }
            last_state = state;

            if (last_master_state != master_state) {
                init_master_state();
            }
            last_master_state = master_state;

            check_timers();
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

        char* lookup_master_state(DoorMasterState_Type s) {
            switch (s) {
                case DM_DISABLED:
                    return "DISABLED";
                case DM_AUTO_SHUT:
                    return "AUTO_SHUT";
                case DM_NORMAL:
                    return "NORMAL";
            }
        }

        DoorMasterState_Type lookup_master_state_string(String master_state_string) {
            if (master_state_string == "AUTO_SHUT") {
                return DM_AUTO_SHUT;
            } else if (master_state_string == "NORMAL") {
                return DM_NORMAL;
            } else {
                return DM_DISABLED; // all lookup errors will yield disabled (used in setting master state)
            }
        }

        char* get_state() {
            return lookup_state(state);
        }

        char* get_master_state() {
            return lookup_master_state(master_state);
        }

        void set_master_state(String master_state_string) {
            master_state = lookup_master_state_string(master_state_string);
        }

};

DOOR* get_door(int which_door);

void DOOR_setup();
void DOOR_loop();

#endif
