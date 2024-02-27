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

/* rough code */

#include "hal.h"
#include "eeprom.h"
#include "eeprom_stm32.h"
#include "progmem.h"
#include "timer.h"
#include "rgblight.h"
#include "debug.h"
#include "keymap.h"
#include "backlight.h"
#include "hook.h"
#include "suspend.h"

/* rgblight_timer for ws2812 power console. here use a variable instead. */
#define rgblight_timer_init() do { rgblight_timer = 1;} while(0)
#define rgblight_timer_enable() do { rgblight_timer = 1;} while(0)
#define rgblight_timer_disable() do { rgblight_timer = 0;} while(0)
#define rgblight_timer_enabled (rgblight_timer)

/* RGB NUM  */
#define RGBLED_NUM 101
#define RGBLED_TEMP  RGBLED_NUM
#define NC RGBLED_NUM
#define SRGBLED_NUM 85 // Switch 82 + Indicator 3
#define BRGBLED_NUM 16

/* Appearance Key Matrix */
#define MATRIX_AROWS 6
#define MATRIX_ACOLS 15

const uint8_t RGBLED_BREATHING_TABLE[128] = {0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124,127,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255};

/* Circuit key matrix to Appearance Matrix */
static const uint8_t KEY_MATRIX_TRANS[11][8] = 
{
    { 0x3E, 0x2E, 0x1E, 0x0E, 0x4E, 0x5E, 0x5D, 0x5B, },
    { 0x52, 0x59, 0x5A, 0x5C, 0x51, 0x50, 0x40, 0x30, },
    { 0x45, 0x4D, 0x1D, 0x20, 0x46, 0x47, 0x48, 0x49, }, 
    { 0x3D, 0x4C, 0x1C, 0x4A, 0x2D, 0x36, 0x37, 0x1B, },
    { 0x3B, 0x3A, 0x39, 0x38, 0x26, 0x27, 0x28, 0x29, },
    { 0x1A, 0x2C, 0x2B, 0x2A, 0x19, 0x18, 0x0D, 0x0C, },
    { 0x0B, 0x0A, 0x09, 0x08, 0x06, 0x16, 0x17, 0x07, },
    { 0x25, 0x13, 0x04, 0x05, 0x03, 0x02, 0x14, 0x15, },
    { 0x12, 0x24, 0x23, 0x00, 0x34, 0x35, 0x21, 0x22, }, 
    { 0x33, 0x32, 0x31, 0x55, 0x41, 0x42, 0x43, 0x44, },
    { 0x10, 0x11, NC, NC, NC, NC, NC, NC, },
};

