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

// Modified by WyoLum to display all "*.WIF" files in the "/IMAGES/" directory.

#include <inttypes.h>
#include <ctype.h>

#include <SPI.h>
#include <SD.h>
#include <avr/pgmspace.h>

// S5813A.h"
// ********************************************************************************
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

class S5813A_Class {
private:
	int temperature_pin;
public:
	int read();
	long readVoltage();  // returns micro volts

	// inline static void attachInterrupt();
	// inline static void detachInterrupt();

	void begin(int input_pin);
	void end();

	S5813A_Class(int input_pin);

};

extern S5813A_Class S5813A;
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


#if defined(__MSP430_CPU__)

// TI LaunchPad defaults
// ---------------------

// LaunchPad / TI MSP430G2553 runs at 3.3V .. 3.5V
#define PIN_TEMPERATURE  A4

// but use the 2.5V internal reference - seems to be better
#define ANALOG_REFERENCE INTERNAL2V5

// ADC maximum voltage at counts
#define ADC_MAXIMUM_uV   2500000L
#define ADC_COUNTS       1024L

#else

// Arduino defaults
// ----------------

// Arduino Leonardo / Atmel MEGA32U4 runs at 5V
#define PIN_TEMPERATURE  A0

// use the default 5V reference
#define ANALOG_REFERENCE DEFAULT

// ADC maximum voltage at counts
#define ADC_MAXIMUM_uV   5000000L
#define ADC_COUNTS       1024L

#endif


// temperature chip parameters
// these may need adjustment for a particular chip
// (typical values taken from data sheet)
#define Vstart_uV 1145000L
#define Tstart_C  100
#define Vslope_uV -11040L


// there is a potential divider on the input, so as scale to the
// correct voltage as would be seen on the temperature output pin
// Divider:
//   Rhigh = 26.7k   Rlow = 17.8k
// (be careful to avoid overflow if values too large for "long")
#define Rdiv_high 267L
#define Rdiv_low 178L
#define REV_PD(v) ((Rdiv_high + Rdiv_low) * (v) / Rdiv_low)


// the default Temperature device
S5813A_Class S5813A(PIN_TEMPERATURE);


S5813A_Class::S5813A_Class(int input_pin) : temperature_pin(input_pin) {
}


// initialise the anolog system
void S5813A_Class::begin(int input_pin) {
	pinMode(input_pin, INPUT);
	analogReference(ANALOG_REFERENCE);
	this->temperature_pin = input_pin;
}


void S5813A_Class::end() {
}


// return sensor output voltage in uV
// not the ADC value, but the value that should be measured on the
// sensor output pin
long S5813A_Class::readVoltage() {
	long vADC = analogRead(this->temperature_pin);
	return REV_PD((vADC * ADC_MAXIMUM_uV) / ADC_COUNTS);
}


// return temperature as integer in Celcius
int S5813A_Class::read() {
	return Tstart_C + ((this->readVoltage() - Vstart_uV) / Vslope_uV);
}
// ********************************************************************************
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


typedef enum {
	EPD_1_44,        // 128 x 96
	EPD_2_0,         // 200 x 96
	EPD_2_7          // 264 x 176
} EPD_size;

typedef enum {           // Image pixel -> Display pixel
	EPD_compensate,  // B -> W, W -> B (Current Image)
	EPD_white,       // B -> N, W -> W (Current Image)
	EPD_inverse,     // B -> N, W -> B (New Image)
	EPD_normal       // B -> B, W -> W (New Image)
} EPD_stage;

typedef void EPD_reader(void *buffer, uint32_t address, uint16_t length);

class EPD_Class {
private:
	SPIClass &SPI;
	int EPD_Pin_EPD_CS;
	int EPD_Pin_PANEL_ON;
	int EPD_Pin_BORDER;
	int EPD_Pin_DISCHARGE;
	int EPD_Pin_PWM;
	int EPD_Pin_RESET;
	int EPD_Pin_BUSY;

	EPD_size size;
	uint16_t stage_time;
	uint16_t factored_stage_time;
	uint16_t lines_per_display;
	uint16_t dots_per_line;
	uint16_t bytes_per_line;
	uint16_t bytes_per_scan;
	PROGMEM const prog_uint8_t *gate_source;
	uint16_t gate_source_length;
	PROGMEM const prog_uint8_t *channel_select;
	uint16_t channel_select_length;

	bool filler;

public:
	// power up and power down the EPD panel
	void begin();
	void end();

