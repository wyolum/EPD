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

#include "EReader.h"
#include "EPD_v2.h"
#include "S5813A.h"

EReader::EReader(){
  initialized = false;
  attached = false;
}

void EReader::reader(void *buffer, uint32_t address, uint16_t length){
  byte *my_buffer = (byte *)buffer;
  uint32_t offset = pingpong * epd_bytes;

  SPI.setClockDivider(SPI_CLOCK_DIV2);

  display_file.seek(offset + address); // TODO: don't set address each time for sequential reads?
  for(uint16_t i=0; i < length; i++){
    my_buffer[i] = display_file.read();
  }
  SPI.setClockDivider(SPI_CLOCK_DIV4);
}

void EReader::error(int code_num){
  char *error_msg = "ERROR_CODE: ";

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.print(error_msg);
  Serial.println(code_num);
  ereader.put_ascii(0, 0, error_msg, BLACK);
  ereader.put_char(0, 16, '0' + code_num, BLACK);
  if(initialized){
    ereader.show();
  }
  ereader.spi_detach();
  while(1){
    for(int i=0; i < code_num + 3; i++){
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
    delay(1000);
  }
}

/**
 * \brief Configure SPI and initialization
 */
void EReader::spi_attach(){	
  if(!attached){
    SPI.begin();
    pinMode(SCK, OUTPUT);
    pinMode(MOSI, OUTPUT);	
    pinMode(MISO, INPUT);
    set_spi_for_epd();
    EPD->begin(); // power up the EPD panel	
    attached = true;
  }
}

/**
 * \brief Disable SPI, change to GPIO and set LOW
 */
void EReader::spi_detach(){
  if(attached){
    EPD->end();
    SPI.end();
    pinMode(SCK, OUTPUT);
    pinMode(MOSI, OUTPUT);
    pinMode(MISO, OUTPUT);
    digitalWrite(SCK, LOW);
    digitalWrite(MOSI, LOW);
    digitalWrite(MISO, LOW);
    attached = false;
  }
}

// call in arduino setup function
void EReader::setup(EPD_size size){
  attached = false;
  pinMode(SD_CS, OUTPUT);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD initialization failed!!");
    Serial.print("SD_CS");
    Serial.println(SD_CS, DEC);
    
    // while(1); delay(100);
  }

  EPD = new EPD_Class(size, EPD_PANEL_ON, EPD_BORDER, EPD_DISCHARGE, EPD_RESET, EPD_BUSY, EPD_EPD_CS);

  if(size == EPD_1_44){
    epd_width = 128L;
    epd_height = 96L;
  }
  else if(size == EPD_2_0){
    epd_width = 200L;
    epd_height = 96L;
  }
  else{
    epd_width = 264L;
    epd_height = 176L;
  }
  epd_bytes = (epd_width * epd_height / 8);


  pinMode(EPD_PWM, OUTPUT);
  pinMode(EPD_BUSY, INPUT);
  pinMode(EPD_RESET, OUTPUT);
  pinMode(EPD_PANEL_ON, OUTPUT);
  pinMode(EPD_DISCHARGE, OUTPUT);
  pinMode(EPD_BORDER, OUTPUT);
  pinMode(EPD_EPD_CS, OUTPUT);
  pinMode(EPD_FLASH_CS, OUTPUT);

  digitalWrite(EPD_PWM, LOW);
  digitalWrite(EPD_RESET, LOW);
  digitalWrite(EPD_PANEL_ON, LOW);
  digitalWrite(EPD_DISCHARGE, LOW);
  digitalWrite(EPD_BORDER, LOW);
  digitalWrite(EPD_EPD_CS, LOW);
  digitalWrite(EPD_FLASH_CS, HIGH);
  
  SPI.begin();
  set_spi_for_epd();

  display_file = SD.open("__EPD__.DSP", FILE_WRITE);
  if(!display_file){
    Serial.println("Could not open file: __EPD__.DSP");
    error(FILE_NOT_FOUND_CODE);
  }    
  unifont_file = SD.open("unifont.wff");
  if(!unifont_file){
    Serial.println("Could not open file: unifont.wff");
    error(FILE_NOT_FOUND_CODE);
  }    


  // _erase();
  pingpong = false;
  clear();
  pingpong = true;
  clear();
  initialized = true;
}

// clear the display
void EReader::clear(){
  display_file.seek(pingpong * epd_bytes);
  for(uint32_t pos = 0; pos < epd_bytes; pos++){
    display_file.write((byte)0);
  }
}

