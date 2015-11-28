/*************************************************************************
* GPS tracker sketch for ESP8266
* Distributed under GPL v2.0
* Written by Stanley Huang <stanleyhuangyc@gmail.com>
*************************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <ESP8266WiFi.h>
#include "SH1106.h"

#define SSID "iPhone"
#define PASSWORD "12121212"
#define HOST "live.freematics.com.au"
#define PORT 8080
#define VIN "AUDI-A3-ETRON"

TinyGPSPlus gps;
WiFiClient client;
int channel = 0;
uint32_t count = 0;

LCD_SH1106 lcd; /* for SH1106 OLED module */

const uint8_t tick[16 * 16 / 8] =
{0x00,0x80,0xC0,0xE0,0xC0,0x80,0x00,0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0x78,0x30,0x00,0x00,0x01,0x03,0x07,0x0F,0x1F,0x1F,0x1F,0x0F,0x07,0x03,0x01,0x00,0x00,0x00,0x00};

const uint8_t cross[16 * 16 / 8] =
{0x00,0x0C,0x1C,0x3C,0x78,0xF0,0xE0,0xC0,0xE0,0xF0,0x78,0x3C,0x1C,0x0C,0x00,0x00,0x00,0x30,0x38,0x3C,0x1E,0x0F,0x07,0x03,0x07,0x0F,0x1E,0x3C,0x38,0x30,0x00,0x00};

String receiveData(const char* expected, int timeout)
{
  uint32_t t = millis();
  bool httpOK = false;
  String data;
  client.setTimeout(timeout);
  do {
    if (!client.available()) continue;
    String line = client.readStringUntil('\n');
    //Serial.println(line);
    if (!httpOK && line.indexOf("200 OK") > 0) {
      httpOK = true;
      continue;
    }
    if (!httpOK) continue;
    if (line.indexOf("OK") >= 0) {
      break;
    } else if (line.indexOf(expected) >= 0) {
      data = line;
    }
  } while (millis() - t < timeout);
  return data;
}

void setup()
{
	lcd.begin();
  lcd.clear();
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  Serial.begin(115200);
  Serial.println("TRACKING TEST");
  //Serial1.begin(38400);

  char lastch = 0;
  lcd.println("Connecting...");

  WiFi.begin(SSID, PASSWORD);  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lcd.clear();
  lcd.print("WIFI");
  lcd.draw(tick, 16, 16);
  lcd.setFontSize(FONT_SIZE_SMALL);
  lcd.print(WiFi.localIP());
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.println();

  // login server
  for (;;) {
    lcd.print("SERVER");
    if (!client.connect(HOST, PORT)) {
        lcd.draw(cross, 16, 16);
        lcd.print('\r');
    } else {
      lcd.draw(tick, 16, 16);
      lcd.println();
      break;
    }
  }
  client.print("GET /push?VIN=");
  client.print(VIN);
  client.print(" HTTP/1.1\r\nConnection: keep-alive\r\n\r\n");
  String data = receiveData("CH:", 3000);
  lcd.print("Channel:");
  if (data.length() >= 4) {
    channel = atoi(data.c_str() + 3);
    lcd.println(channel);
  } else {
    lcd.draw(cross, 16, 16);
    lcd.println(data);
  }

  // test GPS
  for (;;) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' && lastch == '\r') {
        lcd.print("GPS ");
        lcd.draw(tick, 16, 16);
        delay(2000);
        break;
      }
      lastch = c;
    }    
  }

  lcd.clear();
  //sender.attach(1, sendData);
}

bool sendData()
{
  if (!client.connected()) {
    client.connect(HOST, PORT);
  }
  String req;
  req = "GET /push?id=";
  req += channel;
  req += "&C=";
  req += count;
  req += "&GPS=";
  req += gps.time.value();
  req += ',';
  req += (int32_t)(gps.location.lat() * 1000000);
  req += ',';
  req += (int32_t)(gps.location.lng() * 1000000);
  req += ',';
  req += gps.altitude.value();
  req += ',';
  req += (int)gps.speed.kmph();
  req += ',';
  req += gps.satellites.value();
  req += " HTTP/1.1\r\nConnection: keep-alive\r\n\r\n";
  client.print(req);
  return true;
}

void loop()
{
  static uint32_t lastTime = 0;
  if (Serial.available()) {
    char c = Serial.read();
    if (gps.encode(c)) {
      lcd.setFontSize(FONT_SIZE_MEDIUM);
      lcd.setCursor(0, 0);
      if (gps.time.isValid())
      {
        char buf[32];
        sprintf(buf, "%02u:%02u:%02u.%01u", 
          gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond() / 10);
        lcd.print(buf);
      }
      lcd.setFontSize(FONT_SIZE_SMALL);
      lcd.setCursor(80, 0);

      lcd.setCursor(0, 3);
      if (gps.location.isValid())
      {
        lcd.print("LAT:");
        lcd.println(gps.location.lat(), 6);
        lcd.print("LNG:");
        lcd.println(gps.location.lng(), 6);
      }
      lcd.setCursor(0, 5);
      if (gps.speed.isValid())
      {
        lcd.print((unsigned int)gps.speed.kmph());
        lcd.print("kph ");
      }
      if (gps.altitude.isValid())
      {
        lcd.print((int)gps.altitude.meters());
        lcd.print("m ");
      }
      if (gps.satellites.isValid()) {
        lcd.print("SAT:");
        lcd.print(gps.satellites.value());
        lcd.print(' ');
      }
      if (gps.location.isValid() && millis() - lastTime >= 1000) {
        if (count){        
          String data = receiveData("T:", 1000);
          if (data.length() == 0) {
            client.stop();
          }
          lcd.setCursor(0, 7);
          lcd.print(data);
        }
        if (sendData()) {
          Serial.println(millis());
          lcd.print(" C:");
          lcd.print(count);
          lastTime = millis();
        }
        count++;
      }
    }
  }
}

