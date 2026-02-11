#include <avr/io.h>

//
// analog comparator using internal 1.1v reference and AIN1
//
void acomp_init(void) {
  ADCSRB &= ~_BV(ACME);		/* ensure mux enable is off */
  ACSR |= _BV(ACBG);		/* enable bandgap reference */
}

// move to main for debug
// int acomp_read(void) {
//   return (ACSR & _BV(ACO)) != 0;
// }
