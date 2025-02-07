/*
Copyright 2011,2012,2013 Jun Wako <wakojun@gmail.com>

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
#include <stdint.h>
#include "keyboard.h"
#include "matrix.h"
#include "keymap.h"
#include "host.h"
#include "led.h"
#include "keycode.h"
#include "timer.h"
#include "print.h"
#include "debug.h"
#include "command.h"
#include "util.h"
#include "sendchar.h"
#include "bootmagic.h"
#include "eeconfig.h"
#include "backlight.h"
#include "hook.h"
#ifdef SOFTPWM_LED_ENABLE
#include "softpwm_led.h"
#else
#include "breathing_led.h"
#endif
#include "ledmap.h"
#include "ledmap_in_eeprom.h"
#include "keymap_in_eeprom.h"
#ifdef MOUSEKEY_ENABLE
#   include "mousekey.h"
#endif
#ifdef PS2_MOUSE_ENABLE
#   include "ps2_mouse.h"
#endif
#ifdef SERIAL_MOUSE_ENABLE
#include "serial_mouse.h"
#endif
#ifdef ADB_MOUSE_ENABLE
#include "adb.h"
#endif

#ifdef LED_DATA_ENABLE
uint8_t data_num;
#ifndef LED_DATA_ADDR
#define LED_DATA_ADDR 1
#endif
#endif

#ifdef MATRIX_HAS_GHOST
static bool has_ghost_in_row(uint8_t row)
{
    matrix_row_t matrix_row = matrix_get_row(row);
    // No ghost exists when less than 2 keys are down on the row
    if (((matrix_row - 1) & matrix_row) == 0)
        return false;

    // Ghost occurs when the row shares column line with other row
    for (uint8_t i=0; i < MATRIX_ROWS; i++) {
        if (i != row && (matrix_get_row(i) & matrix_row))
            return true;
    }
    return false;
}
#endif


void keyboard_setup(void)
{
    matrix_setup();
}

void keyboard_init(void)
{
    timer_init();
    matrix_init();
#ifdef PS2_MOUSE_ENABLE
    ps2_mouse_init();
#endif
#ifdef SERIAL_MOUSE_ENABLE
    serial_mouse_init();
#endif
#ifdef ADB_MOUSE_ENABLE
    adb_mouse_init();
#endif


#ifdef BOOTMAGIC_ENABLE
    bootmagic();
#endif

#ifdef LEDMAPU_ENABLE
    ledmapu_init();
#endif
#ifdef SOFTPWM_LED_ENABLE
    softpwm_init();
#endif
#ifdef BREATHING_LED_ENABLE
    breathing_led_init();
#endif
#ifdef BACKLIGHT_ENABLE
    backlight_init();
#endif
}

/*
 * Do keyboard routine jobs: scan mantrix, light LEDs, ...
 * This is repeatedly called as fast as possible.
 */
__attribute__ ((weak))
void keyboard_task(void)
{
    static matrix_row_t matrix_prev[MATRIX_ROWS];
#ifdef MATRIX_HAS_GHOST
    static matrix_row_t matrix_ghost[MATRIX_ROWS];
#endif
    static uint8_t led_status = 0;
    matrix_row_t matrix_row = 0;
    matrix_row_t matrix_change = 0;

    matrix_scan();
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix_row = matrix_get_row(r);
        matrix_change = matrix_row ^ matrix_prev[r];
        if (matrix_change) {
#ifdef MATRIX_HAS_GHOST
            if (has_ghost_in_row(r)) {
                /* Keep track of whether ghosted status has changed for
                 * debugging. But don't update matrix_prev until un-ghosted, or
                 * the last key would be lost.
                 */
                if (debug_matrix && matrix_ghost[r] != matrix_row) {
                    matrix_print();
                }
                matrix_ghost[r] = matrix_row;
                continue;
            }
            matrix_ghost[r] = matrix_row;
#endif
            if (debug_matrix) matrix_print();
            matrix_row_t col_mask = 1;
            for (uint8_t c = 0; c < MATRIX_COLS; c++, col_mask <<= 1) {
                if (matrix_change & col_mask) {
                    keyevent_t e = (keyevent_t){
                        .key = (keypos_t){ .row = r, .col = c },
                        .pressed = (matrix_row & col_mask),
                        .time = (timer_read() | 1) /* time should not be 0 */
                    };
                    action_exec(e);
                    hook_matrix_change(e);
                    // record a processed key
                    matrix_prev[r] ^= col_mask;

                    // This can miss stroke when scan matrix takes long like Topre
                    // process a key per task call
                    //goto MATRIX_LOOP_END;
                }
            }
        }
    }
    // call with pseudo tick event when no real key event.
    action_exec(TICK);

//MATRIX_LOOP_END:

    hook_keyboard_loop();

#ifdef MOUSEKEY_ENABLE
    // mousekey repeat & acceleration
    mousekey_task();
#endif


#ifdef PS2_MOUSE_ENABLE
    ps2_mouse_task();
#endif

#ifdef SERIAL_MOUSE_ENABLE
        serial_mouse_task();
#endif

#ifdef ADB_MOUSE_ENABLE
        adb_mouse_task();
#endif

#ifdef LED_DATA_ENABLE  /* rough code yet */
    static bool print_data_led = 0;
    static uint16_t data_timer;
    static uint8_t data_led[20];
    if (print_data_led && timer_elapsed(data_timer) > 75) {
        print_data_led = 0;
        parse_led_data(data_led);
    }
    // update LED
    if (led_status != host_keyboard_leds()) {
        led_status = host_keyboard_leds();
        static uint8_t step = 0;
        // When NKRO is enabled, two keyboards appear in windows. So data will be sent twice.
        static uint8_t data_led_in[4];
        if (led_status & 0b00010000) {
            //timer for data_led
            if (timer_elapsed(data_timer) > 75 ) {
                step = 0;
                data_num = 0;
            } 
            data_led_in[step] = (led_status & 0b0000011) << (6 - 2 * step);
            data_timer = timer_read();
            //xprintf("%d:", step);
            //pbin(data_led_in[step]);
            //print("\n");
            if (++step >= 4) {
                data_led[data_num] = data_led_in[0] | data_led_in[1] | data_led_in[2] | data_led_in[3];
                if (data_led[0] == LED_DATA_ADDR && data_num < 2) {
                    tap_action_code(KC_SLCK);
                }
                step = 0;
                data_num++;
                print_data_led = 1;
            }
        } else {
            xprintf("LED: %02X\n", led_status);
            hook_keyboard_leds_change(led_status);
        }
   }
#else 
    // update LED
    if (led_status != host_keyboard_leds()) {
        led_status = host_keyboard_leds();
        if (debug_keyboard) dprintf("LED: %02X\n", led_status);
        hook_keyboard_leds_change(led_status);
    }
#endif

#ifdef DEBUG_SCAN_SPEED
    //test scan speed
    if (1) {
    //if (USB_DeviceState == DEVICE_STATE_Configured) {
        static uint16_t temp_timer;
        static uint16_t temp_i = 0;
        temp_i++;
        if (timer_elapsed(temp_timer) >= 1000) {
            #if 0  //timer test
            tap_action_code(KC_P3);
            #endif
            temp_timer = timer_read();
            xprintf("%d\n",temp_i);
            temp_i = 0;
        }
    }
#endif
}

#ifdef LED_DATA_ENABLE
__attribute__ ((weak))
void parse_led_data(uint8_t *data_led)
{
}
#endif

void keyboard_set_leds(uint8_t leds)
{
    led_set(leds);
}
