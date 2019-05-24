/*
Copyright 2012 Jun Wako <wakojun@gmail.com>

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
#include <stdbool.h>
#include <string.h>
#include "ch.h"
#include "hal.h"

/*
 * scan matrix
 */
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"
#include "wait.h"
#include "timer.h"

#ifndef DEBOUNCE
#   define DEBOUNCE 5
#endif

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

static bool debouncing = false;
static uint16_t debouncing_time = 0;


inline
uint8_t matrix_rows(void)
{
    return MATRIX_ROWS;
}

inline
uint8_t matrix_cols(void)
{
    return MATRIX_COLS;
}

void matrix_init(void)
{
    /* Row(sense) */
    palSetPadMode(GPIOD, 1,  PAL_MODE_INPUT_PULLDOWN);
    palSetPadMode(GPIOD, 4,  PAL_MODE_INPUT_PULLDOWN);
    //palSetPadMode(GPIOD, 5,  PAL_MODE_INPUT_PULLDOWN);
    //palSetPadMode(GPIOD, 6,  PAL_MODE_INPUT_PULLDOWN);
    //palSetPadMode(GPIOD, 7,  PAL_MODE_INPUT_PULLDOWN);

    /* Column(strobe) */
    palSetPadMode(GPIOB, 2,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOB, 3,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOB, 17, PAL_MODE_OUTPUT_PUSHPULL);
   // palSetPadMode(GPIOB, 19, PAL_MODE_OUTPUT_PUSHPULL);
   // palSetPadMode(GPIOC, 0,  PAL_MODE_OUTPUT_PUSHPULL);
   // palSetPadMode(GPIOC, 9,  PAL_MODE_OUTPUT_PUSHPULL);
   // palSetPadMode(GPIOC, 10, PAL_MODE_OUTPUT_PUSHPULL);
   // palSetPadMode(GPIOC, 11, PAL_MODE_OUTPUT_PUSHPULL);
   // palSetPadMode(GPIOD, 0,  PAL_MODE_OUTPUT_PUSHPULL);

    memset(matrix, 0, MATRIX_ROWS * sizeof(matrix_row_t));
    memset(matrix_debouncing, 0, MATRIX_ROWS * sizeof(matrix_row_t));

    matrix_init_quantum();
}

uint8_t matrix_scan(void)
{
    for (int row = 0; row < MATRIX_ROWS; row++) {
        matrix_row_t data = 0;

        // strobe row
        switch (row) {
            case 0: palSetPad(GPIOB, 2);    break;
            case 1: palSetPad(GPIOB, 3);    break;
            case 2: palSetPad(GPIOB, 17);   break;
            case 3: palSetPad(GPIOB, 19);   break;
            case 4: palSetPad(GPIOC, 0);    break;
            case 5: palSetPad(GPIOC, 9);    break;
            case 6: palSetPad(GPIOC, 10);   break;
            case 7: palSetPad(GPIOC, 11);   break;
            case 8: palSetPad(GPIOD, 0);    break;
        }

        // need wait to settle pin state
        // if you wait too short, or have a too high update rate
        // the keyboard might freeze, or there might not be enough
        // processing power to update the LCD screen properly.
        // 20us, or two ticks at 100000Hz seems to be OK
        wait_us(20);

        // read col data: { PTD1, PTD4, PTD5, PTD6, PTD7 }
        data = ((palReadPort(GPIOD) & 0xF0) >> 3) |
               ((palReadPort(GPIOD) & 0x02) >> 1);

        // un-strobe row
        switch (row) {
            case 0: palClearPad(GPIOB, 2);  break;
            case 1: palClearPad(GPIOB, 3);  break;
            case 2: palClearPad(GPIOB, 17); break;
            case 3: palClearPad(GPIOB, 19); break;
            case 4: palClearPad(GPIOC, 0);  break;
            case 5: palClearPad(GPIOC, 9);  break;
            case 6: palClearPad(GPIOC, 10); break;
            case 7: palClearPad(GPIOC, 11); break;
            case 8: palClearPad(GPIOD, 0);  break;
        }

        if (matrix_debouncing[row] != data) {
            matrix_debouncing[row] = data;
            debouncing = true;
            debouncing_time = timer_read();
        }
    }


    if (debouncing && timer_elapsed(debouncing_time) > DEBOUNCE) {
        for (int row = 0; row < MATRIX_ROWS; row++) {
            matrix[row] = matrix_debouncing[row];
        }
        debouncing = false;
    }
    matrix_scan_quantum();
    return 1;
}

inline
bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & (1<<col));
}

inline
matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

void matrix_print(void)
{
    xprintf("\nr/c 01234567\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        xprintf("%X0: ", row);
        matrix_row_t data = matrix_get_row(row);
        for (int col = 0; col < MATRIX_COLS; col++) {
            if (data & (1<<col))
                xprintf("1");
            else
                xprintf("0");
        }
        xprintf("\n");
    }
}

/* Column pin configuration
 */



