/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

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
#include "keycode.h"
#include "host.h"
#include "timer.h"
#include "print.h"
#include "debug.h"
#include "mousekey.h"

#include "action_util.h"



report_mouse_t mouse_report = {};

static uint8_t mouse_repeat_start = 0;
static uint8_t mouse_wheel_timer = 0;

static uint8_t mousekey_accel = 0;

static void mousekey_debug(void);

// rapidfire key
bool rapidfire_key[KC_F12+1] = {0};
bool rapidfire_mode = false;
static uint16_t last_timer_fire = 0;

/*
 * Mouse keys  acceleration algorithm
 *  http://en.wikipedia.org/wiki/Mouse_keys
 *
 *  speed = delta * max_speed * (repeat / time_to_max)**((1000+curve)/1000)
 */
/* milliseconds between the initial key press and first repeated motion event (0-2550) */
//uint8_t mk_delay = MOUSEKEY_DELAY/10;
/* milliseconds between repeated motion events (0-255) */
//uint8_t mk_interval = MOUSEKEY_INTERVAL;
/* steady speed (in action_delta units) applied each event (0-255) */
//uint8_t mk_max_speed = MOUSEKEY_MAX_SPEED;
/* number of events (count) accelerating to steady speed (0-255) */
//uint8_t mk_time_to_max = MOUSEKEY_TIME_TO_MAX;
/* ramp used to reach maximum pointer speed (NOT SUPPORTED) */
//int8_t mk_curve = 0;
/* wheel params */
//uint8_t mk_wheel_max_speed = MOUSEKEY_WHEEL_MAX_SPEED;
//uint8_t mk_wheel_time_to_max = MOUSEKEY_WHEEL_TIME_TO_MAX;


static uint16_t last_timer = 0;


static void rapidfire_key_task(void) {
    static uint8_t rapidfire_time_interval = 30;
    static bool rapidfire_pressed = 1;
    if (rapidfire_mode && timer_elapsed(last_timer_fire) > rapidfire_time_interval ) {
        rapidfire_time_interval = 30 + (rand()&0xf) ; // 30 + (0->15)
        uint8_t rapidfire_mode_check = 0;
        for (uint8_t i=0; i<KC_F12+1; i++) { //00 to Key F12
            if (rapidfire_key[i] == true) {
                rapidfire_mode_check = 1;
                uint16_t rapidfire_key_code = i?i:0x50F4; // 0 for Mouse LBtn
                process_action_code(rapidfire_key_code, rapidfire_pressed);
            }
        }
        rapidfire_pressed ^= 1;
        rapidfire_mode = rapidfire_mode_check;
        last_timer_fire = timer_read();
    }
}
    
void mousekey_task(void)
{
    rapidfire_key_task();

    /*
        Refer to matrix.c, temporarily add it 
        to prevent the mouse from moving too fast when using stm32
    */
    static uint16_t mouse_scan_timestamp = 0;
    uint16_t time_check = timer_read();
    if (mouse_scan_timestamp == time_check) return 1;
    mouse_scan_timestamp = time_check;

    
    if (timer_elapsed(last_timer) < (mouse_repeat_start == 2 ? 5 : 40)) //delay about 40ms to start //(mouse_repeat ? mk_interval : mk_delay*10))
        return;

    //int8_t *p = &mouse_report.x;
    //if ((*p++ | *p++ | *p++ | *p++) == 0) {
    if ((mouse_report.x | mouse_report.y | mouse_report.v | mouse_report.h) == 0) {
    //if (mouse_report.x == 0 && mouse_report.y == 0 && mouse_report.v == 0 && mouse_report.h == 0)
        mouse_repeat_start = 0;
        return;
    } else if (mouse_repeat_start < 2) {
        mouse_repeat_start++;
        return;
    }
    //control wheel speed
    int8_t mouse_v, mouse_h;
    mouse_v = mouse_report.v;
    mouse_h = mouse_report.h;
    if ((++mouse_wheel_timer) & 0b1111) {
        mouse_report.v = 0;
        mouse_report.h = 0;
    }
    mousekey_send();
    mouse_report.v = mouse_v;
    mouse_report.h = mouse_h;

    static uint8_t timer1 = 0;
    if (++timer1 > MOUSEKEY_REPEAT_UPDATE_INTERVAL) { //control mouse speedup
#if defined(BLE_NAME) || defined(BLE_NAME_VARIABLE)
        timer1 = (keyboard_protocol & (1<<7))? 5:0;
#else
        timer1 = 0;
#endif
        int8_t  *p_report = &mouse_report.x;
        for (uint8_t i=0; i<4; i++, *p_report++) {
            if (*p_report == 0) continue;
            if (mousekey_accel) { // Move at a constant speed
                *p_report = (MOUSEKEY_MOVE_REPEAT_MAX / 4) * mousekey_accel * (*p_report > 0? 1: -1); // mousekey_accel 1 2 4
            } else if (*p_report > 0) {
                if (++(*p_report) > MOUSEKEY_MOVE_REPEAT_MAX) {
                    *p_report = MOUSEKEY_MOVE_REPEAT_MAX;
                } 
            } else if (--(*p_report) < -MOUSEKEY_MOVE_REPEAT_MAX) {
                 *p_report = -MOUSEKEY_MOVE_REPEAT_MAX;
            }
        }
    }

}

