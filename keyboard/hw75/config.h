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

#ifndef CONFIG_H
#define CONFIG_H


/* USB Device descriptor parameter */
#define VENDOR_ID       0xFEED
#define PRODUCT_ID      0x2275
#define DEVICE_VER      0x0001
/* in python2: list(u"whatever".encode('utf-16-le')) */
/*   at most 32 characters or the ugly hack in usb_main.c borks */
#define MANUFACTURER "YDKB"
#define USBSTR_MANUFACTURER    'Y', 0, 'A', 0, 'N', 0, 'G', 0
#define PRODUCT "HW75 Keyboard (USB_DMCR)"
#define USBSTR_PRODUCT         'H', 0, 'W', 0, '7', 0, '5', 0, ' ', 0, 'K', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, '(', 0, 'U', 0, 'S', 0, 'B', 0, '_', 0, 'D', 0, 'M', 0, 'C', 0, 'R', 0,')',0 
#define NOT_AVR

/* key matrix size */
#define MATRIX_ROWS 11
#define MATRIX_COLS 8

/* debounce for both key up and down */
#define DEBOUNCE_DN 5
#define DEBOUNCE_UP 6

/* Set 0 if debouncing isn't needed */
#define LEDMAPU_ENABLE
#define SUSPEND_ACTION
/* number of backlight levels */

#define MOUSEKEY_REPEAT_UPDATE_INTERVAL 10
//#define NOEEP_BACKLIGHT_ENABLE
#define EEPROM_EMU_STM32F103x8
#define TAPPING_TOGGLE  2
#define IMPROVED_TAP

/* Locking resynchronize hack */

/* key combination for command */
#define IS_COMMAND() ( \
    keyboard_report->mods == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT)) \
)



/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION

#endif
