#include <Arduino.h>

#include <Wire.h>
#include <HT16K33Disp.h>

#define RX_PIN 6
#define LED_PIN 13
#define GARAGE_DOOR_PIN 7       // Pin to activate garage door opener
#define MIN_LEGIT_TIME 50000    // 50ms in microseconds
#define MAX_LEGIT_TIME 300000   // 300ms in microseconds
#define RESET_AVG_SAMPLES 25

// Garage door activation parameters
#define PULSE_SEQUENCE_INTERVAL 1000  // 1000ms between pulses in sequence
#define PULSE_SEQUENCE_COUNT 3        // Need 3 contiguous pulses
#define GARAGE_DOOR_ACTIVE_TIME 2000  // Keep pin 7 HIGH for 2 seconds
#define PULSE_TIMING_TOLERANCE 200    // Allow ±200ms tolerance for 1000ms timing

// Digital filter parameters (now adjustable at runtime)
unsigned long DEBOUNCE_TIME_US = 1000;   // 1ms debounce time
unsigned long MIN_STABLE_TIME_US = 5000; // 5ms minimum stable time before state change
int FILTER_SAMPLES = 5;                  // Number of samples for majority vote
unsigned long MIN_LEGIT_TIME_RUNTIME = 50000;    // 50ms in microseconds
unsigned long MAX_LEGIT_TIME_RUNTIME = 350000;   // 300ms in microseconds

#define MAX_FILTER_SAMPLES 15   // Maximum allowed filter samples

// Debug output control
#define DEBUG_FILTER 0          // Set to 1 to enable filter debugging
#define DEBUG_PULSE_WIDTH 1     // Set to 1 to enable pulse width debugging
#define DEBUG_STATE_CHANGES 0   // Set to 1 to enable state change debugging

// Simple tuning interface
void print_tuning_menu() {
  Serial.println("\n=== FILTER TUNING MENU ===");
  Serial.println("Commands:");
  Serial.println("1-9: Set filter samples (1=very loose, 9=very strict)");
  Serial.println("a/A: Decrease/Increase min stable time (currently " + String(MIN_STABLE_TIME_US/1000) + "ms)");
  Serial.println("b/B: Decrease/Increase debounce time (currently " + String(DEBOUNCE_TIME_US/1000) + "ms)");
  Serial.println("c/C: Decrease/Increase min pulse width (currently " + String(MIN_LEGIT_TIME_RUNTIME/1000) + "ms)");
  Serial.println("d/D: Decrease/Increase max pulse width (currently " + String(MAX_LEGIT_TIME_RUNTIME/1000) + "ms)");
  Serial.println("s: Show current settings");
  Serial.println("h: Show this menu");
  Serial.println("============================\n");
}

void process_tuning_command() {
  if(Serial.available()) {
    char cmd = Serial.read();
    
    switch(cmd) {
      case '1': case '2': case '3': case '4': case '5': 
      case '6': case '7': case '8': case '9':
        FILTER_SAMPLES = cmd - '0';
        Serial.println("Filter samples set to: " + String(FILTER_SAMPLES) + " (1=loose, 9=strict)");
        break;
        
      case 'a':
        MIN_STABLE_TIME_US = max(1000UL, MIN_STABLE_TIME_US - 1000);
        Serial.println("Min stable time: " + String(MIN_STABLE_TIME_US/1000) + "ms");
        break;
      case 'A':
        MIN_STABLE_TIME_US += 1000;
        Serial.println("Min stable time: " + String(MIN_STABLE_TIME_US/1000) + "ms");
        break;
        
      case 'b':
        DEBOUNCE_TIME_US = max(100UL, DEBOUNCE_TIME_US - 100);
        Serial.println("Debounce time: " + String(DEBOUNCE_TIME_US/1000) + "ms");
        break;
      case 'B':
        DEBOUNCE_TIME_US += 100;
        Serial.println("Debounce time: " + String(DEBOUNCE_TIME_US/1000) + "ms");
        break;
        
      case 'c':
        MIN_LEGIT_TIME_RUNTIME = max(10000UL, MIN_LEGIT_TIME_RUNTIME - 5000);
        Serial.println("Min pulse width: " + String(MIN_LEGIT_TIME_RUNTIME/1000) + "ms");
        break;
      case 'C':
        MIN_LEGIT_TIME_RUNTIME += 5000;
        Serial.println("Min pulse width: " + String(MIN_LEGIT_TIME_RUNTIME/1000) + "ms");
        break;
        
      case 'd':
        MAX_LEGIT_TIME_RUNTIME = max(MIN_LEGIT_TIME_RUNTIME + 10000, MAX_LEGIT_TIME_RUNTIME - 10000);
        Serial.println("Max pulse width: " + String(MAX_LEGIT_TIME_RUNTIME/1000) + "ms");
        break;
      case 'D':
        MAX_LEGIT_TIME_RUNTIME += 10000;
        Serial.println("Max pulse width: " + String(MAX_LEGIT_TIME_RUNTIME/1000) + "ms");
        break;
        
      case 's':
        Serial.println("\n=== CURRENT SETTINGS ===");
        Serial.println("Filter samples: " + String(FILTER_SAMPLES) + " (1=loose, 9=strict)");
        Serial.println("Min stable time: " + String(MIN_STABLE_TIME_US/1000) + "ms");
        Serial.println("Debounce time: " + String(DEBOUNCE_TIME_US/1000) + "ms");
        Serial.println("Min pulse width: " + String(MIN_LEGIT_TIME_RUNTIME/1000) + "ms");
        Serial.println("Max pulse width: " + String(MAX_LEGIT_TIME_RUNTIME/1000) + "ms");
        Serial.println("=======================\n");
        break;
        
      case 'h':
        print_tuning_menu();
        break;
        
      default:
        // Ignore other characters
        break;
    }
  }
}

