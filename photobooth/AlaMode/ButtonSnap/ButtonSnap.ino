#include <Adafruit_NeoPixel.h>
#define PIN A0

/* Fun rainbow values
20145,32385,44625,56865,848640,3982080,7115520,10248960,13382400,16515840,13762605,10616925,7471245,4325565,1179885,7905,

9661440,12794880,15928320,14352420,11206740,8061060,4915380,1769700,5610,17850,30090,42330,54570,261120,3394560,6528000,

16121865,12976185,9830505,6684825,3539145,393465,10965,23205,35445,47685,59925,1632000,4765440,7898880,11032320,14165760,

*/
uint32_t rainbow[] = {16121865,12976185,9830505,6684825,3539145,393465,10965,23205,35445,47685,59925,1632000,4765440,7898880,11032320,14165760
};
int ButtonPin = 2;
long PhotoDelay = 5000;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(19200);
  pinMode(ButtonPin, INPUT);
  digitalWrite(ButtonPin, HIGH); //turn on pullups
  strip.begin();
  strip.show();

}

void loop() {
  if (digitalRead(ButtonPin) == LOW)
  {
    Serial.println("snap");// send to Pi
    StartCountdown(PhotoDelay/1000); //Start blinky Lights
    // The Pi will be processing the image for a while. Could add
    // red green ready light/strip
    delay (2000); // allow pi to process the previous image
  }

}

void StartCountdown(int PhotoDelay)
{
  // synchronously wait and tick the lights off (they shouldn't be able 
  // to press the button now anyway (maybe check for cancel?)
  for (int x =0; x< 16; x++)
  {  
    strip.setPixelColor(x,rainbow[x]);
  }
  strip.show();

 for (int i = 0; i < PhotoDelay; i++)
  {
    delay(1000);
    strip.setPixelColor(i*3,0);
    strip.setPixelColor(i*3+1,0);
    strip.setPixelColor(i*3+2,0);
    strip.show();
  }
  strip.setPixelColor(15,0);
  strip.show();
}
