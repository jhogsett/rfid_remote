## Project Description

Arduino based RFID Reader with Remote Transmitter, and Remote Receiver with Garage Door Actiation

## Hardware

- Arduino Nano (2, 1 each for transmitter and receiver side)
- Ming Microsystems TX-99 300 MHz Transmitter (5 volts, one data bit)
- Ming Microsystems RE-99 300 MHz Receiver (5 volts, one data bit)
- RC522 RFID Reader for the transmitter side 
- 5 volt relay and MOSFET driver for garage door activation on receiver side

## Software

### Sender

- (details to be filled in later)

### Receiver

- main.cpp
- receives RE-99 pulses on pin 6
- digitally filters pulses to be valid in the range 50-350 ms
- **when it detects three contiguous valid pulses 1000 ms apart, it sets pin 7 to HIGH for two seconds then back to LOW to activate the garage door opener**

## Development History

- Started as an Arduino sketch simply translating pin 6 input level to pin 13 output level to see pulses
- Converted to main.cpp
- Digital filter with serial monitor tuning developed by Copilot Agent
