// -*- mode: c++ -*-
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.  See the License for the specific language
// governing permissions and limitations under the License.
// derived from copyright 2013 Pervasive Displays, Inc.


// This program is to illustrate the display operation as described in
// the datasheets.  The code is in a simple linear fashion and all the
// delays are set to maximum, but the SPI clock is set lower than its
// limit.  Therfore the display sequence will be much slower than
// normal and all of the individual display stages be clearly visible.

#include <inttypes.h>
#include <ctype.h>

#include <SPI.h>
#include <SD.h>
#include "EPD.h"
#include "S5813A.h"
#include "SD_EPD.h"

#define EPD_LARGE



// configure images for display size
// change these to match display size above

// no futher changed below this point

// current version number

// Add Images library to compiler path
#include <Images.h>  // this is just an empty file

// globals
bool pingpong = false;

File imgFile;

// I/O setup
void setup() {
  Serial.begin(115200);
  sdepd.setup(EPD_2_7);
}


static int state = 0;
char *match = ".DAT";
bool keeper(char* fn){
  byte n = strlen(fn);
  bool out = true;
  for(byte i = n - 4; i < n; i++){
    if(fn[i] != match[i - n + 4]){
      out = false;
      break;
    }
  }
  return out;
}
void display_togglepix(uint16_t x, uint16_t y){
}

void display_setpix(uint16_t x, uint16_t y, bool val){
}

void display_line(int16_t startx, int16_t starty, int16_t stopx, int16_t stopy, bool val){
}

void display_show(){
}

//***  ensure clock is ok for EPD
void set_spi_for_epd() {
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
}


void erase_img(){
}

void draw_img(){
}

void display_ellipse(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, bool val){
}



uint8_t display_char(uint16_t x, uint16_t y, uint16_t unic, bool color){
}

uint16_t display_ascii_string(uint16_t x, uint16_t y, char *ascii, bool color){
  char c = 'A';
  for(uint8_t i = 0; ascii[i] > 0; i++){
    x += display_char(x, y, ascii[i], color);
  }
  return x;
}

uint16_t display_unicode_string(uint16_t x, uint16_t y, uint16_t *unicode, bool color){
  for(uint8_t i = 0; unicode[i] > 0; i++){
    x += display_char(x, y, unicode[i], color);
  }
  return x;
}

// main loop
unsigned long int loop_count = 0;
unsigned long int last_loop_time = 0;
#define PI 3.141592653
uint16_t unic = 0;
uint16_t minute = 368;

void loop() {
  uint16_t UNICODE_MSG[10];
  UNICODE_MSG[0] = (uint16_t)'U';
  UNICODE_MSG[1] = (uint16_t)'N';
  UNICODE_MSG[2] = (uint16_t)'I';
  UNICODE_MSG[3] = (uint16_t)'F';
  UNICODE_MSG[4] = (uint16_t)'O';
  UNICODE_MSG[5] = (uint16_t)'N';
  UNICODE_MSG[6] = (uint16_t)'T';
  UNICODE_MSG[7] = (uint16_t)' ';
  UNICODE_MSG[8] = 128L * 79L + 9 + 8;
  UNICODE_MSG[9] = 0;
  
  // sdepd.clear(); // should be libraries job to clear next screen
  if(loop_count % 2){
    sdepd.display_wif("/IMAGES/CAT_SM.WIF", 130, 100);
    sdepd.display_wif("/IMAGES/APHRODIT.WIF", 0, 0);
  }
  else{
    sdepd.display_wif("/IMAGES/CAT_SM.WIF", 130, 0);
    sdepd.display_wif("/IMAGES/AANDJ.WIF", -1, -1);
  }
  sdepd.show();
  loop_count++;
  sdepd.sleep(1000);
  return;
  sdepd.put_char(16, 0, '0' + loop_count, true);

  // sdepd.draw_line(10, 10, 120, 120, true);
  sdepd.draw_ellipse(100, 25, 10, 10, true);
  sdepd.draw_ellipse(70, 50, 20, 20, true);
  sdepd.put_ascii(10, 120, "WyoLum ROCKS!!", true);
  sdepd.put_unicode(10, 140, UNICODE_MSG, true);
  sdepd.toggle_line(70, 0, 120, 50);
  // sdepd.toggle_ellipse(140, 75, 100, 35);
  Serial.print("done");
  return;
  // display_ascii_string(60, 16, "Hello World!", true);
  // display_ascii_string(60, 32, "WyoLum Smart Badge", true);
  // display_ascii_string(60, 48, "Open Hardware 2013", true);
  // display_ascii_string(60, 64, "Be there or be square!!!", true);

  uint16_t startx, starty, stopx, stopy;
  uint16_t R, r, cx, cy, num_r, cool_0, num_x, num_y;
  float theta;

  cool_0 = 128L * 79L + 9;
  cool_0 = 256L * 0x21 + 0x5F;
  R = (sdepd.epd_height - 40) / 2;
  r = R - 5;
  num_r = r - 13;
  cx = sdepd.epd_width - R - 5;
  cy = sdepd.epd_height/2;
  
  sdepd.display_wif("/IMAGES/BRIAN.WIF", 0, 0);
  display_ellipse(cx, cy, R, R, true);
  
  for(int i=0; i < 12; i++){
    theta = -i * PI / 6. + PI;
    num_x = num_r * sin(theta) + cx - 0;
    num_y = num_r * cos(theta) + cy - 8;

    if(i == 0){
      display_char(num_x, num_y, cool_0 + 12, false);
    }
    else{
      display_char(num_x, num_y, cool_0 + i, false);
    }
    startx = r * sin(theta) + cx;
    starty = r * cos(theta) + cy;
    stopx = R * sin(theta) + cx;
    stopy = R * cos(theta) + cy;
    display_line(startx, starty, stopx, stopy, true);
  }
  byte hour = minute / 60;

  theta = PI - minute * PI / 30.;
  stopx = 60 * sin(theta) + cx;
  stopy = 60 * cos(theta) + cy;
  display_line(cx, cy, stopx, stopy, true);

  theta = PI - (minute / 60.) * PI / 6.;
  stopx = 40 * sin(theta) + cx;
  stopy = 40 * cos(theta) + cy;
  display_line(cx, cy, stopx, stopy, true);
  display_show();

  delay(10000);
  minute += 1;
  
}

