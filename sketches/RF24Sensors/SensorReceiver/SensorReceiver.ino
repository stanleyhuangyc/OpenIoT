
/*
* Getting Started example sketch for nRF24L01+ radios
* This is a very basic example of how to send data from one node to another
* Updated: Dec 2014 by TMRh20
*/

#include <SPI.h>
#include "RF24.h"

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(7,8);
/**********************************************************/

byte addresses[][6] = {"SNDER","RCVER"};

typedef struct {
  byte id;
  unsigned long time;  
} DATA_BLOCK;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("RF24 Data Receiver"));
  
  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
 // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_MAX);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  
  // Start the radio listening for data
  radio.startListening();
}

void loop() {
    if( radio.available()){
      while (radio.available()) {                                   // While there is data ready
        DATA_BLOCK data;
        radio.read( &data, sizeof(data) );             // Get the payload
        Serial.print('[');
        Serial.print(data.id);
        Serial.print(']');
        Serial.println(data.time);
      }
 }
} // Loop

