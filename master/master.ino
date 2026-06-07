#include <Arduino.h>
#include "SoftwareSerial.h"
#include "Wire.h"
#include "Adafruit_PWMServoDriver.h"
#include <MultiRGBWLeds.h>
#include "DYPlayerArduino.h"
#include "IRremote.h"

// PCA9685 (PWM Driver)
Adafruit_PWMServoDriver PCA9685 = Adafruit_PWMServoDriver(0x40, Wire);

// MP3 Player
SoftwareSerial mySerial(3, 11); // RX, TX (MP3 Player)
const int volume = 25;          // Volume level
DY::Player player(&mySerial);

// IR Receiver
const int receiver = 13;          // Signal Pin of IR receiver to Arduino Digital Pin 13
IRrecv irrecv(receiver);          // Create instance of 'irrecv'
uint32_t last_decodedRawData = 0; // Variable to store the last decodedRawData

// ATtiny I2C Slave Address
#define slaveAddress 0x23

// Relay (Humidifier)
const int humedifierRelay = 7;

// Relay (Lightsaber)
const int lighsaberRelay = 8;

// Fan
const int fanInA = 9;
const int fanInB = 10;
const int speed = 120;

// Function to send commands to ATtiny
void sendToATTiny(uint16_t command)
{
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

    // PCA9685 (PWM Driver)
    Wire.begin();
    PCA9685.begin();
    PCA9685.setPWMFreq(1600); // Maximum PWM frequency for LEDs

    // MultiRGBWLeds: LED Positions
    int backLeft[4] = {12, 13, 14, 15}; // {R, G, B, W}
    int frontLeft[4] = {8, 9, 10, 11};  // {R, G, B, W}
    int frontRight[4] = {4, 5, 6, 7};   // {R, G, B, W}
    int backRight[4] = {0, 1, 2, 3};    // {R, G, B, W}
    MultiRGBWLeds::begin(backLeft, frontLeft, frontRight, backRight);

    // Relay (Humidifier)
    pinMode(humedifierRelay, OUTPUT);

    // Relay (Lightsaber)
    pinMode(lighsaberRelay, OUTPUT);

    // Fan
    pinMode(fanInA, OUTPUT);
    pinMode(fanInB, OUTPUT);

    // Check if ATtiny slave is available. Keep retrying until it is found
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
}

void loop()
{
    if (irrecv.decode())
    { // Have we received an IR signal?
        // Check if it is a repeat IR code
        if (irrecv.decodedIRData.flags)
        {
            // Set the current decodedRawData to the last decodedRawData
            irrecv.decodedIRData.decodedRawData = last_decodedRawData;
        }

        Serial.print("Button HEX: 0x");
        Serial.println(irrecv.decodedIRData.decodedRawData, HEX);

        switch (irrecv.decodedIRData.decodedRawData)
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

        case 0xF30CFF00: // Button 1 pressed
            Serial.println("Playing: 'I Did - Original'");
            player.playSpecified(1);
            iDidOriginal();
            break;

        case 0xE718FF00: // Button 2 pressed
            Serial.println("Playing: 'I Did - Cumbia'");
            player.playSpecified(2);
            iDidCumbia();
            break;

        case 0xA15EFF00: // Button 3 pressed
            Serial.println("Playing: 'I Did - Lofi'");
            player.playSpecified(3);
            iDidLofi();
            break;

        case 0xF708FF00: // Button 4 pressed
            Serial.println("Playing: 'I Did - Metal'");
            player.playSpecified(4);
            iDidMetal();
            break;

        case 0xE31CFF00: // Button 5 pressed
            Serial.println("Playing: 'For Whom The Bell Tolls'");
            player.playSpecified(5);
            forWhomTheBellTolls();
            break;

        case 0xBF40FF00: // Button Play/Pause pressed
            Serial.println("Playing: EMPTY MP3");
            player.playSpecified(6); // Empty sound so used as stop
            break;
        }

        // Store the last decodedRawData
        last_decodedRawData = irrecv.decodedIRData.decodedRawData;
        irrecv.resume(); // Receive the next value
    }
}

