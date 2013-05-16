#include <inttypes.h>
#include <ctype.h>

#include <SPI.h>
#include <SD.h>
#include "EPD.h"
#include "S5813A.h"
#include "EReader.h"

uint16_t UNICODE_MSG[10];

// I/O setup
void setup() {
  Serial.begin(115200);
  ereader.setup(EPD_2_7);

  UNICODE_MSG[0] = (uint16_t)'U';
  UNICODE_MSG[1] = (uint16_t)'N';
  UNICODE_MSG[2] = (uint16_t)'I';
  UNICODE_MSG[3] = (uint16_t)'F';
  UNICODE_MSG[4] = (uint16_t)'O';
  UNICODE_MSG[5] = (uint16_t)'N';
  UNICODE_MSG[6] = (uint16_t)'T';
  UNICODE_MSG[7] = (uint16_t)' ';
  UNICODE_MSG[8] = 128L * 79L + 9 + 8;
  UNICODE_MSG[9] = 0;
}


// main loop
unsigned long int loop_count = 0;

void loop() {
  if(loop_count % 4 == 0){
    ereader.display_wif("/IMAGES/AANDJ.WIF", 0, 0);
  }
  else if(loop_count % 4 == 1){
    ereader.display_wif("/IMAGES/WYOLUM.WIF", 0, 0);
  }
  else if(loop_count % 4 == 2){
    ereader.display_wif("/IMAGES/LENA.WIF", -264/2, 0);
    ereader.display_wif("/IMAGES/LENA.WIF", 264/2, 0);
  }
  else{
    ereader.display_wif("/IMAGES/AANDJ.WIF", -264 / 2,  176 / 2);
    ereader.display_wif("/IMAGES/AANDJ.WIF",  264 / 2, -176 / 2);
    ereader.display_wif("/IMAGES/CAT_SM.WIF", 264 / 2, 176 / 2);
    ereader.display_wif("/IMAGES/APHRODIT.WIF", 0, 0);
    ereader.toggle_ellipse(random(0, 264), random(0, 176), 20, 20);
    ereader.put_ascii(random(0, 200), random(16, 150), "WyoLum ROCKS!!", true);
    ereader.setpix(128, 0, true);
    ereader.setpix(128, 2, true);
    ereader.setpix(128, 6, true);
    ereader.put_unicode(10, 140, UNICODE_MSG, true);
    ereader.toggle_line(70, 0, 120, 50);
  }
  loop_count++;
  ereader.show();
  ereader.sleep(1000);
}

