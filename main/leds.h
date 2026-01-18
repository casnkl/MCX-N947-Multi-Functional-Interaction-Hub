#ifndef LEDS_H_
#define LEDS_H_

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


typedef struct {
	GPIO_Type *gpio;
	uint32_t pin;
} LED_TypeDef_t;

extern LED_TypeDef_t LEDs[8];

extern volatile uint32_t sw1_flag;
extern volatile uint8_t sw2_flag;
extern volatile uint8_t sw3_flag;
extern volatile uint8_t exit_flag;
extern volatile uint8_t sw4_flag;
extern volatile uint32_t timer_flag;
extern lpadc_conv_result_t result;
extern ctimer_match_config_t matchConfig;

void oled_leds_meniu();

void resets_led();

void leds_delay_control();

void encoder_leds();


#endif /* LEDS_H_ */
