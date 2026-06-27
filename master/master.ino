#include <Arduino.h>
#include "SoftwareSerial.h"
#include "Wire.h"
#include <MultiRGBWLeds.h>
#include "DYPlayerArduino.h"
#include "IRremote.h"

// ============================================================
//  FEATURE FLAGS
//  ------------------------------------------------------------
//  The basic build is 4 RGBW lamps + audio, which works with
//  every flag below set to 0. Turn on the extra hardware you
//  actually wired up by setting its flag to 1.
//
//  You can either edit the defaults here, OR override them at
//  build time without touching this file (the closest thing to
//  an "env var" on Arduino), e.g. with arduino-cli:
//    --build-property "build.extra_flags=-DENABLE_SMOKE=1"
//  or in platformio.ini:
//    build_flags = -DENABLE_SMOKE=1
// ============================================================
#ifndef ENABLE_FRONT_STRIP
#define ENABLE_FRONT_STRIP 0 // ATtiny85 NeoPixel front strip (I2C slave)
#endif
#ifndef ENABLE_SMOKE
#define ENABLE_SMOKE 0 // Humidifier + fan
#endif
#ifndef ENABLE_LIGHTSABER
#define ENABLE_LIGHTSABER 0 // Lightsaber relay
#endif

// RGBW lamps -- the MultiRGBWLeds library owns the PCA9685 (default 0x40).
MultiRGBWLeds leds;

// MP3 Player -- always required for audio
SoftwareSerial mySerial(3, 11); // RX, TX (MP3 Player)
const int volume = 25;          // Volume level
DY::Player player(&mySerial);

// IR Receiver -- always required
const int receiver = 13;          // Signal Pin of IR receiver to Arduino Digital Pin 13
IRrecv irrecv(receiver);          // Create instance of 'irrecv'
bool ambientOn = false;               // Power button toggles the static ambient scene
unsigned long lastPressMs = 0;        // millis() of the last accepted IR press
const unsigned long debounceMs = 500; // ignore repeat presses within this window

// Audio amp keep-alive: a low, looping tone (track 6 on the SD) plays whenever
// nothing else is, so the amp never sees silence and never auto-sleeps.
const int keepAliveTrack = 6;
unsigned long lastKeepAliveMs = 0;
const unsigned long keepAliveCheckMs = 1000; // how often to re-check play state

// ATtiny I2C Slave Address (front strip)
#define slaveAddress 0x23

// Relay (Humidifier) -- smoke
const int humedifierRelay = 7;

// Relay (Lightsaber)
const int lighsaberRelay = 8;

// Fan -- smoke
const int fanInA = 9;
const int fanInB = 10;
const int speed = 120;

// ------------------------------------------------------------
//  Optional-hardware wrappers
//  These no-op when their feature is disabled, so the show
//  functions below stay readable and the #if guards live here.
// ------------------------------------------------------------

// Send a command to the ATtiny front-strip slave.
// The original protocol included a 500ms settle delay that the
// show timings are tuned around -- we keep it even when the strip
// is disabled so the lamp choreography stays in sync with the audio.
void frontStripSend(uint16_t command)
{
#if ENABLE_FRONT_STRIP
    Wire.beginTransmission(slaveAddress);
    Wire.write(highByte(command));
    Wire.write(lowByte(command));
    Wire.endTransmission();
    Serial.print("Sent command: 0x");
    Serial.println(command, HEX);

    delay(500); // Allow time for the slave to process

    // Request confirmation from ATtiny
    Wire.requestFrom(slaveAddress, 1);
    if (Wire.available())
    {
        byte confirmation = Wire.read();
        Serial.print("Confirmation: 0x");
        Serial.println(confirmation, HEX);
    }
#else
    (void)command;
    delay(500); // Keep choreography in sync with the audio
#endif
}

// Smoke = humidifier + fan, switched on/off together.
void smokeOn()
{
#if ENABLE_SMOKE
    // Turn fan ON
    analogWrite(fanInA, speed); // PWM Speed. 255 is full speed
    digitalWrite(fanInB, LOW);  // Direction
    // Activate humidifier
    digitalWrite(humedifierRelay, HIGH);
#endif
}

void smokeOff()
{
#if ENABLE_SMOKE
    digitalWrite(humedifierRelay, LOW);
    // Turn fan OFF
    digitalWrite(fanInA, LOW);
    digitalWrite(fanInB, LOW);
#endif
}

