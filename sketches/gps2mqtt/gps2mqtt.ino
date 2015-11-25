#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <TinyGPS++.h>
#include "config.h"
#include "thing.h"

TinyGPSPlus gps;

Ticker lightSensorTicker;
long lastMsg = 0;
char msg[48];
int value = 0;

void callback(char* topic, byte* payload, unsigned int length) {
 Serial1.print("Message arrived [");
 Serial1.print(topic);
 Serial1.print("] ");

 Serial1.write(payload, length);
 Serial1.println();

 if (strcmp(topic, "/8d746c7c-7e73-e88a-cbb8-6057a0f137a6/sw1/com") == 0) {
   controlSwitch(16, payload[0] == '1' ? HIGH : LOW);
 }
}

void controlSwitch(int digitalPin, int value) {
 Serial1.print("Set switch ");
 Serial1.print(digitalPin);
 Serial1.print(" to ");
 Serial1.println(value);
 digitalWrite(digitalPin, value);
}

void sendLightSensorData() {
 Serial1.print("Sending light sensor data");
 if (client.connected()) {
   int sensorValue = analogRead(A0);
   String data;
   data += sensorValue;
   client.publish("/8d746c7c-7e73-e88a-cbb8-6057a0f137a6/n1/com", data.c_str());
 }
}

void setup() {
 pinMode(16, OUTPUT);
 Serial.begin(115200);
 Serial1.begin(115200);
 thing.setup_wifi(WIFI_SSID, WIFI_PASSWD);
 thing.setup_mqtt(MQTT_SERVER, 1883, callback);
 lightSensorTicker.attach(5, sendLightSensorData);
}

bool processGPS()
{
  if (Serial.available()) {
    char c = Serial.read();
    if (gps.encode(c)) {
      if (gps.location.isValid() && gps.satellites.value() > 3) {
        return true;
      }
    }
  }
  return false;
}

void sendGPS()
{
  Serial1.print("TIME:");
  Serial1.print(gps.time.value());
  Serial1.print(" LAT:");
  Serial1.print(gps.location.lat());
  Serial1.print(" LON:");
  Serial1.print(gps.location.lng());
  Serial1.print(" ALT:");
  Serial1.print(gps.altitude.meters());
  Serial1.print(" SPEED:");
  Serial1.print((int)gps.speed.kmph());
  Serial1.print(" SAT:");
  Serial1.print(gps.satellites.value());
  Serial1.println();
  // generate JSON data
  
}

void loop() {
  thing.loop();
  if (processGPS()) {
     sendGPS();
  }  
}
