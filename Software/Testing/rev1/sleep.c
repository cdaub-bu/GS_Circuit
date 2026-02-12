#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

void sleep_init(void)
{
      cli();		/* disable interrupts while setting up */

  // Set the sleep mode to Power Down (most power saving)
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
      // Turn off peripherals to save power (ADC, Timers, etc.)
      ADCSRA = 0; // Turn off ADC
      power_all_disable(); // Power off ADC, Timer 0 and 1, serial interface
      // turn off the comparator (saves 18uA!)
      ACSR |= _BV(ACD);

      // turn off pull-up on button
      PORTB &= ~1;

      sleep_enable(); // Enable sleep mode
      sei(); // Ensure global interrupts are enabled before sleeping
}