void saberOn()
{
#if ENABLE_LIGHTSABER
    digitalWrite(lighsaberRelay, HIGH);
#endif
}

void saberOff()
{
#if ENABLE_LIGHTSABER
    digitalWrite(lighsaberRelay, LOW);
#endif
}

// ------------------------------------------------------------
//  Ambient scene
//  The calm resting state the rig falls back to. The Power button
//  snaps to it; finished shows fade up into it.
// ------------------------------------------------------------

// Snap straight to the ambient scene (used by the Power toggle).
void setAmbient()
{
    leds.set(
        LampPosition::FrontLeft, LampColor::White, 1,
        LampPosition::BackLeft, LampColor::Orange, 5,
        LampPosition::FrontRight, LampColor::White, 4,
        LampPosition::BackRight, LampColor::Red, 2);
    ambientOn = true;
}

// Fade up into the ambient scene from dark over durationMs.
void fadeToAmbient(uint16_t durationMs)
{
    leds.resetAllPositions(); // known starting point (off) so the fade is clean
    leds.crossFade(
        LampPosition::FrontLeft, LampColor::White, 0, LampColor::White, 1,
        LampPosition::BackLeft, LampColor::Orange, 0, LampColor::Orange, 5,
        LampPosition::FrontRight, LampColor::White, 0, LampColor::White, 4,
        LampPosition::BackRight, LampColor::Red, 0, LampColor::Red, 2,
        durationMs);
    ambientOn = true;
}

// Called when a light show finishes: hold for a couple of seconds,
// then gently drift back into the ambient scene.
void returnToAmbient()
{
    leds.resetAllPositions(); // clear whatever the show left lit
    delay(500);              // a couple of seconds of darkness after the show
    fadeToAmbient(3000);      // gentle fade-in
}

void setup()
{
    // Terminal
    Serial.begin(9600);

    // MP3 Player
    player.begin();
    player.setVolume(volume);

    // IR Receiver
    irrecv.enableIRIn(); // Start the receiver

    // RGBW lamps. begin() creates/configures the PCA9685 and calls Wire.begin().
    // Each lamp is four PCA9685 channels (0-15) in {R, G, B, W} order.
    const uint8_t backLeft[4] = {4, 5, 6, 7};
    const uint8_t frontLeft[4] = {0, 1, 2, 3};
    const uint8_t frontRight[4] = {12, 13, 14, 15};
    const uint8_t backRight[4] = {8, 9, 10, 11};
    leds.begin(backLeft, frontLeft, frontRight, backRight);

#if ENABLE_SMOKE
    // Relay (Humidifier) + Fan
    pinMode(humedifierRelay, OUTPUT);
    pinMode(fanInA, OUTPUT);
    pinMode(fanInB, OUTPUT);
#endif

#if ENABLE_LIGHTSABER
    // Relay (Lightsaber)
    pinMode(lighsaberRelay, OUTPUT);
#endif

#if ENABLE_FRONT_STRIP
    // Check if ATtiny slave is available. Keep retrying until it is found.
    while (true)
    {
        Wire.beginTransmission(slaveAddress);
        byte busStatus = Wire.endTransmission();

        if (busStatus == 0)
        {
            Serial.println("Slave found.");
            break; // Exit the loop and continue execution
        }

        Serial.println("Slave not found. Retrying...");
        delay(1000); // Wait 1 second before retrying
    }
#endif

    // Power-on default: rig rests in the ambient scene.
    returnToAmbient(); 
    setAmbient();
}

