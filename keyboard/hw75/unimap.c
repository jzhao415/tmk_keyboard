/*
Copyright 2022 YANG <drk@live.com>

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
#include "unimap_trans.h"
#include "bootloader.h"
#include "wait.h"
#include "keycode.h"
#include "action.h"
#include "action_util.h"
#include "keymap.h"
#include "command.h"
#include "rgblight.h"
#include "led.h"
#include "suspend.h"

extern uint8_t function_led_state;
enum function_id {
    TRICKY_ESC,
    LALT_F4,
    FN_RESET,
    FN_WIN_LOCK,
    SPACE2,
    UP2,
    RGBLED_ACTION = 0x0A,
};


enum macro_id {
    HOME_PGUP,
    END_PGDN,
};
#define AC_FN0 ACTION_LAYER_MOMENTARY(1)
#define AC_FN1 ACTION_FUNCTION(FN_RESET)
#ifdef KEYMAP_SECTION_ENABLE
const action_t actionmaps[][UNIMAP_ROWS][UNIMAP_COLS] __attribute__ ((section (".keymap.keymaps"))) = {
#else
const action_t actionmaps[][UNIMAP_ROWS][UNIMAP_COLS] PROGMEM = {
#endif
    UNIMAP_DEFAULT,
    UNIMAP_DEFAULT,
    UNIMAP_LED(LM01,TRNS,TRNS),
    UNIMAP_LED(LM23,TRNS,TRNS),
    UNIMAP_LED(LM45,TRNS,TRNS),
    UNIMAP_LED(LM67,TRNS,TRNS),
    UNIMAP_LED(TRNS,TRNS,TRNS),
    UNIMAP_LED(  NO,TRNS,TRNS),
};

keypos_t unimap_translate(keypos_t key)
{
    uint8_t unimap_pos = unimap_trans[key.row][key.col];
    return (keypos_t) {
        .row = ((unimap_pos & 0xf0) >> 4),
        .col = (unimap_pos & 0x0f)
    };
}

action_t action_for_key(uint8_t layer, keypos_t key)
{
    keypos_t uni = unimap_translate(key);
    if ((uni.row << 4 | uni.col) == UNIMAP_NO) {
        return (action_t)ACTION_NO;
    }
    return actionmaps[(layer)][(uni.row & 0x7)][(uni.col)];
}

void action_function(keyrecord_t *record, uint8_t id, uint8_t opt)
{
    static uint8_t mod_keys_registered;
    uint8_t pressed_mods = get_mods();
    switch (id) {
        case TRICKY_ESC:
            // opt 10, LShift; opt 1(not 1000), RShift; opt 100,LAlt F4
            if (record->event.pressed) {
                uint8_t esc_shift_mods = opt&0b10;
                if (opt&1) esc_shift_mods |= MOD_BIT(KC_RSHIFT);
                uint8_t esc_lalt = opt&0b100;
                if ((pressed_mods & esc_shift_mods) && (~pressed_mods & MOD_BIT(KC_LCTRL))) {
                    mod_keys_registered = KC_GRV;
                } else if (pressed_mods & esc_lalt) {
                    mod_keys_registered = KC_F4;
                } else {
                    mod_keys_registered = KC_ESC;
                }
                register_code(mod_keys_registered);
                send_keyboard_report();
            } else {
                unregister_code(mod_keys_registered);
                send_keyboard_report();
            }
            break;
        case LALT_F4:
            if (record->event.pressed) {
                if (pressed_mods & MOD_BIT(KC_LALT)) { 
                    mod_keys_registered = KC_F4;
                } else {
                    mod_keys_registered = KC_4;
                }
                register_code(mod_keys_registered);
                send_keyboard_report();
            } else {
                unregister_code(mod_keys_registered);
                send_keyboard_report();
            }
            break;
        case SPACE2:
            if (record->event.pressed) {
                if (pressed_mods & (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT))) {
                    break;
                } else {
                    register_code(KC_SPACE);
                    send_keyboard_report();
                }
            } else {
                unregister_code(KC_SPACE);
                send_keyboard_report();
            }
            break;
        case UP2:
            if (record->event.pressed) {
                if (pressed_mods & MOD_BIT(KC_RSHIFT)) {
                    break;
                } else {
                    register_code(KC_UP);
                    send_keyboard_report();
                }
            } else {
                unregister_code(KC_UP);
                send_keyboard_report();
            }
            break;
    }
    if (record->event.pressed) {
        switch (id) {
            case FN_RESET:
                if (pressed_mods == MOD_BIT(KC_LCTRL)) {
                    command_extra(KC_B);
                }
                register_mods(0b00110011);
                unregister_mods(0b00110011);
                register_mods(0b11001100);
                unregister_mods(0b11001100);
                clear_keyboard(); 
                break;
            case FN_WIN_LOCK:
                function_led_state ^= (1<<0);
                lock_mods ^= MODS_GUI_MASK;
                keyboard_function_led_set();
                break;
            case RGBLED_ACTION:
                rgblight_action(opt);
                break;
        }
    }
}

const macro_t *action_get_macro(keyrecord_t *record, uint8_t id, uint8_t opt)
{
    static uint16_t key_timer;
    switch(id) {
        case HOME_PGUP: 
            if (record->event.pressed) {
                key_timer = timer_read(); 
            }
            else {
                if (timer_elapsed(key_timer) > 200) { 
                    return MACRO( T(HOME), END );
                }
                else {
                    return MACRO( T(PGUP), END );
                }
            }
            break;
        case END_PGDN: 
            if (record->event.pressed) {
                key_timer = timer_read(); 
            }
            else { 
                if (timer_elapsed(key_timer) > 200) {
                    return MACRO( T(END), END );
                }
                else {
                    return MACRO( T(PGDN), END );
                }
            }
            break;
    }
    return MACRO_NONE;
};

bool command_extra(uint8_t code)
{
    switch (code) {
        case KC_B:
            clear_keyboard();
            wait_ms(1200);
            //soft reset
            suspend_power_down_action();
            *(uint32_t *)(0xE000ED0CUL) = 0x05FA0000UL | (*(uint32_t *)(0xE000ED0CUL) & 0x0700) | 0x04;
            break;
        default:
            return false;   // yield to default command
    }
    return true;
}