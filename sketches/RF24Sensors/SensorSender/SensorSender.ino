
/*
* This sketch senses temperature/humidity with a DHT11 module
* and senses current/voltage level with a MAX471 module
* and transmit data via a nRF24L01 module on SPI
*
* Written by Stanley Huang for Project OpenIoT
*/

#include <SPI.h>
#include <RF24.h>
#include <DHT.h>

#define DEV_ID 4
#define DHT11_PIN 8
#define CURRENT_PIN A0
#define VOLTAGE_PIN A1
#define START_VOLTAGE 10

/* Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(9,10);

byte addresses[][6] = {"SNDER","RCVER"};

DHT dht(DHT11_PIN, DHT11);

#define WORKING_VOLTAGE 4.95

typedef struct {
  byte id;
  unsigned long time;
  int temperature; /* 0.1C */
  unsigned int humidity; /* 0.1% */
  unsigned int voltage; /* 1/100V */
  int current; /* 1/100A */
  int watt; /* W */
} DATA_BLOCK;

void getVoltAmp(float& v, float& a)
{
  int vn = 0;
  int an = 0;

  for (byte n = 0; n < 10; n++) {
    vn += analogRead(VOLTAGE_PIN);
    an += analogRead(CURRENT_PIN);
    delay(100);
  }
  v = (float)vn / 10 / 1024 * WORKING_VOLTAGE * 5;
  a = (float)an / 10 / 1024 * WORKING_VOLTAGE;
}

float getVoltage()
{
  int vn = 0;
  for (byte n = 0; n < 10; n++) {
    vn += analogRead(VOLTAGE_PIN);
    delay(100);
  }
  return (float)vn / 10 / 1024 * WORKING_VOLTAGE * 5;
}

void setup() {
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  // start when voltage above requirement
  for (;;) {
    float v = getVoltage();
    if (v >= START_VOLTAGE) break;
    delay(2000);
  }
  digitalWrite(13, LOW);

  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_MAX);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1,addresses[1]);
}

void loop() {
  DATA_BLOCK data = {DEV_ID, millis()};
  float v;
  float a;
  getVoltAmp(v, a);
  data.voltage = (unsigned int)(v * 100);
  data.current = (int)(a * 100);  
  data.watt = (int)(a * v);
  data.temperature = (int)(dht.readTemperature() * 10);
  data.humidity = (int)(dht.readHumidity() * 10);

  radio.write( &data, sizeof(data));

  delay(4000);
}
