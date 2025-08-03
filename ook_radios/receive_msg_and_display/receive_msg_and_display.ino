// ask_receiver.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to receive messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) receiver with an Rx-B1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12
 
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

#include <Wire.h>
#include <DS3231-RTC.h>
#include <HT16K33Disp.h>

RH_ASK driver;
// RH_ASK driver(2000, 4, 5, 0); // ESP8266 or ESP32: do not use pin 11 or 2
// RH_ASK driver(2000, 3, 4, 0); // ATTiny, RX on D3 (pin 2 on attiny85) TX on D4 (pin 3 on attiny85), 
// RH_ASK driver(2000, PD14, PD13, 0); STM32F4 Discovery: see tx and rx on Orange and Red LEDS

HT16K33Disp disp1(0x70, 3);
#define DISPLAY_BRIGHTNESS 2


void setup()
{
#ifdef RH_HAVE_SERIAL
    Serial.begin(9600);   // Debugging only
#endif
    if (!driver.init())
#ifdef RH_HAVE_SERIAL
         Serial.println("init failed");
#else
        ;
#endif

    Wire.begin();
    byte brightness1[3] = { DISPLAY_BRIGHTNESS, DISPLAY_BRIGHTNESS, DISPLAY_BRIGHTNESS };
    disp1.init(brightness1);
}
 
void loop()
{
    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
    uint8_t buflen = sizeof(buf);
    uint8_t numc;
 
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
        // Message with a good checksum received, dump it.
        buf[buflen] = '\0';
        disp1.scroll_string(buf);
    }
}