/* led addr in appearance matrix */
static const uint8_t SLED_MATRIX_MASK[MATRIX_AROWS][MATRIX_ACOLS] = 
{
    {13,NC,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {14,15,16,17,18,19,20,21,22,23,24,25,26,27,28},
    {43,42,41,40,39,38,37,36,35,34,33,32,31,30,29},
    {44,45,46,47,48,49,50,51,52,53,54,55,NC,56,57},
    {71,70,69,68,67,66,65,64,63,62,61,NC,60,59,58},
    {72,73,74,NC,NC,75,NC,NC,NC,76,77,78,79,80,81},
};

/* indicator ws2812 from bottom to top: 82 83 84 */
static const uint8_t INDICAOTR_RGB[3] = { 84, 83, 82 }; 

/* Bottom WS2812 addr (from left to right) */
static const uint8_t BRGB_COL_MASK[16] = { 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85 }; 

static uint8_t snake_control[7][2] = {{0,0},{0,1},{0,2},{0,3},{0,4},{0,5},{0,6}};

static uint8_t fading_single_key[SRGBLED_NUM+1] = {0};
static uint8_t hue_single_key[SRGBLED_NUM+1] = {0};
static uint8_t fading_single_underglow[BRGBLED_NUM] = {0};
static uint8_t hue_single_underglow[BRGBLED_NUM] = {0};
uint8_t sw_rgb_on = 1;

uint8_t rgblight_timer = 0;
rgblight_config_t rgblight_config;
struct cRGB led[RGBLED_NUM+1] = {0};
extern bool rgb_indicator_on;
extern struct cRGB indicator_ws2812_color;
rgblight_config_t rgblight_config = {.enable = 1, .mode = 12,.hue = 640, .sat = 255, .val = 255};


static int8_t wave_col_l[6] = {0};
static int8_t wave_col_r[6] = {0};
static uint8_t wave_num = 0;
static uint8_t horse_race_remain = 0;
//rgblight_layer
bool rgb_layer_update = false;

extern uint32_t layer_state; 

void sethsv(uint16_t hue, uint8_t saturation, uint8_t brightness, struct cRGB *led1)
{
    /*
    original code: https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
    */
    uint8_t r, g, b;
    uint8_t temp[5];
    uint8_t n = (hue >> 8) % 3;
    uint8_t x = ((((hue & 255) * saturation) >> 8) * brightness) >> 8;
    uint8_t s = ((256 - saturation) * brightness) >> 8;
    temp[0] = temp[3] = s;
    temp[1] = temp[4] = x + s;
    temp[2] = brightness - x;
    r = temp[n + 2];
    g = temp[n + 1];
    b = temp[n];
    setrgb(r,g,b, led1);
}

void sethsv_s255(uint8_t hue, uint8_t brightness, struct cRGB *led1)
{
    /* reduce calculation of some effects */
    uint8_t temp[5];
    uint8_t n = (hue*3) >> 8;
    temp[0] = temp[3] = 0;
    temp[1] = temp[4] = ((uint8_t)(hue * 3 -1) * brightness) >> 8;
    temp[2] = brightness - temp[1];
    (*led1).r = temp[n + 2];
    (*led1).g = temp[n + 1];
    (*led1).b = temp[n];
}

void setrgb(uint8_t r, uint8_t g, uint8_t b, struct cRGB *led1)
{
    (*led1).r = r;
    (*led1).g = g;
    (*led1).b = b;
}


uint32_t eeconfig_read_rgblight(void)
{
    return eeprom_read_dword(EECONFIG_RGBLIGHT);
}

void eeconfig_write_rgblight(uint32_t val)
{
    eeprom_update_dword(EECONFIG_RGBLIGHT, val);
}

void eeconfig_write_rgblight_default(void)
{
    dprintf("eeconfig_write_rgblight_default\n");
    rgblight_config.enable = 1;
    rgblight_config.mode = 13;
    rgblight_config.hue = 640;
    rgblight_config.sat = 255;
    rgblight_config.val = 255;
    eeconfig_write_rgblight(rgblight_config.raw);
}

void eeconfig_debug_rgblight(void) {
    dprintf("rgblight_config eeprom\n");
    dprintf("enable = %d\n", rgblight_config.enable);
    dprintf("mode = %d\n", rgblight_config.mode);
    dprintf("hue = %d\n", rgblight_config.hue);
    dprintf("sat = %d\n", rgblight_config.sat);
    dprintf("val = %d\n", rgblight_config.val);
}

void rgblight_init(void)
{
    if (!eeconfig_is_enabled()) {
        dprintf("rgblight_init eeconfig is not enabled.\n");
        eeconfig_init();
        eeconfig_write_rgblight_default();
    }
    sw_rgb_on = eeprom_read_byte(9)? 1 : 0; // address 9
    rgblight_config.raw = eeconfig_read_rgblight();
    if (rgblight_config.val == 0) rgblight_config.val = 127;


    rgblight_timer_init(); // setup the timer

    if (rgblight_config.enable) {
        rgblight_mode(rgblight_config.mode);
    } else {
        rgblight_set();
        rgblight_timer_disable();
    }
}


void rgblight_mode(uint8_t mode)
{
    if (!rgblight_config.enable) {
        return;
    }
    if (mode < 1) mode = RGBLIGHT_MODES;
    else if (mode > RGBLIGHT_MODES) mode = 1;
    rgblight_config.mode = mode;

    eeconfig_write_rgblight(rgblight_config.raw);
    dprintf("rgblight mode: %u\n", rgblight_config.mode);
    if (rgblight_config.mode > 0) {
        rgblight_timer_enable();
    }

    if (mode == 1) {
        rgblight_sethsv(rgblight_config.hue, rgblight_config.sat, rgblight_config.val);
        rgblight_set();
    } else if (mode == 2) {
        memset(&fading_single_key[0], 0, SRGBLED_NUM);
        rgblight_clear();
    } else if (mode == 4) {
        hook_layer_change(layer_state);
    } else if (mode >= 9 && mode <= 10) {
        horse_race_remain = 0;
    }
}

inline
void rgblight_toggle(void)
{
    rgblight_config.enable ^= 1;
    eeconfig_write_rgblight(rgblight_config.raw);
    dprintf("rgblight toggle: rgblight_config.enable = %u\n", rgblight_config.enable);
    if (rgblight_config.enable) {
        rgblight_mode(rgblight_config.mode);
    } else {
        rgblight_clear();
        rgblight_set();
    }
}


void rgblight_action(uint8_t action)
{
    /*  0 toggle
    1 mode-    2 mode+
    3 hue-     4 hue+
    5 sat-     6 sat+
    7 val-     8 val+
    */
    uint16_t hue = rgblight_config.hue;
    int16_t sat = rgblight_config.sat;
    int16_t val = rgblight_config.val;
    int8_t increament = 1;
    if (action & 1) increament = -1;
    if (get_mods() & MOD_BIT(KC_LSHIFT)) {
        increament *= -1;
    } 
    switch (action) {
        case 0: 
            rgblight_toggle();
            break;
        case 1:
        case 2:
            rgblight_mode(rgblight_config.mode + increament);
            break;
        case 3:
        case 4:
            hue = (rgblight_config.hue + 768 + RGBLIGHT_HUE_STEP * increament) % 768;
            break;
        case 5:
        case 6:
            sat = rgblight_config.sat + RGBLIGHT_SAT_STEP * increament;
            if (sat > 255) sat = 255;
            if (sat < 0) sat = 0;
            break;
        case 7:
        case 8:
            val = rgblight_config.val + RGBLIGHT_VAL_STEP * increament;
            if (val > 255) val = 255;
            if (val < 0) val = 0;
            break;
        case 0xD:
            rgblight_clear();
            rgblight_set();
            sw_rgb_on ^= 1;
            eeprom_update_byte(9, sw_rgb_on); //save to eeprom
            break;
        default:
            break;
    }
    if (action >= 3) rgblight_sethsv(hue, sat, val);
}

void rgblight_sethsv_noeeprom(uint16_t hue, uint8_t sat, uint8_t val)
{
    if (rgblight_config.enable) {
        sethsv(hue, sat, val, &led[RGBLED_TEMP]);
        for (uint8_t i=0; i< RGBLED_NUM; i++) {
            led[i] = led[RGBLED_TEMP];
        }
        rgblight_set();
    }
}

void rgblight_sethsv(uint16_t hue, uint8_t sat, uint8_t val)
{
    if (rgblight_config.enable) {
        if (rgblight_config.mode == 1) {
            // same static color
            rgblight_sethsv_noeeprom(hue, sat, val);
        } 
        rgblight_config.hue = hue;
        rgblight_config.sat = sat;
        rgblight_config.val = val;
        eeconfig_write_rgblight(rgblight_config.raw);
        dprintf("rgblight set hsv [EEPROM]: %u,%u,%u\n", rgblight_config.hue, rgblight_config.sat, rgblight_config.val);
    }
}

void rgblight_setrgb(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint8_t i=0;i<RGBLED_NUM;i++) {
        led[i].r = r;
        led[i].g = g;
        led[i].b = b;
    }
    rgblight_set();
}

