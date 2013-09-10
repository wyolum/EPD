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

#include <Wire.h>
const uint8_t SPLASH = 1;
const uint8_t ASCII  = 2;
const uint8_t UNICODE = 3;
const uint8_t IMAGE = 4;
const uint8_t LINE = 5;
const uint8_t RECTANGLE = 6;
const uint8_t ELLIPSE = 7;
const uint8_t CD = 8;
const uint8_t CLEAR = 9;
const uint8_t SHOW = 16;
const uint16_t WIDTH = 264;

uint16_t UNICODE_MSG[11] = {25105, 20204, 29233, 30717, 36882, 31185, 25216, 0};


bool ready(){
  uint8_t w;
  bool out = false;
  Wire.requestFrom(4, 1);    // request 6 bytes from slave device #2
  if(Wire.available()){
    w = Wire.read();
    if(w && 0b00000001){
      out = true;
    }
  }
  return out;
}

void splash(){
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(SPLASH);
  Wire.endTransmission();    // stop transmitting
}
void ascii(uint16_t x, uint16_t y, char *msg){
  uint16_t xy = y * WIDTH + x;
  
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(ASCII);
  Wire.write(xy & 0xFF);
  Wire.write(xy >> 8);
  Wire.write(true);
  Wire.write(msg);
  Wire.endTransmission();    // stop transmitting
}

void unicode(uint16_t x, uint16_t y, uint16_t *msg){
  uint16_t xy = y * WIDTH + x;
  
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(UNICODE);
  Wire.write(xy & 0xFF);
  Wire.write(xy >> 8);
  Wire.write(true);
  Wire.write((char*)msg);
  Wire.endTransmission();    // stop transmitting
}

void image(uint16_t x, uint16_t y, char *path){
  uint16_t xy = y * WIDTH + x;
  
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(IMAGE);
  Wire.write(xy & 0xFF);
  Wire.write(xy >> 8);
  Wire.write(path);
  Wire.endTransmission();    // stop transmitting
}

void line(uint16_t x, uint16_t y, uint16_t x1, uint16_t y1, bool color){
  uint16_t xy = y * WIDTH + x;
  uint16_t xy1 = y1 * WIDTH + x1;
  
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(LINE);
  Wire.write(xy & 0xFF);
  Wire.write(xy >> 8);
  Wire.write(xy1 & 0xFF);
  Wire.write(xy1 >> 8);
  Wire.write(color);
  Wire.endTransmission();    // stop transmitting
}

void rect(uint16_t x, uint16_t y, 
	  uint16_t x1, uint16_t y1, 
	  bool color, bool fill){
  uint16_t xy = y * WIDTH + x;
  uint16_t xy1 = y1 * WIDTH + x1;
  
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(RECTANGLE);
  Wire.write(xy & 0xFF);
  Wire.write(xy >> 8);
  Wire.write(xy1 & 0xFF);
  Wire.write(xy1 >> 8);
  Wire.write(color);
  Wire.write(fill);
  Wire.endTransmission();    // stop transmitting
}

void oval(uint16_t x, uint16_t y, 
	  uint16_t x1, uint16_t y1, 
	  bool color, bool fill){
  uint16_t xy = y * WIDTH + x;
  uint16_t xy1 = y1 * WIDTH + x1;
  
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(ELLIPSE);
  Wire.write(xy & 0xFF);
  Wire.write(xy >> 8);
  Wire.write(xy1 & 0xFF);
  Wire.write(xy1 >> 8);
  Wire.write(color);
  Wire.write(fill);
  Wire.endTransmission();    // stop transmitting
  Serial.println("Done");
}

void clear(){
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(CLEAR);
  Wire.endTransmission();    // stop transmitting
}

void show(){
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(SHOW);
  Wire.endTransmission();    // stop transmitting
}

void setup(){
  Wire.begin(); // no address for bus master
  Serial.begin(115200);
}

uint16_t x, y;

void loop(){
  Serial.println("loop()");
  delay(500);
  char c;
  while(!ready()){
    delay(100);
    Serial.println("not ready");
  }
  if(Serial.available()){
    c = Serial.read();
    if(c == 's'){
      splash();
      Serial.println("Splash!");
    }
    else if(c == 'c'){
      clear();
      Serial.println("clear");
    }
    else if(c == 't'){
      ascii(x, y, "Hello World");
      Serial.println("Hello");
    }
    else if(c == 'u'){
      unicode(x, y, UNICODE_MSG);
      Serial.println("Unicode");
    }
    else if(c == 'i'){
      image(50, 25, "SPLASH.WIF");
      Serial.println("image");
    }
    else if(c == 'l'){
      line(50, 25, 0, 100, true);
      Serial.println("line");
    }
    else if(c == 'r'){
      rect(50, 25, 100, 50, true, true);
      Serial.println("rect");
    }
    else if(c == 'o'){
      oval(50, 25, 100, 50, true, true);
      Serial.println("oval");
    }
    else if(c == 'S'){
      show();
      Serial.println("show");
    }
    else if(c == 'r'){
      if(ready()){
	Serial.println("ready");
      }
      else{
	Serial.println("not ready");
      }
    }
    while(Serial.available()){
      Serial.read();
    }
  }
}

void XY(uint16_t *xy_p, uint16_t *x_p, uint16_t *y_p){
  uint16_t xy = *xy_p;

  *x_p = xy % WIDTH;
  *y_p = xy / WIDTH;
}
