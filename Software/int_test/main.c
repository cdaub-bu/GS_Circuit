/*
 * try to get the pin change interrupt working
 */

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#define SW_PORT PORTB
#define SW_PIN PINB
#define SW_DDR DDRB
#define SW_MASK 1

#define LED_DDR DDRB
#define LED_BIT 3
#define LED_PORT PORTB
#define LED_MASK _BV(LED_BIT)

#define LED2_BIT 4
#define LED2_MASK _BV(LED2_BIT)

volatile uint8_t tick;

ISR(PCINT0_vect) {
  ++tick;
  LED_PORT ^= LED_MASK;
}

void goToSleep() {
  // Set the sleep mode to Power Down (most power saving)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  // Turn off peripherals to save power (ADC, Timers, etc.)
  ADCSRA = 0; // Turn off ADC
  power_all_disable(); // Power off ADC, Timer 0 and 1, serial interface

  // turn off the comparator (saves 18uA!)
  ACSR |= _BV(ACD);

  sleep_enable(); // Enable sleep mode
  sei(); // Ensure global interrupts are enabled before sleeping
  sleep_cpu(); // Put the CPU to sleep

  // --- Program execution resumes here after a pin change interrupt ---

  sleep_disable(); // Disable sleep mode
  power_all_enable(); // Re-enable all peripherals
}


int main (void)
{
  LED_DDR |= LED_MASK;
  LED_DDR |= LED2_MASK;
  SW_PORT |= SW_MASK;

  GIMSK |= _BV(PCIE);
  PCMSK |= _BV(PCINT0);
  GIFR |= _BV(PCIF);

  sei();

  while(1) {
    LED_PORT |= LED2_MASK;
    _delay_ms(500);
    LED_PORT &= ~LED2_MASK;
    _delay_ms(500);
    goToSleep();
  }
}


