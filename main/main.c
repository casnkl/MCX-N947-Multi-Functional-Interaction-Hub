
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
#include "leds.h"
#include "temperature.h"
#include "light_intensity.h"
#include "game.h"

/* * INTERRUPT HANDLERS
 * Each handler sets a specific flag when a button is pressed.
 * These flags are then processed in the main loop (polling).
 */

/* GPIO40_IRQn: Handles SW1 interrupt - Typically used for Option 1 / Selection */
void GPIO4_INT_0_IRQHANDLER(void)
{
    uint32_t pin_flags0 = GPIO_GpioGetInterruptChannelFlags(GPIO4, 0U);
    sw1_flag = 1; // Set flag for Software Button 1
    GPIO_GpioClearInterruptChannelFlags(GPIO4, pin_flags0, 0U);
    SDK_ISR_EXIT_BARRIER;
}

/* GPIO30_IRQn: Handles SW2 interrupt - Typically used for Option 2 / Selection */
void GPIO3_INT_0_IRQHANDLER(void) {
    uint32_t pin_flags0 = GPIO_GpioGetInterruptChannelFlags(GPIO3, 0U);
    sw2_flag = 1; // Set flag for Software Button 2
    GPIO_GpioClearInterruptChannelFlags(GPIO3, pin_flags0, 0U); 
    SDK_ISR_EXIT_BARRIER;
}

/* GPIO00_IRQn: Handles Exit/Back interrupt - Used to return to previous menu */
void GPIO0_INT_0_IRQHANDLER(void) {
    uint32_t pin_flags0 = GPIO_GpioGetInterruptChannelFlags(GPIO0, 0U);
    exit_flag = 1; // Set global exit flag
    GPIO_GpioClearInterruptChannelFlags(GPIO0, pin_flags0, 0U); 
    SDK_ISR_EXIT_BARRIER;
}

/* GPIO31_IRQn: Handles SW4 interrupt - Typically used for LED Games Menu */
void GPIO3_INT_1_IRQHANDLER(void) {
    uint32_t pin_flags1 = GPIO_GpioGetInterruptChannelFlags(GPIO3, 1U);
    sw4_flag = 1; // Set flag for Software Button 4
    GPIO_GpioClearInterruptChannelFlags(GPIO3, pin_flags1, 1U); 
    SDK_ISR_EXIT_BARRIER;
}

/* GPIO01_IRQn: Handles SW3 interrupt - Typically used for Games Menu */
void GPIO0_INT_1_IRQHANDLER(void) {
    uint32_t pin_flags1 = GPIO_GpioGetInterruptChannelFlags(GPIO0, 1U);
    sw3_flag = 1; // Set flag for Software Button 3
    GPIO_GpioClearInterruptChannelFlags(GPIO0, pin_flags1, 1U); 
    SDK_ISR_EXIT_BARRIER;
}

/* Timer Callback: Used for periodic tasks like sensor readings */
void ctimer_match_callback(uint32_t flags)
{
    timer_flag = 1; 
}

/* Renders the Main Menu frames on the OLED display */
void OLED_main_meniu(){
    sendOLED((uint8_t*)frame1, 42, OLED_DATA);
    setPage(1);
    setSeg(0);
    sendOLED((uint8_t*)frame2, 80, OLED_DATA);
    setPage(2);
    setSeg(0);
    sendOLED((uint8_t*)frame3, 100, OLED_DATA);
    setPage(3);
    setSeg(0);
    sendOLED((uint8_t*)frame4, 38, OLED_DATA);
    setPage(4);
    setSeg(0);
    sendOLED((uint8_t*)frame12, 38, OLED_DATA);
}

int main(void) {
    /* Initialize System Hardware */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    /* Initialize Peripherals: LEDs, Buttons, Switches, and Sensors */
    BOARD_InitLEDsPins();
    BOARD_InitBUTTONsPins();
    SHIELD_InitLEDsPins();
    SHIELD_InitBUTTONsPins();
    SHIELD_DIPSwitchPins();
    SHIELD_RotaryPins();
    SHIELD_NAVSwitchPins();

    /* I2C Clock Configuration for OLED communication */
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1u);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

    initOLED();
    OLED_main_meniu(); // Draw initial menu
    
    /* Generate RNG Seed using floating ADC reads */
    seed_generator();

    /* MAIN EVENT LOOP */
    while(1)
    {   
        /* Option 1: Temperature Monitoring */
        if(sw1_flag){
            sw1_flag = 0;
            resetOLED();
            temperatures(); // Enter temperature module
            resetOLED();
            OLED_main_meniu(); // Return to main menu
        }

        /* Option 2: Light Intensity Monitoring */
        if(sw2_flag){
            sw2_flag = 0;
            light(); // Enter light intensity module
            resetOLED();
            OLED_main_meniu();
        }

        /* Option 3: Games Submenu */
        if(sw3_flag){
            sw3_flag = 0;
            resetOLED();
            game_meniu();
            while(!exit_flag){
                if(sw1_flag){
                    sw1_flag = 0;
                    guess_number(); // Game: Guess the 8-bit number
                    resetOLED();
                    game_meniu();
                }
                if(sw2_flag){
                    sw2_flag = 0;
                    row_game(); // Game: Memory sequence
                    resetOLED();
                    game_meniu();
                }
            }
            exit_flag = 0;
            resetOLED();
            OLED_main_meniu();
        }   

        /* Option 4: LED Effects Submenu */
        if(sw4_flag){
            sw4_flag = 0;
            resetOLED();
            oled_leds_meniu();
            while(!exit_flag){
                if(sw1_flag){
                    sw1_flag = 0;
                    leds_delay_control(); // Potentiometer speed control
                }
                if(sw2_flag){
                    sw2_flag = 0;
                    encoder_leds(); // Rotary encoder direction control
                }
            }
            exit_flag = 0;
            resetOLED();
            OLED_main_meniu();
        }
    }
    return 0;
}
