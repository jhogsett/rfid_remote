// ask_reliable_datagram_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_ASK driver to control a ASK radio.
// It is designed to work with the other example ask_reliable_datagram_server
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12
 
#include <RHReliableDatagram.h>
#include <RH_ASK.h>
#include <SPI.h>

// #define PAIR1
#define PAIR2

#ifdef PAIR1
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2
#endif

#ifdef PAIR2
#define CLIENT_ADDRESS 3
#define SERVER_ADDRESS 4
#endif

#define DATARATE 480
#define RETRIES 3
#define TIMEOUT 1000

// Singleton instance of the radio driver
RH_ASK driver(DATARATE, 11, 12, 10, false);
// RH_ASK driver(2000, 4, 5, 0); // ESP8266 or ESP32: do not use pin 11 or 2
// RH_ASK driver(2000, PD14, PD13, 0); STM32F4 Discovery: see tx and rx on Orange and Red LEDS
 
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);
 
// #define PTT_PIN 10

void setup() 
{
  Serial.begin(115200);

  // pinMode(PTT_PIN, OUTPUT);

  if (!manager.init())
    Serial.println("init failed");

  manager.setRetries(10);
  manager.setTimeout(TIMEOUT);
}
 
uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];

int msgcount = 0;
int ackfailcount = 0;
int sendfailcount = 0;
 
void loop()
{
  // Serial.println("Sending to ask_reliable_datagram_server");

  sprintf(data, "%ld", millis());
  int len = strlen(data);
    
  // Send a message to manager_server
  if (manager.sendtoWait(data, len, SERVER_ADDRESS))
  {

    // Now wait for a reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;   
    if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
    {
      msgcount++;
  
      // Serial.print("got reply from : 0x");
      // Serial.print(from, HEX);
      // Serial.print(": ");
      // Serial.println((char*)buf);
      // Serial.print("Resends: ");
      // Serial.println(manager.retransmissions());
      char buf[20];
      sprintf(buf, "%d A:%d S:%d R:%d", msgcount, ackfailcount, sendfailcount, manager.retransmissions());
      Serial.println(buf);
    }
    else
    {
      // Serial.println("No reply, is ask_reliable_datagram_server running?");
      ackfailcount++;
    }
  }
  else {
    Serial.println("sendtoWait failed");
    sendfailcount++;
  }
  delay(500);
}
