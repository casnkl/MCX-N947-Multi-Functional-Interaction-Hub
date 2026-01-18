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

/* LED Configuration Array: Maps physical GPIOs to the 8-LED ring on the shield */
LED_TypeDef_t LEDs[8] = {
        {GPIO4, SHIELD_LED1_GPIO_PIN},
        {GPIO0, SHIELD_LED2_GPIO_PIN},
        {GPIO0, SHIELD_LED3_GPIO_PIN},
        {GPIO0, SHIELD_LED4_GPIO_PIN},
        {GPIO2, SHIELD_LED5_GPIO_PIN},
        {GPIO2, SHIELD_LED6_GPIO_PIN},
        {GPIO2, SHIELD_LED7_GPIO_PIN},
        {GPIO2, SHIELD_LED8_GPIO_PIN},
};

/* Global Flags for Interrupt Handling */
volatile uint32_t sw1_flag = 0;
volatile uint8_t sw2_flag = 0;
volatile uint8_t sw3_flag = 0;
volatile uint8_t exit_flag = 0;
volatile uint8_t sw4_flag = 0;
volatile uint32_t timer_flag = 0;
lpadc_conv_result_t result;
ctimer_match_config_t matchConfig;

/* Displays the LED Interaction Submenu on the OLED */
void oled_leds_meniu(){
    sendOLED((uint8_t*)frame1, 42, OLED_DATA);
    setPage(1);
    setSeg(0);
    sendOLED((uint8_t*)frame14, 88, OLED_DATA); // Option 1: Potentiometer Speed
    setPage(2);
    setSeg(0);
    sendOLED((uint8_t*)frame15, 100, OLED_DATA); // Option 2: Encoder Control
}

/* Helper function to turn off all LEDs in the ring */
void resets_led(){
    for(int i = 0; i < 8; i++){
        GPIO_PinWrite(LEDs[i].gpio, LEDs[i].pin, 0);
    }
}

/**
 * FEATURE 1: Potentiometer Delay Control
 * Uses a potentiometer (ADC) to control the rotation speed of a "chase" LED effect.
 */
void leds_delay_control(){
    matchConfig = CTIMER0_Match_0_config;
    matchConfig.matchValue = 37500000U; // Default starting speed
    CTIMER_SetupMatch(CTIMER0_PERIPHERAL, CTIMER0_MATCH_0_CHANNEL, &matchConfig);

    uint16_t pot_value = 0;
    uint16_t old_pot_value = 0;
    uint8_t old_led = 0;
    uint8_t current_led = 0;
    const uint8_t num_leds = sizeof(LEDs) / sizeof(LED_TypeDef_t);
    uint8_t direction = 1; // 1 for Clockwise, 0 for Counter-Clockwise
                    
    ADC0->CMD->CMDL = 0x00; // Configure ADC for potentiometer input

    resetOLED();
    sendOLED((uint8_t*)frame13, 56, OLED_DATA); // Display "Speed:" label
                    
    while(!exit_flag){
        CTIMER_StartTimer(CTIMER0_PERIPHERAL);

        /* Toggle rotation direction using SW2 interrupt */
        if(sw2_flag)
        {
            direction = !direction;
            sw2_flag = 0; 
        }
                    
        /* Manual Check for Timer Overrun to set the flag */
        if (CTIMER0->TC > CTIMER0->MR[0])
        {
            timer_flag = 1;
            CTIMER0->TCR = 2; // Reset Timer
            CTIMER0->TCR = 1; // Start Timer
        }

        /* Update Logic on Timer Tick */
        if(timer_flag == 1){
            LPADC_DoSoftwareTrigger(ADC0, 1);
            LPADC_GetConvResultBlocking(ADC0, &result, 0);
            pot_value = result.convValue >> 3;

            /* OLED Update: Only refresh if the change is significant (noise filter) */
            if((abs(pot_value - old_pot_value) >= 100)){
                setSeg(57);
                uint8_t clear[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                sendOLED(clear, 6, OLED_DATA);
                                
                uint16_t value = pot_value << 12; // Scaled value for display
                uint16_t div = 1;
                while(value / div >= 10) div *= 10;
                while(div > 0) {
                    uint8_t digit = (value / div) % 10;
                    const char *tmp = &font[digit][0];
                    sendOLED((uint8_t*)tmp, 6, OLED_DATA);
                    div /= 10;
                }
                old_pot_value = pot_value;
            }
                            
            /* Safety threshold to prevent timer stalling at very low values */
            if (pot_value <= 100) pot_value = 50;

            /* Dynamically adjust Timer Match Value based on Potentiometer */
            matchConfig.matchValue = pot_value << 12; 
            CTIMER_SetupMatch(CTIMER0_PERIPHERAL, CTIMER0_MATCH_0_CHANNEL, &matchConfig);

            /* Shift the active LED in the ring */
            GPIO_PinWrite(LEDs[old_led].gpio, LEDs[old_led].pin, 0);
            GPIO_PinWrite(LEDs[current_led].gpio, LEDs[current_led].pin, 1);

            if(direction) {
                old_led = current_led;
                current_led = (current_led + 1) % num_leds;
            } else {
                old_led = current_led;
                current_led = (current_led == 0) ? (num_leds - 1) : (current_led - 1);
            }
            timer_flag = 0; 
        }
    }
    /* Cleanup before exiting */
    exit_flag = 0;
    CTIMER_Reset(CTIMER0);
    CTIMER_StopTimer(CTIMER0);
    resetOLED();
    resets_led();
    oled_leds_meniu();
}

/**
 * FEATURE 2: Rotary Encoder LED Control
 * Uses a quadrature encoder to light up LEDs sequentially based on rotation.
 */
void encoder_leds(){
    resetOLED();
    printfOLED("LEDs");
    uint8_t state;
    uint8_t last_state = GPIO_PinRead(GPIO3, SHIELD_ROTARY_2_GPIO_PIN);
    uint8_t counter = 0;

    while(!exit_flag){
        state = GPIO_PinRead(GPIO3, SHIELD_ROTARY_2_GPIO_PIN);
        
        /* Detect rotation (state change in Channel B) */
        if(state != last_state){
            setSeg(25);
            uint8_t delete[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            sendOLED(delete, 6, OLED_DATA);

            /* Quadrature Decoding: Determine direction by comparing Channel A and B */
            if(GPIO_PinRead(GPIO3, SHIELD_ROTARY_1_GPIO_PIN) != state){
                // Counter-Clockwise
                counter = (counter == 0) ? 7 : counter - 1;
            } else {
                // Clockwise
                counter = (counter == 7) ? 0 : counter + 1;
            }
            
            last_state = state;
            
            /* Update OLED with current LED index */
            const char *tmp = &font[counter][0];
            sendOLED((uint8_t*)tmp, 6, OLED_DATA);
            
            /* Light up LEDs cumulatively (0 to current index) */
            resets_led();
            for(int i = 0; i <= counter; i++){
                GPIO_PinWrite(LEDs[i].gpio, LEDs[i].pin, 1);
            }
        }
    }
    exit_flag = 0;
    resetOLED();
    resets_led();
    oled_leds_meniu();
}
