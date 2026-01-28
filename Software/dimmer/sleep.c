#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

void sleep_init(void)
{
  cli();		/* disable interrupts while setting up */

  /* Disable unused peripherals to save power */
  ADCSRA &= ~(1 << ADEN);   // Disable ADC
  power_all_disable();

  /* Configure PB0 as input */
  DDRB &= ~(1 << PB0);
  PORTB |= (1 << PB0);      // Enable pull-up (optional)

  /* Enable pin-change interrupt on PB0 */
  PCMSK |= (1 << PCINT0);   // Unmask PCINT0
  GIMSK |= (1 << PCIE);     // Enable pin-change interrupts

  sei();                    // Enable global interrupts
}

