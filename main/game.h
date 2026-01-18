#ifndef GAME_H_
#define GAME_H_

#include "board.h"
#include "app.h"
#include <stdio.h>
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MCXN947_cm33_core0.h"
#include "fsl_debug_console.h"
#include "fsl_lpi2c.h"
#include "fsl_device_registers.h"
#include "oled.h"
#include "math.h"

void seed_generator();

uint8_t pseudo_random_number_generator(uint8_t size);

void game_meniu();

void guess_number();

void row_game();

#endif // GAME_H
