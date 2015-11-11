#include <SPI.h>
#include <SD.h>
#include "EReader.h"

// I/O setup
void setup() {
  Serial.begin(115200);
  Serial.println("WyoLum, LLC 2013");
  Serial.println("Buy Open Source!!");
  Serial.print("SD Chip Select PIN: ");
  Serial.println(SD_CS, DEC);
  ereader.setup(EPD_2_7);
}

unsigned long int loop_count = 0;
const int N_DIGIT = 10;
const int PIXEL_PER_CHAR = 18;
const int START_STOP = 10;
const int START_X = 25;
const int HOLD_MS = 10000;

uint8_t code39[] = {
  1, 1, 1, 3, 3, 1, 3, 1, 1, // 0
  3, 1, 1, 3, 1, 1, 1, 1, 3, // 1
  1, 1, 3, 3, 1, 1, 1, 1, 3, // 2
  3, 1, 3, 3, 1, 1, 1, 1, 1, // 3
  1, 1, 1, 3, 3, 1, 1, 1, 3, // 4
  3, 1, 1, 3, 3, 1, 1, 1, 1, // 5
  1, 1, 3, 3, 3, 1, 1, 1, 1, // 6
  1, 1, 1, 3, 1, 1, 3, 1, 3, // 7
  3, 1, 1, 3, 1, 1, 3, 1, 1, // 8
  1, 1, 3, 3, 1, 1, 3, 1, 1, // 9
  1, 3, 1, 1, 3, 1, 3, 1, 1  // *
};

uint8_t encode_val(uint8_t val, int x, int y0, int y1){
  uint8_t width;
  for(uint8_t j = 0; j < 9; j++){
    width = code39[val * 9 + j];
    if (j % 2 == 0){ // only draw black lines      
      ereader.draw_vline(x, y0, y1, true, width);
    }
    x += width;
  }
  return PIXEL_PER_CHAR;
}

void loop(){
  unsigned long int val;
  uint16_t x = START_X;

  Serial.println("start of loop()");
  ereader.wake();

  x += encode_val(START_STOP, x, 0, 100);
  for(uint8_t i = 0; i < N_DIGIT; i++){
    val = random(0, 10);
    ereader.put_char(x + 9, 110, '0' + val, true);
    x += encode_val(val, x, 0, 100);
  }
  x += encode_val(START_STOP, x, 0, 100);
  ereader.show();
  ereader.sleep(HOLD_MS);
}
