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
#include "light_intensity.h"

uint8_t adc_f; // Flag to trigger a new ADC reading after a full LED cycle

void light(){
    /* 1. TIMER CONFIGURATION
     * Set up CTIMER0 to generate a match interrupt. 
     * This controls the timing for the 8-LED progress ring.
     */
    matchConfig = CTIMER0_Match_0_config;
    matchConfig.matchValue = 562500000U; // Adjust value for desired blinking speed
    CTIMER_SetupMatch(CTIMER0_PERIPHERAL, CTIMER0_MATCH_0_CHANNEL, &matchConfig);
    CTIMER_StartTimer(CTIMER0_PERIPHERAL);

    /* 2. INITIAL ADC READING
     * Configure ADC0 command for the Photodiode sensor input.
     */
    ADC0->CMD->CMDL = 0x20; // Set command for light sensor input channel
    LPADC_DoSoftwareTrigger(ADC0, 1); // Trigger the first conversion
    LPADC_GetConvResultBlocking(ADC0, &result, 0); // Wait for data

    /* Convert 16-bit raw value to a manageable 13-bit range for display */
    uint16_t light_value = result.convValue >> 3;
            
    resetOLED();
    sendOLED((uint8_t*)frame6, 94, OLED_DATA); // Display "Light:" or icon frame
    setSeg(95); // Move cursor to the value position on OLED

    uint8_t current_led = 0; // Index for the 8-LED ring (0 to 7)

    /* 3. DECIMAL TO OLED CONVERSION
     * Algorithm to extract each digit of the light_value for character rendering.
     */
    uint16_t div = 1;
    while(light_value / div >= 10) {
        div *= 10;
    }

    while(div > 0) {
        uint8_t digit = (light_value / div) % 10;
        const char *tmp = &font[digit][0]; // Fetch character from font table
        sendOLED((uint8_t*)tmp, 6, OLED_DATA);
        div /= 10;
    }   

    /* Initial LED state update */
    if(timer_flag){
        GPIO_PinWrite(LEDs[current_led].gpio, LEDs[current_led].pin, 1);
        current_led = (current_led == 8) ? 0 : current_led + 1; 
        timer_flag = 0;
    }

    /* 4. MONITORING LOOP
     * Continues until the 'exit_flag' is set by the Back button interrupt.
     */
    while(!exit_flag){
                
        /* Update OLED value only when a full LED cycle is complete (adc_f == 1) */
        if(adc_f){
            LPADC_DoSoftwareTrigger(ADC0, 1);
            LPADC_GetConvResultBlocking(ADC0, &result, 0);
            
            resets_led(); // Clear the LED ring for the next cycle
            setSeg(95); 
            
            /* Clear previous numerical value on OLED using blank pixels */
            uint8_t delet[18] = {0x00}; 
            sendOLED((uint8_t*)delet, 18, OLED_DATA);
            
            setSeg(95);
            light_value = result.convValue >> 3;
            
            /* Re-display updated value */
            div = 1;
            while(light_value / div >= 10) div *= 10;
            while(div > 0) {
                uint8_t digit = (light_value / div) % 10;
                const char *tmp = &font[digit][0];
                sendOLED((uint8_t*)tmp, 6, OLED_DATA);
                div /= 10;
            }   
            adc_f = 0; // Reset ADC trigger flag
        }

        /* LED RING LOGIC
         * Each timer interrupt lights up the next LED in the circle.
         * When the 8th LED (index 7) is reached, trigger a new sensor reading.
         */
        if(timer_flag){
            GPIO_PinWrite(LEDs[current_led].gpio, LEDs[current_led].pin, 1);
            
            // Check if we finished the circle (8 LEDs)
            adc_f = (current_led == 7) ? 1 : 0;
            current_led = (current_led == 7) ? 0 : current_led + 1; 
            
            timer_flag = 0; // Clear the timer interrupt flag
        }
    }

    /* 5. CLEANUP & EXIT
     * Stop hardware resources before returning to the main menu.
     */
    exit_flag = 0;
    CTIMER_Reset(CTIMER0);
    CTIMER_StopTimer(CTIMER0);
    resets_led(); // Turn off all LEDs
}