void rgblight_clear(void)
{
    memset(&led[0], 0, 3 * RGBLED_NUM);
}

void rgblight_set(void)
{
    if (!rgblight_config.enable) {
        rgblight_clear();
    } 
    
    if (rgb_indicator_on) {
        led[82] = indicator_ws2812_color;
        led[83] = indicator_ws2812_color;
        led[84] = indicator_ws2812_color;
    }
    #if 0 //reduce the brightness of ws2812
    uint8_t *p = &led[0];

    for (uint16_t i=0;i<(SRGBLED_NUM-3)*3;i++, *p++) {
        *p = sw_rgb_on? (*p)/2 : 0;
    }
    #endif

    ws2812_setleds(led, RGBLED_NUM);
}

inline
void rgblight_task(void)
{
    if (rgblight_config.enable && rgblight_timer_enabled) {
        // Mode = 1, static light, do nothing here
        switch (rgblight_config.mode) {
            case 1:
                rgblight_sethsv_noeeprom(rgblight_config.hue, rgblight_config.sat, rgblight_config.val);
                break;
            case 2:
                rgblight_effect_click_fading();
                break;
            case 3:
                rgblight_effect_click_wave();
                break;
            case 4:
                rgblight_effect_layer();
                break;
            case 5 ... 6:
                rgblight_effect_snake_auto(rgblight_config.mode-4); // 1 fast, 2 normal
                break;
            case 7:
                rgblight_effect_stars(rgblight_config.mode-6); // 1 fast, 2 normal
                break;
            case 8:
                rgblight_effect_raindrop(rgblight_config.mode-7); // 0 fast, 1 normal
                break;
            case 9:
                rgblight_effect_horse_race(); 
                break;
            case 10:
                rgblight_effect_hit(); 
                break;
            case 11 ... 12:
                rgblight_effect_breathing(rgblight_config.mode-9);  // 2 slow, 3 fast
                break;
            case 13 ... 14:
                rgblight_effect_rainbow_mood(rgblight_config.mode-13); // 2 slow, 3 fast
                break;
            case 15 ... 18:
                rgblight_effect_rainbow_swirl(rgblight_config.mode-13); // 2 fast,
                break;
            case 19 ... 22:
                rgblight_effect_snake(rgblight_config.mode-19); //0 fast
                break;
            case 23 ... 24:
                rgblight_effect_knight(rgblight_config.mode-23);
                break;
        }
    }
}