void loop()
{
    // Keep the audio amp awake. If nothing is playing, (re)start the looping
    // keep-alive tone. Throttled so we don't spam the DFPlayer over serial.
    unsigned long tNow = millis();
    if (tNow - lastKeepAliveMs >= keepAliveCheckMs)
    {
        lastKeepAliveMs = tNow;
        if (player.checkPlayState() == DY::PlayState::Stopped)
        {
            player.playSpecified(keepAliveTrack);
        }
    }

    if (irrecv.decode())
    { // Have we received an IR signal?
        unsigned long now = millis();
        uint32_t code = irrecv.decodedIRData.decodedRawData;
        bool isRepeat = irrecv.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;

        // Guardrail: ignore NEC repeat frames (button held) and any press within
        // the debounce window. Stops one physical press being read twice, which
        // made the Power toggle flicker on/off.
        if (isRepeat || code == 0 || (now - lastPressMs) < debounceMs)
        {
            irrecv.resume();
            return;
        }
        lastPressMs = now;

        Serial.print("Button HEX: 0x");
        Serial.println(code, HEX);

        switch (code)
        {

            // BUTTON HEX CODES:
            // Power              0xBA45FF00
            // VOL+               0xB946FF00
            // FUNC/STOP          0xB847FF00
            // |<< (Rewind)       0xBB44FF00
            // >|| (Play/Pause)   0xBF40FF00
            // >>| (Fast Forward) 0xBC43FF00
            // Arrow Down         0xF807FF00
            // VOL-               0xEA15FF00
            // Arrow Up           0xF609FF00
            // 0                  0xE916FF00
            // EQ                 0xE619FF00
            // ST/REPT            0xF20DFF00
            // 1                  0xF30CFF00
            // 2                  0xE718FF00
            // 3                  0xA15EFF00
            // 4                  0xF708FF00
            // 5                  0xE31CFF00
            // 6                  0xA55AFF00
            // 7                  0xBD42FF00
            // 8                  0xAD52FF00
            // 9                  0xB54AFF00

        case 0xBA45FF00: // Power button -- toggle static ambient scene (no audio, no animation)
            ambientOn = !ambientOn;
            if (ambientOn)
            {
                Serial.println("Ambient ON");
                setAmbient();
            }
            else
            {
                Serial.println("Ambient OFF");
                leds.resetAllPositions();
            }
            break;

        case 0xF30CFF00: // Button 1 pressed
            Serial.println("Playing: 'I Did - Original'");
            player.playSpecified(1);
            iDidOriginal();
            returnToAmbient();
            break;

        case 0xE718FF00: // Button 2 pressed
            Serial.println("Playing: 'I Did - Cumbia'");
            player.playSpecified(2);
            iDidCumbia();
            returnToAmbient();
            break;

        case 0xA15EFF00: // Button 3 pressed
            Serial.println("Playing: 'I Did - Lofi'");
            player.playSpecified(3);
            iDidLofi();
            returnToAmbient();
            break;

        case 0xF708FF00: // Button 4 pressed
            Serial.println("Playing: 'I Did - Metal'");
            player.playSpecified(4);
            iDidMetal();
            returnToAmbient();
            break;

        case 0xE31CFF00: // Button 5 pressed
            Serial.println("Playing: 'For Whom The Bell Tolls'");
            player.playSpecified(5);
            forWhomTheBellTolls();
            returnToAmbient();
            break;
        }

        irrecv.resume(); // Receive the next value
    }
}

void iDidOriginal()
{
    iDidIntro();

    smokeOn();
    saberOn();

    leds.set(
        LampPosition::BackLeft, LampColor::Orange, 10,
        LampPosition::FrontLeft, LampColor::Red, 3,
        LampPosition::FrontRight, LampColor::White, 6,
        LampPosition::BackRight, LampColor::Red, 10);

    frontStripSend(0xAAAA); // Has a 500ms delay (kept for audio sync)... + 0.5 = 17.5

    delay(1800); // + 1.8 = 19.3

    leds.crossFade(
        LampPosition::BackLeft, LampColor::Orange, 10, LampColor::Orange, 3,
        LampPosition::FrontLeft, LampColor::Red, 3, LampColor::Red, 1,
        LampPosition::FrontRight, LampColor::White, 6, LampColor::White, 1,
        LampPosition::BackRight, LampColor::Red, 10, LampColor::Red, 3,
        5500);

    leds.set(
        LampPosition::BackLeft, LampColor::Orange, 10,
        LampPosition::FrontLeft, LampColor::Red, 3,
        LampPosition::FrontRight, LampColor::White, 6,
        LampPosition::BackRight, LampColor::Red, 10);

    delay(5900); // + 5.9 = 30.7

    leds.crossFade(
        LampPosition::BackLeft, LampColor::Orange, 10, LampColor::Orange, 0,
        LampPosition::FrontLeft, LampColor::Red, 3, LampColor::Red, 0,
        LampPosition::FrontRight, LampColor::White, 6, LampColor::White, 0,
        LampPosition::BackRight, LampColor::Red, 10, LampColor::Red, 0,
        3000);

    smokeOff();
    saberOff();

    delay(3000);
}