HT16K33Disp *disp1, *disp2, *disp3;
byte device_display_count = 0;

// Digital filter state variables
typedef enum {
  FILTER_IDLE,
  FILTER_RISING_EDGE,
  FILTER_HIGH_STABLE,
  FILTER_FALLING_EDGE,
  FILTER_LOW_STABLE
} filter_state_t;

typedef struct {
  filter_state_t state;
  filter_state_t last_state;  // Added for debugging state changes
  unsigned long last_change_time;
  unsigned long pulse_start_time;
  unsigned long last_sample_time;
  bool raw_samples[MAX_FILTER_SAMPLES];  // Use fixed maximum size
  int sample_index;
  bool filtered_state;
  bool last_filtered_state;
  unsigned long debounce_start_time;
  unsigned long debug_last_print_time; // Added for debug timing
} digital_filter_t;

digital_filter_t pulse_filter;

// Garage door activation state variables
typedef struct {
  unsigned long pulse_times[PULSE_SEQUENCE_COUNT];  // Timestamps of last pulses
  int pulse_count;                                  // Number of pulses detected in sequence
  bool garage_door_active;                         // Is garage door currently activated?
  unsigned long garage_door_start_time;           // When garage door activation started
  unsigned long last_valid_pulse_time;            // Timestamp of last valid pulse
} garage_door_state_t;

garage_door_state_t garage_door_state;

#define FIRST_DISPLAY 0x70
#define LAST_DISPLAY  0x77
#define MAX_DISPLAYS  8

int count_displays(){
  int num_devices = 0;
  for (byte address = FIRST_DISPLAY; address <= LAST_DISPLAY; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0)
      num_devices++;
    }
  return num_devices;
}

// Initialize the digital filter
void init_digital_filter() {
  pulse_filter.state = FILTER_IDLE;
  pulse_filter.last_state = FILTER_IDLE;
  pulse_filter.last_change_time = 0;
  pulse_filter.pulse_start_time = 0;
  pulse_filter.last_sample_time = 0;
  pulse_filter.sample_index = 0;
  pulse_filter.filtered_state = false;
  pulse_filter.last_filtered_state = false;
  pulse_filter.debounce_start_time = 0;
  pulse_filter.debug_last_print_time = 0;
  
  // Initialize sample buffer with false values
  for(int i = 0; i < MAX_FILTER_SAMPLES; i++) {
    pulse_filter.raw_samples[i] = false;
  }
}

// Initialize garage door state
void init_garage_door_state() {
  garage_door_state.pulse_count = 0;
  garage_door_state.garage_door_active = false;
  garage_door_state.garage_door_start_time = 0;
  garage_door_state.last_valid_pulse_time = 0;
  
  // Initialize pulse times array
  for(int i = 0; i < PULSE_SEQUENCE_COUNT; i++) {
    garage_door_state.pulse_times[i] = 0;
  }
}

