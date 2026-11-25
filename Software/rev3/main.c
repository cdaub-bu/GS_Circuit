/*
 * Rev 3 GSA operating software
 * Light sensor version, for the modified circuit:
 *   - LEDs driven directly (91 ohm each) from PB4 and PB1 (MISO),
 *     active low (pin low = LED on).
 *     MISO is driven by the ATtiny during ISP, so the LED doesn't
 *     interfere with programming (it just flickers)
 *   - TEPT4400 phototransistor, emitter to GND, collector to VCC
 *     through 10K and to PB3 (ADC3).  Dark = high reading.
 *
 * use timer 0 interrupt at 10kHz to generate 0-99% at 100Hz PWM rate
 *
 * Implemented as an FSM with the following states:
 *
 * WAKE_DIM     - dimmer button pressed (or power-up).
 *                lights on for DIM_TIME, then back to the state
 *                we came from (resume)
 *
 * WAIT_DARK    - lights off, napping in power-down.  Check the sensor
 *                on each watchdog wake-up (~8s).
 *                goto WAKE_ILLUM after DARK_SAMPLES dark readings in a row
 *
 * WAKE_ILLUM   - night lights on while dark.  Check the sensor every
 *                CHECK_TIME (the LEDs are shielded from the sensor).
 *                goto WAIT_DARK after LIGHT_SAMPLES light readings in a row
 *
 * A button press wakes from a nap (pin change interrupt) and goes to
 * WAKE_DIM from any state.
 *
 * store dimmer level [0...NDIM-1] in EEPROM address 0
 * default to dimmer level 1 if EEPROM is invalid
 */

#define USE_EEPROM

// blink on each sensor reading: 1 = light, 2 = dark
// and save readings/4 to EEPROM (read with avrdude -U eeprom:r:-:h)
//   16-byte blocks: 16 = power-up reading, 32 = latest reading,
//   48 = boot count, 64 = reset cause (MCUSR)
// #define DEBUG_SENSOR

// rate of long_tick/sec
#define TICKS_SEC 100

// check the sensor this often while the lights are on
// (about the same as the ~8s watchdog nap while they're off)
#define CHECK_TIME 8*TICKS_SEC

// time lights stay on when button pressed
#define DIM_TIME 2*TICKS_SEC

// light sensor thresholds (10-bit ADC, VCC reference, higher = darker)
// estimates for 10K pull-up and 4.5V (3xAA) -- calibrate on the bench!
#define DARK_LEVEL 1000		/* above this is dark (~10 lux) */
#define LIGHT_LEVEL 940		/* below this is light (~50 lux) */
// consecutive readings (one per ~8s) needed to change state
#define DARK_SAMPLES 2
#define LIGHT_SAMPLES 2

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#ifdef USE_EEPROM
#include <avr/eeprom.h>
#endif

#include "timer_10kHz.h"
#include "adc.h"
#include "wdt.h"

#include "sleep.h"

// LEDs on PB4 and PB1 (one LED per pin, max ~25mA each)
#define LED_DDR DDRB
#define LED1_BIT 4
#define LED2_BIT 1
#define LED1_MASK (_BV(LED1_BIT))
#define LED2_MASK (_BV(LED2_BIT))
#define LED_MASK (LED1_MASK|LED2_MASK)
#define LED_PORT PORTB
// LEDs are wired from VCC to the pin, so they light when the pin is low
#define LEDS_ON()  (LED_PORT &= ~LED_MASK)
#define LEDS_OFF() (LED_PORT |= LED_MASK)

#define SW_DDR DDRB
#define SW_BIT 0
#define SW_PORT PORTB
#define SW_PIN PINB
#define SW_MASK _BV(SW_BIT)

// light sensor on PB3 = ADC3
#define SENSOR_ADC 3

volatile uint8_t tick = 0;
volatile uint32_t long_tick = 0;
volatile uint16_t button_tick = 0;
volatile int8_t pct = 1;
volatile uint8_t button_press;

#define PERIOD 100

// Timer0 Compare Match A ISR (10 kHz)
// dim the LEDs
ISR(TIMER0_COMPA_vect)
{
  ++tick;
  if( tick == PERIOD) {
    tick = 0;
    ++long_tick;
    ++button_tick;
  }
  if( tick < pct)
    LEDS_ON();
  else
    LEDS_OFF();
}

// button pressed during a nap (only enabled while napping)
ISR(PCINT0_vect)
{
  if( !(SW_PIN & SW_MASK))
    button_press = 1;
}

// watchdog wakes us from a nap to check the sensor
ISR(WDT_vect)
{
}

uint8_t dim[] = { 0, 1, 2, 5, 10, 20, 50, 99 };
#define NDIM (sizeof(dim)/sizeof(dim[0]))
static int dim_level;

static int bounce;
#define DEBOUNCE 12

// read the light sensor, powering the ADC only while reading
uint16_t light_read(void) {
  uint16_t v;

  power_adc_enable();
  adc_init();
  adc_read( SENSOR_ADC);	/* discard first conversion */
  v = adc_read( SENSOR_ADC);
  ADCSRA = 0;
  power_adc_disable();
  return v;
}

