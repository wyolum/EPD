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

// Written in collaboration with justin@wyolum.com
// and developed on a Wyolum AlaMode http://wyolum.com/

// Operation from reset:
// * display version
// * display compiled-in display setting
// * display FLASH detected or not
// * clear screen
// * delay 5 seconds
// * display an image
// * delay ? seconds
// * back to image display


#include <inttypes.h>
#include <ctype.h>

#include <SPI.h>
#include <SD.h>
#include "EReader.h"
#include "EPD.h"


// Change this for different display size
// supported sizes: 1_44 2_0 2_7
// #define EPD_SIZE EPD_2_0
#define EPD_SIZE EPD_2_7


// current version number
#define DEMO_VERSION "1"


#if defined(__MSP430_CPU__)

// TI LaunchPad IO layout
const int Pin_PANEL_ON = P2_3;
const int Pin_BORDER = P2_5;
const int Pin_DISCHARGE = P2_4;
const int Pin_PWM = P2_1;
const int Pin_RESET = P2_2;
const int Pin_BUSY = P2_0;
const int Pin_EPD_CS = P2_6;
const int Pin_FLASH_CS = P2_7;
const int Pin_SW2 = P1_3;
const int Pin_RED_LED = P1_0;

#else

// Arduino IO layout
const int Pin_PANEL_ON = 2;
const int Pin_BORDER = 3;
const int Pin_DISCHARGE = 4;
const int Pin_PWM = 5;
const int Pin_RESET = 6;
const int Pin_BUSY = 7;
const int Pin_EPD_CS = 8;

#endif

#define INDEX_FILE "index.txt"
// the maximum number of characters in an image path
#define MAXIMUM_PATH_LENGTH 32

// pre-processor convert to string
#define MAKE_STRING1(X) #X
#define MAKE_STRING(X) MAKE_STRING1(X)


// define the E-Ink display
// EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS, SPI);



// ensure clock is ok for EPD
void set_spi_for_epd() {
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);
}

// checkerboard
void test_pattern1(void *buffer, uint32_t address, uint16_t length) {
	byte *my_buffer = (byte *)buffer;
	bool row = address % 16 > 8;
	for(uint16_t i = 0; i < length; i++){
	  bool col = i % 2;
	  *(my_buffer + i) = 0xFF * (row ^ col);
	}
	set_spi_for_epd();  // ensure SPI OK for EPD
}
// horizontal stripes
void test_pattern2(void *buffer, uint32_t address, uint16_t length){
	byte *my_buffer = (byte *)buffer;
	for (uint16_t i = 0; i < length; ++i){
	  // current_image.seek(address + i);
	  *(my_buffer + i) = 0xFF * ((address % 8) > 4);
	}
	set_spi_for_epd();  // ensure SPI OK for EPD
}


// I/O setup
void setup() {
  Serial.begin(115200);
  ereader.setup(EPD_2_7); // starts SD
	Serial.println();
	Serial.println();
	Serial.println("Demo version: " DEMO_VERSION);
	Serial.println("Display: " MAKE_STRING(EPD_SIZE));
	Serial.println();

	Serial.begin(115200);
}


static int state = 0;

// main loop
unsigned long int loop_count = 0;
void loop() {
  ereader.EPD.begin(); // power up the EPD panel
	switch(state) {
	default:
	case 0:
		// clear the screen
		ereader.EPD.clear();
		// clear -> image1
		ereader.EPD.frame_fixed(0, EPD_compensate);
		ereader.EPD.frame_fixed(0, EPD_white);
		ereader.EPD.frame_cb(0, test_pattern1, EPD_inverse);
		ereader.EPD.frame_cb(0, test_pattern1, EPD_normal);
		state = 1;
		break;
	case 1:        // swap images
	  Serial.println("CASE=1");
		ereader.EPD.frame_cb(0, test_pattern1, EPD_compensate);
		ereader.EPD.frame_cb(0, test_pattern1, EPD_white);
		ereader.EPD.frame_cb(0, test_pattern2, EPD_inverse);
		ereader.EPD.frame_cb(0, test_pattern2, EPD_normal);
		state = 2;
		break;
	case 2:        // swap images
	  Serial.println("CASE=2");
		ereader.EPD.frame_cb(0, test_pattern2, EPD_compensate);
		ereader.EPD.frame_cb(0, test_pattern2, EPD_white);
		ereader.EPD.frame_cb(0, test_pattern1, EPD_inverse);
		ereader.EPD.frame_cb(0, test_pattern1, EPD_normal);
		state = 1;
		break;
	}
	ereader.EPD.end();   // power down the EPD panel

	delay(10000);
}