	void setFactor(int temperature = 25) {
		this->factored_stage_time = this->stage_time * this->temperature_to_factor_10x(temperature) / 10;
	}

	// clear display (anything -> white)
	void clear() {
		this->frame_fixed_repeat(0xff, EPD_compensate);
		this->frame_fixed_repeat(0xff, EPD_white);
		this->frame_fixed_repeat(0xaa, EPD_inverse);
		this->frame_fixed_repeat(0xaa, EPD_normal);
	}

	// assuming a clear (white) screen output an image
	void image(PROGMEM const prog_uint8_t *image) {
		this->frame_fixed_repeat(0xaa, EPD_compensate);
		this->frame_fixed_repeat(0xaa, EPD_white);
		this->frame_data_repeat(image, EPD_inverse);
		this->frame_data_repeat(image, EPD_normal);
	}

	// change from old image to new image
	void image(PROGMEM const prog_uint8_t *old_image, PROGMEM const prog_uint8_t *new_image) {
	  this->frame_data_repeat(old_image, EPD_compensate);
	  this->frame_data_repeat(old_image, EPD_white);
	  this->frame_data_repeat(new_image, EPD_inverse);
	  this->frame_data_repeat(new_image, EPD_normal);
	}

	// Low level API calls
	// ===================

	// single frame refresh
	void frame_fixed(uint8_t fixed_value, EPD_stage stage);
	void frame_data(PROGMEM const prog_uint8_t *new_image, EPD_stage stage);
	void frame_cb(uint32_t address, EPD_reader *reader, EPD_stage stage);

	// stage_time frame refresh
	void frame_fixed_repeat(uint8_t fixed_value, EPD_stage stage);
	void frame_data_repeat(PROGMEM const prog_uint8_t *new_image, EPD_stage stage);
	void frame_cb_repeat(uint32_t address, EPD_reader *reader, EPD_stage stage);

	// convert temperature to compensation factor
	int temperature_to_factor_10x(int temperature);

	// single line display - very low-level
	// also has to handle AVR progmem
	void line(uint16_t line, const uint8_t *data, uint8_t fixed_value, bool read_progmem, EPD_stage stage);

	// inline static void attachInterrupt();
	// inline static void detachInterrupt();

	EPD_Class(EPD_size size,
		  int panel_on_pin,
		  int border_pin,
		  int discharge_pin,
		  int pwm_pin,
		  int reset_pin,
		  int busy_pin,
		  int chip_select_pin,
		  SPIClass &SPI_driver);

};
// ********************************************************************************

// EPD.cpp
// ********************************************************************************
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


#include <limits.h>

// delays - more consistent naming
#define Delay_ms(ms) delay(ms)
#define Delay_us(us) delayMicroseconds(us)

// inline arrays
#define ARRAY(type, ...) ((type[]){__VA_ARGS__})
#define CU8(...) (ARRAY(const uint8_t, __VA_ARGS__))


static void PWM_start(int pin);
static void PWM_stop(int pin);

static void SPI_put(uint8_t c);
static void SPI_put_wait(uint8_t c, int busy_pin);
static void SPI_send(uint8_t cs_pin, const uint8_t *buffer, uint16_t length);


EPD_Class::EPD_Class(EPD_size size,
		     int panel_on_pin,
		     int border_pin,
		     int discharge_pin,
		     int pwm_pin,
		     int reset_pin,
		     int busy_pin,
		     int chip_select_pin,
		     SPIClass &SPI_driver) :
	EPD_Pin_PANEL_ON(panel_on_pin),
	EPD_Pin_BORDER(border_pin),
	EPD_Pin_DISCHARGE(discharge_pin),
	EPD_Pin_PWM(pwm_pin),
	EPD_Pin_RESET(reset_pin),
	EPD_Pin_BUSY(busy_pin),
	EPD_Pin_EPD_CS(chip_select_pin),
	SPI(SPI_driver) {

	this->size = size;
	this->stage_time = 480; // milliseconds
	this->lines_per_display = 96;
	this->dots_per_line = 128;
	this->bytes_per_line = 128 / 8;
	this->bytes_per_scan = 96 / 4;
	this->filler = false;


	// display size dependant items
	{
		static uint8_t cs[] = {0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00};
		static uint8_t gs[] = {0x72, 0x03};
		this->channel_select = cs;
		this->channel_select_length = sizeof(cs);
		this->gate_source = gs;
		this->gate_source_length = sizeof(gs);
	}

	// set up size structure
	switch (size) {
	default:
	case EPD_1_44:  // default so no change
		break;

	case EPD_2_0: {
		this->lines_per_display = 96;
		this->dots_per_line = 200;
		this->bytes_per_line = 200 / 8;
		this->bytes_per_scan = 96 / 4;
		this->filler = true;
		static uint8_t cs[] = {0x72, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00};
		static uint8_t gs[] = {0x72, 0x03};
		this->channel_select = cs;
		this->channel_select_length = sizeof(cs);
		this->gate_source = gs;
		this->gate_source_length = sizeof(gs);
		break;
	}

	case EPD_2_7: {
		this->stage_time = 630; // milliseconds
		this->lines_per_display = 176;
		this->dots_per_line = 264;
		this->bytes_per_line = 264 / 8;
		this->bytes_per_scan = 176 / 4;
		this->filler = true;
		static uint8_t cs[] = {0x72, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfe, 0x00, 0x00};
		static uint8_t gs[] = {0x72, 0x00};
		this->channel_select = cs;
		this->channel_select_length = sizeof(cs);
		this->gate_source = gs;
		this->gate_source_length = sizeof(gs);
		break;
	}
	}

	this->factored_stage_time = this->stage_time;
}


