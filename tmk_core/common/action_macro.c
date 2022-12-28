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
#include "action.h"
#include "action_util.h"
#include "action_macro.h"
#include "wait.h"

#ifndef NO_DEBUG
#include "debug.h"
#else
#include "nodebug.h"
#endif


#ifndef NO_ACTION_MACRO

#define MACRO_READ()  (macro = MACRO_GET(macro_p++))
void action_macro_play(const macro_t *macro_p)
{
    macro_t macro = END;
    
#if 0
    uint8_t interval = 0;
    uint8_t mod_storage = 0;
#endif

    if (!macro_p) return;
    while (true) {
        switch (MACRO_READ()) {
            uint16_t macro_code16 = 0;
            case KEY_DOWN:
            case KEY_UP:
            case ACTION_DOWN:
            case ACTION_UP:
                macro_code16 = MACRO_GET16(macro_p++);
                bool pressed = (macro%2);
                if (macro < 3) macro_code16 &= 0xff;
                else macro_p++;
                dprintf("ACTION|KEY(%04X,%X)\n", macro_code16, pressed);
                process_action_code(macro_code16, pressed);
                break;

            case ACTION_TAP:
                macro_code16 = MACRO_GET16(macro_p++);
                macro_p++;
                dprintf("ACTION_T(%04X)\n", macro_code16);
                tap_action_code(macro_code16);
                break;
            case WAIT:
                MACRO_READ();
                dprintf("WAIT(%u)\n", macro*2);
                wait_ms_x2(macro);
                //{ uint8_t ms = macro; while (ms--) wait_ms(1); }
                break;
            /*
             * 16bit unicode character 
             * emoji is 0x1F000: 0x1F000 = 126976, 0xF000 = 61440 = 26976 + 34464
             * https://unicode.org/emoji/charts/full-emoji-list.html
             */ 
            case UNICODE: 
            case EMOJI:
                macro_code16 = MACRO_GET16(macro_p++);
                macro_p++;
                dprintf("UNICODE(0x%04X)\n", macro_code16);
                register_mods(MOD_BIT(KC_LALT));
                if (macro == EMOJI) {
                    type_pnum(1);
                    macro_code16 -= 34464;
                }
                type_numbers(macro_code16,1);
                unregister_mods(MOD_BIT(KC_LALT));
                break;
            case 0x04 ... 0x73:
                dprintf("KEY_T(%02X)\n", macro);
                tap_action_code(macro);
                break;
            case 0x84 ... 0xF3: //LShift + Key
                tap_action_code( MOD_BIT(KC_LSHIFT)<<8 | (macro&0x7F) );
                break;
#if 0
            case INTERVAL:
                interval = MACRO_READ();
                dprintf("INTERVAL(%u)\n", interval);
                break;
            case MOD_STORE:
                mod_storage = get_mods();
                break;
            case MOD_RESTORE:
                set_mods(mod_storage);
                send_keyboard_report();
                break;
            case MOD_CLEAR:
                clear_mods();
                send_keyboard_report();
                break;
            case 0x04 ... 0x73:
                dprintf("DOWN(%02X)\n", macro);
                register_code(macro);
                break;
            case 0x84 ... 0xF3:
                dprintf("UP(%02X)\n", macro);
                unregister_code(macro&0x7F);
                break;
#endif
            case END:
            default:
                return;
        }
        // interval
        //{ uint8_t ms = interval; while (ms--) wait_ms(1); }
    }
}
#endif