void mousekey_on(uint8_t code)
{
    if (code < KC_MS_UP) return;
    if      (code == KC_MS_UP)       mouse_report.y = -1; 
    else if (code == KC_MS_DOWN)     mouse_report.y =  1;
    else if (code == KC_MS_LEFT)     mouse_report.x = -1; 
    else if (code == KC_MS_RIGHT)    mouse_report.x =  1;
    else if (code <= KC_MS_BTN5)     mouse_report.buttons |= (1<<(code-KC_MS_BTN1)); //0xF4 - 0xF8
    else if (code == KC_MS_WH_UP)    mouse_report.v =  1; 
    else if (code == KC_MS_WH_DOWN)  mouse_report.v = -1; 
    else if (code == KC_MS_WH_LEFT)  mouse_report.h = -1;
    else if (code == KC_MS_WH_RIGHT) mouse_report.h =  1;
    else if (code <= KC_MS_ACCEL2)   mousekey_accel |= (1<<(code-KC_MS_ACCEL0)); //0xFD - 0xFF
    if (code >= KC_MS_WH_UP && code <= KC_MS_WH_RIGHT) mouse_wheel_timer = 0;
}

void mousekey_off(uint8_t code)
{
    if (code < KC_MS_UP) return;
    if      (code == KC_MS_UP      ) { if (mouse_report.y < 0) mouse_report.y = 0; }
    else if (code == KC_MS_DOWN    ) { if (mouse_report.y > 0) mouse_report.y = 0; }
    else if (code == KC_MS_LEFT    ) { if (mouse_report.x < 0) mouse_report.x = 0; }
    else if (code == KC_MS_RIGHT   ) { if (mouse_report.x > 0) mouse_report.x = 0; }
    else if (code <= KC_MS_BTN5    ) { mouse_report.buttons &= ~(1<<(code-KC_MS_BTN1)); }//0xF4 - 0xF8
    else if (code == KC_MS_WH_UP   ) { if (mouse_report.v > 0) mouse_report.v = 0; }
    else if (code == KC_MS_WH_DOWN ) { if (mouse_report.v < 0) mouse_report.v = 0; }
    else if (code == KC_MS_WH_LEFT ) { if (mouse_report.h < 0) mouse_report.h = 0; }
    else if (code == KC_MS_WH_RIGHT) { if (mouse_report.h > 0) mouse_report.h = 0; }
    else if (code <= KC_MS_ACCEL2  ) { mousekey_accel &= ~(1<<(code-KC_MS_ACCEL0)); } //0xFD - 0xFF

}

void mousekey_send(void)
{
    mousekey_debug();
    host_mouse_send(&mouse_report);
    last_timer = timer_read();
}

void mousekey_clear(void)
{
    mouse_report = (report_mouse_t){};
    mousekey_accel = 0;
}

static void mousekey_debug(void)
{
    if (!debug_mouse) return;
    print("mousekey [btn|x y v h](rep/acl): [");
    phex(mouse_report.buttons); print("|");
    print_decs(mouse_report.x); print(" ");
    print_decs(mouse_report.y); print(" ");
    print_decs(mouse_report.v); print(" ");
    print_decs(mouse_report.h); print("](");
    print_dec(mousekey_accel); print(")\n");
}
