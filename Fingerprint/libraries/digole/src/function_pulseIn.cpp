/* 
 * pulseIn Function for the Spark Core - Version 0.1.1 (Beta)
 * Copyright (2014) Timothy Brown - See: LICENSE
 *
 * Due to the current timeout issues with Spark Cloud
 * this will return after 10 seconds, even if the
 * input pulse hasn't finished.
 *
 * Input: Trigger Pin, Trigger State
 * Output: Pulse Length in Microseconds (10uS to 10S)
 *
 */
#include "application.h"

unsigned long pulseIn(uint16_t pin, uint8_t state) {
    
    GPIO_TypeDef* portMask = (PIN_MAP[pin].gpio_peripheral); // Cache the target's peripheral mask to speed up the loops.
    uint16_t pinMask = (PIN_MAP[pin].gpio_pin); // Cache the target's GPIO pin mask to speed up the loops.
    unsigned long pulseCount = 0; // Initialize the pulseCount variable now to save time.
    unsigned long loopCount = 0; // Initialize the loopCount variable now to save time.
    unsigned long loopMax = 20000000; // Roughly just under 10 seconds timeout to maintain the Spark Cloud connection.
    
    // Wait for the pin to enter target state while keeping track of the timeout.
    while (GPIO_ReadInputDataBit(portMask, pinMask) != state) {
        if (loopCount++ == loopMax) {
            return 0;
        }
    }
    
    // Iterate the pulseCount variable each time through the loop to measure the pulse length; we also still keep track of the timeout.
    while (GPIO_ReadInputDataBit(portMask, pinMask) == state) {
        if (loopCount++ == loopMax) {
            return 0;
        }
        pulseCount++;
    }
    
    // Return the pulse time in microseconds by multiplying the pulseCount variable with the time it takes to run once through the loop.
    return pulseCount * 0.405; // Calculated the pulseCount++ loop to be about 0.405uS in length.
}