void iDidOriginal()
{
    iDidIntro();

    // Turn fan ON
    analogWrite(fanInA, speed); // PWM Speed. 255 is full speed
    digitalWrite(fanInB, LOW);  // Direction. To change direction, change use analogWrite(fanPinInB, 255); and digitalWrite(fanPinInA, LOW);

    // Activate humedifierRelay
    digitalWrite(humedifierRelay, HIGH);

    // Activate lighsaberRelay
    digitalWrite(lighsaberRelay, HIGH);

    MultiRGBWLeds::set(
        BACK_LEFT, ORANGE, 10,
        FRONT_LEFT, ORANGE, 10,
        FRONT_RIGHT, RED, 10,
        BACK_RIGHT, RED, 10);

    sendToATTiny(0xAAAA); // Send command to ATtiny. This has a delay of 500ms... + 0.5 = 17.5

    delay(1800); // + 1.8 = 19.3

    MultiRGBWLeds::crossFade(
        BACK_LEFT, ORANGE, 10, ORANGE, 3,
        FRONT_LEFT, ORANGE, 10, ORANGE, 3,
        FRONT_RIGHT, RED, 10, RED, 3,
        BACK_RIGHT, RED, 10, RED, 3,
        5500);

    MultiRGBWLeds::set(
        BACK_LEFT, ORANGE, 10,
        FRONT_LEFT, ORANGE, 10,
        FRONT_RIGHT, RED, 10,
        BACK_RIGHT, RED, 10);

    delay(5900); // + 5.9 = 30.7

    MultiRGBWLeds::crossFade(
        BACK_LEFT, ORANGE, 10, ORANGE, 0,
        FRONT_LEFT, ORANGE, 10, ORANGE, 0,
        FRONT_RIGHT, RED, 10, RED, 0,
        BACK_RIGHT, RED, 10, RED, 0,
        3000);

    // Deactivate humedifierRelay
    digitalWrite(humedifierRelay, LOW);

    // Deactivate lighsaberRelay
    digitalWrite(lighsaberRelay, LOW);

    // Turn fan OFF
    digitalWrite(fanInA, LOW);
    digitalWrite(fanInB, LOW);

    delay(3000);
}

void iDidCumbia()
{
    iDidIntro();

    // Turn fan ON
    analogWrite(fanInA, speed); // PWM Speed. 255 is full speed
    digitalWrite(fanInB, LOW);  // Direction. To change direction, change use analogWrite(fanPinInB, 255); and digitalWrite(fanPinInA, LOW);

    // Activate humedifierRelay
    digitalWrite(humedifierRelay, HIGH);

    // Activate lighsaberRelay
    digitalWrite(lighsaberRelay, HIGH);

    // La primera vuelta de sideToSide la hacemos manual apra poder meter el sendToATTiny en el medio

    ////////////////////////////////////////////////////////////////////////
    // Arranca la cumbia bebeeeeee
    MultiRGBWLeds::set(
        BACK_LEFT, GREEN, 10,
        FRONT_LEFT, GREEN, 10,
        FRONT_RIGHT, INDIGO, 10,
        BACK_RIGHT, INDIGO, 10);

    sendToATTiny(0xBBBB); // Send command to ATtiny. This has a delay of 500ms

    delay(164);

    MultiRGBWLeds::set(
        BACK_LEFT, INDIGO, 10,
        FRONT_LEFT, INDIGO, 10,
        FRONT_RIGHT, GREEN, 10,
        BACK_RIGHT, GREEN, 10);

    delay(664);

    MultiRGBWLeds::sideToSide(LEFT_RIGHT, GREEN, INDIGO, 664, 3); // axis, color1, color2, time, count
    MultiRGBWLeds::sideToSide(LEFT_RIGHT, LIME, PINK, 664, 4);    // axis, color1, color2, time, count

    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();
    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();
    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();

    paaPaaPaaPaaPaPaaPaaPaPaaPeePeePeePeePePeePeePePee();

    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();
    cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa();

    MultiRGBWLeds::crossFade(
        BACK_LEFT, WHITE, 10, WHITE, 0,
        FRONT_LEFT, WHITE, 10, WHITE, 0,
        FRONT_RIGHT, WHITE, 10, WHITE, 0,
        BACK_RIGHT, WHITE, 10, WHITE, 0,
        8000);

    // Deactivate humedifierRelay
    digitalWrite(humedifierRelay, LOW);

    // Deactivate lighsaberRelay
    digitalWrite(lighsaberRelay, LOW);

    // Turn fan OFF
    digitalWrite(fanInA, LOW);
    digitalWrite(fanInB, LOW);
}

