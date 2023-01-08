/*
Copyright 2013 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include "host.h"
#include "report.h"
#include "debug.h"
#include "action_util.h"
#include "timer.h"

static inline void update_key_byte(uint8_t code, bool add);
#ifdef NKRO_ENABLE
static inline void update_key_bit(uint8_t code,bool add);
#endif

static uint8_t real_mods = 0;
static uint8_t weak_mods = 0;

// for Win_Lock And shift + (Shift_KEY) = KEY
uint8_t block_mods = 0;
uint8_t lock_mods = 0;
bool has_mods_key = 0;


// TODO: pointer variable is not needed
//report_keyboard_t keyboard_report = {};
report_keyboard_t *keyboard_report = &(report_keyboard_t){};

#ifndef NO_ACTION_ONESHOT
static int8_t oneshot_mods = 0;
#if (defined(ONESHOT_TIMEOUT) && (ONESHOT_TIMEOUT > 0))
static int16_t oneshot_time = 0;
#endif
#endif


void send_keyboard_report(void) {
    keyboard_report->mods  = real_mods;
    keyboard_report->mods |= weak_mods;
    keyboard_report->mods &= ~block_mods;
    keyboard_report->mods &= ~lock_mods;
#ifndef NO_ACTION_ONESHOT
    if (oneshot_mods) {
#if (defined(ONESHOT_TIMEOUT) && (ONESHOT_TIMEOUT > 0))
        if (TIMER_DIFF_16(timer_read(), oneshot_time) >= ONESHOT_TIMEOUT) {
            dprintf("Oneshot: timeout\n");
            clear_oneshot_mods();
        }
#endif
        keyboard_report->mods |= oneshot_mods;
        if (has_anykey()) {
            clear_oneshot_mods();
        }
    }
#endif
    host_keyboard_send(keyboard_report);
}

/* key */
void update_key(uint8_t key, bool add)
{
#ifdef NKRO_ENABLE
    /* make keyboard_protocol compatiable with bt mode.
       1<<0: Protocol   0: Boot Protocol, 1:Report Protocol(default)
       1<<4: NKRO  //not ready for chibios
       1<<7: BT 
       */
#ifdef __AVR__
    if ((keyboard_protocol & (1<<7 | 1<<4 | 1<<0)) == (1<<4 | 1<<0)) {
#else 
    if ((keyboard_protocol & (1<<7 | 1<<0)) == (1<<0) && keyboard_nkro) {
#endif
        update_key_bit(key, add);
        return;
    }
#endif
    update_key_byte(key, add);
}

void clear_keys(void)
{
    memset(&keyboard_report->raw[0], 0, KEYBOARD_REPORT_SIZE);
    // not clear mods
    //memset(&keyboard_report->raw[1], 0, KEYBOARD_REPORT_SIZE - 1);
    //for (int8_t i = 1; i < KEYBOARD_REPORT_SIZE; i++) {
    //    keyboard_report->raw[i] = 0;
    //}
}


/* modifier */
uint8_t get_mods(void) { return real_mods; }
void add_mods(uint8_t mods) { real_mods |= mods; }
void del_mods(uint8_t mods) { real_mods &= ~mods; }
void set_mods(uint8_t mods) { real_mods = mods; }
void clear_mods(void) { real_mods = 0; }

/* weak modifier */
uint8_t get_weak_mods(void) { return weak_mods; }
void add_weak_mods(uint8_t mods) { weak_mods |= mods; }
void del_weak_mods(uint8_t mods) { weak_mods &= ~mods; }
void set_weak_mods(uint8_t mods) { weak_mods = mods; }
void clear_weak_mods(void) { weak_mods = 0; }

/* Oneshot modifier */
#ifndef NO_ACTION_ONESHOT
void set_oneshot_mods(uint8_t mods)
{
    oneshot_mods = mods;
#if (defined(ONESHOT_TIMEOUT) && (ONESHOT_TIMEOUT > 0))
    oneshot_time = timer_read();
#endif
}
void clear_oneshot_mods(void)
{
    oneshot_mods = 0;
#if (defined(ONESHOT_TIMEOUT) && (ONESHOT_TIMEOUT > 0))
    oneshot_time = 0;
#endif
}
#endif

/*
 * inspect keyboard state
 */
uint8_t has_anykey(void)
{
    uint8_t cnt = 0;
    for (uint8_t i = 1; i < KEYBOARD_REPORT_SIZE; i++) {
        if (keyboard_report->raw[i])
            cnt++;
    }
    return cnt;
}

uint8_t has_anymod(void)
{
    return bitpop(real_mods);
}

uint8_t get_first_key(void)
{
#ifdef NKRO_ENABLE
    if (keyboard_protocol == 1 && keyboard_nkro) {
        uint8_t i = 0;
        for (; i < KEYBOARD_REPORT_BITS && !keyboard_report->nkro.bits[i]; i++)
            ;
        return i<<3 | biton(keyboard_report->nkro.bits[i]);
    }
#endif
    return keyboard_report->keys[0];
}



static inline void update_key_byte(uint8_t code, bool add)
{
    int8_t i = KEYBOARD_REPORT_KEYS;
    int8_t empty = -1;
    for (; i >= 0; i--) {
        if (keyboard_report->keys[i] == 0) {
            empty = i;
        } else if (keyboard_report->keys[i] == code) {
            if (add) break; // already has key, no need to add;
            else     keyboard_report->keys[i] = 0; //del key.
        }
    }
    if (add && i == -1) {
        keyboard_report->keys[empty] = code;
    }
}

static inline void update_key_bit(uint8_t code, bool add)
{
    if ((code>>3) < KEYBOARD_REPORT_BITS) {
        uint8_t *p = &keyboard_report->nkro.bits[code>>3];
        uint8_t value = 1<<(code&7);
        add? (*p |= value):(*p &= ~value);
    } else {
        if (add) dprintf("add_key_bit: can't add: %02X\n", code);
        else     dprintf("del_key_bit: can't del: %02X\n", code);
    }
}
