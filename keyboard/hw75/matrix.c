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

#include "ch.h"
#include "hal.h"

/*
 * scan matrix
 */
#include "action.h"
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"
#include "wait.h"
#include "rgblight.h"
#include "hook.h"


/* debounce for both key up and down */
#define DEBOUNCE_DN_MASK (uint8_t)(~(0x80 >> 5))
#define DEBOUNCE_UP_MASK (uint8_t)(0x80 >> 5)

/* touchbar key order. from left to right. */
static const uint8_t TOUCH_COL_MASK[6] = {6, 4, 2, 7, 5, 3};

extern debug_config_t debug_config;

static matrix_row_t matrix[MATRIX_ROWS] = {0};
static uint16_t matrix_scan_timestamp = 0;
static uint8_t matrix_debouncing[MATRIX_ROWS][MATRIX_COLS] = {0};

static uint8_t get_key(void);

static void init_cols(void);
static void select_row(uint8_t row);

#if 0
void pulse_delay(void)
{
    return;
    uint8_t t=40;
    while (t--) {
        __asm__("  nop;");
    }
    //for ( us *= 48/6;us>0;us--)
    //__asm__("  nop;");
}
#endif
void matrix_init(void)
{
    //JTAG-DP Disabled and SW-DP Enabled..  need this to use PB3
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

    debug_config.enable = 1;
    debug_config.matrix = 0;
    //wait_ms(1000);

    rgblight_init();

    init_cols();
}


uint8_t matrix_scan(void)
{
#if 0
    static uint32_t test=0;
    static uint16_t test_timestamp = 0;
    test++;
    if (timer_elapsed(test_timestamp) >= 1000) {
        test_timestamp = timer_read();
        xprintf("speed:%d\n",test);
        test = 0;
    }
#endif
    //scan matrix every 1ms
    uint16_t time_check = timer_read();
    if (matrix_scan_timestamp == time_check) return 1;
    matrix_scan_timestamp = time_check;
    
    /* 74hc165, all keys in */
    palSetPad(GPIOB, 3);    // CE PB3
    palClearPad(GPIOB, 4);  // PL PB4
    __asm__("  nop;");
    palSetPad(GPIOB, 4);
    palClearPad(GPIOB, 3);

    uint8_t *debounce = &matrix_debouncing[0][0];
    for (uint8_t row=0; row<MATRIX_ROWS; row++) {
        for (uint8_t col=0; col<MATRIX_COLS; col++, *debounce++) {
            uint8_t key = get_key();
            *debounce = (*debounce >> 1) | key;

            /* 74hc595, next key */
            palClearPad(GPIOA, 5);  // CP PA5
            __asm__("  nop;");
            palSetPad(GPIOA, 5);

            /* matrix keys without touchbar */
            if (!(row == MATRIX_ROWS -1 && col > 1)) {
                matrix_row_t *p_row = &matrix[row];
                matrix_row_t col_mask = ((matrix_row_t)1 << col);
                if        (*debounce >= DEBOUNCE_DN_MASK) {  //debounce KEY DOWN 
                    *p_row |=  col_mask;
                } else if (*debounce <= DEBOUNCE_UP_MASK) { //debounce KEY UP
                    *p_row &= ~col_mask;
                } 
            }
            
        }
    }

    return 1;
}

inline
bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & ((matrix_row_t)1<<col));
}

inline
matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

void matrix_print(void)
{
    print("\nr/c 01234567\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        phex(row); print(": ");
        print_bin_reverse8(matrix_get_row(row));
        print("\n");
    }
}

/* Column pin configuration
 *  col: 0   1   2   3   4   5   6   7   8   9   10  11  12  13 14
 *  pin: B15 A2  B0  B1  B10 B11 B12 B13 B14 B9  B8  B3  B4  B5 A15
 */
static void  init_cols(void)
{
    // PA6 SPI1_MISO，KEY。
    // PA5 SPI1_CLK,  CP, 
    // PB3 SCAN CE
    // PB4 SCAN PL
	palSetGroupMode(GPIOB,  (1<<4 | 1<<3), 0 , PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOA, 5, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOA, 6, PAL_MODE_INPUT_PULLUP);

    //FN key PB0
    palSetPadMode(GPIOB, 0, PAL_MODE_INPUT_PULLUP);

}

 
uint8_t get_key(void)
{
    return (palReadPad(GPIOA, 6)==PAL_HIGH) ? 0 : 0x80;
}

static void unselect_rows(void)
{
}

static void select_row(uint8_t row)
{
}