//for Matrix led
//__attribute__((always_inline))
inline
void rgblight_matrix(uint8_t row, uint8_t col)
{
    if (!rgblight_config.enable) return;
    if (rgblight_config.mode == 2) {
        if (!rgblight_timer_enabled) rgblight_timer_enable();
        fading_single_key[SLED_MATRIX_MASK[row][col]]  = 126;
        uint8_t hue = rand();
        hue_single_key[SLED_MATRIX_MASK[row][col]] = hue;
        // Indicator
        if (row >= 2 && row <= 4) {
            fading_single_key[SRGBLED_NUM + 1 - row] = 126;
            hue_single_key[SRGBLED_NUM + 1 - row] = hue;
        }
        // 15 cols and 16 BRGB.  Light two every click.
        uint8_t underglow_led = BRGB_COL_MASK[col] - SRGBLED_NUM;
        fading_single_underglow[underglow_led] = fading_single_underglow[underglow_led - 1] = 126;
        hue_single_underglow[underglow_led] = hue_single_underglow[underglow_led - 1] = hue;
    }
    if (rgblight_config.mode == 3) {
        if (!rgblight_timer_enabled) rgblight_timer_enable();
        if (wave_col_l[(wave_num + 1)%RGBLIGHT_EFFECT_MAX_WAVES] < -2 && wave_col_r[(wave_num + 1)%RGBLIGHT_EFFECT_MAX_WAVES] >= MATRIX_COLS+2) {
            if (++wave_num >= RGBLIGHT_EFFECT_MAX_WAVES) wave_num = 0;
        }
        wave_col_l[wave_num] = wave_col_r[wave_num] = col;
        uint8_t hue = rand();
        hue_single_key[wave_num] = hue;
    }
}

void rgblight_effect_breathing(uint8_t interval)
{
    static uint8_t pos = 0;
    static int8_t increament = 1;
    rgblight_sethsv_noeeprom(rgblight_config.hue, rgblight_config.sat, RGBLED_BREATHING_TABLE[pos]);
    pos = pos + interval*increament;
    if (pos < interval || pos+interval > 126) {
        increament *= -1;
    }
}

void rgblight_effect_click_fading(void)
{
    uint8_t i;
    bool key_fading = 0;
    for (i = 0; i < SRGBLED_NUM; i++) {
        if (fading_single_key[i] > 1) {
            sethsv_s255(hue_single_key[i], RGBLED_BREATHING_TABLE[fading_single_key[i]], &led[i]);
            fading_single_key[i] -= 2;
            key_fading = 1;
        }
    }
    /* BRGBLED */
    for (i = 0; i < BRGBLED_NUM; i++) {
        if (fading_single_underglow[i] > 1) {
            sethsv_s255(hue_single_underglow[i], RGBLED_BREATHING_TABLE[fading_single_underglow[i]], &led[i+SRGBLED_NUM]);
            fading_single_underglow[i] -= 2;
            key_fading = 1;
        }
    }
    rgblight_set();
}

void rgblight_effect_click_wave(void)
{
    static uint8_t step = 0;
    uint8_t i;
    if (++step > 0) {
        step = 0;
        rgblight_clear();
        for (uint8_t j=0; j<RGBLIGHT_EFFECT_MAX_WAVES; j++) {
            sethsv_s255(hue_single_key[j], 255, &led[RGBLED_TEMP]);
            if (wave_col_l[j] >= -2) {
                    if (wave_col_l[j] >= 0) {
                    for (i=0; i<MATRIX_AROWS; i++) {
                        led[SLED_MATRIX_MASK[i][wave_col_l[j]]] = led[RGBLED_TEMP];
                    } 
                    // BRGB
                    led[ BRGB_COL_MASK[wave_col_l[j]] ] = led[ BRGB_COL_MASK[wave_col_l[j]] - 1 ] = led[RGBLED_TEMP];
                }
                wave_col_l[j]--;
            } 
            if (wave_col_r[j] < (MATRIX_ACOLS + 2)) {
                if (wave_col_r[j] < MATRIX_ACOLS) {
                    for (i=0; i<MATRIX_AROWS; i++) {
                        led[SLED_MATRIX_MASK[i][wave_col_r[j]]] = led[RGBLED_TEMP];
                    }
                    // BRGB
                    led[ BRGB_COL_MASK[wave_col_r[j]] ] = led[ BRGB_COL_MASK[wave_col_r[j]] - 1 ] = led[RGBLED_TEMP];
                } else if (wave_col_r[j] == MATRIX_ACOLS) {
                    // Indicator
                    led[SRGBLED_NUM - 3] = led[SRGBLED_NUM - 2] = led[SRGBLED_NUM - 1] = led[RGBLED_TEMP];
                }
                wave_col_r[j]++;
            } 
        }
        rgblight_set();
    }
}

