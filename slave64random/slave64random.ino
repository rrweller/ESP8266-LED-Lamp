#include <Gaussian.h>


#define FASTLED_INTERRUPT_RETRY_COUNT 0 
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include "reactive_common.h"

#define LED_PIN D2
#define NUM_LEDS 64

#define MIC_LOW 4
//#define MIC_HIGH 160
  //My phone
#define MIC_HIGH 300
  //Chromecast

#define SAMPLE_SIZE 6
#define LONG_TERM_SAMPLES 250
#define BUFFER_DEVIATION 400
#define BUFFER_SIZE 3

#define LAMP_ID 1
WiFiUDP UDP;

const char *ssid = "sound_reactive"; // The SSID (name) of the Wi-Fi network you want to connect to
const char *password = "123456789";  // The password of the Wi-Fi network

CRGB leds[NUM_LEDS];

struct averageCounter *samples;
struct averageCounter *longTermSamples;
struct averageCounter* sanityBuffer;

Gaussian sat = Gaussian(1, 0.37); //creates gaussian with mean of 1 and variance of 0.4

int globalHue = 120;
float globalBrightness = 255;
int hueOffset = 120;
float fadeScale = 1.15;
float hueIncrement = 0.7; //was 0.7
float colorIncrement = 2;
float saturation = 255;

struct led_command {
  uint8_t opmode;
  uint32_t data;
};

unsigned long lastReceived = 0;
unsigned long lastHeartBeatSent;
const int heartBeatInterval = 100;
unsigned long audiocheck = 0;
const int audiocheckInterval = 100; //interval to check for low audio to detect the next song
bool fade = false;
unsigned long cooldown = 0;
const int cooldownInterval = 15000;
int lowcount = 0;

struct led_command cmd;
void connectToWifi();

void setup()
{
  globalHue = 0;
  samples = new averageCounter(SAMPLE_SIZE);
  longTermSamples = new averageCounter(LONG_TERM_SAMPLES);
  sanityBuffer    = new averageCounter(BUFFER_SIZE);
  
  while(sanityBuffer->setSample(250) == true) {}
  while (longTermSamples->setSample(200) == true) {}

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);

  Serial.begin(115200); // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  WiFi.begin(ssid, password); // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");

  connectToWifi();
  sendHeartBeat();
  setAll(0, 0, 0); //reset LEDs to off
  colorWipe(0, 0, 255, 2);
  delay(100);
  setAll(0, 0, 0);
  UDP.begin(7001);
}

void sendHeartBeat() {
    struct heartbeat_message hbm;
    hbm.client_id = LAMP_ID;
    hbm.chk = 77777;
    //Serial.println("Sending heartbeat");
    IPAddress ip(192,168,4,1);
    UDP.beginPacket(ip, 7171); 
    int ret = UDP.write((char*)&hbm,sizeof(hbm));
    //printf("Returned: %d, also sizeof hbm: %d \n", ret, sizeof(hbm));
    UDP.endPacket();
    lastHeartBeatSent = millis();
}

void loop()
{
  if (millis() - lastHeartBeatSent > heartBeatInterval) {
    sendHeartBeat();
  }
  
  int packetSize = UDP.parsePacket();
  if (packetSize)
  {
    UDP.read((char *)&cmd, sizeof(struct led_command));
    lastReceived = millis();
  }

  if(millis() - lastReceived >= 5000)
  {
    connectToWifi();
  }
    
  int opMode = cmd.opmode;
  int analogRaw = cmd.data;

  switch (opMode) {
    case 1:
      fade = false;
      soundReactive(analogRaw);
      break;
      
    case 2:
      fade = false;
      allWhite();
      break;

    case 3:
      chillFade();
      break;
      
    case 4:
      fade = false;
      CylonBounce(0xff, 0, 0, 4, 4, 100);
        //color, color, color, eyesize, speeddelay(bigger = slower), returndelay
      break;
      
    case 5:
      fade = false;
      TwinkleRandom(10, 200, false);
        //count, timepassed(speed), onlyonepixel?
      break;

    case 6:
      fade = false;
      meteorRain(0xff,0xff,0xff,10, 64, true, 30);
        //color, color, color, meteorsize, taildecay, randomtail?, speeddelay
      break;

    case 7:
      fade = false;
      Sparkle(random(255), random(255), random(255), 0);
        //color, color, color, speeddelay
      break;

    case 8:
      fade = false;
      colorWipe(0x00,0xff,0x00, 50);
      colorWipe(0x00,0x00,0x00, 50);
        //color, color, color, speeddelay
      break;

    case 9:
      fade = false;
      Fire(55,120,15);
        //cooling(higher = shorter flames, ideal from 20-100), sparking(higher = more active, ideal from 50,200), speeddelay 
      break;
  }
  
}