// Process valid pulse for garage door activation sequence
void process_garage_door_sequence(unsigned long current_time_ms) {
  // Check if this pulse is part of a valid sequence
  bool is_sequence_pulse = false;
  
  if(garage_door_state.pulse_count == 0) {
    // First pulse in potential sequence
    is_sequence_pulse = true;
  } else {
    // Check if this pulse is approximately 1000ms after the last one
    unsigned long time_since_last = current_time_ms - garage_door_state.last_valid_pulse_time;
    
    if(time_since_last >= (PULSE_SEQUENCE_INTERVAL - PULSE_TIMING_TOLERANCE) && 
       time_since_last <= (PULSE_SEQUENCE_INTERVAL + PULSE_TIMING_TOLERANCE)) {
      // This pulse is within the timing window
      is_sequence_pulse = true;
    } else {
      // Timing is off, reset sequence and start over
      garage_door_state.pulse_count = 0;
      is_sequence_pulse = true; // This could be the start of a new sequence
      Serial.println("Pulse sequence reset - timing off");
    }
  }
  
  if(is_sequence_pulse) {
    // Add this pulse to the sequence
    garage_door_state.pulse_times[garage_door_state.pulse_count] = current_time_ms;
    garage_door_state.pulse_count++;
    garage_door_state.last_valid_pulse_time = current_time_ms;
    
    Serial.print("Valid pulse ");
    Serial.print(garage_door_state.pulse_count);
    Serial.print(" of ");
    Serial.print(PULSE_SEQUENCE_COUNT);
    Serial.println(" in sequence");
    
    // Check if we have completed the sequence
    if(garage_door_state.pulse_count >= PULSE_SEQUENCE_COUNT) {
      // Activate garage door!
      garage_door_state.garage_door_active = true;
      garage_door_state.garage_door_start_time = current_time_ms;
      garage_door_state.pulse_count = 0; // Reset for next sequence
      
      digitalWrite(GARAGE_DOOR_PIN, HIGH);
      
      Serial.println("*** GARAGE DOOR ACTIVATED! ***");
      Serial.print("Pin ");
      Serial.print(GARAGE_DOOR_PIN);
      Serial.print(" set HIGH for ");
      Serial.print(GARAGE_DOOR_ACTIVE_TIME);
      Serial.println(" ms");
    }
  }
}

// Update garage door activation state
void update_garage_door_state(unsigned long current_time_ms) {
  if(garage_door_state.garage_door_active) {
    // Check if it's time to deactivate
    if(current_time_ms - garage_door_state.garage_door_start_time >= GARAGE_DOOR_ACTIVE_TIME) {
      garage_door_state.garage_door_active = false;
      digitalWrite(GARAGE_DOOR_PIN, LOW);
      
      Serial.println("*** GARAGE DOOR DEACTIVATED ***");
      Serial.print("Pin ");
      Serial.print(GARAGE_DOOR_PIN);
      Serial.println(" set LOW");
    }
  }
}