void rgblight_effect_snake_auto(uint8_t interval)
{
    static uint8_t step = 0;
    uint8_t i;
    int8_t increament;
    if (++step < interval*16) return;
    step = 0;
    rgblight_clear();
    /* get goal */
    if (snake_control[5][0] == snake_control[6][0] && snake_control[5][1] == snake_control[6][1]) {
        hue_single_key[0] = hue_single_key[1];
        snake_control[6][0] = rand() % MATRIX_AROWS;
        wave_col_l[0] = snake_control[6][1] = rand() % MATRIX_ACOLS;
        if (SLED_MATRIX_MASK[ snake_control[6][0] ][ snake_control[6][1] ] == SRGBLED_NUM) snake_control[6][0] = 1;
        uint8_t hue = rand();
        hue_single_key[1] = hue;
    }

    /* set snake color */
    sethsv_s255(hue_single_key[0], 255, &led[RGBLED_TEMP]);
    for (i=0; i<6; i++) {
        led[ SLED_MATRIX_MASK[ snake_control[i][0] ][ snake_control[i][1] ] ] = led[RGBLED_TEMP];
    }
    
    /* set target color */
    sethsv_s255(hue_single_key[1], 255, &led[RGBLED_TEMP]); 
    led[ SLED_MATRIX_MASK[ snake_control[6][0] ][ snake_control[6][1] ] ] = led[RGBLED_TEMP];
    // indicator and BRGB
    for (uint8_t i = 0; i < BRGBLED_NUM + 3; i++) {
        led[SRGBLED_NUM - 3 + i] = led[RGBLED_TEMP];
    }

    rgblight_set();

    /* move forward */
    for (i=0; i<5; i++) {
        snake_control[i][0] = snake_control[i+1][0];
        snake_control[i][1] = snake_control[i+1][1];
    }
    if (snake_control[5][0] != snake_control[6][0]) {
        increament = (snake_control[5][0] > snake_control[6][0])? -1 : 1;
        for (i=0; i<3; i++) {
            if (snake_control[5][0] + increament == snake_control[i][0] && snake_control[5][1] == snake_control[i][1]) {
                if (snake_control[5][1] + 1 < (MATRIX_ACOLS-1)) snake_control[5][1]++;
                else snake_control[5][1]--;
                return;
            }
        } 
        snake_control[5][0] += increament;
        return;
    } 
    if (snake_control[5][1] != snake_control[6][1]) {
        increament = (snake_control[5][1] > snake_control[6][1])? -1 : 1;
        for (i=0; i<3; i++) {
            if (snake_control[5][0] == snake_control[i][0] && snake_control[5][1] + increament == snake_control[i][1]) {
                if (snake_control[5][0] + 1 < (MATRIX_AROWS-1)) snake_control[5][0]++;
                else snake_control[5][0]--;
                return;
            }
        } 
        snake_control[5][1] += increament;
        return;
    }
}

void rgblight_effect_layer(void)
{
    rgblight_clear();
    if (rgb_layer_update) {
        rgb_layer_update = false;
        uint8_t top_layer = biton(layer_state);
        xprintf("top layer: %d\n",top_layer);
        if (top_layer > 0) {
            if (top_layer == 7) setrgb(182, 182, 182, &led[RGBLED_TEMP]);  // white for layer 7
            else sethsv((top_layer-1)*128, 255, 255, &led[RGBLED_TEMP]); 
            keypos_t key_check;
            for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
                key_check.row = i;
                for (uint8_t j = 0; j < MATRIX_COLS; j++) {
                    key_check.col = j;
                    uint16_t key_check_code = action_for_key(top_layer, key_check).code;
                    if (key_check_code >= 2) {  //key is not NO or TRANS
                        uint8_t matrix_a_pos = KEY_MATRIX_TRANS[i][j];
                        led[SLED_MATRIX_MASK[matrix_a_pos>>4][matrix_a_pos&0xf]] = led[RGBLED_TEMP];
                    }
                }
            }
            // indicator and BRGB
            for (uint8_t i = 0; i < BRGBLED_NUM + 3; i++) {
                led[SRGBLED_NUM - 3 + i] = led[RGBLED_TEMP];
            }
        }
        rgblight_set(); 
    }
}

