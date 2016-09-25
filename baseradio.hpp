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
#ifndef __baseradio_hpp__
#define __baseradio_hpp__

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define noPROGRAMTYPENAMES

#ifdef PROGRAMTYPENAMES
extern const char * const _program_types[];
#endif

// http://www.nrscstandards.org/DocumentArchive/NRSC-4%201998.pdf
class RDSDecoder
{
private:
  char rtbuf[65];   // text collection buf
protected:
  char ps[9];       // station name
  char rt[65];      // radio text
  int8_t pty;       // program type
  char time[6];     // hh:mm local time
  char date[11];    // dd.mm.yyyy
  uint8_t tchannel; // channel ID for RT, on change the buffer is cleared
public:
  const char *get_ps() { return ps; }
  const char *get_rt() { return rt; }
  const char *get_date() { return date; }
  const char *get_time() { return time; }
#ifdef PROGRAMTYPENAMES
  const char *get_ptyn() { return (pty>=0)?_program_types[pty]:_program_types[0]; } 
#else
  const char *get_ptyn() { return ""; }
#endif

  void decode_group(uint16_t b1,uint16_t b2,uint16_t b3,uint16_t b4);

  void reset()
  {
    memset(ps,0,sizeof(ps));
    memset(rt,0,sizeof(rt));
    memset(rtbuf,0,sizeof(rtbuf));
    pty=-1;
    memset(time,0,sizeof(time));
    memset(date,0,sizeof(date));
    tchannel=0;
  }    
  
  RDSDecoder()
  {
    reset(); 
  }
  
};

// base class for radio modules
// defines interface and implements common functionality such as i2c
// protocol
//
class BaseRadio
{
protected:
  RDSDecoder *decoder;

  void i2c_stop();
  bool i2c_start();
  uint8_t i2c_send(uint8_t *buf,uint8_t count);
  uint8_t i2c_recv(uint8_t *buf,uint8_t count);
  uint8_t i2c_write(uint8_t slave,uint8_t *buf,uint8_t count);
  uint8_t i2c_read(uint8_t slave,uint8_t *buf,uint8_t count);

public:
  virtual const char* name() = 0;
  virtual void init() = 0;
  virtual void set_frequency(int32_t f) = 0;
  virtual int32_t get_frequency() = 0;
  virtual uint8_t is_tuned() = 0;
  virtual uint8_t is_stereo() = 0;
  virtual uint8_t is_connected() = 0;
  virtual int32_t get_min_frequency() { return 8700; }
  virtual int32_t get_max_frequency() { return 10800; }
  virtual uint8_t get_rssi() { return 0; }
  virtual void set_mono(uint8_t onoff) { }
  virtual void set_soft_mute(uint8_t onoff) { }
  virtual void sleep() { }
  virtual void wakeup() { }
  virtual void run() { }
  virtual void set_decoder(RDSDecoder *d) { decoder=d; }
  virtual void seek_up() { };
  virtual void seek_down() { };
  BaseRadio() : decoder(NULL) { }
};

#endif