void EPD_Class::begin() {

	// power up sequence
	SPI_put(0x00);

	digitalWrite(this->EPD_Pin_RESET, LOW);
	digitalWrite(this->EPD_Pin_PANEL_ON, LOW);
	digitalWrite(this->EPD_Pin_DISCHARGE, LOW);
	digitalWrite(this->EPD_Pin_BORDER, LOW);
	digitalWrite(this->EPD_Pin_EPD_CS, LOW);

	PWM_start(this->EPD_Pin_PWM);
	Delay_ms(5);
	digitalWrite(this->EPD_Pin_PANEL_ON, HIGH);
	Delay_ms(10);

	digitalWrite(this->EPD_Pin_RESET, HIGH);
	digitalWrite(this->EPD_Pin_BORDER, HIGH);
	digitalWrite(this->EPD_Pin_EPD_CS, HIGH);
	Delay_ms(5);

	digitalWrite(this->EPD_Pin_RESET, LOW);
	Delay_ms(5);

	digitalWrite(this->EPD_Pin_RESET, HIGH);
	Delay_ms(5);

	// wait for COG to become ready
	while (HIGH == digitalRead(this->EPD_Pin_BUSY)) {
	}

	// channel select
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x01), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, this->channel_select, this->channel_select_length);

	// DC/DC frequency
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x06), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0xff), 2);

	// high power mode osc
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x07), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x9d), 2);


	// disable ADC
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x08), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x00), 2);

	// Vcom level
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x09), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0xd0, 0x00), 3);

	// gate and source voltage levels
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x04), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, this->gate_source, this->gate_source_length);

	Delay_ms(5);  //???

	// driver latch on
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x03), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x01), 2);

	// driver latch off
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x03), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x00), 2);

	Delay_ms(5);

	// charge pump positive voltage on
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x05), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x01), 2);

	// final delay before PWM off
	Delay_ms(30);
	PWM_stop(this->EPD_Pin_PWM);

	// charge pump negative voltage on
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x05), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x03), 2);

	Delay_ms(30);

	// Vcom driver on
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x05), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x0f), 2);

	Delay_ms(30);

	// output enable to disable
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x02), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x24), 2);
}


void EPD_Class::end() {

	this->frame_fixed(0x55, EPD_normal); // dummy frame
	this->line(0x7fffu, 0, 0x55, false, EPD_normal); // dummy_line

	Delay_ms(25);

	digitalWrite(this->EPD_Pin_BORDER, LOW);
	Delay_ms(30);

	digitalWrite(this->EPD_Pin_BORDER, HIGH);

	// latch reset turn on
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x03), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x01), 2);

	// output enable off
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x02), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x05), 2);

	// Vcom power off
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x05), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x0e), 2);

	// power off negative charge pump
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x05), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x02), 2);

	// discharge
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x04), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x0c), 2);

	Delay_ms(120);

	// all charge pumps off
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x05), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x00), 2);

	// turn of osc
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x07), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x0d), 2);

	// discharge internal - 1
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x04), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x50), 2);

	Delay_ms(40);

	// discharge internal - 2
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x04), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0xA0), 2);

	Delay_ms(40);

	// discharge internal - 3
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x04), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x00), 2);

	// turn of power and all signals
	digitalWrite(this->EPD_Pin_RESET, LOW);
	digitalWrite(this->EPD_Pin_PANEL_ON, LOW);
	digitalWrite(this->EPD_Pin_BORDER, LOW);
	digitalWrite(this->EPD_Pin_EPD_CS, LOW);

	digitalWrite(this->EPD_Pin_DISCHARGE, HIGH);

	SPI_put(0x00);

	Delay_ms(150);

	digitalWrite(this->EPD_Pin_DISCHARGE, LOW);
}


