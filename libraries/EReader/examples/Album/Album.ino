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
#include <avr/interrupt.h>
#include <ctype.h>
#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/power.h> //Needed for powering down perihperals such as the ADC/TWI and Timers

#include <SPI.h>
#include <SD.h>
#include "EPD_V2.h"
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

// these are involved in determining when we go to sleep
// after a certain number of milliseconds with no button presses go to sleep.
// if a button is pressed, it resets the timer.
long lastWakeTime; //reset with every interaction
#define AWAKETIME 80000 // how long to stay awake
#define FOCUSTIME 12250 // how long keep optimal responsiveness

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
  SD.begin(SD_CS); // hack to wake SD up after detach.  returns false but seems to work.
  get_cwd_path(); // copy current dir path to path
  cwd.close();
  cwd = SD.open(path);
  n_wif = count_wifs(cwd);
}

void next_dir(){
  current_dir++;
  current_dir %= n_dir;
  open_cwd();
  // if(current_wif > n_wif - 1){
  current_wif = 0;
  //}
}
void prev_dir(){
  current_dir--;
  if(current_dir < 0){
    current_dir = n_dir - 1;
  }
  open_cwd();
  //if(current_wif > n_wif - 1){
  current_wif = 0;
  //}
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

void setup() {
  int n = strlen(ROOT_DIR);
  bool done = false;

  // Serial.begin(115200);
  // Serial.println("WyoLum, LLC 2013");
  // Serial.println("Buy Open Source Hardware!");
  ereader.setup(EPD_2_7); // starts SD
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(SEL_PIN, INPUT);
  pinMode(MODE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  root = SD.open(ROOT_DIR);
  if(!root){
    //Serial.print("Root not found:\n    ");
    //Serial.println(ROOT_DIR);
    ereader.error(SD_ERROR_CODE);
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

  for(int ii=0; ii < 3; ii++){
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  // ereader.spi_detach();
}


void ser_interact(){
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
  digitalWrite(LED_PIN, LOW);
  ereader.spi_attach();  
  erase_img(wif); // wif is still the old file
  wif.close();    // keep close and open calls in the same spot
  get_wif_path(); // store new wif name in path
  wif = SD.open(path); 
  draw_img(wif); // draw new wif
  uint16_t start = millis();
  // ereader.spi_detach(); // this call takes .8 seconds to execute!
  if(true){
    for(int ii=0; ii<4; ii++){
      digitalWrite(LED_PIN, ii % 2 == 0 );
      if(ii < 3){
	delay(25);
      }
    }
  }
}

// main loop
unsigned long int loop_count = 0;
void loop() {
  bool update_needed = false;
  int pulse = millis() % (2000 * (2 - ereader.attached));
  if(pulse  < 50){
    digitalWrite(LED_PIN, HIGH);
  }
  else if(150 < pulse && pulse < 250){
    digitalWrite(LED_PIN, HIGH);
  }
  else{
    digitalWrite(LED_PIN, LOW);
  }
  long current = millis();
  if ((current - lastWakeTime) > FOCUSTIME && ereader.attached){
    digitalWrite(LED_PIN, LOW);
    ereader.spi_detach(); // this call takes .8 seconds to execute!
    // Serial.println("ereader detached");
  }
  else if ((current - lastWakeTime) > AWAKETIME){
    //   Serial.println("should sleep");
    goToSleep();
  }
  

  // ser_interact();
  if(analogRead(MODE_PIN) > 512){
//    Serial.println("Mode");
    prev_wif();
    update_needed = true;
  }
  if(digitalRead(SEL_PIN)){
//    Serial.println("Sel");
    next_wif();
    update_needed = true;
  }
  if(digitalRead(UP_PIN)){
//    Serial.println("Up");
    prev_dir();
    update_needed = true;
  }
  if(digitalRead(DOWN_PIN)){
//     Serial.println("down");
    next_dir();
    update_needed = true;
  }
  if(update_needed){
    lastWakeTime = current;
    display();
  }
  else{
  }
}

void draw_img(File imgFile){
  /*
    int temperature = S5813A.read();
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" Celcius");
  */

  //*** maybe need to ensure clock is ok for EPD

  reset_wif();
  ereader.EPD->frame_cb(0, SD_reader, EPD_inverse);
  reset_wif();
  ereader.EPD->frame_cb(0, SD_reader, EPD_normal);
  //ereader.EPD->end();   // power down the EPD panel
  
}

void erase_img(File imgFile){

  //*** maybe need to ensure clock is ok for EPD

  ereader.EPD->begin(); // power up the EPD panel
  reset_wif();
  ereader.EPD->frame_cb(0, SD_reader, EPD_compensate);
  reset_wif();
  ereader.EPD->frame_cb(0, SD_reader, EPD_white);
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

void goToSleep(){
  digitalWrite(LED_PIN, LOW);
//  ereader.spi_detach();
  // ereader.EPD->end();   // make sure EPD panel is off
  delay(500);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(0,wake,RISING); // pin 2 intterupt UP button on V3 
  attachInterrupt(1,wake,RISING);// pin 3 interrupt  SEL button on V3

 #ifndef COMPLICATED
 //Shut off ADC, TWI, SPI, Timer0, Timer1
 // ADCSRA &= ~(1<<ADEN); //Disable ADC
 // ACSR = (1<<ACD); //Disable the analog comparator
 // DIDR0 = 0x3F; //Disable digital input buffers on all ADC0-ADC5 pins
 // DIDR1 = (1<<AIN1D)|(1<<AIN0D); //Disable digital input buffer on AIN1/0
  
  power_twi_disable();
//  power_spi_disable();
  power_usart0_disable();
//  power_timer0_disable(); //Needed for delay_ms and apparently for pin interrupts
  power_timer1_disable();
  power_timer2_disable(); 
 #endif
  sleep_mode(); // this immediately goes to sleep
  // and when we wake up, we will execute this
  detachInterrupt(0);// we don't want to keep getting interrupted when waking
  detachInterrupt(1);

  sleep_disable();
  power_twi_enable();
//  power_spi_enable();
  power_usart0_enable();
//  power_timer0_enable(); //Needed for delay_ms
  power_timer1_enable();
  power_timer2_enable(); 

  //Serial.println("returning from sleep");
  lastWakeTime = millis();


}
void wake()
{
  // apparently the uart takes a while to reconfigure on waking up
  // Serial.begin(115200);
  delay(100); // crude debounce
}

