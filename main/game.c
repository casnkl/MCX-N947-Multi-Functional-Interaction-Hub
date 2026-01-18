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
#include "game.h"

uint32_t seed = 0;

/* --- RANDOM NUMBER GENERATION (RNG) --- */

/**
 * Generates a unique seed by reading a floating ADC pin 32 times.
 * Each iteration captures the LSB of the ADC noise to build a 32-bit seed.
 */
void seed_generator(){
    ADC0->CMD->CMDL = 0x00; // Configure for floating input channel
    for(uint8_t i = 0; i < 32; i++){
        LPADC_DoSoftwareTrigger(ADC0, 1);
        LPADC_GetConvResultBlocking(ADC0, &result, 0);
        // Extract noise from bit 4 and shift it into the seed
        seed |= (((uint32_t)(result.convValue >> 4) & 0x1) << i);
    }
}

/**
 * Permutes the seed bits to increase entropy using bitwise shifts and multipliers.
 */
void entrophy_generator(){
    seed ^= seed >> 13;
    seed *= 0x85ebca6b;
    seed ^= seed >> 16;
}

/**
 * Returns a pseudo-random number within the range [0, size-1].
 */
uint8_t pseudo_random_number_generator(uint8_t size){
    return (uint8_t)seed % size;
}

/* --- GAME LOGIC --- */

void game_meniu(){
    printfOLED("CHOSE ONE GAME:\n1. GUESS THE NUMBER\n2. R0W GAME");
}

/**
 * Game 1: Guess the Number
 * The system generates an 8-bit number. The user must match it using 8 DIP switches.
 */
void guess_number(){
    matchConfig = CTIMER0_Match_0_config;
    entrophy_generator();
    uint8_t number = pseudo_random_number_generator(255); // Target number (0-255)
    
    resetOLED();
    setSeg(10);
    setPage(3);
    sendOLED((uint8_t*)frame7, 110, OLED_DATA); // Prompt: "Set switches and press exit"
    
    uint8_t value;
    /* Wait for user to set switches and press the 'exit' button to confirm */
    while(!exit_flag){
        // Combine 8 digital inputs (DIP switches) into a single byte
        value = ((uint8_t)GPIO_PinRead(GPIO0, SHIELD_DIP_8_GPIO_PIN) << 7) +
                ((uint8_t)GPIO_PinRead(GPIO0, SHIELD_DIP_7_GPIO_PIN) << 6) +
                ((uint8_t)GPIO_PinRead(GPIO0, SHIELD_DIP_6_GPIO_PIN) << 5) +
                ((uint8_t)GPIO_PinRead(GPIO0, SHIELD_DIP_5_GPIO_PIN) << 4) +
                ((uint8_t)GPIO_PinRead(GPIO0, SHIELD_DIP_4_GPIO_PIN) << 3) +
                ((uint8_t)GPIO_PinRead(GPIO0, SHIELD_DIP_3_GPIO_PIN) << 2) +
                ((uint8_t)GPIO_PinRead(GPIO0, SHIELD_DIP_2_GPIO_PIN) << 1) +
                ((uint8_t)GPIO_PinRead(GPIO0, SHIELD_DIP_1_GPIO_PIN) << 0);
    }
    exit_flag = 0;
    
    /* Processing delay for visual feedback */
    resetOLED();
    setSeg(33);
    setPage(3);
    sendOLED((uint8_t*)frame8, 60, OLED_DATA); // Display "Checking..."
    matchConfig.matchValue = 450000000U;
    CTIMER_SetupMatch(CTIMER0, CTIMER0_MATCH_0_CHANNEL, &matchConfig);
    CTIMER_StartTimer(CTIMER0);
    while(!timer_flag); 
    timer_flag = 0;
    CTIMER_Reset(CTIMER0);
    CTIMER_StopTimer(CTIMER0);

    /* Result Comparison */
    if(number == value){
        resetOLED();
        sendOLED((uint8_t*)frame9, 236, OLED_DATA); // "YOU WIN" frame
    } else {
        resetOLED();
        sendOLED((uint8_t*)frame10, 84, OLED_DATA); // "YOU LOSE" frame
        setSeg(85);
        
        /* Display the user's input value in decimal */
        uint8_t div = 1;
        while(value / div >= 10) div *= 10;
        while(div > 0) {
            uint8_t digit = (value / div) % 10;
            const char *tmp = &font[digit][0];
            sendOLED((uint8_t*)tmp, 6, OLED_DATA);
            div /= 10;
        }
        
        /* Display the correct target number */
        setSeg(0);
        setPage(2);
        sendOLED((uint8_t*)frame11, 92, OLED_DATA); // "Correct was:"
        printVar("%ld", (uint32_t)number, 0, 93, 2);
    }

    /* Wait before returning to menu */
    matchConfig.matchValue = 1500000000U;
    CTIMER_SetupMatch(CTIMER0, CTIMER0_MATCH_0_CHANNEL, &matchConfig);
    CTIMER_StartTimer(CTIMER0);
    while (!timer_flag);
    timer_flag = 0;
    CTIMER_Reset(CTIMER0);
    CTIMER_StopTimer(CTIMER0);
}

