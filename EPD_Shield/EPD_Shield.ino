// -*- mode: c++ -*-
// Copyright 2013 Pervasive Displays, Inc.
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
#include "FLASH.h"

#ifdef EPD_SMALL
  #define EPD_SIZE EPD_1_44
  #define short EPD_WIDTH 128L
  #define EPD_HEIGHT 96L
#elifdef EPD_MEDIUM
  #define EPD_SIZE EPD_2_0
  #define EPD_WIDTH 200L
  #define EPD_HEIGHT 96L
#else
  #define EPD_SIZE EPD_2_7
  #define EPD_WIDTH 264L
  #define EPD_HEIGHT 176L
#endif
#define EPD_BYTES (EPD_WIDTH * EPD_HEIGHT / 8)


// configure images for display size
// change these to match display size above

// no futher changed below this point

// current version number

// Add Images library to compiler path
#include <Images.h>  // this is just an empty file

// Arduino IO layout
const int Pin_TEMPERATURE = A4;
const int Pin_PANEL_ON = 2;
const int Pin_BORDER = 3;
const int Pin_DISCHARGE = 4;
const int Pin_PWM = 5;
const int Pin_RESET = 6;
const int Pin_BUSY = 7;
const int Pin_EPD_CS = 8;
const int Pin_FLASH_CS = 9;


const uint8_t UNIFONT_RECLEN = 33;
uint8_t unifont_data[UNIFONT_RECLEN - 1];

// globals
bool pingpong = false;

// define the E-Ink display
EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS, SPI);

File display_file;
File unifont_file;
File imgFile;
File root;

// I/O setup
void setup() {

  Serial.begin(115200);
	//pinMode(Pin_RED_LED, OUTPUT);
	//pinMode(Pin_SW2, INPUT);
	pinMode(Pin_TEMPERATURE, INPUT);
	pinMode(Pin_PWM, OUTPUT);
	pinMode(Pin_BUSY, INPUT);
	pinMode(Pin_RESET, OUTPUT);
	pinMode(Pin_PANEL_ON, OUTPUT);
	pinMode(Pin_DISCHARGE, OUTPUT);
	pinMode(Pin_BORDER, OUTPUT);
	pinMode(Pin_EPD_CS, OUTPUT);
	pinMode(Pin_FLASH_CS, OUTPUT);

	//digitalWrite(Pin_RED_LED, LOW);
	digitalWrite(Pin_PWM, LOW);
	digitalWrite(Pin_RESET, LOW);
	digitalWrite(Pin_PANEL_ON, LOW);
	digitalWrite(Pin_DISCHARGE, LOW);
	digitalWrite(Pin_BORDER, LOW);
	digitalWrite(Pin_EPD_CS, LOW);
	digitalWrite(Pin_FLASH_CS, HIGH);

	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);

	// wait for USB CDC serial port to connect.  Arduino Leonardo only
	while (!Serial) {
	}

	Serial.println();
	Serial.println();
	Serial.println("WyoLum EPD Shield");
	Serial.print("Display: ");
	Serial.println(EPD_SIZE);
	Serial.println();

	// configure temperature sensor
	S5813A.begin(Pin_TEMPERATURE);
  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output
  // or the SD library functions will not work.
   pinMode(10, OUTPUT);

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  
  // clear display
  display_new();
  root = SD.open("/IMAGES");
  
  // queue up images
  EPD.clear();
}

void display_new(){
  Serial.println("display_new()");
  display_file = SD.open("__EPD__.DSP", FILE_WRITE);
  unifont_file = SD.open("unifont.wff");
  display_clear(false);
  display_clear(true);
}
void display_clear(bool pingpong){
  display_file.seek(pingpong * EPD_BYTES);
  for(uint32_t pos = 0; pos < EPD_BYTES; pos++){
    display_file.write((byte)0);
  }
}

void display_reader(void *buffer, uint32_t address, uint16_t length){
  byte *my_buffer = (byte *)buffer;
  uint32_t offset = pingpong * EPD_BYTES;

  display_file.seek(offset + address);
  for(uint16_t i=0; i < length; i++){
    my_buffer[i] = display_file.read();
  }
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
  if(out){
    Serial.print(fn);
  }
  return out;
}