void rgblight_effect_stars(uint8_t interval)
{
    static uint8_t timer_step = 0;
    if (++timer_step > interval * 2) { //1 fast, 2 normal
        timer_step = 0;
        uint8_t random_sled_row, random_sled_col;
        random_sled_row = rand() % MATRIX_AROWS;
        random_sled_col = rand() % MATRIX_ACOLS;
        uint8_t pressed_sled = SLED_MATRIX_MASK[random_sled_row][random_sled_col];
        if (fading_single_key[pressed_sled] <= 1) {
            fading_single_key[pressed_sled] = 126;
            uint8_t hue = rand();
            hue_single_key[pressed_sled] = hue;
            //indicator
            if (random_sled_row >= 2 && random_sled_row <= 4) {
                fading_single_key[SRGBLED_NUM + 1 - random_sled_row] = 126;
                hue_single_key[SRGBLED_NUM + 1 - random_sled_row] = hue;
            }
            //BRGB
            uint8_t underglow_led = BRGB_COL_MASK[random_sled_col] - SRGBLED_NUM;
            fading_single_underglow[underglow_led] = fading_single_underglow[underglow_led - 1] = 126;
            hue_single_underglow[underglow_led] = hue_single_underglow[underglow_led - 1] = hue;
        } 
    }
    rgblight_effect_click_fading();
}

void rgblight_effect_raindrop(uint8_t interval)
{
    static uint8_t step = 0;
    uint8_t i,j;
    if (++step > interval*2) {  //0 fast , 1 normal
        rgblight_clear();
        uint8_t random_sled_col;
        random_sled_col = rand() % MATRIX_ACOLS;
        if (fading_single_key[random_sled_col] == 0) {
            fading_single_key[random_sled_col] = MATRIX_AROWS + 2; // matrix_row + underglow_row(1) + raindrop_length(2) - 1
            hue_single_key[random_sled_col] = rand(); // reset color of this col.
            step = 0;
        }
        for (j=0; j<MATRIX_ACOLS; j++) {
            if (fading_single_key[j]) {
                sethsv_s255(hue_single_key[j], 255, &led[RGBLED_TEMP]);
                for (i = MATRIX_AROWS + 1; i < MATRIX_AROWS+3; i++) {  //start i= matrix_row + underglow_row(1), end + raindrop_length(2)
                    uint8_t tmp_row = i-fading_single_key[j];
                    if (tmp_row >= 0 && tmp_row < MATRIX_AROWS) {
                        led[SLED_MATRIX_MASK[tmp_row][j]] = led[RGBLED_TEMP];
                    } else if (tmp_row == MATRIX_AROWS) {
                        led[ BRGB_COL_MASK[j] ] = led[RGBLED_TEMP];
                        //indicator and col
                        if (j == 14) {
                            for (uint8_t i = 0; i < 4; i++) {
                                led[SRGBLED_NUM - 3 + i] = led[ BRGB_COL_MASK[14] ];
                            }
                        }
                    }
                }
                fading_single_key[j]--;
            } 
        }
        rgblight_set();
    }
} 

