/*
  RFID Remote Transmitter Test Code
  
  Interactive test transmitter for garage door receiver timing sensitivity testing.
  Sends various pulse sequences via serial commands to test receiver tolerance.
  
  Commands:
  - v: Send valid 3-pulse sequence (1000ms spacing)
  - f: Send fast sequence (800ms spacing - minimum valid)
  - s: Send slow sequence (1200ms spacing - maximum valid)
  - t: Send too-fast sequence (750ms spacing - should be invalid)
  - T: Send too-slow sequence (1300ms spacing - should be invalid)
  - c: Send custom timing sequence (prompts for timing values)
  - i: Send interference test (extra pulses mixed with valid sequence)
  - r: Send random timing test
  - h: Show help menu
*/

#include <Arduino.h>

// Pin definitions
#define TX_PIN LED_BUILTIN    // Use built-in LED pin for transmission (usually pin 13)
#define PULSE_WIDTH 500       // Width of each pulse in milliseconds (500ms for proper RF timing)

// Timing test values
#define VALID_SPACING 500     // Valid pulse spacing (500ms gap = 1000ms total pulse-to-pulse)
#define FAST_SPACING 300      // Fast but still valid (300ms gap = 800ms total)
#define SLOW_SPACING 700      // Slow but still valid (700ms gap = 1200ms total)
#define TOO_FAST_SPACING 250  // Too fast - should be rejected (250ms gap = 750ms total)
#define TOO_SLOW_SPACING 800  // Too slow - should be rejected (800ms gap = 1300ms total)

void setup() {
  Serial.begin(115200);
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);  // Start with transmitter off
  
  Serial.println("*** RFID REMOTE TRANSMITTER TEST ***");
  Serial.println("Interactive pulse sequence generator for receiver testing");
  print_help_menu();
}

void print_help_menu() {
  Serial.println("\n=== TRANSMITTER TEST COMMANDS ===");
  Serial.println("v - Valid sequence (500ms gaps = 1000ms pulse-to-pulse)");
  Serial.println("f - Fast sequence (300ms gaps = 800ms pulse-to-pulse)");
  Serial.println("s - Slow sequence (700ms gaps = 1200ms pulse-to-pulse)");
  Serial.println("t - Too fast sequence (250ms gaps = 750ms pulse-to-pulse)");
  Serial.println("T - Too slow sequence (800ms gaps = 1300ms pulse-to-pulse)");
  Serial.println("c - Custom timing (enter your own gap values)");
  Serial.println("i - Interference test (valid sequence + extra pulses)");
  Serial.println("r - Random timing test (10 sequences with random gaps)");
  Serial.println("1 - Single pulse test");
  Serial.println("2 - Two pulse test (incomplete sequence)");
  Serial.println("h - Show this help menu");
  Serial.println("=====================================");
  Serial.println("NOTE: Gap time + 500ms pulse = total pulse-to-pulse time");
  Serial.println("Ready for commands...");
}

void send_pulse() {
  digitalWrite(TX_PIN, HIGH);
  delay(PULSE_WIDTH);
  digitalWrite(TX_PIN, LOW);
  
  Serial.print("Pulse sent (");
  Serial.print(PULSE_WIDTH);
  Serial.println("ms width)");
}

void send_pulse_sequence(int pulse_count, unsigned long gap_ms, String description) {
  Serial.println("\n*** " + description + " ***");
  Serial.print("Sending ");
  Serial.print(pulse_count);
  Serial.print(" pulses with ");
  Serial.print(gap_ms);
  Serial.print("ms gaps (");
  Serial.print(gap_ms + PULSE_WIDTH);
  Serial.println("ms total pulse-to-pulse)...");
  
  for(int i = 0; i < pulse_count; i++) {
    Serial.print("Pulse ");
    Serial.print(i + 1);
    Serial.print(" of ");
    Serial.print(pulse_count);
    Serial.print(" - ");
    
    send_pulse();
    
    // Don't delay after the last pulse
    if(i < pulse_count - 1) {
      Serial.print("Gap: ");
      Serial.print(gap_ms);
      Serial.println("ms...");
      delay(gap_ms);
    }
  }
  
  Serial.println("Sequence complete!\n");
}

void send_interference_test() {
  Serial.println("\n*** INTERFERENCE TEST ***");
  Serial.println("Sending: pulse -> 500ms -> pulse -> 1000ms -> pulse -> 600ms -> pulse -> 1000ms -> pulse");
  Serial.println("(Should only detect the last 3 pulses as valid sequence)");
  
  // Send interfering pulses
  Serial.println("Interference pulse 1:");
  send_pulse();
  delay(500);
  
  Serial.println("Interference pulse 2:");
  send_pulse();
  delay(1000);
  
  // Now send valid sequence
  Serial.println("Valid sequence start:");
  Serial.println("Valid pulse 1:");
  send_pulse();
  delay(1000);
  
  Serial.println("Valid pulse 2:");
  send_pulse();
  delay(1000);
  
  Serial.println("Valid pulse 3:");
  send_pulse();
  
  Serial.println("Interference test complete!\n");
}