// Digital filter function - returns true if a valid pulse edge is detected
bool process_digital_filter(bool raw_input, unsigned long current_time_us) {
  // Sample the input at regular intervals (every 100us)
  if(current_time_us - pulse_filter.last_sample_time >= 100) {
    pulse_filter.last_sample_time = current_time_us;
    
    // Store raw sample in circular buffer
    pulse_filter.raw_samples[pulse_filter.sample_index] = raw_input;
    pulse_filter.sample_index = (pulse_filter.sample_index + 1) % FILTER_SAMPLES;
    
    // Majority vote filtering - count true samples (only use active samples)
    int true_count = 0;
    for(int i = 0; i < FILTER_SAMPLES; i++) {
      if(pulse_filter.raw_samples[i]) true_count++;
    }
    
    // Update filtered state based on majority vote
    bool new_filtered_state = (true_count > FILTER_SAMPLES / 2);
    
    // Debug output for filter state (every 10ms to avoid spam)
    #if DEBUG_FILTER
    if(current_time_us - pulse_filter.debug_last_print_time >= 10000) {
      pulse_filter.debug_last_print_time = current_time_us;
      Serial.print("Raw: ");
      Serial.print(raw_input ? "H" : "L");
      Serial.print(" | Votes: ");
      Serial.print(true_count);
      Serial.print("/");
      Serial.print(FILTER_SAMPLES);
      Serial.print(" | Filtered: ");
      Serial.print(new_filtered_state ? "H" : "L");
      Serial.print(" | State: ");
      Serial.println(pulse_filter.state);
    }
    #endif
    
    // Store previous state for change detection
    pulse_filter.last_state = pulse_filter.state;
    
    // State machine for edge detection with hysteresis
    switch(pulse_filter.state) {
      case FILTER_IDLE:
        if(new_filtered_state && !pulse_filter.filtered_state) {
          pulse_filter.state = FILTER_RISING_EDGE;
          pulse_filter.debounce_start_time = current_time_us;
          #if DEBUG_STATE_CHANGES
          Serial.println("State: IDLE -> RISING_EDGE");
          #endif
        }
        break;
        
      case FILTER_RISING_EDGE:
        if(new_filtered_state) {
          // Check if we've been stable high long enough
          if(current_time_us - pulse_filter.debounce_start_time >= MIN_STABLE_TIME_US) {
            pulse_filter.state = FILTER_HIGH_STABLE;
            pulse_filter.pulse_start_time = current_time_us;
            #if DEBUG_STATE_CHANGES
            Serial.print("State: RISING_EDGE -> HIGH_STABLE (pulse start: ");
            Serial.print(pulse_filter.pulse_start_time);
            Serial.println(")");
            #endif
          }
        } else {
          // False trigger, go back to idle
          pulse_filter.state = FILTER_IDLE;
          #if DEBUG_STATE_CHANGES
          Serial.println("State: RISING_EDGE -> IDLE (false trigger)");
          #endif
        }
        break;
        
      case FILTER_HIGH_STABLE:
        if(!new_filtered_state) {
          pulse_filter.state = FILTER_FALLING_EDGE;
          pulse_filter.debounce_start_time = current_time_us;
          #if DEBUG_STATE_CHANGES
          Serial.println("State: HIGH_STABLE -> FALLING_EDGE");
          #endif
        }
        break;
        
      case FILTER_FALLING_EDGE:
        if(!new_filtered_state) {
          // Check if we've been stable low long enough
          if(current_time_us - pulse_filter.debounce_start_time >= MIN_STABLE_TIME_US) {
            pulse_filter.state = FILTER_LOW_STABLE;
            
            // Calculate pulse width and validate
            unsigned long pulse_width = current_time_us - pulse_filter.pulse_start_time;
            
            #if DEBUG_PULSE_WIDTH
            Serial.print("Pulse width measured: ");
            Serial.print(pulse_width);
            Serial.print(" us (");
            Serial.print(pulse_width / 1000);
            Serial.print(" ms) | Valid range: ");
            Serial.print(MIN_LEGIT_TIME_RUNTIME / 1000);
            Serial.print("-");
            Serial.print(MAX_LEGIT_TIME_RUNTIME / 1000);
            Serial.print(" ms | ");
            #endif
            
            if(pulse_width >= MIN_LEGIT_TIME_RUNTIME && pulse_width <= MAX_LEGIT_TIME_RUNTIME) {
              // Valid pulse detected - return true to indicate pulse end
              pulse_filter.state = FILTER_IDLE;
              pulse_filter.filtered_state = false;
              #if DEBUG_PULSE_WIDTH
              Serial.println("VALID PULSE!");
              #endif
              #if DEBUG_STATE_CHANGES
              Serial.println("State: FALLING_EDGE -> IDLE (valid pulse)");
              #endif
              return true;
            } else {
              // Invalid pulse width - ignore
              pulse_filter.state = FILTER_IDLE;
              #if DEBUG_PULSE_WIDTH
              Serial.println("INVALID PULSE (out of range)");
              #endif
              #if DEBUG_STATE_CHANGES
              Serial.println("State: FALLING_EDGE -> IDLE (invalid pulse width)");
              #endif
            }
          }
        } else {
          // Still high, go back to stable high
          pulse_filter.state = FILTER_HIGH_STABLE;
          #if DEBUG_STATE_CHANGES
          Serial.println("State: FALLING_EDGE -> HIGH_STABLE (still high)");
          #endif
        }
        break;
        
      case FILTER_LOW_STABLE:
        if(new_filtered_state) {
          pulse_filter.state = FILTER_RISING_EDGE;
          pulse_filter.debounce_start_time = current_time_us;
          #if DEBUG_STATE_CHANGES
          Serial.println("State: LOW_STABLE -> RISING_EDGE");
          #endif
        } else {
          // Check if we've been low long enough to go back to idle
          if(current_time_us - pulse_filter.debounce_start_time >= DEBOUNCE_TIME_US) {
            pulse_filter.state = FILTER_IDLE;
            #if DEBUG_STATE_CHANGES
            Serial.println("State: LOW_STABLE -> IDLE");
            #endif
          }
        }
        break;
    }
    
    pulse_filter.filtered_state = new_filtered_state;
  }
  
  return false; // No valid pulse edge detected
}


