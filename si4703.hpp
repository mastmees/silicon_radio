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
#ifndef __si4703_hpp__
#define __si4703_hpp__

#include "baseradio.hpp"

// Many thanks go to Matthias Hertel. Life would have been much
// harder without http://mathertel.github.io/Radio/
 
// POWERCFG (2)
#define DSMUTE       0x8000 // disable softmute
#define DMUTE        0x4000 // disable mute
#define MONO         0x2000 // force mono
#define RDSM_VERBOSE 0x0800 // RDS verbose mode
#define SKMODE       0x0400 // stop seek at band end
#define SEEKUP       0x0200 // seek direction, default is DOWN
#define SEEK         0x0100 // start seek
#define DISABLE      0x0040 // powerup disable
#define ENABLE       0x0001 // powerup enabe
//CHANNEL (3)
#define TUNE         0x8000 // tune enable
#define CHANNEL_MASK 0x03ff // channel bits
//SYSCONFIG1 (4)
#define RDSIEN       0x8000 // RDS interrupt enable
#define STCIEN       0x4000 // seek/tune complete interrupt enable
#define RDS          0x1000 // enable RDS
#define DE           0x0800 // 50us international de-emphasis (default is USA 75us)
#define AGCD         0x0400 // AGC disable
#define BLEND0       0x0000 // stereo mono blend adjustments 
#define BLEND1       0x0040
#define BLEND2       0x0080
#define BLEND3       0x00c0
#define BLENDMASK    0x00c0
#define GPIO1HIGH    0x0003 // set GPIO1 high
#define GPIO1LOW     0x0002 // set GPIO1 low
#define GPIO2HIGH    0x000c // set GPIO2 high
#define GPIO2LOW     0x0008 // set GPIO2 low
#define GPIO2INT     0x0004 // use GPIO2 as RDS/STC interrupt output
// SYSCONFIG2 (5)
#define SEEKTH_MASK  0xFF00 // seek threshold bits, range is 00..7F
#define SEEKTH_INIT  0x1400 // init to this value
#define VOLUME_MASK  0x000F // audio volume bits, 0 volume is mute
#define BAND_EU_US   0x0000 // eu/us band 87.5-108
#define BAND_JP_W    0x0040 // jpn wide band 76-108
#define BAND_JP      0x0080 // jpn narrow band 76-90
#define BAND_MASK    0x00c0 // band selection bits
#define SPACE_200    0x0000 // 200kHz band spacing (us,au)
#define SPACE_100    0x0010 // 100kHz (eu,jp)
#define SPACE_50     0x0020 // 50kHz
#define SPACE_MASK   0x0030 // channel spacing bits
// SYSCONFIG3 (6)
#define VOLEXT       0x0100 // -30dB attenuation of output volume
#define SKSNR_MASK   0x00F0 // seek SNR threshold 
#define SKSNR_INIT   0x0030 // initial value
#define SKCNT_MASK   0x000F // seek FM pulse detection threshold
#define SKCNT_INIT   0x0003 // initial value
// TEST1 (7)
#define XOSCEN       0x8000 // crystal oscillator enable
#define AHIZEN       0x4000 // audio high-Z enable (disconnect outputs)
// STATUSRSSI (10)
#define RDSR         0x8000 // RDS ready
#define STC          0x4000 // seek/tune complete
#define SFBL         0x2000 // seek fail band limit
#define AFCRL        0x1000 // AFC railed (indicates invalid channel)
#define RDSS         0x0800 // RDS syncronized (verbose mode only)
#define SI           0x0100 // stereo indicator
#define RSSI_MASK    0x00FF // RSSI level bits
// READCHANNEL (11)
#define READCHAN_MASK 0x03ff // currently set channel number

#define RADIO_RST_HIGH() (PORTC|=0x08)
#define RADIO_RST_LOW() (PORTC&=(~0x08))
#define RADIO_SDA_LOW() (PORTC&=(~0x10))
#define RADIO_SDA_HIGH() (PORTC|=0x10)

class SI4703 : public BaseRadio
{
  enum {
    DEVICEID=0,    CHIPID=1,      POWERCFG=2,     CHANNEL=3,
    SYSCONFIG1=4,  SYSCONFIG2=5,  SYSCONFIG3=6,   TEST1=7, 
    TEST2=8,       BOOTCONFIG=9,  STATUSRSSI=10,  READCHAN=11,
    RDSA=12,       RDSB=13,       RDSC=14,        RDSD=15
  } SI4307REGISTERS;

  uint16_t registers[16]; // 'shadow' copy of registers

  // read starts from upper byte of register 0x0a, address wraps to 0
  // after lower byte of last register is read  
  void read()
  {
    uint8_t i;
    uint16_t buf[16];
    i2c_read(0x10,(uint8_t*)buf,32);
    // swap bytes, and shift register file
    for (i=0;i<6;i++)
      registers[i+10]=(buf[i]<<8)|(buf[i]>>8);
    for (i=6;i<16;i++)
      registers[i-6]=(buf[i]<<8)|(buf[i]>>8);
  }
  
  // write starts from upper byte of register 0x02 and
  // address wraps to 0 after reading lower byte of last
  // register, but only registers 2..7 are interesting
  void write()
  {
    uint8_t i;
    uint16_t buf[6];
    for (i=0;i<6;i++)
      buf[i]=(registers[i+2]<<8)|(registers[i+2]>>8);
    i2c_write(0x10,(uint8_t*)buf,12);
  }

  
public:

  const char *name() { return "Si4703"; }