/**
 * Game 2: Row Game (Memory/Sequence)
 * The system blinks a sequence of 6 LEDs. The user must replicate it using the NAV switch.
 */
void row_game(){
    resetOLED();
    uint8_t led_index[] = {6, 2, 0, 4}; // Left, Right, Up, Down mapping
    uint8_t led_apration[] = {0, 0, 0, 0}; // Stores the generated sequence
    uint8_t led_verification[] = {0, 0, 0, 0}; // Stores the user's sequence

    matchConfig.matchValue = 75000000U;
    CTIMER_SetupMatch(CTIMER0_PERIPHERAL, CTIMER0_MATCH_0_CHANNEL, &matchConfig);

    uint8_t n, index, j = 0;
    uint8_t lives = 3;
    bool game_flag = 0;

    /* Phase 1: Show the Sequence */
    CTIMER_StartTimer(CTIMER0);
    for(int i = 0; i < 6; i++){
        entrophy_generator();
        index = pseudo_random_number_generator(4);
        GPIO_PinWrite(LEDs[led_index[index]].gpio, LEDs[led_index[index]].pin, 1);
        while(!timer_flag); // LED ON duration
        timer_flag = 0;
        GPIO_PinWrite(LEDs[led_index[index]].gpio, LEDs[led_index[index]].pin, 0);
        while(!timer_flag); // Delay between LEDs
        timer_flag = 0;
        
        led_apration[index] |= (1 << i); // Store position in sequence bitmask
    }
    CTIMER_Reset(CTIMER0);
    CTIMER_StopTimer(CTIMER0);    
    
    /* Phase 2: User Input and Validation */
    uint8_t last_state = GPIO_PinRead(GPIO3, SHIELD_NAV_A_LEFT_GPIO_PIN) +
                         GPIO_PinRead(GPIO1, SHIELD_NAV_B_RIGHT_GPIO_PIN) +
                         GPIO_PinRead(GPIO3, SHIELD_NAV_C_UP_GPIO_PIN) +
                         GPIO_PinRead(GPIO0, SHIELD_NAV_D_DOWN_GPIO_PIN);

    matchConfig.matchValue = 600000000U;
    CTIMER_SetupMatch(CTIMER0_PERIPHERAL, CTIMER0_MATCH_0_CHANNEL, &matchConfig);
    
    setPage(3);
    setSeg(43);
    printfOLED("LIVES:");
    printVar("%d", (uint32_t)lives, 0, 82, 3);

    while(lives > 0){
        n = 6; // Expecting 6 inputs
        while(n > 0){
            uint8_t state = GPIO_PinRead(GPIO3, SHIELD_NAV_A_LEFT_GPIO_PIN) +
                            GPIO_PinRead(GPIO1, SHIELD_NAV_B_RIGHT_GPIO_PIN) +
                            GPIO_PinRead(GPIO3, SHIELD_NAV_C_UP_GPIO_PIN) +
                            GPIO_PinRead(GPIO0, SHIELD_NAV_D_DOWN_GPIO_PIN);
            
            /* State change detected (User pressed a NAV button) */
            if(last_state != state){
                resets_led();        
                if(!GPIO_PinRead(GPIO3, SHIELD_NAV_A_LEFT_GPIO_PIN)){
                    GPIO_PinWrite(LEDs[6].gpio, LEDs[6].pin, 1);
                    led_verification[0] |= (1 << j);
                    n--; j++;
                } else if(!GPIO_PinRead(GPIO1, SHIELD_NAV_B_RIGHT_GPIO_PIN)){
                    GPIO_PinWrite(LEDs[2].gpio, LEDs[2].pin, 1);
                    led_verification[1] |= (1 << j);
                    n--; j++;
                } else if(!GPIO_PinRead(GPIO3, SHIELD_NAV_C_UP_GPIO_PIN)){
                    GPIO_PinWrite(LEDs[0].gpio, LEDs[0].pin, 1);
                    led_verification[2] |= (1 << j);
                    n--; j++;
                } else if(!GPIO_PinRead(GPIO0, SHIELD_NAV_D_DOWN_GPIO_PIN)){   
                    GPIO_PinWrite(LEDs[4].gpio, LEDs[4].pin, 1);
                    led_verification[3] |= (1 << j);
                    n--; j++;
                }
                last_state = state;
            }
        }

        /* Verify Sequence */
        j = 0;
        resets_led();
        uint8_t match_count = 0;
        for(uint8_t i = 0; i < 4; i++){
            if(led_apration[i] == led_verification[i]) match_count++;
        }
        
        if(match_count == 4){
            resetOLED();
            printfOLED("YOU WIN!");
            game_flag = 1;
        } else {
            lives--;
            printVar("%d", (uint32_t)lives, 0, 82, 3);
            // Reset user input for next try if lives remaining
            for(int k=0; k<4; k++) led_verification[k] = 0;
            j = 0;
        }

        if(game_flag) break;
    }

    if(!game_flag){
        resetOLED();
        printfOLED("YOU LOSE!");
    }
    
    // Final delay to show result
    CTIMER_StartTimer(CTIMER0);
    while(!timer_flag);
    timer_flag = 0;
    CTIMER_Reset(CTIMER0);
    CTIMER_StopTimer(CTIMER0);
}