void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(RX_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(GARAGE_DOOR_PIN, OUTPUT);  // Initialize garage door pin as output
  
  // Ensure garage door pin starts LOW
  digitalWrite(GARAGE_DOOR_PIN, LOW);

  // Initialize the digital filter
  init_digital_filter();
  
  // Initialize garage door state
  init_garage_door_state();
  
  // Show tuning menu
  print_tuning_menu();

  Serial.println("*** GARAGE DOOR RECEIVER INITIALIZED ***");
  Serial.print("Listening for pulse sequences on pin ");
  Serial.println(RX_PIN);
  Serial.print("Will activate garage door on pin ");
  Serial.print(GARAGE_DOOR_PIN);
  Serial.print(" after ");
  Serial.print(PULSE_SEQUENCE_COUNT);
  Serial.print(" pulses spaced ");
  Serial.print(PULSE_SEQUENCE_INTERVAL);
  Serial.println("ms apart");

  device_display_count = count_displays();
  switch(device_display_count){
    case 1:
    {
      byte brightness[1] = { 9 }; // Amber
      disp1 = new HT16K33Disp(0x70, 1);
      disp1->init(brightness);
      break;
    }
    case 2:
    {
      byte brightness[2] = { 15, 15 }; // Red
      disp1 = new HT16K33Disp(0x70, 2);
      disp2 = new HT16K33Disp(0x71, 1);
      disp1->init(brightness);
      break;
    }
    case 3:
    {
      byte brightness[3] = { 1, 1, 1 }; // Green
      disp1 = new HT16K33Disp(0x70, 3);
      disp2 = new HT16K33Disp(0x71, 1);
      disp3 = new HT16K33Disp(0x72, 1);
      disp1->init(brightness);
      break;
    }
  }
}

void loop() {
  unsigned long maxtime = 0L;
  unsigned long mintime = (unsigned long)-1L;
  int sample_count = 0;
  char buffer[20] = {"\0"};
  bool running1 = false;

  while(true){
    unsigned long current_time_us = micros();
    unsigned long current_time_ms = millis();
    bool raw_input = digitalRead(RX_PIN);
    
    // Process tuning commands
    process_tuning_command();
    
    // Process the digital filter
    bool valid_pulse_detected = process_digital_filter(raw_input, current_time_us);
    
    // Update LED based on filtered state
    digitalWrite(LED_PIN, pulse_filter.filtered_state ? HIGH : LOW);
    
    // Update garage door state (handles deactivation timing)
    update_garage_door_state(current_time_ms);
    
    // If a valid pulse was detected, process it
    if(valid_pulse_detected) {
      unsigned long pulse_width = current_time_us - pulse_filter.pulse_start_time;
      
      // Process this pulse for garage door activation sequence
      process_garage_door_sequence(current_time_ms);
      
      if(++sample_count > RESET_AVG_SAMPLES){
        sample_count = 1;
        Serial.println("Resetting Bounds");
        maxtime = 0L;
        mintime = (unsigned long)-1L;
      }

      if(pulse_width < mintime)
        mintime = pulse_width;
      if(pulse_width > maxtime)
        maxtime = pulse_width;

      int rdiff = pulse_width / 1000L;
      int rmintime = mintime / 1000L;
      int rmaxtime = maxtime / 1000L;

      sprintf(buffer, "%4d%4d%4d", rdiff, rmintime, rmaxtime);

      Serial.print("Filtered Duration, Min, Max: ");
      Serial.print(rdiff);
      Serial.print(", ");
      Serial.print(rmintime);
      Serial.print(", ");
      Serial.print(rmaxtime);
      Serial.print(" (State: ");
      Serial.print(pulse_filter.state);
      Serial.println(")");
    }

    unsigned long dtime = millis();
    if(!running1)
      disp1->begin_scroll_string(buffer, 100, 100);

    running1 = disp1->step_scroll_string(dtime);  
  }
}

int main() {
  // Initialize Arduino core
  init();
  
  // Call setup function once
  setup();
  
  // Call loop function repeatedly
  while (true) {
    loop();
  }
  
  return 0;
}