// convert a temperature in Celcius to
// the scale factor for frame_*_repeat methods
int EPD_Class::temperature_to_factor_10x(int temperature) {
	if (temperature <= -10) {
		return 170;
	} else if (temperature <= -5) {
		return 120;
	} else if (temperature <= 5) {
		return 80;
	} else if (temperature <= 10) {
		return 40;
	} else if (temperature <= 15) {
		return 30;
	} else if (temperature <= 20) {
		return 20;
	} else if (temperature <= 40) {
		return 10;
	}
	return 7;
}


// One frame of data is the number of lines * rows. For example:
// The 1.44” frame of data is 96 lines * 128 dots.
// The 2” frame of data is 96 lines * 200 dots.
// The 2.7” frame of data is 176 lines * 264 dots.

// the image is arranged by line which matches the display size
// so smallest would have 96 * 32 bytes

void EPD_Class::frame_fixed(uint8_t fixed_value, EPD_stage stage) {
	for (uint8_t line = 0; line < this->lines_per_display ; ++line) {
		this->line(line, 0, fixed_value, false, stage);
	}
}


void EPD_Class::frame_data(PROGMEM const prog_uint8_t *image, EPD_stage stage){
	for (uint8_t line = 0; line < this->lines_per_display ; ++line) {
		this->line(line, &image[line * this->bytes_per_line], 0, true, stage);
	}
}


void EPD_Class::frame_cb(uint32_t address, EPD_reader *reader, EPD_stage stage) {
	static uint8_t buffer[264 / 8];
	for (uint8_t line = 0; line < this->lines_per_display; ++line) {
		reader(buffer, address + line * this->bytes_per_line, this->bytes_per_line);
		this->line(line, buffer, 0, false, stage);
	}
}


void EPD_Class::frame_fixed_repeat(uint8_t fixed_value, EPD_stage stage) {
	long stage_time = this->factored_stage_time;
	do {
		unsigned long t_start = millis();
		this->frame_fixed(fixed_value, stage);
		unsigned long t_end = millis();
		if (t_end > t_start) {
			stage_time -= t_end - t_start;
		} else {
			stage_time -= t_start - t_end + 1 + ULONG_MAX;
		}
	} while (stage_time > 0);
}


void EPD_Class::frame_data_repeat(PROGMEM const prog_uint8_t *image, EPD_stage stage) {
	long stage_time = this->factored_stage_time;
	do {
		unsigned long t_start = millis();
		this->frame_data(image, stage);
		unsigned long t_end = millis();
		if (t_end > t_start) {
			stage_time -= t_end - t_start;
		} else {
			stage_time -= t_start - t_end + 1 + ULONG_MAX;
		}
	} while (stage_time > 0);
}


void EPD_Class::frame_cb_repeat(uint32_t address, EPD_reader *reader, EPD_stage stage) {
	long stage_time = this->factored_stage_time;
	do {
		unsigned long t_start = millis();
		this->frame_cb(address, reader, stage);
		unsigned long t_end = millis();
		if (t_end > t_start) {
			stage_time -= t_end - t_start;
		} else {
			stage_time -= t_start - t_end + 1 + ULONG_MAX;
		}
	} while (stage_time > 0);
}


