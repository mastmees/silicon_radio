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
#ifndef __display_hpp__
#define __display_hpp__
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

// display class for dual bubble display, with scrolling text support
//
class Display
{
  char buf[65];  // this is string currently displayed, so that scrolling can happen
  uint8_t cp;    // next character address in buf
  uint8_t fofs;  // visible frame offset
  
  // write single character at adr, starting from left
  void write(uint8_t adr,uint8_t data)
  {
    if (adr>3)
      adr=(adr&3)|4;           // two lowest bits are adr, next two are chipsels
    else
      adr=(adr&3)|8;
    adr=adr^3;                 // the display addresses characters from right to left
    PORTB=(PORTB&0xf0)|adr|0x0c; // apply address, both CS high
    PORTD=data|0x80;           // apply data, WR high
    PORTB=(PORTB&0xf0)|adr;    // appropriate CS low
    _delay_us(1);
    PORTD=data&0x7f;           // WR low
    _delay_us(1);
    PORTD=data|0x80;           // WR high
    PORTB|=0x0c;               // both CS high
  }
  
public:

  Display()
  {
    clear();
  }

  // show a single frame from current offset
  void refresh(void)
  {
    int8_t i;
    for (i=0;(i+fofs)<cp && i<8;i++)
      write(i,buf[i+fofs]);
    while (i<8) {
      write(i,' ');
      i++;
    }
  }

  // advance visible frame by one character and update display
  // returns 1 if frame wrapped back to beginning, 0 if more
  // text to show
  int8_t scroll(void)
  {
    if (fofs>=cp) {
      fofs=0;
      refresh();
      return 1;
    }
    fofs++;
    refresh();
    return fofs>=cp;
  }
  
  // clear display, and string buffer
  void clear(void)
  {
    memset(buf,'\0',sizeof(buf));
    cp=0;
    fofs=0;
    refresh();
  }
  
  // write a character to display buffer at current
  // cursor position. the display is not refreshed
  void putc(char c)
  {
    uint8_t i;
    if (cp>=sizeof(buf)-1)
    {
      for (i=0;i<sizeof(buf)-1;i++)
        buf[i]=buf[i+1];
      cp=i;
    }      
    if (c>='a' && c<='z')
      c=c-('a'-'A');
    buf[cp++]=c;
    buf[cp]='\0';
  }

  // put a string to display buffer, visible frame to beginning,
  // and refresh display
  void puts(const char* s)
  {
    char c;
    cp=0;
    while (s && *s && cp<sizeof(buf)-1)
    {
      c=*s++;
      if (c>='a' && c<='z')
        c=c-('a'-'A');
      buf[cp++]=c;
    }
    buf[cp]='\0';
    fofs=0;
    refresh();
  }

  // print 32 bit decimal number at cursor position
  // and refresh display
  void putn(int32_t n)
  {
    if (n<0) {
      putc('-');
      n=0-n;
    }
    if (n>9)
      putn(n/10);
    putc((n%10)+'0');
    refresh();
  }

};
#endif