void rgblight_effect_horse_race(void)
{
    static uint8_t step = 0;
    static uint8_t start_wait = 0;
    uint8_t i,j;
    if (++step > 0) { 
        memset(&led[0], 0, 3 * (SRGBLED_NUM - 3));  //do not clear led[RGBLED_NUM] every time
        if (horse_race_remain == 0) {
            horse_race_remain = MATRIX_AROWS;
            start_wait = 20;  // wait time before start
            for (i=0; i<MATRIX_AROWS; i++) {
                fading_single_key[i] = MATRIX_ACOLS + RGBLIGHT_EFFECT_HORSE_LENGTH - 1;
                hue_single_key[i] = rand(); // horse color
                memset(&led[0], 0, 3 * RGBLED_NUM);
            }
        }
        if (start_wait) { 
            start_wait--;
            return;
        }
        for (j=0; j<MATRIX_AROWS; j++) {
            if (fading_single_key[j] >= MATRIX_ACOLS + RGBLIGHT_EFFECT_HORSE_LENGTH) {
                horse_race_remain = 0;
                return;
            } 
            if (fading_single_key[j]) {
                sethsv_s255(hue_single_key[j], 255, &led[RGBLED_TEMP]);
                for (i=MATRIX_ACOLS; i<MATRIX_ACOLS+RGBLIGHT_EFFECT_HORSE_LENGTH; i++) {
                    uint8_t tmp_col = i-fading_single_key[j];
                    if (tmp_col >= 0 && tmp_col < MATRIX_ACOLS) {
                        led[SLED_MATRIX_MASK[j][tmp_col]] = led[RGBLED_TEMP];
                        // BRGB: same as row0
                        if (j == 0) {
                            led[ BRGB_COL_MASK[tmp_col] ] = led[ BRGB_COL_MASK[tmp_col] - 1 ] = led[RGBLED_TEMP];
                        }
                    } 
                }
                uint8_t random_delay1, random_delay2;
                random_delay1 = rand() % MATRIX_ROWS;
                random_delay2 = rand() % MATRIX_ROWS;
                if (j != random_delay1 && j != random_delay2) {
                    if (--fading_single_key[j] == 0) { //hores j -> goal.
                        if (--horse_race_remain) {
                            if (horse_race_remain > 2) {
                                led[ SRGBLED_NUM - 6 + horse_race_remain ] = led[RGBLED_NUM];
                            }
                        }
                    }
                }
            } 
        }
        rgblight_set();
    }
}

void rgblight_effect_hit(void)
{
    static uint8_t step = 0;
    uint8_t i,j;
    uint16_t hue;
    if (++step > 0) { 
        if (horse_race_remain == 0) {  // use horse_race_remain for this effect as rgblight_restart
            horse_race_remain = MATRIX_AROWS;
            for (i=0; i<7; i++) {
                wave_col_l[i] = 0;
                wave_col_r[i] = MATRIX_COLS-1;
            }
        }
        uint8_t random_l, random_l2, random_r, random_r2;
        random_l = rand() % (MATRIX_AROWS + 1);
        random_r = rand() % (MATRIX_AROWS + 1);
        random_l2 = rand() % (MATRIX_AROWS + 1);
        random_r2 = rand() % (MATRIX_AROWS + 1);
        for (j=0; j<MATRIX_AROWS+1; j++) {
            if (wave_col_l[j] > wave_col_r[j]) {
                if (j < MATRIX_AROWS){
                    for (i=1; i<(MATRIX_ACOLS-1); i++) {
                        memset(&led[SLED_MATRIX_MASK[j][i]], 0, 3); // clear a row
                    }
                } else {
                    memset(&led[1], 0, 3 * (RGBLED_NUM - 2));
                }
                wave_col_l[j] = 0;
                wave_col_r[j] = (MATRIX_ACOLS-1);
                continue;
            }
            hue = (rgblight_config.hue+j*128)%768;
            if ((j != random_l && j != random_l2) && wave_col_l[j] >= 0) {
                if (j < MATRIX_AROWS) sethsv(hue, rgblight_config.sat, 255, &led[SLED_MATRIX_MASK[j][wave_col_l[j]]]);
                else sethsv(hue, rgblight_config.sat, 255, &led[ BRGB_COL_MASK[wave_col_l[j]] ]); //BRGB
                wave_col_l[j]++;
            }
            hue = (hue+300)%768;
            if ((j != random_r && j != random_r2) && wave_col_r[j] > 0) {
                if (j < MATRIX_AROWS) sethsv(hue, rgblight_config.sat, 255, &led[SLED_MATRIX_MASK[j][wave_col_r[j]]]);
                else {
                    sethsv(hue, rgblight_config.sat, 255, &led[ BRGB_COL_MASK[wave_col_r[j]] ]); //BRGB
                    if (wave_col_r[j] == MATRIX_ACOLS-1) led[BRGB_COL_MASK[15]] = led[BRGB_COL_MASK[14]];
                }
                wave_col_r[j]--;
            }
            if (wave_col_l[j] > (MATRIX_ACOLS-1) || wave_col_r[j] == 0) {
                horse_race_remain = 0;
                rgblight_clear();
            }
        }
        rgblight_set();
    }
}

void rgblight_effect_rainbow_mood(uint8_t interval)
{
    static uint16_t current_hue = 0;
    rgblight_sethsv_noeeprom(current_hue, rgblight_config.sat, rgblight_config.val);
    current_hue = (current_hue + interval * 3) % 768;
}

