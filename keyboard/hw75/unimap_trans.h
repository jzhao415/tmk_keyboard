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
#ifndef UNIMAP_COMMON_H
#define UNIMAP_COMMON_H

#include <stdint.h>
#include "unimap.h"


/* Mapping to Universal keyboard layout
 *
 * Universal keyboard layout
 *         ,-----------------------------------------------.
 * |Esc|   |F1 |F2 |F3 |F4 |F5 |F6 |F7 |F8 |F9 |F10|F11|F12|     |PrS|ScL|Pau|     |VDn|VUp|Mut|
 * `---'   `-----------------------------------------------'     `-----------'     `-----------'
 * ,-----------------------------------------------------------. ,-----------. ,---------------.
 * |  `|  1|  2|  3|  4|  5|  6|  7|  8|  9|  0|  -|  =|JPY|Bsp| |Ins|Hom|PgU| |NmL|  /|  *|  -|
 * |-----------------------------------------------------------| |-----------| |---------------|
 * |Tab  |  Q|  W|  E|  R|  T|  Y|  U|  I|  O|  P|  [|  ]|  \  | |Del|End|PgD| |  7|  8|  9|  +|
 * |-----------------------------------------------------------| `-----------' |---------------|
 * |CapsL |  A|  S|  D|  F|  G|  H|  J|  K|  L|  ;|  '|  #|Retn|               |  4|  5|  6|KP,|
 * |-----------------------------------------------------------|     ,---.     |---------------|
 * |Shft|  <|  Z|  X|  C|  V|  B|  N|  M|  ,|  .|  /| RO|Shift |     |Up |     |  1|  2|  3|KP=|
 * |-----------------------------------------------------------| ,-----------. |---------------|
 * |Ctl|Gui|Alt|MHEN|     Space      |HENK|KANA|Alt|Gui|App|Ctl| |Lef|Dow|Rig| |  0    |  .|Ent|
 * `-----------------------------------------------------------' `-----------' `---------------'
 */
const uint8_t PROGMEM unimap_trans[MATRIX_ROWS][MATRIX_COLS] = {
    { UM_PGDN,UM_PGUP,UM_HOME, UM_DEL, UM_END, UM_RGHT,UM_DOWN,UM_RCTL, }, // 0  to 7
    { UM_LALT,UM_RALT,UM_RGUI, UM_LEFT,UM_LGUI,UM_LCTL,UM_LSFT,UM_CAPS, }, // 8  to 15
    { UM_B,   UM_UP,  UM_BSPC, UM_TAB, UM_N,   UM_M,   UM_COMM,UM_DOT,  }, // 16 to 23
    { UM_ENT, UM_RSFT,UM_EQL,  UM_SLSH,UM_BSLS,UM_H,   UM_J,   UM_MINS, }, // 24 to 31
    { UM_QUOT,UM_SCLN,UM_L,    UM_K,   UM_Y,   UM_U,   UM_I,   UM_O,    }, // 32 to 39
    { UM_0,   UM_RBRC,UM_LBRC, UM_P,   UM_9,   UM_8,   UM_F12, UM_F11,  }, // 40 to 47
    { UM_F10, UM_F9,  UM_F8,   UM_F7,  UM_F5,  UM_6,   UM_7,   UM_F6,   }, // 48 to 55
    { UM_T,   UM_3,   UM_F3,   UM_F4,  UM_F2,  UM_F1,  UM_4,   UM_5, }, // 56 to 63
    { UM_2,   UM_R,   UM_E,    UM_ESC, UM_F,   UM_G,   UM_Q,   UM_W, }, // 64 to 71
    { UM_D,   UM_S,   UM_A,    UM_SPC, UM_Z,   UM_X,   UM_C,   UM_V, }, // 72 to 79
    //{ UM_GRV, UM_Q,   UM_P3,   UM_P6,  UM_P2,  UM_P5,  UM_P1,  UM_P4, }  // 80 to 87
    { UM_GRV, UM_1,   UM_NO,   UM_NO,  UM_NO,  UM_NO,  UM_F13,  UM_F14, }  // 80 to 87
};

#endif
