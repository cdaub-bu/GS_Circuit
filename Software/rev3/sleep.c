#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

//
// power down until the watchdog or a button press (PB0) wakes us,
// then restart timer 0
//
void nap(void)
{
      cli();		/* disable interrupts while setting up */

  // Set the sleep mode to Power Down (most power saving)
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);

      // Turn off peripherals to save power (ADC, Timers, etc.)
      ADCSRA = 0; // Turn off ADC
      power_all_disable(); // Power off ADC, Timer 0 and 1, serial interface
      // turn off the comparator (saves 18uA!)
      ACSR |= _BV(ACD);

      // wake on button (pull-up stays on, only draws current when pressed)
      PCMSK |= _BV(PCINT0);
      GIMSK |= _BV(PCIE);

      sleep_enable(); // Enable sleep mode
      sei(); // Ensure global interrupts are enabled before sleeping
      sleep_cpu();

      // ---- SLEEP ----

      sleep_disable();
      PCMSK &= ~_BV(PCINT0);	/* button is polled while awake */
      power_timer0_enable();
}