//------------------------------Wifi Stuff------------------------------

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    CylonBounce(0xff, 0, 0, 4, 4, 100);
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
  
  lastReceived = millis();
}
float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve)
{

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;

  // condition curve parameter
  // limit range

  if (curve > 10)
    curve = 10;
  if (curve < -10)
    curve = -10;

  curve = (curve * -.1);  // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin)
  {
    inputValue = originalMin;
  }
  if (inputValue > originalMax)
  {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin)
  {
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal = zeroRefCurVal / OriginalRange; // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax)
  {
    return 0;
  }

  if (invFlag == 0)
  {
    rangedValue = (pow(normalizedCurVal, curve) * NewRange) + newBegin;
  }
  else // invert the ranges
  {
    rangedValue = newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }

  return rangedValue;
}

//------------------------------Default Functions------------------------------

void allWhite() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255, 255, 235);
  }
  delay(5);
  FastLED.show();
}

void chillFade() {
  static int fadeVal = 0;
  static int counter = 0;
  static int from[3] = {0, 234, 255};
  static int to[3]   = {255, 0, 214};
  static int i, j;
  static double dsteps = 500.0;
  static double s1, s2, s3, tmp1, tmp2, tmp3;
  static bool reverse = false;
  if (fade == false) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(from[0], from[1], from[2]);
    }
    s1 = double((to[0] - from[0])) / dsteps; 
    s2 = double((to[1] - from[1])) / dsteps; 
    s3 = double((to[2] - from[2])) / dsteps; 
    tmp1 = from[0], tmp2 = from[1], tmp3 = from[2];
    fade = true;
  }

  if (!reverse) 
  {
    tmp1 += s1;
    tmp2 += s2; 
    tmp3 += s3; 
  }
  else 
  {
    tmp1 -= s1;
    tmp2 -= s2; 
    tmp3 -= s3; 
  }

  for (j = 0; j < NUM_LEDS; j++)
    leds[j] = CRGB((int)round(tmp1), (int)round(tmp2), (int)round(tmp3)); 
  FastLED.show(); 
  delay(5);

  counter++;
  if (counter == (int)dsteps) {
    reverse = !reverse;
    tmp1 = to[0], tmp2 = to[1], tmp3 = to[2];
    counter = 0;
  }
}

void soundReactive(int analogRaw) {

 int sanityValue = sanityBuffer->computeAverage();
 if (!(abs(analogRaw - sanityValue) > BUFFER_DEVIATION)) {
    sanityBuffer->setSample(analogRaw);
 }
  analogRaw = fscale(MIC_LOW, MIC_HIGH, MIC_LOW, MIC_HIGH, analogRaw, 0.4);
  if(millis() - audiocheck > audiocheckInterval)  //check for audio level
    {
      if(millis() - cooldown > cooldownInterval)
      {
        if(analogRaw <= 5)   //check if there is no audio
          {
            lowcount++;
            Serial.print("Audio low! This is the ");
            Serial.print(lowcount);
            Serial.println(" time!");
          }
        else
          {
            lowcount = 0;
          }
       if(lowcount > 10)
        {
         globalHue = random(0,255);
         colorIncrement = random(6, 18)/10.;
         float randsat = sat.random();
         if(randsat <= 1)
         {
          saturation = 255. * randsat;
         } else if(randsat > 1)
         {
          saturation = 255. * (1. - (randsat - 1.));
         }
         lowcount = 0;
         cooldown = millis();
         Serial.print("Changing hue to : ");
         Serial.println(globalHue);
         Serial.print("Changing color increment to : ");
         Serial.println(colorIncrement);
         Serial.print("Changing saturation to : ");
         Serial.println(saturation);
        }
     }
    audiocheck = millis();
   }

  if (samples->setSample(analogRaw))
    return;
    
  uint16_t longTermAverage = longTermSamples->computeAverage();
  uint16_t useVal = samples->computeAverage();
  longTermSamples->setSample(useVal);

  /*int diff = (useVal - longTermAverage);
  if (diff > 5)
  {
    if (globalHue < 235)
    {
      globalHue += hueIncrement;
    }
  }
  else if (diff < -5)
  {
    if (globalHue > 2)
    {
      globalHue -= hueIncrement;
    }
  }*/
  
    int curshow = fscale(MIC_LOW, MIC_HIGH, 0.0, (float)NUM_LEDS, (float)useVal, 0);
       //int curshow = map(useVal, MIC_LOW, MIC_HIGH, 0, NUM_LEDS)
    for (int i = 0; i < NUM_LEDS; i++)
  {
    if (i < curshow)
    {
      leds[i] = CHSV(globalHue + (i * colorIncrement), saturation, 255);
    }
    else
    {
      leds[i] = CRGB(leds[i].r / fadeScale, leds[i].g / fadeScale, leds[i].b / fadeScale);
    }
    
  }
  delay(5);
  FastLED.show(); 
}

