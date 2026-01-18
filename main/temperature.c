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

uint8_t adc_flag = 0; // Trigger flag for ADC conversion after a full LED cycle

void temperatures(){
    /* 1. TIMER SETUP
     * Configure CTIMER0 for the LED ring update frequency.
     * The match value determines the interval between lighting up consecutive LEDs.
     */
    matchConfig = CTIMER0_Match_0_config;
    matchConfig.matchValue = 562500000U;
    CTIMER_SetupMatch(CTIMER0_PERIPHERAL, CTIMER0_MATCH_0_CHANNEL, &matchConfig);
    CTIMER_StartTimer(CTIMER0_PERIPHERAL);

    /* 2. SENSOR INITIALIZATION
     * Set ADC0 Command Low to channel 0x03 (connected to the thermistor).
     */
    ADC0->CMD->CMDL = 0x03; 
    LPADC_DoSoftwareTrigger(ADC0, 1); // Start first conversion
    LPADC_GetConvResultBlocking(ADC0, &result, 0); // Blocking read for initial data

    /* Right-shift 16-bit raw value to 13-bit for display scaling */
    uint16_t thermistor_value = result.convValue >> 3;

    /* Display "Temp:" frame or icon and set cursor position */
    sendOLED((uint8_t*)frame5, 32, OLED_DATA);
    setSeg(33);

    uint8_t current_led = 0; // Index for the 8-LED progress circle

    /* 3. VALUE RENDERING
     * Converts the numerical ADC value to decimal digits for OLED font display.
     */
    uint16_t div = 1;
    while(thermistor_value / div >= 10) {
        div *= 10;
    }

    while(div > 0) {
        uint8_t digit = (thermistor_value / div) % 10;
        const char *tmp = &font[digit][0];
        sendOLED((uint8_t*)tmp, 6, OLED_DATA);
        div /= 10;
    }   

    /* Update first LED if the timer has already ticked */
    if(timer_flag){
        GPIO_PinWrite(LEDs[current_led].gpio, LEDs[current_led].pin, 1);
        current_led = (current_led == 7) ? 0 : current_led + 1; 
        timer_flag = 0;
    }
    
    /* 4. MAIN MONITORING LOOP
     * Runs until 'exit_flag' is triggered via the Back button interrupt.
     */
    while(!exit_flag){
                
        /* Triggered once every 8 timer ticks (full LED circle) */
        if(adc_flag){
            resets_led(); // Clear LEDs for the next cycle
            setSeg(33);
            
            /* Clear old value on OLED by sending blank (0x00) pixels */
            uint8_t delet[18] = {0x00};
            sendOLED((uint8_t*)delet, 18, OLED_DATA);

            /* Perform a fresh ADC read from the thermistor */
            LPADC_DoSoftwareTrigger(ADC0, 1);
            LPADC_GetConvResultBlocking(ADC0, &result, 0);
            thermistor_value = result.convValue >> 3;

            /* Render the new temperature value */
            setSeg(33);
            div = 1;
            while(thermistor_value / div >= 10) div *= 10;
            while(div > 0) {
                uint8_t digit = (thermistor_value / div) % 10;
                const char *tmp = &font[digit][0];
                sendOLED((uint8_t*)tmp, 6, OLED_DATA);
                div /= 10;
            }   
            adc_flag = 0; // Reset trigger
        }

        /* 5. VISUAL FEEDBACK (LED RING)
         * Lights up one LED at a time. When the ring is full (LED 7), 
         * it triggers a new ADC reading cycle.
         */
        if(timer_flag){
            GPIO_PinWrite(LEDs[current_led].gpio, LEDs[current_led].pin, 1);
            
            // If we reached the end of the circle, set adc_flag to refresh data
            adc_flag = (current_led == 7) ? 1 : 0;
            current_led = (current_led == 7) ? 0 : current_led + 1; 
            
            timer_flag = 0; // Clear hardware timer flag
        }
    }

    /* 6. EXIT PROCEDURE
     * Cleanup hardware states before returning to the main menu.
     */
    exit_flag = 0;
    CTIMER_Reset(CTIMER0);
    CTIMER_StopTimer(CTIMER0);
    resets_led(); // Ensure all LEDs are OFF
}
