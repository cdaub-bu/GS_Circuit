#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

void setup_wdt() {
  // initialize WDT
  cli();			/* interrupts off */
  wdt_reset();
  // Set WDT to interrupt mode, 8s timeout
  WDTCR = (1<<WDCE) | (1<<WDE); // Change enable
  WDTCR = (1<<WDIE) | (1<<WDP2) | (1<<WDP1); // WDTIE=1, 1s
  //  WDTCR = (1<<WDIE) | (1<<WDP3) | (1<<WDP0); // WDTIE=1, 8s
  sei(); // Enable interrupts
}