//------------------------------Custom Functions------------------------------

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {  
  setAll(0,0,0);
  
  for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {
    
    
    // fade brightness all LEDs one step
    for(int j=0; j<NUM_LEDS; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(j, meteorTrailDecay );        
      }
    }
    
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
        setPixels(i-j, red, green, blue);
      } 
    }
   
    FastLED.show();
    delay(SpeedDelay);
  }
}

void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){

  for(int i = 0; i < NUM_LEDS-EyeSize-2; i++) {
    setAll(0,0,0);
    setPixels(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixels(i+j, red, green, blue); 
    }
    setPixels(i+EyeSize+1, red/10, green/10, blue/10);
    FastLED.show();
    delay(SpeedDelay);
  }

  delay(ReturnDelay);

  for(int i = NUM_LEDS-EyeSize-2; i > 0; i--) {
    setAll(0,0,0);
    setPixels(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixels(i+j, red, green, blue); 
    }
    setPixels(i+EyeSize+1, red/10, green/10, blue/10);
    FastLED.show();
    delay(SpeedDelay);
  }
  
  delay(ReturnDelay);
}

void TwinkleRandom(int Count, int SpeedDelay, boolean OnlyOne) {
  for (int i=0; i<Count; i++) {
     setPixels(random(NUM_LEDS),random(0,255),random(0,255),random(0,255));
     FastLED.show();
     delay(SpeedDelay);
     if(OnlyOne) { 
       setAll(0,0,0); 
     }
   }
  
  delay(SpeedDelay);
}

void Sparkle(byte red, byte green, byte blue, int SpeedDelay) {
  setAll(0,0,0);
  int Pixel = random(NUM_LEDS);
  setPixels(Pixel,red,green,blue);
  FastLED.show();
  delay(SpeedDelay);
  setPixels(Pixel,0,0,0);
}

void colorWipe(byte red, byte green, byte blue, int SpeedDelay) {
  setAll(0,0,0);
  for(uint16_t i=0; i<NUM_LEDS; i++) {
      setPixels(i, red, green, blue);
      FastLED.show();
      delay(SpeedDelay);
  }
}

void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[NUM_LEDS];
  int cooldown;
  setAll(0,0,0);
  
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS; i++) {
    cooldown = random(0, ((Cooling * 10) / NUM_LEDS) + 2);
    
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
    
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < NUM_LEDS; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  FastLED.show();
  delay(SpeedDelay);
}

void setPixelHeatColor (int Pixel, byte temperature) {
  setAll(0,0,0);
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature/255.0)*191);
 
  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // figure out which third of the spectrum we're in:
  if( t192 > 0x80) {                     // hottest
    setPixels(Pixel, 255, 255, heatramp);
  } else if( t192 > 0x40 ) {             // middle
    setPixels(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixels(Pixel, heatramp, 0, 0);
  }
}


//------------------------------Dependency Functions------------------------------

void fadeToBlack(int ledNo, byte fadeValue) {
   leds[ledNo].fadeToBlackBy( fadeValue ); 
}

void setPixels(int Pixel, byte red, byte green, byte blue) {
   leds[Pixel].r = red;
   leds[Pixel].g = green;
   leds[Pixel].b = blue;
}

void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixels(i, red, green, blue); 
  }
  FastLED.show();
}