void display_setpix(uint16_t x, uint16_t y, bool val){
  bool my_display = !pingpong;
  byte dat;
  uint8_t bit_idx = x % 8;
  uint32_t pos = my_display * EPD_BYTES + y * EPD_WIDTH / 8 + x / 8;
  if(pos < EPD_BYTES){
    display_file.seek(pos + (!pingpong * EPD_BYTES));
    dat = display_file.read();
    if(val){
      dat |= (1 << bit_idx);
    }
    else{
      dat &= (~1 << bit_idx);
    }
    display_file.seek(pos + (!pingpong) * EPD_BYTES);
    display_file.write(dat);
  }
}

void display_line(int16_t startx, int16_t starty, int16_t stopx, int16_t stopy, bool val){
  float dx = (stopx - startx);
  float dy = (stopy - starty);
  int16_t l = sqrt(dx * dx + dy * dx);
  int16_t x, y;
  for(uint16_t t = 0; t < l; t++){
    x = startx + dx * t / l;
    y = starty + dy * t / l;
    display_setpix(x, y, val);
  }
}

void display_show(){
  // copy image data to old_image data
  char buffer[EPD_WIDTH];

  erase_img();
  display_clear(pingpong);
  pingpong = !pingpong;
  draw_img();
}

//***  ensure clock is ok for EPD
void set_spi_for_epd() {
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
}


void erase_img(){
  int temperature = S5813A.read();
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" Celcius");

  //*** maybe need to ensure clock is ok for EPD
  set_spi_for_epd();

  EPD.begin(); // power up the EPD panel
  EPD.setFactor(temperature); // adjust for current temperature
  EPD.frame_cb_repeat(0, display_reader, EPD_compensate);
  EPD.frame_cb_repeat(0, display_reader, EPD_white);  
}

void draw_img(){
  //*** maybe need to ensure clock is ok for EPD
  set_spi_for_epd();

  EPD.frame_cb_repeat(0, display_reader, EPD_inverse);
  EPD.frame_cb_repeat(0, display_reader, EPD_normal);
  EPD.end();   // power down the EPD panel
  
}

void display_ellipse(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, bool val){
  float step = atan(min(1./rx, 1./ry));
  Serial.println(step);
  for(float theta = 0; theta < 2 * PI; theta+=step){
    display_setpix(rx * cos(theta) + cx, ry * sin(theta) + cy, val);
  }
}

uint8_t unifont_read_char(uint32_t i, uint8_t *dest){
  uint8_t n_byte;
  unifont_file.seek(i * UNIFONT_RECLEN);
  n_byte = (uint8_t)unifont_file.read();

  for(uint8_t i = 0; i < n_byte; i++){
    dest[i] = (uint8_t)unifont_file.read();
  }
  Serial.println(n_byte);
  return n_byte;
}

bool char_is_blank(uint32_t unic){
  bool out = true;
  uint8_t n_byte;
  uint8_t i;
  n_byte = unifont_read_char(unic, unifont_data);
  for(i = 0; i < n_byte && out; i++){
    if(unifont_data[i] > 0){
      out = false;
    }
  }
  return out;
}

uint8_t display_char(uint16_t x, uint16_t y, uint16_t unic, bool color){
  uint32_t pos;
  uint8_t char_width;
  uint8_t out = 16;

  if(x < EPD_WIDTH && y < EPD_HEIGHT){
    char_width = unifont_read_char(unic, unifont_data) / 16;
    for(uint16_t i = 0; i < 16; i++){
      display_file.seek((i + y) * EPD_WIDTH / 8 + x / 8 + (!pingpong) * EPD_BYTES);
      for(byte j = 0; j < char_width; j++){
	if(color){
	  display_file.write(unifont_data[char_width * i + j]);
	}
	else{
	  display_file.write(~unifont_data[char_width * i + j]);
	}
      }
    } 
    out = char_width * 8;
  }
  return out;
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

void loop() {
  /*
  display_line(10, 10, 120, 120, true);
  display_ellipse(150, 75, 100, 35, true);
  display_ascii_string(60, 16, "Hello World!", true);
  display_ascii_string(60, 32, "WyoLum Smart Badge", true);
  display_ascii_string(60, 48, "Open Hardware 2013", true);
  display_ascii_string(60, 64, "Be there or be square!!!", true);
  */
  uint16_t x = 0, y = 0;
  for(y = 0; y < EPD_HEIGHT; y += 16){
    for(x = 0; x < EPD_WIDTH; ){
      if(!char_is_blank(unic++)){
	x += display_char(x, y, unic, true);
      }
    }
  }
  display_show();
  // delay(5000);

}

