#include <avr/io.h>
#include <util/delay.h>

#include "adc.h"

/* Initialize ADC */
void adc_init(void)
{
    /* 
     * ADMUX:
     * REFS[2:0] = 000 → Vcc as reference
     * ADLAR = 0 → right-adjust result
     * MUX[3:0] = 0000 → ADC0 selected initially
     */
  ADMUX = 0;

    /*
     * ADCSRA:
     * ADEN  = 1 → enable ADC
     * ADPS2:0 = 011 → prescaler 8
     * ADC clock = 1 MHz / 8 = 125 kHz (needs 50-200 kHz)
     */
    ADCSRA =
        (1 << ADEN)  |
        (1 << ADPS1) |
        (1 << ADPS0);
}

/* Read ADC channel (0–3) */
uint16_t adc_read(uint8_t channel)
{
    /* Mask channel to valid range */
    channel &= 0x03;

    /* Clear old channel selection, keep reference */
    ADMUX = (ADMUX & 0xF0) | channel;

    /* Start conversion */
    ADCSRA |= (1 << ADSC);

    /* Wait for conversion to finish */
    while (ADCSRA & (1 << ADSC))
        ;

    /* Read result */
    return ADC;
}

