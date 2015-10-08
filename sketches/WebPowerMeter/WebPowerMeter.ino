
/*
* This sketch measures AC current with SCT-013 CT sensor
* connected directly to A0 of ESP8266 and shows
* realtime power consumption on web-based gauge
*
* Written by Stanley Huang
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>

const char *ssid = "HOMEWIFI";
const char *password = "PASSWORD";

ESP8266WebServer server ( 80 );
Ticker ticker;

#define SENSE_DURATION 1000 /* ms */
#define PIN_CURRENT_SENSOR A0
#define CURRENT_SENSOR_RATIO 0.067 * 2.56

typedef struct {
  byte id;
  unsigned long time;
  float voltage; /* V */
  float current; /* A */
  float watt; /* W */
  float temperature; /* C */
  float humidity; /* % */
} DATA_BLOCK;

DATA_BLOCK meter = {0, 0, 240};
const int led = 13;

void handleRoot() {
	digitalWrite ( led, 1 );
	char buf[1024];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf (buf, sizeof(buf),
"<!DOCTYPE html>\
<html>\
<head>\
<script src='http://code.jquery.com/jquery-1.9.1.js'></script>\
<script src='http://openm2m.com.au/powermeter/common.js'></script>\
<script src='http://openm2m.com.au/powermeter/gauge.js'></script>\
<script src='http://code.highcharts.com/highcharts.js'></script>\
<script src='http://code.highcharts.com/highcharts-more.js'></script>\
<script src='http://code.highcharts.com/modules/exporting.js'></script>\
</head>\
<body>\
<div id='container' style='min-width: 310px; max-width: 400px; height: 300px; margin: 0 auto'></div><div>Uptime: %02u:%02u:%02u</div>\
</body></html>",
		hr, min % 60, sec % 60
	);
	server.send (200, "text/html", buf);
	digitalWrite ( led, 0 );
}

unsigned int a0 = 0;

void readADC()
{
  a0 = analogRead(PIN_CURRENT_SENSOR);
}

void handleSensor()
{
  char buf[256];
  int a = meter.current * 10;
  digitalWrite ( led, HIGH );
  sprintf(buf, "<?xml version=\"1.0\"?><sensors><t>%lu</t><a>%d.%d</a><v>%u</v><w>%lu</w></sensors>",
    millis() - meter.time, a / 10, a % 10, (unsigned int)meter.voltage, (unsigned long)meter.watt);
  Serial.println(buf);
  server.send ( 200, "text/xml", buf );
  digitalWrite(led, LOW);
}

void handleNotFound() {
	digitalWrite ( led, 1 );
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for ( uint8_t i = 0; i < server.args(); i++ ) {
		message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	}

	server.send ( 404, "text/plain", message );
	digitalWrite ( led, 0 );
}

void sensePower()
{
  static uint32_t a2n = 0;
  static uint32_t an = 0;
  static uint32_t n = 0;
  static uint32_t lastTime = 0;
  if (millis() - lastTime > SENSE_DURATION) {
    // only half cycle valid and half samples counted
    n /= 2;
    if (n == 0) n = 1;
    meter.current = sqrt((float)a2n / n) * CURRENT_SENSOR_RATIO;
    meter.watt = meter.current < 0.05 ? 0 : (float)an * meter.voltage / n * CURRENT_SENSOR_RATIO;
    n = 0;
    a2n = 0;
    an = 0;
    meter.time = lastTime = millis();
    Serial.print(meter.watt);
    Serial.println("W");
  } else {
    a2n += (uint32_t)a0 * a0;
    an += a0;
    n++;
  }
}

void setup ( void ) {
	pinMode ( led, OUTPUT );
	digitalWrite ( led, 0 );
	Serial.begin ( 115200 );
	WiFi.begin ( ssid, password );
	Serial.println ( "" );

	// Wait for connection
	while ( WiFi.status() != WL_CONNECTED ) {
		delay ( 500 );
		Serial.print ( "." );
	}

	Serial.println ( "" );
	Serial.print ( "Connected to " );
	Serial.println ( ssid );
	Serial.print ( "IP address: " );
	Serial.println ( WiFi.localIP() );

	server.on ( "/", handleRoot );
	server.on ( "/sensor.xml", handleSensor );
	server.onNotFound ( handleNotFound );
	server.begin();
	Serial.println ( "HTTP server started" );

	ticker.attach_ms(5, readADC);
}

void loop ( void ) {
        sensePower();
	server.handleClient();
}

