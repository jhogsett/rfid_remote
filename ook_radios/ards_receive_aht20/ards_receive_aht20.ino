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
#include <HT16K33Disp.h>

#define PAIR1
// #define PAIR2

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
RH_ASK driver(DATARATE);
// RH_ASK driver(2000, 4, 5, 0); // ESP8266 or ESP32: do not use pin 11 or 2
// RH_ASK driver(2000, PD14, PD13, 0); STM32F4 Discovery: see tx and rx on Orange and Red LEDS
 
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, SERVER_ADDRESS);
 
HT16K33Disp *disp1, *disp2, *disp3;
#define DISPLAY_BRIGHTNESS 2

#define NUM_TEMP_WORDS 21
const char *temp_words[NUM_TEMP_WORDS] = {"CRIO","ICEY","FROZ","BRRR","CHIL","COLD","COOL","POOR","MILD","OKAY","NICE","WARM","COZY","HEAT","BAKE","SEAR","FIRE","BURN","ICKY","DAMN","PYRO"};

#define BASE_TEMP 32.0
#define MAX_TEMP 132.0
#define DIVISION 5.0

int temp_to_condition(char *buffer, const char *pattern, float temp){
  if(temp < BASE_TEMP){
    temp = BASE_TEMP;
  } else if(temp > MAX_TEMP){
    temp = MAX_TEMP;
  }
  temp -= BASE_TEMP;
  temp /= DIVISION;
  int index = int(temp);
  sprintf(buffer, pattern, temp_words[index]);
  return index;
}

void setup() 
{
  Serial.begin(115200);

  // pinMode(PTT_PIN, OUTPUT);

  if (!manager.init())
    Serial.println("init failed");

  manager.setRetries(RETRIES);
  manager.setTimeout(TIMEOUT);

  Wire.begin();
  byte brightness[3] = { 1, 9, 15 }; // Green/Amber/Red
  disp1 = new HT16K33Disp(0x70, 3);
  disp2 = new HT16K33Disp(0x71, 1);
  disp3 = new HT16K33Disp(0x72, 1);
  disp1->init(brightness);
}
 
uint8_t data[] = "!!!";
// Dont put this on the stack:
uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
 
int recvcount = 0;
int failcount = 0;

bool running1, running2, running3 = false;

void float_to_fixed(float value, char *buffer, const char *pattern, byte decimals=1){
  int split = 10 * decimals;
  int ivalue = int(value * split);
  int valuei = ivalue / split;
  int valued = ivalue % split;
  sprintf(buffer, pattern, valuei, valued);
}

// https://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
// HI = -42.379 + 2.04901523*T + 10.14333127*RH - .22475541*T*RH - .00683783*T*T - .05481717*RH*RH + .00122874*T*T*RH + .00085282*T*RH*RH - .00000199*T*T*RH*RH

void loop()
{
  if (manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from))
    {
      // Serial.print("got request from : 0x");
      // Serial.print(from, HEX);
      // Serial.print(": ");

      // for(int i = 0; i < sizeof(buf); i++){
      //   Serial.print(buf[i]);
      //   Serial.print(" ");
      // }
      // Serial.println();

      float temp = (float)*((float *)buf);
      float humid = (float)*((float *)buf+1);

      // Serial.print("Temp: ");
      // Serial.println(temp);
      // Serial.print("Humid: ");
      // Serial.println(humid);

      recvcount++;

      // buf[len] = '\0';

      int resend_count = manager.retransmissions();

      // char dispbuf[30];
      // sprintf(dispbuf, "%s %d %d", (char*)buf, recvcount, failcount);
      // sprintf(dispbuf, "%d F%d R%d", recvcount, failcount, resend_count);
      // Serial.println(dispbuf);

      // disp1.scroll_string(dispbuf);

      // Serial.print("Resends: ");
      // Serial.println(resend_count);

      float heat_index = 0.0;
      float steadman_index = 0.5 * (temp + 61.0 + ((temp - 68.0) * 1.2) + (humid * 0.094));
      float initial_index = (steadman_index + temp) / 2.0;
      if(initial_index >= 80.0){
        heat_index = -42.379 
                    + (2.04901523 * temp) 
                    + (10.14333127 * humid) 
                    - (0.22475541 * temp * humid) 
                    - (6.83783e-3 * pow(temp, 2)) 
                    - (5.481717e-2 * pow(humid, 2)) 
                    + (1.22874e-3 * pow(temp, 2) * humid) 
                    + (8.5282e-4 * temp * pow(humid, 2)) 
                    - (1.99e-6 * pow(temp, 2) * pow(humid, 2));

        float adjust = 0.0;

        if (humid < 13 && temp >= 80 && temp <= 112){
          adjust = ( (13 - humid) / 4) * sqrt((17 - abs(temp - 95)) / 17);
          heat_index -= adjust;

        } else if (humid > 85 && temp >= 80 && temp <= 87){
          adjust = ((humid - 85) / 10) * ((87 - temp) / 5);
          heat_index += adjust;
        }
      } else {
        heat_index = initial_index;
      }

      Serial.println(heat_index);

      char condition[10];

      int condition_index = temp_to_condition(condition, "%s",  heat_index);

      char temps[10];
      if(temp < 100.0){
        float_to_fixed(temp, temps, "%2d.%1d");
      } else {
        float_to_fixed(temp, temps, "%3d.%1d");
      }

      char humids[10];
      sprintf_P(humids, PSTR("%3d "), int(humid));
      // float_to_fixed(humid, humids, "%3d.%1d");

      char indexs[10];
      float_to_fixed(heat_index, indexs, "%3d ");

      char buffer[30];

      // switch(device_display_count){
      //   case 1:
      //   {
      //     sprintf(buffer, "%4s", condition);
      //     break;
      //   }
      //   case 2:
      //   {
      //     if(temp < 100.0){
      //       sprintf(buffer, "%4s %4s", temps, condition);
      //     } else {
      //       sprintf(buffer, "%5s%4s", temps, condition);
      //     }
      //     break;
      //   }
      //   case 3:
      //   {
      if(condition_index < 7 || condition_index > 12){
        if(temp < 100.0){
          sprintf(buffer, "%4s %4s%4s", temps, indexs, condition);
        } else {
          sprintf(buffer, "%5s%4s%4s", temps, indexs, condition);
        }
      } else if(condition_index < 9 || condition_index > 10){
        if(temp < 100.0){
          sprintf(buffer, "%4s %4s%4s", temps, condition, indexs);
        } else {
          sprintf(buffer, "%5s%4s%4s", temps, condition, indexs);
        }
      } else{
        if(temp < 100.0){
          sprintf(buffer, "%4s%5s%4s", condition, temps, indexs);
        } else {
          sprintf(buffer, "%4s%4s %4s", condition, temps, indexs);
        }
      }
        //   break;
        // }
      // }

      unsigned long time = millis();
      if(!running1)
        disp1->begin_scroll_string(buffer, 100, 100);

        running1 = disp1->step_scroll_string(time);

      // Send a reply back to the originator client
      if (!manager.sendtoWait(data, sizeof(data), from)){
        Serial.println("sendtoWait failed");
        failcount++;
      }
    }
  }
}
