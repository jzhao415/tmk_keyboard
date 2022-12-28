#ifndef DEBOUNCE_PK_H
#define DEBOUNCE_PK_H

#include <stdbool.h>
#include "print.h"
#ifdef __AVR__
#include "avr/avr_config.h"
#endif
/*
 * debouce data like 0b00100101. '>>' + key_dn when scanning.
 * When DEBOUNCE_DN is 3,  key dn when debouce >= 0b11110000.
 * When DEBOUNCE_UP is 4,  key up when debouce <= 0b00000111.
 * Ignore 0 and 0xff. 0 is always up and 0xff is always down.
 * DEBOUNCE_X must be 0-6 when uint8_t matrix_debouncing[][].
 */
#ifndef DEBOUNCE_DN
#define DEBOUNCE_DN 5
#endif

#ifndef DEBOUNCE_NK
#define DEBOUNCE_NK 1
#endif

#ifndef DEBOUNCE_UP
#define DEBOUNCE_UP 5
#endif

#if (DEBOUNCE_DN < 7) && (DEBOUNCE_NK < 7) && (DEBOUNCE_UP < 7)
#define DEBOUNCE_DN_MASK (uint8_t)(~(0x80 >> DEBOUNCE_DN))
#define DEBOUNCE_NK_MASK (uint8_t)(~(0x80 >> DEBOUNCE_NK))
#define DEBOUNCE_UP_MASK (uint8_t)(0x80 >> DEBOUNCE_UP)
#else
#error "DEBOUNCE VALUE must not exceed 6"
#endif



#endif
