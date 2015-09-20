
/*
* This sketch receives data from data sender
*
* Written by Stanley Huang for Project OpenIoT
*/

#include <SPI.h>
#include "RF24.h"

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(9,10);
/**********************************************************/

byte addresses[][6] = {"SNDER","RCVER"};

typedef struct {
  byte id;
  unsigned long time;
  int temperature;
  int humidity;
  unsigned int voltage;
  int current;
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
        DATA_BLOCK data = {0};
        radio.read( &data, sizeof(data));
        Serial.print('[');
        Serial.print(data.id);
        Serial.print(']');
        Serial.print(data.time);
        if (data.temperature || data.humidity || data.voltage) {
          Serial.print(' ');
          Serial.print((float)data.temperature / 10, 1);
          Serial.print("C ");
          Serial.print((float)data.humidity / 10, 1);
          Serial.print("% ");
          Serial.print((float)data.voltage / 100, 1);
          Serial.print("V ");
          Serial.print((float)data.current / 100, 2);
          Serial.print('A');
        }
        Serial.println();
      }
 }
}