void iDidCumbia()
{
    iDidIntro();

    smokeOn();
    saberOn();

    // La primera vuelta de sideToSide la hacemos manual apra poder meter el frontStripSend en el medio

    ////////////////////////////////////////////////////////////////////////
    // Arranca la cumbia bebeeeeee
    leds.set(
        LampPosition::BackLeft, LampColor::Green, 10,
        LampPosition::FrontLeft, LampColor::Green, 10,
        LampPosition::FrontRight, LampColor::Indigo, 10,
        LampPosition::BackRight, LampColor::Indigo, 10);

    frontStripSend(0xBBBB); // Has a 500ms delay (kept for audio sync)

    delay(164);

    leds.set(
        LampPosition::BackLeft, LampColor::Indigo, 10,
        LampPosition::FrontLeft, LampColor::Indigo, 10,
        LampPosition::FrontRight, LampColor::Green, 10,
        LampPosition::BackRight, LampColor::Green, 10);

    delay(664);

    leds.sideToSide(Axis::LeftRight, LampColor::Green, LampColor::Indigo, 664, 3); // axis, color1, color2, halfPeriodMs, cycles
    leds.sideToSide(Axis::LeftRight, LampColor::Sky, LampColor::Pink, 664, 4);    // axis, color1, color2, halfPeriodMs, cycles

    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();
    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();
    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();

    paaPaaPaaPaaPaPaaPaaPaPaaPeePeePeePeePePeePeePePee();

    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();
    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();

    leds.crossFade(
        LampPosition::BackLeft, LampColor::Red, 10, LampColor::Red, 0,
        LampPosition::FrontLeft, LampColor::Red, 10, LampColor::Red, 0,
        LampPosition::FrontRight, LampColor::Red, 10, LampColor::Red, 0,
        LampPosition::BackRight, LampColor::Red, 10, LampColor::Red, 0,
        8000);

    smokeOff();
    saberOff();
}

void iDidLofi()
{
    iDidIntro();

    leds.set(
        LampPosition::BackLeft, LampColor::Indigo, 10,
        LampPosition::FrontLeft, LampColor::Indigo, 10,
        LampPosition::FrontRight, LampColor::Magenta, 10,
        LampPosition::BackRight, LampColor::Magenta, 10);

    frontStripSend(0xBBBB); // Has a 500ms delay (kept for audio sync)... + 0.5 = 17.5
}

void iDidMetal()
{
    iDidIntro();

    leds.set(
        LampPosition::BackLeft, LampColor::Orange, 10,
        LampPosition::FrontLeft, LampColor::Orange, 10,
        LampPosition::FrontRight, LampColor::Red, 10,
        LampPosition::BackRight, LampColor::Red, 10);

    frontStripSend(0xAAAA); // Has a 500ms delay (kept for audio sync)... + 0.5 = 17.5
}

void forWhomTheBellTolls()
{
    delay(23300);
    leds.crossFade(
        LampPosition::BackLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontRight, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::BackRight, LampColor::Red, 0, LampColor::Red, 10,
        100);
}

void iDidIntro()
{
    leds.resetAllPositions();
    delay(1200); // + 1.2

    //////////////////////////////////////////
    // Up to here we MUST sum 1.2 seconds!!!
    leds.crossFade(
        LampPosition::FrontLeft, LampColor::Sky, 0, LampColor::Sky, 10,
        LampPosition::FrontRight, LampColor::White, 0, LampColor::White, 10,
        8000);

    leds.crossFade(
        LampPosition::FrontLeft, LampColor::Sky, 10, LampColor::Sky, 0,
        LampPosition::FrontRight, LampColor::White, 10, LampColor::White, 0,
        4500);

    //////////////////////////////////////////
    // Up to here we MUST sum 13.7 seconds!!!
    delay(3300); // + 3.3 = 17
}

void paaPaaPaaPaaPaPaaPaaPaPaaPeePeePeePeePePeePeePePee()
{
    leds.sideToSide(Axis::LeftRight, LampColor::Green, LampColor::Indigo, 664, 4); // axis, color1, color2, halfPeriodMs, cycles
    leds.sideToSide(Axis::LeftRight, LampColor::Sky, LampColor::Pink, 664, 4);    // axis, color1, color2, halfPeriodMs, cycles
}

void cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa()
{

    // cuaaa1
    leds.crossFade(
        LampPosition::BackLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontRight, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::BackRight, LampColor::Red, 0, LampColor::Red, 10,
        400);

    delay(270);

    leds.resetAllPositions();

    // cucu2.1
    leds.flash(
        LampPosition::BackLeft, LampColor::Orange, 10,
        LampPosition::BackRight, LampColor::Orange, 10,
        200, 100);
        
        // cucu2.2
        leds.flash(
        LampPosition::FrontRight, LampColor::Yellow, 10,
        LampPosition::FrontLeft, LampColor::Yellow, 10,
        200, 100);

    // cuaaa3
    leds.crossFade(
        LampPosition::BackLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontRight, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::BackRight, LampColor::Red, 0, LampColor::Red, 10,
        400);

    delay(270);

    leds.resetAllPositions();

    // cucu4.1
    leds.flash(
        LampPosition::FrontRight, LampColor::Yellow, 10,
        LampPosition::BackRight, LampColor::Yellow, 10,
        200, 100);

    // cucu4.2
    leds.flash(
        LampPosition::BackLeft, LampColor::Yellow, 10,
        LampPosition::FrontLeft, LampColor::Yellow, 10,
        200, 100);

    // cuarara5
    leds.spin(LampColor::Aqua, Direction::Clockwise, 90, 2); // color, direction, stepMs, rotations
    leds.resetAllPositions();

    delay(200);

    // cucuaa6.1
    leds.flash(
        LampPosition::FrontRight, LampColor::Yellow, 10,
        LampPosition::BackRight, LampColor::Yellow, 10,
        200, 100);

    // cucuaa6.2
    leds.crossFade(
        LampPosition::BackLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontRight, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::BackRight, LampColor::Red, 0, LampColor::Red, 10,
        400);

    delay(350);

    leds.resetAllPositions();

    // cucu7.1
    leds.flash(
        LampPosition::FrontRight, LampColor::Yellow, 10,
        LampPosition::BackRight, LampColor::Yellow, 10,
        200, 100);

    // cucu7.2
    leds.flash(
        LampPosition::BackLeft, LampColor::Yellow, 10,
        LampPosition::FrontLeft, LampColor::Yellow, 10,
        200, 100);

    // cuarara8
    leds.spin(LampColor::Orange, Direction::Anticlockwise, 90, 2); // color, direction, stepMs, rotations
    leds.resetAllPositions();

    delay(200);

    // cucuaa9.1
    leds.flash(
        LampPosition::FrontRight, LampColor::Yellow, 10,
        LampPosition::BackRight, LampColor::Yellow, 10,
        200, 100);

    // cucuaa9.2
    leds.crossFade(
        LampPosition::BackLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontRight, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::BackRight, LampColor::Red, 0, LampColor::Red, 10,
        400);

    delay(300);

    leds.resetAllPositions();

    // cuuucu10.1
    leds.set(
        LampPosition::BackRight, LampColor::Yellow, 10,
        LampPosition::BackLeft, LampColor::Yellow, 10);
    delay(300);

    // cuuucu10.2
    leds.set(
        LampPosition::FrontLeft, LampColor::Yellow, 10,
        LampPosition::FrontRight, LampColor::Yellow, 10);
    delay(300);

    // cuaaa11
    leds.crossFade(
        LampPosition::BackLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontRight, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::BackRight, LampColor::Red, 0, LampColor::Red, 10,
        400);

    delay(300);

    leds.resetAllPositions();

    // cuuucu12.1
    leds.set(
        LampPosition::BackRight, LampColor::Yellow, 10,
        LampPosition::BackLeft, LampColor::Yellow, 10);
    delay(400);

    // cuuucu12.2
    leds.set(
        LampPosition::FrontLeft, LampColor::Yellow, 10,
        LampPosition::FrontRight, LampColor::Yellow, 10);
    delay(300);

    // cuaaaaaa13
    leds.crossFade(
        LampPosition::BackLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontLeft, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::FrontRight, LampColor::Red, 0, LampColor::Red, 10,
        LampPosition::BackRight, LampColor::Red, 0, LampColor::Red, 10,
        500);

    delay(800);
}