// display a WIF image at x, y
bool EReader::display_wif(char *path, int16_t x, int16_t y){
  bool out = false;

  /*copy image to next screen buffer*/
  char buff[epd_width / 8];
  unsigned short img_width;
  unsigned short img_height;
  int16_t i, j;
  int16_t xstart, xend, ystart, yend;
  int16_t cursor_pos;

  bool my_display = !pingpong;
  uint32_t pos = my_display * epd_bytes;
  File imgFile = SD.open(path);

  if(!imgFile){
    Serial.print("Could not open file: ");
    Serial.println(path);
  }
  out = true;
  display_file.seek(pos);

  SD_image_dims(imgFile, &img_height, &img_width);

  xstart = x < 0? 0:x;
  xend = x + img_width;
  if(xend > epd_width){
    xend = epd_width;
  }
 
  ystart = y < 0? 0:y;
  yend = y + img_height;
  if(yend > epd_height){
    yend = epd_height;
  }
  for(i = ystart; i < yend; i++){
    SD_image_reader(imgFile, buff, (i - y) * epd_width / 8, epd_width / 8);
    cursor_pos = pos + (xstart + i * epd_width) / 8;
    if(((my_display * epd_bytes) < cursor_pos) && 
       (cursor_pos < (epd_bytes * (my_display + 1)))){
      display_file.seek(cursor_pos);
      for(j = xstart / 8; j < xend / 8; j++){
	display_file.write(buff[j - x / 8]);
      }
    }
  }
  imgFile.close();
  return out;
}

// toggle a pixel at x, y
void EReader::togglepix(uint16_t x, uint16_t y){
  // toggle pixel located at x, y
  bool my_display = !pingpong;
  byte dat;
  uint8_t bit_idx = x % 8;
  uint32_t pos = my_display * epd_bytes + y * epd_width / 8 + x / 8;
  if(x / 8 < epd_width && y / 8 < epd_height){
    display_file.seek(pos);
    dat = display_file.read();
    if((dat >> bit_idx) & 1){
      dat &= ~(1 << bit_idx);
    }
    else{
      dat |= (1 << bit_idx);
    }
    display_file.seek(pos);
    display_file.write(dat);
  }
}


// set a pixel to a value
void EReader::setpix(uint16_t x, uint16_t y, bool val){
  // toggle pixel located at x, y
  bool my_display = !pingpong;
  byte dat;
  uint8_t bit_idx = x % 8;
  uint32_t pos = my_display * epd_bytes + y * epd_width / 8 + x / 8;
  if(x / 8 < epd_width && y < epd_height){
    display_file.seek(pos);
    dat = display_file.read();
    if(val){
      dat |= (1 << bit_idx);
    }
    else{
      dat &= ~(1 << bit_idx);
    }
    display_file.seek(pos);
    display_file.write(dat);
  }
}
  
// display a line from start to stop. toggle each pix on line
void EReader::toggle_line(int16_t startx, int16_t starty, int16_t stopx, int16_t stopy){
  float dx = (stopx - startx);
  float dy = (stopy - starty);
  int16_t l = sqrt(dx * dx + dy * dy);
  int16_t x, y, lastx, lasty;

  lastx = lasty = -1; // impossible value
  for(uint16_t t = 0; t < l; t++){
    x = startx + dx * t / l;
    y = starty + dy * t / l;
    if(lastx != x || lasty != y){ // don't write to a pix more than once (oops)
      togglepix(x, y);
    }
    lastx = x;
    lasty = y;
  }
}

// display a line from start to stop in specified color: true=black, false=white
void EReader::draw_line(int16_t startx, int16_t starty, int16_t stopx, int16_t stopy, bool color){
  float dx = (stopx - startx);
  float dy = (stopy - starty);
  int16_t l = sqrt(dx * dx + dy * dy);
  int16_t x, y, lastx, lasty;

  lastx = lasty = -1; // impossible value
  for(uint16_t t = 0; t < l; t++){
    x = startx + dx * t / l;
    y = starty + dy * t / l;
    if(lastx != x || lasty != y){ // don't write to a pix more than once (oops)
      setpix(x, y, color); 
    }
    lastx = x;
    lasty = y;
  }
}
  
// display a line from start to stop in specified color: true=black, false=white
void EReader::draw_vline(int16_t x, int16_t starty, int16_t stopy, bool color, uint8_t thickness){
  float dy = (stopy - starty);

  for(int16_t y = starty; y < stopy; y++){
    for(int16_t i = 0; i < thickness; i++){
      setpix(x + i, y, color);
    }
  }
}
  
