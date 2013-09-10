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
// example code written by Nicholas Zambetti was very helpful in starting this project.

// To use BADGEr as a EPaper display with another Arduino controlling.


#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include "EReader.h"

const uint16_t WIDTH = 264;
const uint16_t STATUS_NOT_READY = 0;
const uint16_t STATUS_READY = 1;

// globals
uint8_t command_buff[32];
char *SPLASH_PATH = "/SPLASH.WIF";
uint8_t status = STATUS_NOT_READY;

void setup(){
  //Serial.begin(115200);
  ereader.setup(EPD_2_7); // starts SD
  ereader.show();
  Wire.begin(4);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent); // register event
  status = STATUS_READY;
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  status = ~STATUS_READY;
}

void loop(){
  uint16_t *xy_start_p, *xy_stop_p;
  uint16_t x, y, x1, y1;
  bool color, fill;

  // handle I2C events
  switch(command_buff[0]){
  case 0: // no action
    break;
  case 1: // splash
    xy_start_p = (uint16_t*)(command_buff + 1); // [1], [2]
    ereader.display_wif(SPLASH_PATH, 0, 0);
    ereader.show();
    break;
  case 2: // ASCII
    xy_start_p = (uint16_t*)(command_buff + 1); // [1], [2]
    color = (bool)command_buff[3];
    XY(xy_start_p, &x, &y);
    ereader.put_ascii(x, y, (char*)(command_buff + 4), color);
    break;
  case 3: // unicode
    xy_start_p = (uint16_t*)(command_buff + 1); // [1], [2]
    color = (bool)command_buff[3];
    XY(xy_start_p, &x, &y);
    ereader.put_unicode(x, y, (uint16_t*)(command_buff + 4), color);
    break;
  case 4: // image
    xy_start_p = (uint16_t*)(command_buff + 1); // [1], [2]
    XY(xy_start_p, &x, &y);
    ereader.display_wif((char*)(command_buff + 3), x, y);
    break;
  case 5: // line
    xy_start_p = (uint16_t*)(command_buff + 1); // [1], [2]
    xy_stop_p = (uint16_t*)(command_buff + 3); // [3], [4]
    color = (bool)command_buff[5];

    XY(xy_start_p, &x, &y);
    XY(xy_stop_p, &x1, &y1);
    ereader.draw_line(x, y, x1, y1, color);
    break;
  case 6: // rect
    xy_start_p = (uint16_t*)(command_buff + 1); // [1], [2]
    xy_stop_p = (uint16_t*)(command_buff + 3); // [3], [4]
    color = (bool)command_buff[5];
    fill = (bool)command_buff[6];

    XY(xy_start_p, &x, &y);
    XY(xy_stop_p, &x1, &y1);
    ereader.draw_box(x, y, x1, y1, color, fill);
    break;
  case 7: // oval
    xy_start_p = (uint16_t*)(command_buff + 1); // [1], [2]
    xy_stop_p = (uint16_t*)(command_buff + 3); // [3], [4]
    color = (bool)command_buff[5];
    fill = (bool)command_buff[6];

    XY(xy_start_p, &x, &y);
    XY(xy_stop_p, &x1, &y1);
    ereader.draw_ellipse((x1 + x) / 2, (y1 + y) / 2, 
			 (x1 - x) / 2, (y1 - y) / 2, color, fill);
    break;
  case 8: // cd
    break;
  case 9: // clear
    ereader.clear();
    ereader.show();
    break;
  case 16: // show
    ereader.show();
    break;
  default:
    break;
  }
  command_buff[0] = 0; // clear command
  status = STATUS_READY; // ready for next command
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()

void receiveEvent(int howMany)
{
  uint8_t i = 0;
  if((status & 1) && Wire.available()){
    status = STATUS_NOT_READY;
    while(Wire.available()){
      command_buff[i++] = Wire.read(); // receive byte as a character
    }
  }
  else{
    while(Wire.available()){
      Wire.read();            // ignore data before ready
    }
  }
}

void requestEvent(){
  Wire.write(status);
}

void XY(uint16_t *xy_p, uint16_t *x_p, uint16_t *y_p){
  uint16_t xy = *xy_p;

  *x_p = xy % WIDTH;
  *y_p = xy / WIDTH;
}