void flash_led( int n) {
      // flash the LED
  for( int i=0; i<n; i++) {
    pct = dim[3];
    _delay_ms(50);
    pct = 0;
    _delay_ms(50);
  }
  _delay_ms(200);
}

#ifdef DEBUG_SENSOR
// fill a 16-byte EEPROM block, so it shows up however avrdude maps addresses
void debug_save( uint8_t block, uint8_t v) {
  for( uint8_t i=0; i<16; i++)
    eeprom_update_byte( (uint8_t *)(16*block + i), v);
}
#endif

typedef enum {
  WAKE_DIM,
  WAIT_DARK,
  WAKE_ILLUM
} a_state;

int main (void)
{
  static a_state state;
  static a_state resume;	/* where to go after WAKE_DIM */
  static uint32_t last_long;
  static uint8_t button;	/* 1 = pressed */
  static uint8_t last_button;
  static uint8_t samples;	/* consecutive dark/light readings */
  static uint32_t check_tick;	/* last sensor check while lights on */
  uint16_t light;

#ifdef DEBUG_SENSOR
  debug_save( 4, MCUSR);
  debug_save( 3, eeprom_read_byte( (uint8_t *)48) + 1);
#endif
  MCUSR = 0;		/* clear WDRF, or the watchdog is stuck in reset mode */

  SW_PORT |= SW_MASK;	/* enable pull-up */
  LEDS_OFF();		/* before making them outputs */
  LED_DDR |= LED_MASK;
  DIDR0 |= _BV(ADC3D);	/* sensor pin is analog only */

  power_adc_disable();  	/* ADC only on while reading the sensor */

  timer0_init_10kHz();
  setup_wdt();

  long_tick = 0;
  button_tick = 0;

  bounce = DEBOUNCE;
#ifdef USE_EEPROM
  dim_level = eeprom_read_byte( 0);
  if( dim_level >= NDIM)
    dim_level = 1;
#else
  dim_level = 1;
#endif

  pct = 0;

  flash_led(1);

  // show the light briefly, then stay on if already dark
  light = light_read();
#ifdef DEBUG_SENSOR
  debug_save( 1, light >> 2);
  _delay_ms(500);
  flash_led( light > DARK_LEVEL ? 2 : 1);
#endif
  if( light > DARK_LEVEL)
    resume = WAKE_ILLUM;
  else
    resume = WAIT_DARK;
  button_tick = 0;
  state = WAKE_DIM;

  while( 1) {

    // wait for one tick to elapse
    while( long_tick == last_long)
      ;
    last_long = long_tick;

    // check for and process button presses
    // (button_press is also set by the pin change ISR when napping)
    if( bounce) {
      --bounce;
    } else {
      bounce = DEBOUNCE;
      last_button = button;
      button = ((SW_PIN & SW_MASK) == 0);	/* active low */
      if( button && !last_button)
	button_press = 1;
    }
    if( button_press) {
      button_press = 0;
      button = 1;		/* don't count this press again when polled */
      if( state != WAKE_DIM)
	resume = state;
      state = WAKE_DIM;
      ++dim_level;
      if( dim_level >= NDIM)
	dim_level = 1;		/* don't allow 0 percent */
      pct = dim[dim_level];
#ifdef USE_EEPROM
      eeprom_write_byte( 0, dim_level);
#endif
      button_tick = 0;
    }

    switch( state) {

    // adjusting the light level
    case WAKE_DIM:

      if( button_tick > DIM_TIME) {     /* time out on dimmer on-time?  */
	state = resume;
	samples = 0;
	check_tick = long_tick;
	break;
      }

      pct = dim[dim_level];
      break;

    case WAKE_ILLUM:		/* dark out, enable nightlight */
      pct = dim[dim_level];

      /* lights off once it's light again */
      if( long_tick - check_tick >= CHECK_TIME) {
	check_tick = long_tick;
	light = light_read();
#ifdef DEBUG_SENSOR
	debug_save( 2, light >> 2);
#endif
	if( light < LIGHT_LEVEL) {
	  if( ++samples >= LIGHT_SAMPLES) {
	    pct = 0;
	    samples = 0;
	    state = WAIT_DARK;
	  }
	} else {
	  samples = 0;
	}
      }
      break;

    case WAIT_DARK:		/* lights off, wait for dark */
      pct = 0;
      LEDS_OFF();	/* timer stops in nap, so turn off now */

      light = light_read();
#ifdef DEBUG_SENSOR
      debug_save( 2, light >> 2);
      flash_led( light > DARK_LEVEL ? 2 : 1);
      LEDS_OFF();
#endif
      if( light > DARK_LEVEL) {
	if( ++samples >= DARK_SAMPLES) {
	  samples = 0;
	  check_tick = long_tick;
	  state = WAKE_ILLUM;
	  break;
	}
      } else {
	samples = 0;
      }

      // sleep until the watchdog or a button press
      nap();
      break;

    default:
      ;
    }

  }
}
