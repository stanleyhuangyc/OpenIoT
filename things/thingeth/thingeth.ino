#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "config.h"
#include "thing.h"

byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(172, 16, 0, 100);

long lastMsg = 0;
char msg[50];
int value = 0;

void controlSwitch(int digitalPin, int value);

void callback(char* topic, byte* payload, unsigned int length) {
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");

 Serial.write(payload, length);
 Serial.println();

 if (strcmp(topic, "/8d746c7c-7e73-e88a-cbb8-6057a0f137a6/sw1/com") == 0) {
   controlSwitch(16, payload[0] == '1' ? HIGH : LOW);
 }
}

void controlSwitch(int digitalPin, int value) {
 Serial.print("Set switch ");
 Serial.print(digitalPin);
 Serial.print(" to ");
 Serial.println(value);
 digitalWrite(digitalPin, value);
}

void sendLightSensorData() {
 Serial.print("Sending light sensor data");
 if (client.connected()) {
   int sensorValue = analogRead(A0);
   char data[8];
   sprintf(data, "%d", sensorValue);
   client.publish("/8d746c7c-7e73-e88a-cbb8-6057a0f137a6/n1/com", data);
 }
}

void setup()
{
  pinMode(16, OUTPUT);
  Serial.begin(115200);
  Ethernet.begin(mac, ip);  
  thing.setup_mqtt(MQTT_SERVER, 1883, callback);
}

void loop() {
  static uint32_t lastTick = 0;
  if (millis() - lastTick >= 5000) {
    sendLightSensorData();
  }
  thing.loop();  
}
