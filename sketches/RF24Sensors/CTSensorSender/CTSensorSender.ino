
/*
* This sketch measures AC current with SCT-013 CT sensor
* and transmit data via a nRF24L01 module on SPI
*
* Written by Stanley Huang for Project OpenIoT
*/

#include <SPI.h>
#include <RF24.h>

#define DEV_ID 1
#define PIN_CURRENT_SENSOR A0
#define PIN_VOLTAGE_SENSOR A1
#define CURRENT_SENSOR_RATIO 0.067

/* Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(9,10);

byte addresses[][6] = {"SNDER","RCVER"};

typedef struct {
  byte id;
  unsigned long time;
  int temperature; /* 0.1C */
  unsigned int humidity; /* 0.1% */
  unsigned int voltage; /* 1/100V */
  int current; /* 1/100A */
  int watt; /* 1/10W */
} DATA_BLOCK;

DATA_BLOCK data = {DEV_ID};

void sensePower(float& amp, float& volt, float& watt, unsigned int duration)
{
  uint32_t a2n = 0;
  uint32_t an = 0;
  uint32_t t = millis();
  uint32_t n = 0;
  do {
    unsigned int a = analogRead(PIN_CURRENT_SENSOR);
    a2n += (uint32_t)a * a;
    an += a;
    n++;
  } while (millis() - t < duration);
  // only half cycle valid and half samples counted
  n /= 2;
  amp = sqrt((float)a2n / n) * CURRENT_SENSOR_RATIO;
  watt = amp < 0.05 ? 0 : (float)an * volt / n * CURRENT_SENSOR_RATIO;
}

void setup() {
  // we are going to measure very low input voltage
  analogReference(INTERNAL);

  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_HIGH);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1,addresses[1]);
}

void loop() {
  float amp = 0, volt = 240, watt = 0;
  sensePower(amp, volt, watt, 1000);
  
  // fill up data block for transmission
  data.time = millis();
  data.current = amp * 100;
  data.voltage = volt * 100;
  data.watt = watt * 10;

  // send data block
  radio.write( &data, sizeof(data));
}
