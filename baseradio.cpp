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

#include "baseradio.hpp"

// TWI status codes
#define START_SENT 0x08
#define START_REPEATED 0x10
#define MT_SLA_ACK 0x18
#define MT_SLA_NAK 0x20
#define MT_DATA_ACK 0x28
#define MT_DATA_NAK 0x30
#define ARBITRATION_LOST 0x38
#define MR_SLA_ACK 0x40
#define MR_SLA_NAK 0x48
#define MR_DATA_ACK 0x50
#define MR_DATA_NAK 0x58
// macros for basic TWI operations
#define i2c_wait() while (!(TWCR & (1<<TWINT)))
#define i2c_status() (TWSR & 0xF8)
#define i2c_read_ack() (TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWEA))
#define i2c_read_nak() (TWCR=(1<<TWINT)|(1<<TWEN)|(0<<TWEA))

void BaseRadio::i2c_stop()
{
  TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

bool BaseRadio::i2c_start()
{
  TWSR = 0x00; // configure i2c clock
  TWBR = 0x0C;
  TWCR =(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
  i2c_wait();
  if (i2c_status()!=START_SENT)
  {
    i2c_stop();
    return false;
  }
  return true;
}

uint8_t BaseRadio::i2c_send(uint8_t *buf,uint8_t count)
{
  uint8_t tc=0;
  while (count--) {
    TWDR=*buf++;
    tc++;
    TWCR=(1<<TWINT)|(1<<TWEN);
    i2c_wait();
    if (i2c_status()!=MT_DATA_ACK)
    {
      break;
    }
  }
  return tc;
}

uint8_t BaseRadio::i2c_recv(uint8_t *buf,uint8_t count)
{
uint8_t rc=0;
  while (count>1) {
    i2c_read_ack();
    i2c_wait();
    count--;
    if (i2c_status()!=MR_DATA_ACK)
    {
      i2c_stop();
      return rc;
    }
    *buf++=TWDR;
    rc++;
  }
  i2c_read_nak();
  i2c_wait();
  i2c_stop();
  return rc;
}
  
// write count bytes from buf to slave
// return number of bytes successfully written
uint8_t BaseRadio::i2c_write(uint8_t slave,uint8_t *buf,uint8_t count)
{
  uint8_t tc=0;
  if (!i2c_start())
  {
    i2c_stop();
    return tc;
  }
  TWDR=slave<<1; // SLA+W
  TWCR=(1<<TWINT)|(1<<TWEN);
  i2c_wait();
  if (i2c_status()!=MT_SLA_ACK)
  {
    i2c_stop();
    return tc;
  }
  tc=i2c_send(buf,count);
  i2c_stop();
  return tc;
}

// read count bytes from slave to buf
// returns number of bytes successfully read
uint8_t BaseRadio::i2c_read(uint8_t slave,uint8_t *buf,uint8_t count)
{
  uint8_t rc=0;
  if (!i2c_start())
  {
    i2c_stop();
    return false;
  }
  TWDR=(slave<<1)|1; // SLA+R
  TWCR=(1<<TWINT)|(1<<TWEN);
  i2c_wait();
  if (i2c_status()!=MR_SLA_ACK)
  {
    i2c_stop();
    return false;
  }
  rc=i2c_recv(buf,count);
  return rc;
}