void EPD_Class::line(uint16_t line, const uint8_t *data, uint8_t fixed_value, bool read_progmem, EPD_stage stage) {
	// charge pump voltage levels
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x04), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, this->gate_source, this->gate_source_length);

	// send data
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x0a), 2);
	Delay_us(10);

	// CS low
	digitalWrite(this->EPD_Pin_EPD_CS, LOW);
	SPI_put_wait(0x72, this->EPD_Pin_BUSY);

	// even pixels
	for (uint16_t b = this->bytes_per_line; b > 0; --b) {
		if (0 != data) {
#if defined(__MSP430_CPU__)
			uint8_t pixels = data[b - 1] & 0xaa;
#else
			// AVR has multiple memory spaces
			uint8_t pixels;
			if (read_progmem) {
				pixels = pgm_read_byte_near(data + b - 1) & 0xaa;
			} else {
				pixels = data[b - 1] & 0xaa;
			}
#endif
			switch(stage) {
			case EPD_compensate:  // B -> W, W -> B (Current Image)
				pixels = 0xaa | ((pixels ^ 0xaa) >> 1);
				break;
			case EPD_white:       // B -> N, W -> W (Current Image)
				pixels = 0x55 + ((pixels ^ 0xaa) >> 1);
				break;
			case EPD_inverse:     // B -> N, W -> B (New Image)
				pixels = 0x55 | (pixels ^ 0xaa);
				break;
			case EPD_normal:       // B -> B, W -> W (New Image)
				pixels = 0xaa | (pixels >> 1);
				break;
			}
			SPI_put_wait(pixels, this->EPD_Pin_BUSY);
		} else {
			SPI_put_wait(fixed_value, this->EPD_Pin_BUSY);
		}	}

	// scan line
	for (uint16_t b = 0; b < this->bytes_per_scan; ++b) {
		if (line / 4 == b) {
			SPI_put_wait(0xc0 >> (2 * (line & 0x03)), this->EPD_Pin_BUSY);
		} else {
			SPI_put_wait(0x00, this->EPD_Pin_BUSY);
		}
	}

	// odd pixels
	for (uint16_t b = 0; b < this->bytes_per_line; ++b) {
		if (0 != data) {
#if defined(__MSP430_CPU__)
			uint8_t pixels = data[b] & 0x55;
#else
			// AVR has multiple memory spaces
			uint8_t pixels;
			if (read_progmem) {
				pixels = pgm_read_byte_near(data + b) & 0x55;
			} else {
				pixels = data[b] & 0x55;
			}
#endif
			switch(stage) {
			case EPD_compensate:  // B -> W, W -> B (Current Image)
				pixels = 0xaa | (pixels ^ 0x55);
				break;
			case EPD_white:       // B -> N, W -> W (Current Image)
				pixels = 0x55 + (pixels ^ 0x55);
				break;
			case EPD_inverse:     // B -> N, W -> B (New Image)
				pixels = 0x55 | ((pixels ^ 0x55) << 1);
				break;
			case EPD_normal:       // B -> B, W -> W (New Image)
				pixels = 0xaa | pixels;
				break;
			}
			uint8_t p1 = (pixels >> 6) & 0x03;
			uint8_t p2 = (pixels >> 4) & 0x03;
			uint8_t p3 = (pixels >> 2) & 0x03;
			uint8_t p4 = (pixels >> 0) & 0x03;
			pixels = (p1 << 0) | (p2 << 2) | (p3 << 4) | (p4 << 6);
			SPI_put_wait(pixels, this->EPD_Pin_BUSY);
		} else {
			SPI_put_wait(fixed_value, this->EPD_Pin_BUSY);
		}
	}

	if (this->filler) {
		SPI_put_wait(0x00, this->EPD_Pin_BUSY);
	}

	// CS high
	digitalWrite(this->EPD_Pin_EPD_CS, HIGH);

	// output data to panel
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x70, 0x02), 2);
	Delay_us(10);
	SPI_send(this->EPD_Pin_EPD_CS, CU8(0x72, 0x2f), 2);
}


static void SPI_put(uint8_t c) {
	SPI.transfer(c);
}


static void SPI_put_wait(uint8_t c, int busy_pin) {

	SPI_put(c);

	// wait for COG ready
	while (HIGH == digitalRead(busy_pin)) {
	}
}


static void SPI_send(uint8_t cs_pin, const uint8_t *buffer, uint16_t length) {
	// CS low
	digitalWrite(cs_pin, LOW);

	// send all data
	for (uint16_t i = 0; i < length; ++i) {
		SPI_put(*buffer++);
	}

	// CS high
	digitalWrite(cs_pin, HIGH);
}


static void PWM_start(int pin) {
	analogWrite(pin, 128);  // 50% duty cycle
}


static void PWM_stop(int pin) {
	analogWrite(pin, 0);
}

// ********************************************************************************


#ifdef EPD_SMALL
  #define EPD_SIZE EPD_1_44
  #define short EPD_WIDTH 128
  #define EPD_HEIGHT 96
#elifdef EPD_MEDIUM
  #define EPD_SIZE EPD_2_0
  #define EPD_WIDTH 200
  #define EPD_HEIGHT 96
#else
  #define EPD_SIZE EPD_2_7
  #define EPD_WIDTH 264
  #define EPD_HEIGHT 176
#endif
const uint32_t EPD_BYTES = (EPD_WIDTH * EPD_HEIGHT / 8);


