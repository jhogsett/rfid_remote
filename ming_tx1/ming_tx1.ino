/*
  RFID Remote Transmitter Test Code - Compact Version
  
  Interactive test transmitter for garage door receiver timing sensitivity testing.
  
  Commands:
  v - Valid sequence (500ms gaps = 1000ms pulse-to-pulse)
  f - Fast sequence (300ms gaps = 800ms pulse-to-pulse) 
  s - Slow sequence (700ms gaps = 1200ms pulse-to-pulse)
  t - Too fast (250ms gaps = 750ms pulse-to-pulse)
  T - Too slow (800ms gaps = 1300ms pulse-to-pulse)
  d - Double activation test (6 pulses)
  h - Help
*/

#include <Arduino.h>

#define TX_PIN LED_BUILTIN
#define PULSE_WIDTH 500

// Timing values (gaps, not total spacing)
#define VALID_GAP 500     // 500ms gap = 1000ms total
#define FAST_GAP 300      // 300ms gap = 800ms total  
#define SLOW_GAP 700      // 700ms gap = 1200ms total
#define TOO_FAST_GAP 250  // 250ms gap = 750ms total
#define TOO_SLOW_GAP 800  // 800ms gap = 1300ms total

void setup() {
  Serial.begin(115200);
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  
  Serial.println(F("*** RFID TX TEST ***"));
  print_help();
}

void print_help() {
  Serial.println(F("\n=== COMMANDS ==="));
  Serial.println(F("v - Valid (1000ms)"));
  Serial.println(F("f - Fast (800ms)"));
  Serial.println(F("s - Slow (1200ms)"));
  Serial.println(F("t - Too fast (750ms)"));
  Serial.println(F("T - Too slow (1300ms)"));
  Serial.println(F("d - Double test (6 pulses)"));
  Serial.println(F("h - Help"));
  Serial.println(F("================"));
}

void send_pulse() {
  digitalWrite(TX_PIN, HIGH);
  delay(PULSE_WIDTH);
  digitalWrite(TX_PIN, LOW);
  Serial.print(F("P"));
}

void send_seq(int count, int gap) {
  for(int i = 0; i < count; i++) {
    send_pulse();
    if(i < count - 1) {
      Serial.print(F("-"));
      delay(gap);
    }
  }
  Serial.println();
}

void process_cmd() {
  if(Serial.available()) {
    char cmd = Serial.read();
    while(Serial.available()) Serial.read(); // Clear buffer
    
    switch(cmd) {
      case 'v': Serial.print(F("Valid: ")); send_seq(3, VALID_GAP); break;
      case 'f': Serial.print(F("Fast: ")); send_seq(3, FAST_GAP); break;
      case 's': Serial.print(F("Slow: ")); send_seq(3, SLOW_GAP); break;
      case 't': Serial.print(F("TooFast: ")); send_seq(3, TOO_FAST_GAP); break;
      case 'T': Serial.print(F("TooSlow: ")); send_seq(3, TOO_SLOW_GAP); break;
      case 'd': Serial.print(F("Double: ")); send_seq(6, VALID_GAP); break;
      case 'h': print_help(); break;
      case '\n': case '\r': break;
      default: Serial.println(F("? for help")); break;
    }
  }
}

void loop() {
  process_cmd();
  delay(10);
}
