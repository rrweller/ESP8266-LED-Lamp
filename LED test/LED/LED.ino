#include <Adafruit_NeoPixel.h>
 
#define PIN     D2
#define N_LEDS  24
 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRB + NEO_KHZ800);
  byte red1;
  byte red2;
  byte green1;
  byte green2;
  byte blue1;
  byte blue2;
 
void setup() {
  strip.begin();
}
 
void loop() {
  strip.clear();
  doubleSnake();
  fullBlink();
  rainbowFill();
  fullBlink();
}

void doubleSnake()
{
    byte randLoop = random(1,5);
  for(int i2 = 0; i2 < randLoop; i2++)
  {
    red1 = random(0,255);
    red2 = random(0,255);
    green1 = random(0,255);
    green2 = random(0,255);
    blue1 = random(0,255);
    blue2 = random(0,255);
    for(uint16_t i=0; i<strip.numPixels()+40; i++) {
      strip.setPixelColor(i, red1, green1, blue1); // Draw new pixel
      strip.setPixelColor(i-40, 0); // Erase pixel a few steps back
      strip.setPixelColor(300-i, red2, green2, blue2); // Draw new pixel
      strip.setPixelColor(300-(i-40), 0); // Erase pixel a few steps back
      strip.show();
      delay(3);
    }
  }
}

void fullBlink()
{
 uint32_t rgbcolor = strip.ColorHSV(random(0,65536), random(0,255), 255);
 strip.fill(rgbcolor, 1, 300);
 strip.show();
 delay(750);
 strip.clear();
}

void rainbowFill()
{
  for(int i = 0; i < 19; i++)
  {
    red1 = random(0,255);
    green1 = random(0,255);
    blue1 = random(0,255);
    for(int i2 = 1+i; i2 < strip.numPixels(); i2 += 20)
    {
      strip.setPixelColor(i2, red1, green1, blue1);
      strip.show();
      delay(10);
    }
  }
}
 
