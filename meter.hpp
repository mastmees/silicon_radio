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
#ifndef __meter_hpp__
#define __meter_hpp__
#include <avr/io.h>
#include <string.h>

extern uint8_t logtable[]; // in meter.cpp

// VU meter driver, uses OC1A output for PWM signal
// that reflect the currently set value
// the set value can be 0..75, and is converted to internal
// by lookup from logtable. the conversion is done to make
// logarithmic VU more responsive to radio signal level changes
//
class VU_Meter
{
uint16_t value;
public:
  VU_Meter() {}

  void start()
  {
    TCCR1A=0x81; // fast PWM mode 5, output on OC1A, clear at OCR1A
    TCCR1B=0x0a; // clk/8
    OCR1AH=value>>8;
    OCR1AL=value&0xff;
  }
  
  void stop()
  {
    TCCR1A=0x00;
  }
  
  void set(uint8_t val)
  {
    if (val>75)
      val=75;
    value=logtable[val];
    OCR1AH=value>>8;
    OCR1AL=value&0xff;
  }
  
};

#endif
