#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer_10kHz.h"

void timer0_init_10kHz(void)
{
    cli();  // Disable global interrupts

    // CTC mode
    TCCR0A = (1 << WGM01);

    // Prescaler = 8
    TCCR0B = (1 << CS01);

    OCR0A = 99;               // 10 kHz at 8 MHz / 8

    TCNT0 = 0;                // Reset counter

    // Enable Compare Match A interrupt
    TIMSK |= (1 << OCIE0A);

    sei();  // Enable global interrupts
}