  uint8_t is_ready()
  {
    read();
    if (registers[STATUSRSSI]&STC) // if seek/tune completed
    {
      registers[CHANNEL]&=~(TUNE); // stop tuning
      write();
      return 1;
    }
    return 0;
  }

  uint16_t channel_spacing(void)
  {
    switch (registers[SYSCONFIG2]&SPACE_MASK)
    {
      case SPACE_200:
        return 20;
      case SPACE_100:
        return 10;
      case SPACE_50:
        return 5;
    }
    return 10;
  }
    
  // start setting new frequency
  void set_frequency(int32_t f)
  {
    if (f < get_min_frequency())
      f=get_min_frequency();
    if (f> get_max_frequency())
      f=get_max_frequency();
    f = (f-get_min_frequency())/channel_spacing();
    read();
    registers[CHANNEL]&=~(CHANNEL_MASK); // clear out the channel bits
    registers[CHANNEL]|=f;    // set new channel
    registers[CHANNEL]|=TUNE; // set the TUNE bit to start
    write();
    while (!is_ready());
    if (decoder)              // if decoder is enabled, then flush it
      decoder->reset();
  }

  int32_t get_min_frequency()
  {
    return (registers[SYSCONFIG2]&BAND_MASK)?7600:8750;
  }
  
  int32_t get_frequency()
  {
    read();
    int32_t channel=registers[READCHAN]&READCHAN_MASK;
    channel=(channel*channel_spacing())+get_min_frequency();
    return channel;
  }
  
  // status
  uint8_t is_tuned() { return is_ready(); };
  uint8_t is_stereo() { return (registers[STATUSRSSI]&SI)?1:0; };
  uint8_t get_rssi() { return registers[STATUSRSSI]&RSSI_MASK; };

  uint8_t is_connected()
  {
    read();
    return ((registers[DEVICEID]&0xfff)==0x242 &&
       (((registers[CHIPID]>>6)&0x0f)==8 ||
       ((registers[CHIPID]>>6)&0x0f)==9));
  }

  // do recurring processing, such as decoding RDS. Also refreshes register file
  void run(void)
  {
    static uint8_t state=0;
    uint8_t r;
    if (decoder) {
      read();
      // RDS ready bit stays set for at least 40ms when group received, but we only
      // want to process each group once, so need to do edge detection logic
      //
      r=(registers[STATUSRSSI]&RDSR)?1:0;
      switch (state)
      {
        case 0: // waiting for positive edge
          if (r) {
            state=1;
            decoder->decode_group(registers[RDSA],registers[RDSB],registers[RDSC],registers[RDSD]);
          }
          break;
        case 1: // waiting for falling edge
          if (!r)
            state=0;
          break;
        default:
          state=0;
          break;
      }
    }
  }
    
  void init()
  {
    // reset radio, and set I2C communiction mode
    RADIO_SDA_LOW(); RADIO_RST_LOW(); _delay_ms(1);
    RADIO_RST_HIGH(); _delay_ms(1); RADIO_SDA_HIGH();
    read();
    registers[TEST1]|=XOSCEN;               // enable xtal oscillator
    write();
    _delay_ms(500);
    // reset complete
    read();
    registers[POWERCFG]=ENABLE;             // enable powerup
    registers[SYSCONFIG1]|=RDS;             // enable RDS
    registers[SYSCONFIG1]|=BLEND3;          // readily switch to stereo
    // the below two lines need to be changed for
    // country specific parameters
    registers[SYSCONFIG1]|=DE;              // 50kHz Europe setup
    registers[SYSCONFIG2]|=SPACE_100;       // 100kHz channel spacing for Europe
    //
    registers[SYSCONFIG2]&=~(VOLUME_MASK);  // mute volume
    // configure seek settings, although seeking is not implemented
    registers[SYSCONFIG2]|=SEEKTH_INIT;     // set initial seek threshold  
    registers[SYSCONFIG3]&=~(SKSNR_MASK);   // prepare to override SNR
    registers[SYSCONFIG3]&=~(SKCNT_MASK);   // and FM impulse detection thresholds
    registers[SYSCONFIG3]|=SKSNR_INIT;      // set new values
    registers[SYSCONFIG3]|=SKCNT_INIT;      // for both
    write();
    _delay_ms(110);
  } 

  void sleep()
  {
    read();
    registers[POWERCFG]|=ENABLE;
    registers[POWERCFG]|=DISABLE;
    write();
  }
  
  void wakeup()
  {
    read();
    registers[POWERCFG]&=~(DISABLE);
    registers[POWERCFG]|=ENABLE;
    write();
    if (decoder)
      decoder->reset();
  }

  void set_mono(uint8_t onoff)
  {
    read();
    if (onoff)
      registers[POWERCFG]|=MONO;
    else
      registers[POWERCFG]&=~(MONO);
    write();
  };
  
  void set_soft_mute(uint8_t onoff)
  {
    read();
    if (onoff)
      registers[POWERCFG]&=~(DSMUTE);
    else
      registers[POWERCFG]|=DSMUTE;
    write();              
  };

  // set volume to 0..15
  void set_volume(uint8_t volume)
  {
    if (volume > 15)
      volume = 15;
    read();
    registers[SYSCONFIG2]&=~(VOLUME_MASK); // clear volume bits
    registers[SYSCONFIG2]|=volume;         // set new volume
    if (!volume)                           // at zero volume also mute
      registers[POWERCFG]&=~(DMUTE);
    else
      registers[POWERCFG]|=DMUTE;
    write();
  }
  
  SI4703() 
  {
  }
  
};

#endif
