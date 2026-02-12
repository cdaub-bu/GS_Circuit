#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer0_init_100kHz.h"

void timer0_init_100kHz(void)
{
    cli();  // Disable global interrupts

    // Set Timer0 to CTC mode (Clear Timer on Compare Match)
    TCCR0A = (1 << WGM01);     // CTC mode
    TCCR0B = (1 << CS00);      // Prescaler = 1

    OCR0A = 79;                // 100 kHz interrupt rate at 8 MHz clock

    TCNT0 = 0;                 // Reset counter

    TIMSK |= (1 << OCIE0A);    // Enable Timer0 Compare Match A interrupt

    sei();  // Enable global interrupts
}


