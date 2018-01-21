/*
The MIT License (MIT)

Copyright (c) 2016 Madis Kaal <mast@nomad.ee>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "si4703.hpp"
#include "display.hpp"
#include "meter.hpp"
#include "encoder.hpp"

uint16_t EEMEM ee_frequency = 9780; // Retro FM in Tallinn, Estonia
uint16_t frequency;

VU_Meter meter;
SI4703 radio;
RDSDecoder decoder;
Display display;
Encoder encoder;

// baseradio defines some pure virtuals, so we need to
// create a handler for this
extern "C" void __cxa_pure_virtual()
{
  display.clear();
  display.puts("Purevirt");
  while (1); // we'll suffer horrible death by watchdog in few seconds 
}

enum DISPLAY_STATES { SKIP, SHOW, SCROLL };

// there must be at least one function that is always able to
// return SHOW or SCROLL result, otherwise radio_display may
// go to endless loop
uint8_t display_frequency()
{
  display.clear();
  int32_t f=radio.get_frequency()/10;
  if (f<1000)
    display.putc(' ');
  display.putn(f/10);
  display.putc('.');
  display.putn(f%10);
  display.putc(' ');
  if (radio.is_stereo()) {
    display.putc('S');
    display.putc('T');
  }
  else {
    display.putc('M');
    display.putc(' ');
  }
  display.refresh();
  return SHOW;
}

uint8_t display_station()
{
  if (*decoder.get_ps()) {
    display.puts(decoder.get_ps());
    return SHOW;
  }
  return SKIP;
}

uint8_t display_radiotext()
{
  if (*decoder.get_rt()) {
    display.puts(decoder.get_rt());
    return SCROLL;
  }
  return SKIP;
}

// these are functions will be called by main display logic.
// when new frequency is set, then the first function is always called
// main logic reacts to display function return codes
uint8_t (*displayfunctions[])() = {
  display_frequency,
  display_station,
  display_radiotext,
  NULL
};

// this handles encoder inputs and different display modes
//
void radio_display(uint8_t reset=0)
{
static uint8_t state=0,func=0,r;
static uint16_t count=0;
int8_t i;
  if (reset) {
    func=0;
    state=0;
    count=0;
  }
  else
    count++;
  i=encoder.read_encoder();
  switch (i) {
    case 1:
      if (frequency<radio.get_max_frequency()) {
        frequency=frequency+10;
        radio.set_frequency(frequency);
        func=0;
        state=0;
      }
      break;
    case -1:
      if (frequency>radio.get_min_frequency()) {
        frequency=frequency-10;
        radio.set_frequency(frequency);
        func=0;
        state=0;
      }
      break;
  }
  if (encoder.read_button())
    eeprom_write_word(&ee_frequency,frequency);
  meter.stop();
  while (!state) { // find next function with output
    count=0;
    if (displayfunctions[func]==NULL)
      func=0;
    r=displayfunctions[func++]();
    if (r==SHOW)
      state=1;
    if (r==SCROLL)
      state=2;      
  }
  switch (state) {
    case 1: // show text without scrolling
      if (count>=400)
        state=0;
      break;
    case 2: // wait a bit before starting scrolling
      if (count>=120)
        state=3;
      break;
    case 3: // scroll the text
      if (count>20) {
        if (display.scroll())
          state=0;
        count=0;
      }
      break;
  }
  meter.start();
}

ISR(TIMER0_OVF_vect)
{
  // reset timer for next interrupt
  TCNT0=0xc0;
}

ISR(WDT_vect)
{
}

ISR(PCINT0_vect)
{
}

ISR(PCINT1_vect)
{
}

/*
I/O configuration
-----------------
I/O pin                               direction     DDR  PORT
PC0 ON/OFF (low=ON)                   input         0    1
PC1 vu meter backlight led cathode    output        1    1
PC2 button                            input         0    1
PC3 radio /RST                        output        1    1
PC4 SDA                               output        1    1
PC5 SCL                               output        1    1

PD0 D0                                output        1    0
PD1 D1                                output        1    0
PD2 D2                                output        1    0
PD3 D3                                output        1    0
PD4 D4                                output        1    0
PD5 D5                                output        1    0
PD6 D6                                output        1    0
PD7 /WR                               output        1    1

PB0 A0                                output        1    0
PB1 A1                                output        1    0
PB2 /CE1                              output        1    1
PB3 /CE2                              output        1    1
PB4 encoder A                         input,pullup  0    1
PB5 encoder B                         input,pullup  0    1
*/

int main(void)
{
  MCUSR=0;
  MCUCR=0;
  // I/O directions
  DDRC=0x3a;
  DDRD=0xff;
  DDRB=0x0f;
  // initial state
  PORTC=0x3f;
  PORTD=0x80;
  PORTB=0x3c;
  //
  PCMSK1=0x04; // PCINT10 enable
  PCMSK0=0x30; // PCINT4,5 enable
  PCICR=3;     // enable PCINT0,PCINT1
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  // configure watchdog to interrupt&reset
  WDTCSR=(1<<WDE) | (1<<WDCE);
  WDTCSR=(1<<WDE) | (1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0) ; // 2sec timout, interrupt+reset
  TCCR0B=5; // timer0 clock prescaler to 256
  TIMSK0=1; // enable overflow interrupts
  TCNT0=0xc0;
  display.clear();
  sei();
  display.puts("NORADIO");
  if (!radio.is_connected())
    while (1);
  display.clear();
  radio.init();
  frequency=eeprom_read_word(&ee_frequency);
  if (frequency>radio.get_max_frequency())
    frequency=radio.get_max_frequency();
  if (frequency<radio.get_min_frequency())
    frequency=radio.get_min_frequency();
  radio.set_decoder(&decoder);
  radio.set_mono(0);
  radio.set_soft_mute(1);
  enum POWERSTATE { POWER_ON,STAY_ON,POWER_OFF,STAY_OFF };
  uint8_t tcount=0,powerstate=(PINC&1)?POWER_OFF:POWER_ON;
  while (1) {
    sleep_cpu(); // timer ot pin change interrupt wakes us up
    wdt_reset();
    WDTCSR=(1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);
    switch (powerstate)
    {
      case STAY_ON:
        if (PINC&1) {
          powerstate=POWER_OFF;
          break;
        }
        if ((tcount&3)==0)
        {
          radio.run();
          meter.set(radio.get_rssi());
        }
        tcount=(tcount+1)&3;
        radio_display();
        break;
      case STAY_OFF:
        if (!(PINC&1))
          powerstate=POWER_ON;
        break;
      default:
      case POWER_OFF:
        radio.set_volume(0);
        meter.stop();
        PORTC|=2; // meter backlight off
        radio.sleep();
        display.clear();
        powerstate=STAY_OFF;
        break;
      case POWER_ON:
        radio.wakeup();
        radio.set_frequency(frequency);
        PORTC&=~2; // meter backlight on
        meter.start();
        radio_display(1);
        radio.set_volume(3);
        tcount=0;
        powerstate=STAY_ON;
        break;
    }
  }
}