// configure images for display size
// change these to match display size above

// no futher changed below this point

// current version number
#define DEMO_VERSION "W"


// Add Images library to compiler path
//#include <Images.h>  // this is just an empty file

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

//const int Pin_SW2 = 12;
//const int Pin_RED_LED = 13;

// LED anode through resistor to I/O pin
// LED cathode to Ground



// define the E-Ink display
EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS, SPI);

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
	Serial.println("Demo version: " DEMO_VERSION);
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
   pinMode(9, OUTPUT);

  if (!SD.begin(9)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  root = SD.open("/IMAGES");

  // queue up images
  next_image();
  char buffer1[100];
  char buffer2[100];
  for(int i = 0; i < 26; i++){
    buffer1[i] = 'A' + i;
  }
  EPD.clear();
}


static int state = 0;
char *match = ".WIF";
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

void next_image(){
  // copy image data to old_image data
  char buffer[EPD_WIDTH];

  erase_img(imgFile);
  imgFile.close();

  imgFile =  root.openNextFile();
  if(!imgFile){
    root.close();
    root = SD.open("/IMAGES");
    // root.rewindDirectory();
    imgFile.close();
    imgFile =  root.openNextFile();
  }
  while(!keeper(imgFile.name())){
    imgFile.close();
    imgFile =  root.openNextFile();
    if(!imgFile){
      root.close();
      root = SD.open("/IMAGES");
      //root.rewindDirectory();
      imgFile.close();
      imgFile =  root.openNextFile();
    }
  }
  if(!imgFile){
    Serial.println("new image not found");
    while(1){
      delay(1000);
    }
  }
  Serial.print("New File: ");
  Serial.println(imgFile.name());

  set_spi_for_epd();
  draw_img(imgFile);
}

//***  ensure clock is ok for EPD
void set_spi_for_epd() {
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);
}


void erase_img(File imgFile){
  int temperature = S5813A.read();
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" Celcius");

  //*** maybe need to ensure clock is ok for EPD
  // set_spi_for_epd();

  EPD.begin(); // power up the EPD panel
  EPD.setFactor(temperature); // adjust for current temperature
  EPD.frame_cb_repeat(0, SD_reader, EPD_compensate);
  EPD.frame_cb_repeat(0, SD_reader, EPD_white);
  // EPD.end();   // power down the EPD panel
  
}
void draw_img(File imgFile){
  int temperature = S5813A.read();
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" Celcius");

  //*** maybe need to ensure clock is ok for EPD
  // set_spi_for_epd();

  // EPD.begin(); // power up the EPD panel
  // EPD.setFactor(temperature); // adjust for current temperature
  EPD.frame_cb_repeat(0, SD_reader, EPD_inverse);
  EPD.frame_cb_repeat(0, SD_reader, EPD_normal);
  EPD.end();   // power down the EPD panel
  
}

// main loop
unsigned long int loop_count = 0;
unsigned long int last_loop_time = 0;
void loop() {
  
  next_image();
  EPD.end();
  Serial.println("UNPLUG!");
  delay(10000);

}

void SD_reader(void *buffer, uint32_t address, uint16_t length){
  byte *my_buffer = (byte *)buffer;
  unsigned short my_width;
  unsigned short my_height;
  uint32_t my_address; 
  
  
  imgFile.seek(0);
  my_height = (unsigned short)imgFile.read();
  my_height += (unsigned short)imgFile.read() << 8;
  my_width = (unsigned short)imgFile.read();
  my_width += (unsigned short)imgFile.read() << 8;

  // compensate for long/short files widths
  my_address = (address * my_width) / EPD_WIDTH;
  // Serial.print(imgFile.name());
  // Serial.print(" my width: ");
  // Serial.println(my_width);
  if((my_address * 8 / my_width) < my_height){
    imgFile.seek(my_address + 4);
    for(int i=0; i < length && i < my_width / 8; i++){
      *(my_buffer + i) = imgFile.read();
    }
    // fill in rest zeros
    for(int i=my_width / 8; i < length; i++){
      *(my_buffer + i) = 0;
    }      
  }
  else{
    for(int i=0; i < length; i++){
      *(my_buffer + i) = 0;
    }
  }
  //*** add SPI reset here as
  //*** file operations above may have changed SPI mode
}

/*
 * SD card reader Comply with EPD_reader template
 *
 * buffer -- write buffer for outgoing bytes
 * address -- byte index, 0 to display height
 * length -- number of bytes to read
 */

