## MCX-N947-Multi-Functional-Interaction-Hub
Interactive system on an MCXN947 with 4-menu OLED interface. Option 1: thermistor with LED timer display. Option 2: ambient light via photodiode and LEDs. Option 3: games – number guessing and memory sequence. Option 4: LED mini-games with potentiometer or rotary encoder control.

## WORKFLOW REPRESENTATION
The representation of the project workflow is presented below:
![WORKFLOW REPRESENTATION](images/work.drawio.png)

## Project Features

### 1. Temperature Measurement (ADC + Timer)
* **Hardware:** Thermistor connected to an ADC.
* **Functionality:** Displays the current temperature on an OLED screen.
* **Refresh Rate:** Readings are taken every 30 seconds.
* **Visual Indicator:** A ring of 8 LEDs acts as a countdown timer. As the 30-second mark approaches, more LEDs light up sequentially. Both the reading and the LED timing are hardware-controlled via Timers.

### 2. Light Intensity Measurement
* **Hardware:** Photodiode.
* **Functionality:** Similar to the temperature module, it measures ambient light levels and displays the data on the OLED screen using the same timer-based refresh logic.

### 3. Games Menu (Logic & RNG)
* **Guess the Number:**
    * **RNG Seed:** The system generates a true random seed by reading an "unconnected" (floating) ADC pin 32 times. This seed initializes a pseudo-random 8-bit number generator.
    * **Gameplay:** The user inputs an 8-bit number using switches. If the input matches the generated number, the user wins.
* **Memory Sequence (Follow the Pattern):**
    * **Gameplay:** The system displays a sequence of directions (Up, Down, Left, Right) using LEDs.
    * **Challenge:** The user must replicate the sequence. You have 3 lives to successfully complete the pattern.

### 4. LED Interaction Menu
* **Variable Speed Circle:**  A potentiometer controls the rotation speed of a "chase" effect on the LED ring.
    * The current speed value is displayed in real-time on the OLED.
* **Rotary Encoder Control:**  The LEDs light up sequentially based on the rotation of the encoder.
    * Supports both clockwise and counter-clockwise (trigonometric) directions.

## Navigation & Control
* **Selection:** Each menu option is triggered by its own **dedicated button interrupt**.
* **Back Function:** A single dedicated interrupt button allows the user to return to the previous menu.
* **Inputs:** ADC (Thermistor, Photodiode, Potentiometer), 8-bit Switches, Rotary Encoder, and Push Buttons.
* **Outputs:** OLED Display, 8-LED Ring.

* ## Software & Development Tools
* **IDE:** MCUXpresso IDE
* **Configuration:** All peripheral initialization (Clock, ADC, CTIMER, I2C) and GPIO/Pin Muxing were configured using the **MCUXpresso Config Tools**.
* **SDK:** NXP SDK for MCX-N947 (Cortex-M33).
