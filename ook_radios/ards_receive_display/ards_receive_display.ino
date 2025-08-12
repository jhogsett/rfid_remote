// ask_reliable_datagram_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging server
// with the RHReliableDatagram class, using the RH_ASK driver to control a ASK radio.
// It is designed to work with the other example ask_reliable_datagram_client
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12
 
#include <RHReliableDatagram.h>
#include <RH_ASK.h>
#include <SPI.h>

#include <Wire.h>
// #include <DS3231-RTC.h>
#include <HT16K33Disp.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2
 
// Singleton instance of the radio driver
RH_ASK driver(2400);
// RH_ASK driver(2000, 4, 5, 0); // ESP8266 or ESP32: do not use pin 11 or 2
// RH_ASK driver(2000, PD14, PD13, 0); STM32F4 Discovery: see tx and rx on Orange and Red LEDS
 
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);
 
HT16K33Disp disp1(0x70, 3);
#define DISPLAY_BRIGHTNESS 2

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");

  Wire.begin();
  byte brightness1[3] = { DISPLAY_BRIGHTNESS, DISPLAY_BRIGHTNESS, DISPLAY_BRIGHTNESS };
  disp1.init(brightness1);
}
 
uint8_t data[] = "And hello back to you";
// Dont put this on the stack:
uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
 
int failcount = 0;

void loop()
{
  if (manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from))
    {
      Serial.print("got request from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");

      buf[len] = '\0';

      char dispbuf[20];
      sprintf(dispbuf, "%s %d", (char*)buf, failcount);
      Serial.println(dispbuf);

      disp1.scroll_string(dispbuf);

      // Send a reply back to the originator client
      if (!manager.sendtoWait(data, sizeof(data), from)){
        Serial.println("sendtoWait failed");
        failcount++;
      }
    }
  }
}
