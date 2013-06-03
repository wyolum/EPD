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

#include <inttypes.h>
#include <ctype.h>

#include <SPI.h>
#include <SD.h>
#include "EPD.h"
#include "S5813A.h"
#include "EReader.h"

// globals

uint16_t UNICODE_MSG[11] = {25105, 20204, 29233, 30717, 36882, 31185, 25216, 0};
const char* DEFAULT_WIF = "DEFAULT.WIF";
const char* ROOT_DIR = "ALBUM/";
char path[20];
const uint8_t CMD_BUF_LEN = 10;
const uint8_t cmd_pending = 0;
char cmd_buffer[CMD_BUF_LEN];

int current_wif = 0;
int current_dir = -1;
int n_dir;
int n_wif;
File root;
File cwd;
File wif;
bool update = false;
unsigned short my_width;
unsigned short my_height;

/*
  increment image number by one
 */
void next_wif(){
  current_wif++;
  current_wif %= n_wif;
}

/*
  decrement image number by one
 */
void prev_wif(){
  current_wif--;
  if(current_wif < 0){
    current_wif = n_wif - 1;
  }
}

void open_cwd(){
  get_cwd_path(); // copy current dir path to path
  cwd.close();
  cwd = SD.open(path);
  n_wif = count_wifs(cwd);
}

void next_dir(){
  current_dir++;
  current_dir %= n_dir;
  open_cwd();
  if(current_wif > n_wif - 1){
    current_wif = 0;
  }
}
void prev_dir(){
  current_dir--;
  if(current_dir < 0){
    current_dir = n_dir - 1;
  }
  open_cwd();
  if(current_wif > n_wif - 1){
    current_wif = 0;
  }
}

/*
  store current working directory int path
 */
char *get_cwd_path(){
  int n = strlen(ROOT_DIR);
  for(int i=0; i < n; i++){
    path[i] = ROOT_DIR[i];
  }
  path[n - 1] = '/';
  path[n] = 'A' + current_dir;
  path[n+1] = '/';
  path[n+2] = 0;
  // Serial.print("get_cwd_path path:");
  // Serial.println(path);
}

/*
  store current wif file into path
 */
void *get_wif_path(){
  get_cwd_path();
  int n = strlen(path);

  path[n] = 'A' + current_wif;
  path[n + 1] = 0;
  // Serial.print("get_wif_path path:");
  // Serial.println(path);

  strcat(path, ".WIF");
  if(!SD.exists(path)){
    char *msg = "FILE NOT FOUND";
    ereader.put_ascii(10, 50, msg, true);
    ereader.put_ascii(20, 50, path, true);
    ereader.show();
  }
}

bool isWIF(File f){
  char *name = f.name();
  bool out = false;
  int n = strlen(name);

  if(!f.isDirectory()){
    out =  (name[n-4] == '.' &&
	    name[n-3] == 'W' &&
	    name[n-2] == 'I' &&
	    name[n-1] == 'F');
  }
  return out;
}

int count_wifs(File dir){
  File f;
  int out = 0;

  if(dir.isDirectory()){
    dir.rewindDirectory();
    while(f = dir.openNextFile()){
      // Serial.println(f.name());
      if(isWIF(f)){
	out++;
      }
      f.close();
    }
  }
  return out;
}

// I/O setup
const int UP_PIN = 17;
const int DOWN_PIN = 15;
const int SEL_PIN = 16;
const int MODE_PIN = A6;

void setup() {
  int n = strlen(ROOT_DIR);
  bool done = false;

  Serial.begin(115200);
  Serial.println("WyoLum, LLC 2013");
  Serial.println("Buy Open Source Hardware!");
  ereader.setup(EPD_2_7); // starts SD
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(SEL_PIN, INPUT);
  pinMode(MODE_PIN, INPUT);
  root = SD.open(ROOT_DIR);
  if(!root){
    Serial.print("Root not found:\n    ");
    Serial.println(ROOT_DIR);
    while(1) delay(100);
  }
  get_cwd_path();

  n_dir = 0;
  n = strlen(ROOT_DIR); // ROOT_DIR ends with '/'

  for(n_dir=0; !done; n_dir++){
    path[n] = 'A' + n_dir;
    if(SD.exists(path)){
    }
    else{
      done = true;
      n_dir--; // overshot by one
    }
  }
  root.close();
  current_dir = -1;
  next_dir();
  display();
}


void interact(){
  char c;
  update = false;
  if(Serial.available()){
    while(Serial.available()){
      c = Serial.read();
      if(c == 'n'){
	next_wif();
	update = true;
      }
      if(c == 'N'){
	next_dir();
	update = true;
      }
      if(c == 'p'){
	prev_wif();
	update = true;
      }
      if(c == 'P'){
	prev_dir();
	update = true;
      }
    }
    display();
  }
}

void display(){
  erase_img(wif); // wif is still the old file
  wif.close();    // keep close and open calls in the same spot
  get_wif_path(); // store new wif name in path
  wif = SD.open(path); 
  draw_img(wif); // draw new wif
}

// main loop
unsigned long int loop_count = 0;
void loop() {
  interact();
  if(analogRead(MODE_PIN) > 512){
    prev_wif();
    display();
  }
  if(digitalRead(SEL_PIN)){
    next_wif();
    display();
  }
  if(digitalRead(UP_PIN)){
    prev_dir();
    display();
  }
  if(digitalRead(DOWN_PIN)){
    next_dir();
    display();
  }
  delay(10);
}

void draw_img(File imgFile){
  /*
    int temperature = S5813A.read();
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" Celcius");
  */

  //*** maybe need to ensure clock is ok for EPD
  set_spi_for_epd();

  reset_wif();
  ereader.EPD.frame_cb(0, SD_reader, EPD_inverse);
  reset_wif();
  ereader.EPD.frame_cb(0, SD_reader, EPD_normal);
  ereader.EPD.end();   // power down the EPD panel
  
}

void erase_img(File imgFile){

  //*** maybe need to ensure clock is ok for EPD
  set_spi_for_epd();

  ereader.EPD.begin(); // power up the EPD panel
  reset_wif();
  ereader.EPD.frame_cb(0, SD_reader, EPD_compensate);
  reset_wif();
  ereader.EPD.frame_cb(0, SD_reader, EPD_white);
}

void reset_wif(){
  wif.seek(0);
  my_height = (unsigned short)wif.read();
  my_height += (unsigned short)wif.read() << 8;
  my_width = (unsigned short)wif.read();
  my_width += (unsigned short)wif.read() << 8;
}
void SD_reader(void *buffer, uint32_t address, uint16_t length){
  byte *my_buffer = (byte *)buffer;
  uint32_t my_address;   

  // compensate for long/short files widths
  my_address = (address * my_width) / ereader.epd_width;
  // Serial.print(wif.name());
  // Serial.print(" my width: ");
  // Serial.println(my_width);
  if((my_address * 8 / my_width) < my_height){
    wif.seek(my_address + 4);
    for(int i=0; i < length && i < my_width / 8; i++){
      *(my_buffer + i) = wif.read();
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

//***  ensure clock is ok for EPD
void set_spi_for_epd() {
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);
}
