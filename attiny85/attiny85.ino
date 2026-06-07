#include <Wire.h>
#include "FastLED.h"
#include "FastLED_RGBW.h"

#define slaveAddress 0x23
#define NUM_LEDS 55
#define DATA_PIN 3
#define BRIGHTNESS 255 // 0 to 255

// WHAT WAS THIS FOR??
// #define led 1

//////////////////////////////////////////////////////
// DEBUG CODE: Uncomment to test using the onboard LED
// volatile int blinkCount = 0;

//////////////////////////////////////////////////////
// DEBUG CODE: Uncomment to send confirmation to Master
// volatile byte responseCode = 0;

volatile bool fireAnimation = false; // Flag to control fire animation
volatile bool blueAnimation = false; // New flag for blue animation

unsigned long fireStartTime = 0;          // Track fire animation start
unsigned long blueStartTime = 0;          // Track blue animation start
const unsigned long fireDuration = 16700; // Fire duration (16.7 seconds)
const unsigned long blueDuration = 87000; // Blue duration (90 seconds)

CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *)&leds[0];

//////////////////////////////////////////////////////
// Colors and palettes:
// https://fastled.io/docs/group___predefined_palettes.html#ga8932790994cfe78cf535fce88502d649
// https://github.com/FastLED/FastLED/wiki/Pixel-reference
//
// Examples:
// LavaColors_p is:
// {
//     CRGB::Black,
//     CRGB::Maroon,
//     CRGB::Black,
//     CRGB::Maroon,

//     CRGB::DarkRed,
//     CRGB::DarkRed,
//     CRGB::Maroon,
//     CRGB::DarkRed,

//     CRGB::DarkRed,
//     CRGB::DarkRed,
//     CRGB::Red,
//     CRGB::Orange,

//     CRGB::White,
//     CRGB::Orange,
//     CRGB::Red,
//     CRGB::DarkRed
// }

// OceanColors_p is:
// {
//     CRGB::MidnightBlue,
//     CRGB::DarkBlue,
//     CRGB::MidnightBlue,
//     CRGB::Navy,

//     CRGB::DarkBlue,
//     CRGB::MediumBlue,
//     CRGB::SeaGreen,
//     CRGB::Teal,

//     CRGB::CadetBlue,
//     CRGB::Blue,
//     CRGB::DarkCyan,
//     CRGB::CornflowerBlue,

//     CRGB::Aquamarine,
//     CRGB::SeaGreen,
//     CRGB::Aqua,
//     CRGB::LightSkyBlue
// }

// If you want to create a custom palette, you can do it like this:
// Define a custom color palette with 16 colors
const TProgmemRGBPalette16 customPalette PROGMEM = {
    CRGB::DarkRed, CRGB::DarkRed, CRGB::DarkRed, CRGB::Maroon,
    CRGB::Maroon, CRGB::OrangeRed, CRGB::Maroon, CRGB::OrangeRed,
    CRGB::Maroon, CRGB::OrangeRed, CRGB::Red, CRGB::DarkRed,
    CRGB::Maroon, CRGB::Orange, CRGB::Red, CRGB::OrangeRed};

const uint8_t brightnessScale = 150; // Scaling factor for brightness noise
const uint8_t indexScale = 100;      // Scaling factor for color index noise

void setup()
{
  Wire.begin(slaveAddress);
  Wire.onReceive(receiveEvent);

  //////////////////////////////////////////////////////
  // DEBUG CODE: Uncomment to send confirmation to Master
  // Wire.onRequest(sendEvent);

  // WHAT WAS THIS FOR??
  // pinMode(led, OUTPUT);

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));

  FastLED.setBrightness(BRIGHTNESS);
  FastLED.show();
}

void loop()
{
  unsigned long currentTime = millis();

  // Handle fire animation
  if (fireAnimation)
  {
    if (currentTime - fireStartTime < fireDuration)
    {
      runFire();
      FastLED.show();
    }
    else
    {
      fireAnimation = false;
      FastLED.clear();
      FastLED.show();
    }
  }
  // Handle blue animation
  else if (blueAnimation)
  {
    if (currentTime - blueStartTime >= blueDuration)
    {
      blueAnimation = false;
      FastLED.clear();
      FastLED.show();
    }
  }
  //////////////////////////////////////////////////////
  // DEBUG CODE: Uncomment to test using the onboard LED
  // if (blinkCount > 0)
  // {
  //   for (int i = 0; i < blinkCount; i++)
  //   {
  //     digitalWrite(led, HIGH);
  //     delay(250);
  //     digitalWrite(led, LOW);
  //     delay(250);
  //   }
  //   blinkCount = 0; // Reset after blinking
  // }
}

void receiveEvent(int howMany)
{
  int receivedData = Wire.read() << 8 | Wire.read();

  if (receivedData == 0xAAAA)
  {
    //////////////////////////////////////////////////////
    // DEBUG CODE: Uncomment to test using the onboard LED
    // blinkCount = 1;      // Blink once

    //////////////////////////////////////////////////////
    // DEBUG CODE: Uncomment to send confirmation to Master
    // responseCode = 0x11;           // Confirmation for 0xAAAA

    fireAnimation = true;
    fireStartTime = millis();
  }
  else if (receivedData == 0xBBBB)
  {
    //////////////////////////////////////////////////////
    // DEBUG CODE: Uncomment to test using the onboard LED
    // blinkCount = 3;      // Blink three times

    //////////////////////////////////////////////////////
    // DEBUG CODE: Uncomment to send confirmation to Master
    // responseCode = 0x22;   // Confirmation for 0xBBBB
    blueAnimation = true;
    blueStartTime = millis();
    colorFill(CRGB::Blue);
    FastLED.show();
  }
}

//////////////////////////////////////////////////////
// DEBUG CODE: Uncomment to send confirmation to Master
// void sendEvent()
// {
//   Wire.write(responseCode); // Send confirmation to Master
// }

void colorFill(CRGB c)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = c;
  }
}

void runFire()
{
  // Loop through each LED in the strip
  for (int i = 0; i < NUM_LEDS; i++)
  {
    // Generate a noise-based brightness value for the current LED
    uint8_t brightness = inoise8(i * brightnessScale, millis() / 5);

    // Generate a noise-based color index for the current LED
    uint8_t index = inoise8(i * indexScale, millis() / 10);

    // Set the color of the current LED using the custom palette, index, and brightness
    leds[i] = ColorFromPalette(customPalette, index, brightness);

    // Alternative: Use a predefined palette like "LavaColors_p"
    // leds[i] = ColorFromPalette(LavaColors_p, index, brightness);
  }
}