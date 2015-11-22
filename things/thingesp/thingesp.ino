#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include "config.h"
#include "thing.h"

Ticker lightSensorTicker;
long lastMsg = 0;
char msg[48];
int value = 0;

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

void setup() {
 pinMode(16, OUTPUT);
 Serial.begin(115200);
 thing.setup_wifi(WIFI_SSID, WIFI_PASSWD);
 thing.setup_mqtt(MQTT_SERVER, 1883, callback);
 lightSensorTicker.attach(5, sendLightSensorData);
}

void loop() {
  thing.loop();
}
