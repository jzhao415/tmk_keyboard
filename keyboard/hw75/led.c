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

#include "hal.h"
#include "ch.h"
#include "led.h"
#include "action.h"
#include "rgblight.h"
#include "unimap.h"

bool rgb_indicator_on = 0;
struct cRGB indicator_ws2812_color;

uint8_t indicator_rgb = 0;
uint8_t indicator_state = 0;
uint8_t function_led_state = 0;
extern rgblight_config_t rgblight_config;
extern bool rgb_layer_update;
extern const action_t actionmaps[][UNIMAP_ROWS][UNIMAP_COLS];

enum led_type_id {
    LED_RESTORE = 0,  //1-7: layer1-7
    LED_LAYER = 1,  //1-7: layer1-7
    LED_DEFAULT_LAYER = 9, // 9-15: default layer 1-7
    LED_LOCK = 16, // 16: numlock, 17: capslock, 18:scrlock 
    LED_FUNCTION = 21, //21: winlock
};
void ledmapu_init(void)
{
    indicator_rgb = actionmaps[2][0][0].code & 0xFF;
    indicator_ws2812_color.r = actionmaps[0][0][1].code & 0xff;
    indicator_ws2812_color.g = actionmaps[0][0][1].code >> 8;
    indicator_ws2812_color.b = actionmaps[0][0][2].code & 0xff;
} 
void ledmapu_led_on(void) {
    rgb_indicator_on = 1;
    rgblight_set();
    indicator_state = 1; 
} 

void ledmapu_led_off(void) {
    rgb_indicator_on = 0;
    rgblight_set();
    indicator_state = 0; 
} 

void ledmapu_set(uint8_t type, uint8_t state) {
    uint8_t value = (type == LED_RESTORE)? 0 : (indicator_rgb - type);
    if (value < 8) {
        if (state & (1<<value)) {
            ledmapu_led_on();
        } else {
            ledmapu_led_off();
        }
    }
}

void ledmapu_state_restore(void) {
    ledmapu_set(LED_RESTORE, indicator_state);
}


void led_set(uint8_t usb_led)
{
    ledmapu_set(LED_LOCK, usb_led);
    //protect function led from 21
    keyboard_function_led_set();
}

void hook_layer_change(uint32_t state)
{
    ledmapu_set(LED_LAYER, (uint8_t)state>>1);
    if (rgblight_config.enable && rgblight_config.mode == 4) {
        rgb_layer_update = true;
    }
}

void hook_default_layer_change(uint32_t default_layer_state)
{
    ledmapu_set(LED_DEFAULT_LAYER, (uint8_t)default_layer_state>>1);
}

void keyboard_function_led_set(void)
{
    ledmapu_set(LED_FUNCTION, function_led_state);
}

