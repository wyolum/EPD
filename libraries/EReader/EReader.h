// Copyright 2013 WyoLum, LLC
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

#ifndef EREADER_H
#define EREADER_H
#include <inttypes.h>
#include "EPD.h"
#include "SD.h"

// Arduino IO layout
const int EPD_TEMPERATURE = A0; // A4
const int EPD_PANEL_ON = 2; // ORIG V2 or lower
const int EPD_BORDER = 3;  // ORIG V2 or lower
// const int EPD_PANEL_ON = A0; // V3 or higher
// const int EPD_BORDER = A1; // V3 of higher
const int EPD_DISCHARGE = 4;
const int EPD_PWM = 5;
const int EPD_RESET = 6;
const int EPD_BUSY = 7;
const int EPD_EPD_CS = 8;
const int EPD_FLASH_CS = 9;
const uint8_t UNIFONT_RECLEN = 33;
// const int SD_CS = 10; // delete me!
const int SD_CS = 9; // production

const float MCP9700_C0 = 155.15151515;   // 500 mV measured with 3v3 reference at 0 DEG C with 10 bits
const float MCP9700_GAIN = 3.103030303; // 10 mV / DEG C measured with 3v3 ref
// C = C0 + GAIN * Temp
// Temp = (C - C0) / GAIN
float getTemp();

class EReader{
 private:
  bool pingpong;
  // clear screen
  void _erase();
  void _draw();

 public:
  EPD_Class EPD;
  File display_file;
  File unifont_file;
  uint16_t epd_height;
  uint16_t epd_width;
  uint16_t epd_bytes;
  uint8_t unifont_data[UNIFONT_RECLEN - 1];

  // constructor
  EReader();

  // call in arduino setup function
  void setup(EPD_size size);

  // clear the display
  void clear();

  // display a WIF image at x, y
  bool display_wif(char *path, int16_t x, int16_t y);

  // used for EPD calls
  void reader(void *buffer, uint32_t address, uint16_t length);
  
  // toggle a pixel at x, y
  void togglepix(uint16_t x, uint16_t y);

  // set a pixel to a value
  void setpix(uint16_t x, uint16_t y, bool val);
  
  // display a line from start to stop. toggle each pix on line
  void toggle_line(int16_t startx, int16_t starty, int16_t stopx, int16_t stopy);

  // display a line from start to stop in specified color: true=black, false=white
  void draw_line(int16_t startx, int16_t starty, int16_t stopx, int16_t stopy, bool color);
  
  // draw an ellipse centered at cx, cy with horizontal radius rx and vertical radius ry
  // toggle each pix on ellipse
  void toggle_ellipse(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, bool fill);
  void toggle_ellipse(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry);
  // draw an ellipse centered at cx, cy with horizontal radius rx and vertical radius ry
  // in specified color: true=black, false=white
  void draw_ellipse(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, bool color, bool fill);

  // draw a box at specified starting and ending corners.  Start is upper left, end is lower right
  void draw_box(uint16_t startx, uint16_t starty, uint16_t endx, uint16_t endy, bool color, bool fill);
  
  
  // display new image.  Call when image is complete
  void show();

  // power off and delay
  void sleep(uint32_t delay_ms);
  
  // set spi params for epd
  void set_spi_for_epd();
  
  uint16_t put_char(uint16_t x, uint16_t y, uint16_t unic, bool color);
  uint16_t put_ascii(uint16_t x, uint16_t y, char * ascii, bool color);
  uint16_t put_unicode(uint16_t x, uint16_t y, uint16_t * unic, bool color);
  // used to pass to EPD 
  void SD_image_dims(File imgFile, unsigned short *h, unsigned short *w);
  void SD_image_reader(File imgFile, void *buffer, uint32_t address, 
		       uint16_t length);

  uint8_t unifont_read_char(File unifont_file, uint32_t i, uint8_t *dest);
  bool char_is_blank(uint32_t unic);

};

// define the E-Ink display
extern EReader ereader;

void reader_wrap(void *buffer, uint32_t address, uint16_t length);
#endif