void rgblight_effect_rainbow_swirl(uint8_t interval)
{
    static uint16_t current_hue=0;
    uint16_t hue;
    uint8_t i,j;
    uint8_t interval2 = interval/2;
    for (i=0; i<MATRIX_ACOLS; i++) {
        hue = (768/16*i+current_hue)%768;
        sethsv(hue, rgblight_config.sat, rgblight_config.val, &led[RGBLED_TEMP]);
        for (j=0; j < MATRIX_AROWS; j++) {
            led[SLED_MATRIX_MASK[j][i]] = led[RGBLED_TEMP];
        }
        //BRGB
        led[BRGB_COL_MASK[i]] = led[RGBLED_TEMP];
        if (i == MATRIX_ACOLS-1) led[BRGB_COL_MASK[15]] = led[RGBLED_TEMP];
    }
    //indicator = last key in matrix.
    led[SRGBLED_NUM - 3] = led[SRGBLED_NUM - 2] = led[SRGBLED_NUM - 1] = led[RGBLED_TEMP];
    
    rgblight_set();
    current_hue = (current_hue + 768 + interval2*16*(interval%2? 1: -1)) % 768;
}

void rgblight_effect_snake(uint8_t interval)
{
    static int8_t pos = 0 - RGBLIGHT_EFFECT_SNAKE_LENGTH;
    static uint8_t led_step = 0;
    uint8_t i;
    int8_t increament = 1;
    uint8_t interval2 = interval/2; // speed
    if (++led_step > interval2) {
        led_step = 0;
        rgblight_clear();
        if (interval%2) increament = -1;
        for (i=0; i<RGBLIGHT_EFFECT_SNAKE_LENGTH; i++) {
            sethsv((rgblight_config.hue+i*50)%768, rgblight_config.sat, rgblight_config.val, &led[(pos+i*increament+RGBLED_NUM)%RGBLED_NUM]);
        }
        pos += increament;
        if (pos > RGBLED_NUM) pos = 0;
        else if (pos < 0 ) pos = RGBLED_NUM;
        rgblight_set();
    }
}

void rgblight_effect_knight(uint8_t interval)
{
    static int8_t pos = MATRIX_ACOLS - 1;
    static uint8_t sled_step = 0;
    static uint16_t current_hue=0;
    uint8_t i,j;
    static int8_t increament = 1;
    if (++sled_step > interval) {
        bool need_update = 0;
        sled_step = 0;
        rgblight_clear();
        for (i=0; i<RGBLIGHT_EFFECT_KNIGHT_LENGTH; i++) {
            if (pos+i < MATRIX_ACOLS && pos+i >= 0){
                need_update = 1;
                uint8_t tmp_col = pos+i;
                for (j=0; j<MATRIX_AROWS; j++) {
                    led[SLED_MATRIX_MASK[j][tmp_col]] = led[RGBLED_TEMP];
                }
                //BRGB
                led[BRGB_COL_MASK[tmp_col]] = led[RGBLED_TEMP];
                if (tmp_col == MATRIX_ACOLS-1) {
                    led[BRGB_COL_MASK[15]] = led[RGBLED_TEMP];
                    //indicator
                    led[SRGBLED_NUM - 3] = led[SRGBLED_NUM - 2] = led[SRGBLED_NUM - 1] = led[RGBLED_TEMP];
                }

            }
        }
        if (need_update) rgblight_set(); //Keep the first or last col on when increament changes.
        pos += increament;
        if (pos <= 0 - RGBLIGHT_EFFECT_KNIGHT_LENGTH || pos >= MATRIX_ACOLS ) {
            increament *= -1;
            current_hue = (current_hue + 40) % 768;
            sethsv(current_hue, rgblight_config.sat, rgblight_config.val, &led[RGBLED_TEMP]);
        }
    }
}

void suspend_power_down_action(void) {
    if (rgblight_config.enable) {
        rgblight_config.enable = 0;
        rgblight_setrgb(0,0,0);
    }
}

void suspend_wakeup_init_action(void)
{
    rgblight_init();
}

void hook_matrix_change(keyevent_t event)
{
    if (event.pressed) {
        uint8_t key_trans = KEY_MATRIX_TRANS[event.key.row][event.key.col];
        rgblight_matrix(key_trans >> 4, key_trans & 0xf);
    }
}

void hook_keyboard_loop()
{
    static uint16_t rgb_update_timer = 0;
    if (rgblight_timer_enabled && timer_elapsed(rgb_update_timer) > 40) {
        //print("rgb setting\n");
        rgb_update_timer = timer_read();
        rgblight_task();
    }

}

