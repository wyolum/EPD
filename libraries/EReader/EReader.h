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
#include "EPD_V2.h"
#include "SD.h"

// set HW_VERSION to 0 when following EPD schematic
#define HW_VERSION3


#ifndef HW_VERSION3

const int EPD_PANEL_ON = 2; // ORIG V2 or lower
const int EPD_BORDER = 3;  // ORIG V2 or lower
const int UP_PIN = 17;
const int DOWN_PIN = 15;
const int SEL_PIN = 16;
const int MODE_PIN = A6;
#else
const int EPD_PANEL_ON = A2; // V3 or higher
const int EPD_BORDER = A3;   // V3 of higher
const int UP_PIN = 2;        // INT0
const int DOWN_PIN = 15;
const int SEL_PIN = 3;       // INT1
const int MODE_PIN = A6;
#endif

const int LED_PIN = A0;
const int EPD_DISCHARGE = 4;
const int EPD_PWM = 5;
const int EPD_RESET = 6;
const int EPD_BUSY = 7;
const int EPD_EPD_CS = 8;
const int EPD_FLASH_CS = 9;
const uint8_t UNIFONT_RECLEN = 33;
// const int SD_CS = 10; // delete me!
const int SD_CS = 9; // production

const bool BLACK = true;
const bool WHITE = false;
const uint16_t BIGTEXT_OFFSET = 0xfee0;

// error codes
const uint8_t SD_ERROR_CODE = 0;
const uint8_t FILE_NOT_FOUND_CODE = 1;

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
  bool attached; // true if spi display is ready, false if "detached"
  bool initialized;
  File display_file;
  File unifont_file;
  uint16_t epd_height;
  uint16_t epd_width;
  uint16_t epd_bytes;
  uint8_t unifont_data[UNIFONT_RECLEN - 1];
  EPD_Class *EPD;
  // constructor
  EReader();


  void error(int code_num);
  void spi_attach(); /** PDi added on 21 June*/
  void spi_detach(); /** PDi added on 21 June*/

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

  // display a vertical from start to stop in specified color: true=black, false=white thickness pixels thick
  void draw_vline(int16_t x, int16_t starty, int16_t stopy, bool color, uint8_t thickness);
  
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
  // turn on all necessary functions
  void wake();
  
  // set spi params for epd
  void set_spi_for_epd();
  
  uint16_t put_char(uint16_t x, uint16_t y, uint16_t unic, bool color);

/*
  put ASCII string at location x, y
 */
  uint16_t put_ascii(uint16_t x, uint16_t y, char * ascii, bool color);

/*
  put ASCII string at location x, y
 */
  uint16_t put_bigascii(uint16_t x, uint16_t y, char * ascii, bool color);

/*
  put unicode string at location x, y
 */
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