void iDidLofi()
{
    iDidIntro();

    MultiRGBWLeds::set(
        BACK_LEFT, INDIGO, 10,
        FRONT_LEFT, INDIGO, 10,
        FRONT_RIGHT, MAGENTA, 10,
        BACK_RIGHT, MAGENTA, 10);

    sendToATTiny(0xBBBB); // Send command to ATtiny. This has a delay of 500ms... + 0.5 = 17.5
}

void iDidMetal()
{
    iDidIntro();

    MultiRGBWLeds::set(
        BACK_LEFT, ORANGE, 10,
        FRONT_LEFT, ORANGE, 10,
        FRONT_RIGHT, RED, 10,
        BACK_RIGHT, RED, 10);

    sendToATTiny(0xAAAA); // Send command to ATtiny. This has a delay of 500ms... + 0.5 = 17.5
}

void forWhomTheBellTolls()
{
    delay(23300);
    MultiRGBWLeds::crossFade(
        BACK_LEFT, RED, 0, RED, 10,
        FRONT_LEFT, RED, 0, RED, 10,
        FRONT_RIGHT, RED, 0, RED, 10,
        BACK_RIGHT, RED, 0, RED, 10,
        100);
}

void iDidIntro()
{
    MultiRGBWLeds::resetAllPositions();
    delay(1200); // + 1.2

    //////////////////////////////////////////
    // Up to here we MUST sum 1.2 seconds!!!
    MultiRGBWLeds::crossFade(
        FRONT_LEFT, WHITE, 0, WHITE, 10,
        FRONT_RIGHT, SKY, 0, SKY, 10,
        8000);

    MultiRGBWLeds::crossFade(
        // BACK_LEFT, RED, 0, RED, 10,
        FRONT_LEFT, WHITE, 10, WHITE, 0,
        FRONT_RIGHT, SKY, 10, SKY, 0,
        // BACK_RIGHT, RED, 0, RED, 10,
        4500);

    //////////////////////////////////////////
    // Up to here we MUST sum 13.7 seconds!!!
    delay(3300); // + 3.3 = 17
}

void paaPaaPaaPaaPaPaaPaaPaPaaPeePeePeePeePePeePeePePee()
{
    MultiRGBWLeds::sideToSide(LEFT_RIGHT, GREEN, INDIGO, 664, 4); // axis, color1, color2, time, count
    MultiRGBWLeds::sideToSide(LEFT_RIGHT, LIME, PINK, 664, 4);    // axis, color1, color2, time, count
}

