#include "timers.h"

int tn = 0; // keep track of how many timers we have
TIMER *timer_ptrs[MAX_TIMERS];

extern "C" {
#include "user_interface.h"
}

os_timer_t myTimer;

void periodic_timer_isr(void *parg) {
    // will call this every TIMER_STEP ms by ISR
    for (int i=0; i < tn; i++) {
        // Serial.print("i: ");
        // Serial.println(i);
        // Serial.print("value: ");
        // Serial.println((timer_ptrs[i])->value);
        (timer_ptrs[i])->decr(TIMER_STEP);
    }
}

void setup_timers(void) {
    /*
    os_timer_setfn - Define a function to be called when the timer fires

    void os_timer_setfn(
          os_timer_t *pTimer,
          os_timer_func_t *pFunction,
          void *pArg)

    Define the callback function that will be called when the timer reaches zero. The pTimer parameters is a pointer to the timer control structure.

    The pFunction parameters is a pointer to the callback function.

    The pArg parameter is a value that will be passed into the called back function. The callback function should have the signature:
    void (*functionName)(void *pArg)

    The pArg parameter is the value registered with the callback function.
    */

    os_timer_setfn(&myTimer, periodic_timer_isr, NULL);

    /*
    os_timer_arm -  Enable a millisecond granularity timer.

    void os_timer_arm(
          os_timer_t *pTimer,
          uint32_t milliseconds,
          bool repeat)

    Arm a timer such that is starts ticking and fires when the clock reaches zero.

    The pTimer parameter is a pointed to a timer control structure.
    The milliseconds parameter is the duration of the timer measured in milliseconds. The repeat parameter is whether or not the timer will restart once it has reached zero.

    */

    os_timer_arm(&myTimer, TIMER_STEP, true);
} // End of setup_timers

void register_new_timer(void *timerPtr) {
    if (tn < (MAX_TIMERS - 1)) {
        Serial.print("registered timer #");
        Serial.println(tn);
        timer_ptrs[tn++] = (TIMER *)timerPtr;
    }
}