// draw an ellipse centered at cx, cy with horizontal radius rx and vertical radius ry
// toggle each pix on ellipse
void EReader::toggle_ellipse(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, bool fill){
  float x, y;
  int xint, yint;
  uint8_t xbit;
  
  if(fill){
    for(y = cy - ry; y <= cy + ry; y++){
      x = cx - sqrt(rx * rx * (1 - y * y / (ry * ry)));
      xint = (int)round(x);
      xbit = xint % 8;
      
      for(uint8_t b=xbit; b < 8; b++){
      }
	
    }
  }
  else{
    toggle_ellipse(cx, cy, rx, ry);
  }
}

// draw an ellipse centered at cx, cy with horizontal radius rx and vertical radius ry
// toggle each pix on ellipse
void EReader::toggle_ellipse(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry){
  float step = atan(min(1./rx, 1./ry));
  int16_t x, y, lastx, lasty;

  lastx = lasty = -1; // impossible value

  for(float theta = 0; theta < 2 * PI; theta+=step){
    x = rx * cos(theta) + cx;
    y = ry * sin(theta) + cy;
    if(lastx != x || lasty != y){ // don't write to a pix more than once (oops)
      togglepix(x, y);
    }
    lastx = x;
    lasty = y;
  }
}

// draw an ellipse centered at cx, cy with horizontal radius rx and vertical radius ry
// in specified color: true=black, false=white
void EReader::draw_ellipse(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, bool color, bool fill){
  float step = atan(min(1./rx, 1./ry));
  for(float theta = 0; theta < 2 * PI; theta+=step){
    setpix(rx * cos(theta) + cx, ry * sin(theta) + cy, color);
  }
}

void bitprint(uint8_t val){
  for(uint8_t i=0; i < 8; i++){
    Serial.print((val >> i) & 1);
  }
  Serial.println();
}
// draw a box at specified starting and ending corners.  Start is upper left, end is lower right
void EReader::draw_box(uint16_t startx, uint16_t starty, uint16_t endx, uint16_t endy, bool color, bool fill){
  uint32_t pos, y, x8;
  uint8_t fill_byte = 0b11111111 * color;
  uint8_t start_byte, end_byte, n_fill, old_byte, start_i, end_i;
  bool my_display = !pingpong;
  
  if(startx > epd_width){
    startx = epd_width;
  }
  if(endx > epd_width){
    endx = epd_width;
  }
  if(starty > epd_height){
    starty = epd_height;
  }
  if(endy > epd_height){
    endy = epd_height;
  }
  start_i = startx / 8;
  end_i = endx / 8;

  if(end_i > start_i + 1){
    n_fill = end_i - start_i - 1;
  }
  else{
    n_fill = 0;
  }
  if(n_fill > 264/8){
    Serial.print("n_fill too big: ");
    Serial.println(n_fill);
  }

  start_byte = 0;
  for(uint8_t i=startx % 8; i < 8; i++){
    start_byte |= 1 << i;
  }

  end_byte = 0;
  for(uint8_t i=0; i < endx % 8; i++){
    end_byte |= (1 << i);
  }
  if(start_i == end_i){
    start_byte &= end_byte;
  }
  if(!color){
    start_byte = ~start_byte;
    end_byte = ~end_byte;
  }
  if(fill){
    for(y = starty; y < endy; y++){
      pos = my_display * epd_bytes + y * epd_width / 8 + startx / 8;
      // first byte takes some care
      display_file.seek(pos);
      old_byte = display_file.read();
      display_file.seek(pos);
      if(color){
	display_file.write(start_byte | old_byte);
      }
      else{
	display_file.write(start_byte & old_byte);
      }
      for(x8 = 0; x8 < n_fill; x8++){
	display_file.write(fill_byte);
      }
      if(start_i != end_i){
	pos = display_file.position();
	old_byte = display_file.read();
	display_file.seek(pos);
	if(color){
	  display_file.write(end_byte | old_byte);
	}
	else{
	  display_file.write(end_byte & old_byte);
	}
      }
    }
  }
}
// display new image.  Call when image is complete
void EReader::show(){
  // copy image data to old_image data
  char buffer[epd_width];

  _erase();
  // display_char(epd_width / 2, epd_height / 2, '0' + pingpong, true);
  pingpong = !pingpong;
  
  _draw();
  clear();
}