void cuaaCuCuCuaaCuCuCuararaCuCuaaCuCuCuararaCuCuaaCuCuCuaaCuuCuCuaa()
{

    // cuaaa1
    MultiRGBWLeds::crossFade(
        BACK_LEFT, WHITE, 0, WHITE, 10,
        FRONT_LEFT, WHITE, 0, WHITE, 10,
        FRONT_RIGHT, WHITE, 0, WHITE, 10,
        BACK_RIGHT, WHITE, 0, WHITE, 10,
        400);

    delay(270);

    MultiRGBWLeds::resetAllPositions();

    // cucu2.1
    MultiRGBWLeds::flash(
        FRONT_RIGHT, YELLOW, 10,
        BACK_RIGHT, YELLOW, 10,
        200, 100);

    // cucu2.2
    MultiRGBWLeds::flash(
        BACK_LEFT, YELLOW, 10,
        FRONT_LEFT, YELLOW, 10,
        200, 100);

    // cuaaa3
    MultiRGBWLeds::crossFade(
        BACK_LEFT, WHITE, 0, WHITE, 10,
        FRONT_LEFT, WHITE, 0, WHITE, 10,
        FRONT_RIGHT, WHITE, 0, WHITE, 10,
        BACK_RIGHT, WHITE, 0, WHITE, 10,
        400);

    delay(270);

    MultiRGBWLeds::resetAllPositions();

    // cucu4.1
    MultiRGBWLeds::flash(
        FRONT_RIGHT, YELLOW, 10,
        BACK_RIGHT, YELLOW, 10,
        200, 100);

    // cucu4.2
    MultiRGBWLeds::flash(
        BACK_LEFT, YELLOW, 10,
        FRONT_LEFT, YELLOW, 10,
        200, 100);

    // cuarara5
    MultiRGBWLeds::spin(AQUA, CLOCKWISE, 90, 2); // color, direction, speed (+ is slower, - is faster), full rotations
    MultiRGBWLeds::resetAllPositions();

    delay(200);

    // cucuaa6.1
    MultiRGBWLeds::flash(
        FRONT_RIGHT, YELLOW, 10,
        BACK_RIGHT, YELLOW, 10,
        200, 100);

    // cucuaa6.2
    MultiRGBWLeds::crossFade(
        BACK_LEFT, WHITE, 0, WHITE, 10,
        FRONT_LEFT, WHITE, 0, WHITE, 10,
        FRONT_RIGHT, WHITE, 0, WHITE, 10,
        BACK_RIGHT, WHITE, 0, WHITE, 10,
        400);

    delay(350);

    MultiRGBWLeds::resetAllPositions();

    // cucu7.1
    MultiRGBWLeds::flash(
        FRONT_RIGHT, YELLOW, 10,
        BACK_RIGHT, YELLOW, 10,
        200, 100);

    // cucu7.2
    MultiRGBWLeds::flash(
        BACK_LEFT, YELLOW, 10,
        FRONT_LEFT, YELLOW, 10,
        200, 100);

    // cuarara8
    MultiRGBWLeds::spin(ORANGE, ANTICLOCKWISE, 90, 2); // color, direction, speed (+ is slower, - is faster), full rotations
    MultiRGBWLeds::resetAllPositions();

    delay(200);

    // cucuaa9.1
    MultiRGBWLeds::flash(
        FRONT_RIGHT, YELLOW, 10,
        BACK_RIGHT, YELLOW, 10,
        200, 100);

    // cucuaa9.2
    MultiRGBWLeds::crossFade(
        BACK_LEFT, WHITE, 0, WHITE, 10,
        FRONT_LEFT, WHITE, 0, WHITE, 10,
        FRONT_RIGHT, WHITE, 0, WHITE, 10,
        BACK_RIGHT, WHITE, 0, WHITE, 10,
        400);

    delay(300);

    MultiRGBWLeds::resetAllPositions();

    // cuuucu10.1
    MultiRGBWLeds::set(
        BACK_RIGHT, YELLOW, 10,
        BACK_LEFT, YELLOW, 10);
    delay(300);

    // cuuucu10.2
    MultiRGBWLeds::set(
        FRONT_LEFT, YELLOW, 10,
        FRONT_RIGHT, YELLOW, 10);
    delay(300);

    // cuaaa11
    MultiRGBWLeds::crossFade(
        BACK_LEFT, WHITE, 0, WHITE, 10,
        FRONT_LEFT, WHITE, 0, WHITE, 10,
        FRONT_RIGHT, WHITE, 0, WHITE, 10,
        BACK_RIGHT, WHITE, 0, WHITE, 10,
        400);

    delay(300);

    MultiRGBWLeds::resetAllPositions();

    // cuuucu12.1
    MultiRGBWLeds::set(
        BACK_RIGHT, YELLOW, 10,
        BACK_LEFT, YELLOW, 10);
    delay(400);

    // cuuucu12.2
    MultiRGBWLeds::set(
        FRONT_LEFT, YELLOW, 10,
        FRONT_RIGHT, YELLOW, 10);
    delay(300);

    // cuaaaaaa13
    MultiRGBWLeds::crossFade(
        BACK_LEFT, WHITE, 0, WHITE, 10,
        FRONT_LEFT, WHITE, 0, WHITE, 10,
        FRONT_RIGHT, WHITE, 0, WHITE, 10,
        BACK_RIGHT, WHITE, 0, WHITE, 10,
        500);

    delay(800);
}