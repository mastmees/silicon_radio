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
#ifndef __encoder_hpp__
#define __encoder_hpp__
#include <avr/io.h>

#define BUTTON_STATUS()  ((PINC>>2)&1)
#define ENCODER_INPUTS() ((PINB>>4)&0x03)

// this is rotary encoder input functionality for Alps STEC11,STEC12 family
// and others that have a pushbutton function on a shaft as well.
//
class Encoder
{
public:

  // debounce and read button presses
  int8_t read_button()
  {
    static uint8_t b=0;
    b=(b<<1)|BUTTON_STATUS();
    if (b==0xf8)
      return 1;
    return 0;
  }

  // The rotary encoder reading function is from
  // http://www.circuitsathome.com/mcu/reading-rotary-encoder-on-arduino
  // returns change in encoder state (-1,0,1)
  // each encoder step may result in more in one increment/decrement, depending
  // on encoder, this one is for 2 steps per click
  int8_t read_encoder()
  {
    static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
    static uint8_t old_AB = 0;
    static int8_t c=0;
    int8_t cc;
    old_AB <<= 2;                    //remember previous state
    old_AB |= ENCODER_INPUTS();      //add current state
    c+= ( enc_states[( old_AB & 0x0f )]);
    if (c && (c&1)==0) {
      cc=c>>1;
      c=0;
      return cc;
    }
    return 0;  
  }
};

#endif