void send_random_timing_test() {
  Serial.println("\n*** RANDOM TIMING TEST ***");
  Serial.println("Sending 10 sequences with random gap timing (100-900ms range)");
  
  for(int seq = 1; seq <= 10; seq++) {
    unsigned long random_gap = random(100, 901); // 100-900ms gaps
    unsigned long total_spacing = random_gap + PULSE_WIDTH; // Total pulse-to-pulse time
    String description = "Random Sequence " + String(seq) + " (" + String(random_gap) + "ms gap = " + String(total_spacing) + "ms total)";
    
    // Determine if this should be valid (800-1200ms total spacing)
    bool should_be_valid = (total_spacing >= 800 && total_spacing <= 1200);
    Serial.print("Expected result: ");
    Serial.println(should_be_valid ? "VALID" : "INVALID");
    
    send_pulse_sequence(3, random_gap, description);
    delay(2000); // Wait 2 seconds between sequences
  }
  
  Serial.println("Random timing test complete!\n");
}

void send_custom_sequence() {
  Serial.println("\n*** CUSTOM TIMING SEQUENCE ***");
  Serial.println("Enter gap values for 3-pulse sequence (gap = time between pulses):");
  
  unsigned long gap1, gap2;
  
  Serial.print("Gap between pulse 1 and 2 (ms): ");
  while(!Serial.available()) { delay(10); }
  gap1 = Serial.parseInt();
  Serial.print(gap1);
  Serial.print("ms (total spacing: ");
  Serial.print(gap1 + PULSE_WIDTH);
  Serial.println("ms)");
  
  // Clear any remaining characters
  while(Serial.available()) { Serial.read(); }
  
  Serial.print("Gap between pulse 2 and 3 (ms): ");
  while(!Serial.available()) { delay(10); }
  gap2 = Serial.parseInt();
  Serial.print(gap2);
  Serial.print("ms (total spacing: ");
  Serial.print(gap2 + PULSE_WIDTH);
  Serial.println("ms)");
  
  // Clear any remaining characters
  while(Serial.available()) { Serial.read(); }
  
  Serial.println("\nSending custom sequence...");
  Serial.print("Pulse 1 - ");
  send_pulse();
  
  Serial.print("Gap: ");
  Serial.print(gap1);
  Serial.println("ms...");
  delay(gap1);
  
  Serial.print("Pulse 2 - ");
  send_pulse();
  
  Serial.print("Gap: ");
  Serial.print(gap2);
  Serial.println("ms...");
  delay(gap2);
  
  Serial.print("Pulse 3 - ");
  send_pulse();
  
  Serial.println("Custom sequence complete!\n");
}

void process_command() {
  if(Serial.available()) {
    char cmd = Serial.read();
    
    // Clear any remaining characters in buffer
    while(Serial.available()) { Serial.read(); }
    
    switch(cmd) {
      case 'v':
        send_pulse_sequence(3, VALID_SPACING, "VALID SEQUENCE (500ms gaps = 1000ms total)");
        break;
        
      case 'f':
        send_pulse_sequence(3, FAST_SPACING, "FAST SEQUENCE (300ms gaps = 800ms total)");
        break;
        
      case 's':
        send_pulse_sequence(3, SLOW_SPACING, "SLOW SEQUENCE (700ms gaps = 1200ms total)");
        break;
        
      case 't':
        send_pulse_sequence(3, TOO_FAST_SPACING, "TOO FAST SEQUENCE (250ms gaps = 750ms total)");
        break;
        
      case 'T':
        send_pulse_sequence(3, TOO_SLOW_SPACING, "TOO SLOW SEQUENCE (800ms gaps = 1300ms total)");
        break;
        
      case 'c':
        send_custom_sequence();
        break;
        
      case 'i':
        send_interference_test();
        break;
        
      case 'r':
        send_random_timing_test();
        break;
        
      case '1':
        send_pulse_sequence(1, 0, "SINGLE PULSE TEST");
        break;
        
      case '2':
        send_pulse_sequence(2, VALID_SPACING, "TWO PULSE TEST (incomplete sequence)");
        break;
        
      case 'h':
        print_help_menu();
        break;
        
      case '\n':
      case '\r':
        // Ignore newlines
        break;
        
      default:
        Serial.print("Unknown command: '");
        Serial.print(cmd);
        Serial.println("' - Press 'h' for help");
        break;
    }
  }
}

void loop() {
  process_command();
  delay(10); // Small delay to prevent overwhelming the serial buffer
}
