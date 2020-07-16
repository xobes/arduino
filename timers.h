#include <Arduino.h>

#ifndef TIMERS_H
#define TIMERS_H

#define MAX_TIMERS 15
#define TIMER_STEP 100

void register_new_timer(void *tPtr);
void setup_timers(void);

class TIMER {
    public:
        int value;

        TIMER () {
            // this->init();
        }

        void init() {
            value = -1;
            register_new_timer(this);
        }

        void clear() {
            value = -1;
        }

        void set (int t) {
            value = t;
        }

        void decr(int i) {
            // decriment the counter by up to i, but no more than the value itself, yielding zero
            if (i > 0) {
                // Serial.print(value);
                // Serial.print(" - ");
                // Serial.print(i);
                // Serial.print(" = ");
                if (value > i) {
                    value -= i;
                } else if (value > 0) {
                    value = 0;
                }
                // Serial.println(value);
            }
        }

        bool is_set() {
            return (value >= 0);
        }

        bool time_up() {
            return (value == 0);
        }
};

#endif /* TIMERS_H */