void EReader::sleep(uint32_t delay_ms){
  spi_detach();
  delay(delay_ms);
}
void EReader::wake(){
  spi_attach();
}
//***  ensure clock is ok for EPD
void EReader::set_spi_for_epd() {
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
}

void EReader::SD_image_dims(File imgFile, unsigned short *h, unsigned short *w){
  imgFile.seek(0);
  *h = (unsigned short)imgFile.read();
  *h += (unsigned short)imgFile.read() << 8;
  *w = (unsigned short)imgFile.read();
  *w += (unsigned short)imgFile.read() << 8;
}

void EReader::SD_image_reader(File imgFile, void *buffer, uint32_t address, 
		     uint16_t length){
  byte *my_buffer = (byte *)buffer;
  unsigned short my_width;
  unsigned short my_height;
  uint32_t my_address; 
  
  SD_image_dims(imgFile, &my_height, &my_width);

  // compensate for long/short files widths
  my_address = (address * my_width) / epd_width;
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

// #define SLOW

void EReader::_erase(){
  int temperature; // TJS: not used in badge (assume room temperature)
  // int temperature = S5813A.read();
  // Serial.print("Temperature = ");
  // Serial.print(temperature);
  // Serial.println(" Celcius");

  //*** maybe need to ensure clock is ok for EPD
  set_spi_for_epd();

  EPD->begin(); // power up the EPD panel
#ifdef SLOW
  EPD->setFactor(temperature); // adjust for current temperature
  EPD->frame_cb_repeat(0, reader_wrap, EPD_compensate);
  EPD->frame_cb_repeat(0, reader_wrap, EPD_white);  
#else
  EPD->frame_cb(0, reader_wrap, EPD_compensate);
  EPD->frame_cb(0, reader_wrap, EPD_white);  
#endif
}

void EReader::_draw(){
  //*** maybe need to ensure clock is ok for EPD
  set_spi_for_epd();

#ifdef SLOW
  EPD->frame_cb_repeat(0, reader_wrap, EPD_inverse);
  EPD->frame_cb_repeat(0, reader_wrap, EPD_normal);
#else
  EPD->frame_cb(0, reader_wrap, EPD_inverse);
  EPD->frame_cb(0, reader_wrap, EPD_normal);
#endif
  // EPD->end();   // power down the EPD panel
}

uint16_t EReader::put_char(uint16_t x, uint16_t y, uint16_t unic, bool color){
  uint32_t pos;
  uint8_t char_width;
  uint8_t out = 16;

  if(x < epd_width && y < epd_height){
    char_width = unifont_read_char(unifont_file, unic, unifont_data) / 16;
    for(uint16_t i = 0; i < 16; i++){
      display_file.seek((i + y) * epd_width / 8 + x / 8 + (!pingpong) * epd_bytes);
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

uint8_t EReader::unifont_read_char(File unifont_file, uint32_t i, uint8_t *dest){
  uint8_t n_byte;
  unifont_file.seek(i * UNIFONT_RECLEN);
  n_byte = (uint8_t)unifont_file.read();

  for(uint8_t i = 0; i < n_byte; i++){
    dest[i] = (uint8_t)unifont_file.read();
  }
  return n_byte;
}

bool EReader::char_is_blank(uint32_t unic){
  bool out = true;
  uint8_t n_byte;
  uint8_t i;
  n_byte = unifont_read_char(unifont_file, unic, unifont_data);
  for(i = 0; i < n_byte && out; i++){
    if(unifont_data[i] > 0){
      out = false;
    }
  }
  return out;
}
/*
  put ASCII string at location x, y
 */
uint16_t EReader::put_ascii(uint16_t x, uint16_t y, char * ascii, bool color){
  for(uint8_t i = 0; ascii[i] > 0; i++){
    x += put_char(x, y, ascii[i], color);
  }
  return x;
}

/*
  put ASCII string in 16x16 font at location x, y
 */
uint16_t EReader::put_bigascii(uint16_t x, uint16_t y, char *ascii, bool color){
  for(uint8_t i = 0; ascii[i] > 0; i++){
    x += put_char(x, y, (uint16_t)ascii[i] + BIGTEXT_OFFSET, color);
  }
  return x;
}

uint16_t EReader::put_unicode(uint16_t x, uint16_t y, uint16_t *unicode, bool color){
  for(uint8_t i = 0; unicode[i] > 0; i++){
    x += put_char(x, y, unicode[i], color);
  }
  return x;
}

void reader_wrap(void *buffer, uint32_t address, uint16_t length){
  ereader.reader(buffer, address, length);
}


EReader ereader;
