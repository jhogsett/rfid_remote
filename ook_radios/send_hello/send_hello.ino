// ask_transmitter.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to transmit messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) transmitter with an TX-C1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12
 
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif
 
RH_ASK driver;
// RH_ASK driver(2000, 4, 5, 0); // ESP8266 or ESP32: do not use pin 11 or 2
// RH_ASK driver(2000, 3, 4, 0); // ATTiny, RX on D3 (pin 2 on attiny85) TX on D4 (pin 3 on attiny85), 
// RH_ASK driver(2000, PD14, PD13, 0); STM32F4 Discovery: see tx and rx on Orange and Red LEDS
 
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
#ifdef RH_HAVE_SERIAL
    Serial.begin(9600);   // Debugging only
#endif
    if (!driver.init())
#ifdef RH_HAVE_SERIAL
         Serial.println("init failed");
#else
        ;
#endif
}
 
void loop()
{
    const char *msg = "hello";
 
    digitalWrite(LED_BUILTIN, HIGH);
    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
}
