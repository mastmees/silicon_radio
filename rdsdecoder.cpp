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

void RDSDecoder::decode_group(uint16_t rdsa,uint16_t rdsb,uint16_t rdsc,uint16_t rdsd)
{
char c;
  switch (rdsb>>11) {
    case 0: // 0A
    case 1: // 0B
      pty=(rdsb&0x03e0)>>5;  // get program type and station name from
      rdsb=(rdsb&3)<<1;      // basic info block
      ps[rdsb]=rdsd>>8;
      ps[rdsb+1]=rdsd&0xff;
      return;
    case 4: // 2A 64 character radio text 
      if ((rdsb&0x10)!=tchannel) {
        tchannel=rdsb&0x10;
        memcpy(rt,rtbuf,sizeof(rt));
        memset(rtbuf,0,sizeof(rtbuf));
      }
      rdsb=(rdsb&0xf)<<2;
      c=rdsc>>8;
      if (c=='\r')
        c=0;
      rtbuf[rdsb]=c;
      c=rdsc&0xff;
      if (c=='\r')
        c=0;
      rtbuf[rdsb+1]=c;
      c=rdsd>>8;
      if (c=='\r')
        c=0;
      rtbuf[rdsb+2]=c;
      c=rdsd&0xff;
      if (c=='\r')
        c=0;
      rtbuf[rdsb+3]=c;
      rtbuf[64]='\0';
      return;
    case 5: // 2B 32 character radio text
      if ((rdsb&0x10)!=tchannel) {
        tchannel=rdsb&0x10;
        memcpy(rt,rtbuf,sizeof(rt));
        memset(rtbuf,0,sizeof(rtbuf));
      }
      rdsb=(rdsb&0xf)<<1;
      c=rdsd>>8;
      if (c=='\r')
        c=0;
      rtbuf[rdsb]=c;
      c=rdsd&0xff;
      if (c=='\r')
        c=0;
      rtbuf[rdsb+1]=c;
      rtbuf[32]='\0';
      return;
  }
}

#ifdef PROGRAMTYPENAMES
const char * const _program_types[] = {
"", // 0
"News", // 1
"Current", // 2
"Information", // 3
"Sport", // 4
"Education", // 5
"Drama", // 6
"Culture", // 7
"Science", // 8
"Varied", // 9
"Pop", // 10
"Rock", // 11
"Easy listening", // 12
"Light classical", // 13
"Serious classical", // 14
"Music", // 15
"Weather", // 16
"Finance", // 17
"Children", // 18
"Social", // 19
"Religion", // 20
"Phone-in", // 21
"Travel", // 22
"Leisure", // 23
"Jazz", // 24
"Country", // 25
"National", // 26
"Oldies", // 27
"Folk", // 28
"Documentary", // 29
"Alarm test", // 30
"Alarm" // 31
};
#endif
