/*
save some memory
*/
#if !defined(NOT_AVR) && !defined(NO_WATCHDOG)
#include <avr/wdt.h>
#endif

#include "wait.h"

void wait_ms_x2(uint8_t time) {
    while (time--) {
      wait_ms(2); 
    }
#if !defined(NOT_AVR) && !defined(NO_WATCHDOG)
    wdt_reset();
#endif
}
