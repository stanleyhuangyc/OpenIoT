// UDPSensors for Arduino Leonardo with ESP8266
// Written by Stanley Huang

#include <esp8266.h>
#include <dht11.h>

#include <SPI.h>
#include <RF24.h>

#define ssid		"HOMEWIFI"		// you need to change it 
#define password	"PASSWORD"
#define serverIP	"52.64.163.83"
#define	serverPort	"33333"

Esp8266 wifi;
dht11 DHT;

#define DHT11_PIN 14
#define SENSE_INTERVAL 1000 /* ms */

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(14,15);
/**********************************************************/

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

void sendData(DATA_BLOCK* data);

void setup() {
	delay(2000);				// it will be better to delay 2s to wait esp8266 module OK
	Serial.begin(115200);
	Serial1.begin(115200);

        radio.begin();
        // Set the PA Level low to prevent power supply related issues since this is a
       // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
        radio.setPALevel(RF24_PA_MAX);
        
        // Open a writing and reading pipe on each radio, with opposite addresses
        radio.openWritingPipe(addresses[1]);
        radio.openReadingPipe(1,addresses[0]);
        
        // Start the radio listening for data
        radio.startListening();

	wifi.begin(&Serial1, &Serial);
	wifi.reset();
    	wifi.debugPrintln("Connecting...");

        for (;;) {
          if (wifi.connectAP(ssid, password)) {
    	    wifi.debugPrintln("AP Connected");
            break;
          }
	}
	String ip_addr;
	ip_addr = wifi.getIP();
	wifi.debugPrintln("IP:" + ip_addr);

	wifi.setSingleConnect();
	if (wifi.connect(serverIP, serverPort, true)) {
		wifi.debugPrintln("Successfully connected !");
	}	
}

#define PACKET_LEN 256

void sendData(DATA_BLOCK* data)
{
  // read sensors
  int bytes = 0;
  
  Stream *serial = wifi.serial;
  
  //serial = (Stream*)&Serial;
  wifi.sendStart(PACKET_LEN);
  bytes += serial->print("{\"deviceID\": \"12334\",\n\"ports\":{\n");
  bytes += serial->print("\"0\":{");

  bytes += serial->print("\"tick\":");
  bytes += serial->print(data->time);
  bytes += serial->print(",\n");
  
  bytes += serial->print("\"temp\":");
  bytes += serial->print((float)data->temperature / 10, 1);
  bytes += serial->print(",\n");

  bytes += serial->print("\"humid\":");
  bytes += serial->print((float)data->humidity / 10, 1);
  bytes += serial->print(",\n");

  bytes += serial->print("\"watt\":");
  bytes += serial->print((float)data->watt / 10, 1);
  bytes += serial->print("}\n");

  bytes += serial->print("\"current\":");
  bytes += serial->print((float)data->current / 100, 1);
  bytes += serial->print(",\n");

  bytes += serial->print("\"voltage\":");
  bytes += serial->print((float)data->voltage / 100, 1);
  bytes += serial->print("}\n");
  
  bytes += serial->print("\"status:\":");
  bytes += serial->print('1');
  bytes += serial->print(",\n");
  
  bytes += serial->print("},\n\"status\":1\n}\n");
  
  for (; bytes < PACKET_LEN; bytes++) wifi.serial->write(' ');

  wifi.serial->flush();

  /*  
  for (uint32_t t = millis(); millis() - t < 200; ) {
    if (wifi.serial->available())
     Serial.write(wifi.serial->read()); 
  } 
  */
}

void receiveRemoteSensors()
{
  if( radio.available()){
    while (radio.available()) {                                   // While there is data ready
      DATA_BLOCK data = {0};
      radio.read( &data, sizeof(data));
      if (data.id == 0) continue;
      Serial.print('[');
      Serial.print(data.id);
      Serial.print(']');
      uint32_t s = data.time / 1000;
      if (s >= 3600) {
        Serial.print(s / 3600);
        Serial.print(':');
        s %= 3600;
      }
      int m = s / 60;
      s %= 60;
      if (m < 10) Serial.print('0');
      Serial.print(m);
      Serial.print(':');
      if (s < 10) Serial.print('0');
      Serial.print(s);
      if (data.voltage || data.current) {
        Serial.print(' ');
        Serial.print(data.watt ? (float)data.watt / 10 : (float)data.voltage * data.current / 10000, 1);
        Serial.print("W ");
        Serial.print((float)data.current / 100, 2);
        Serial.print("A ");
        Serial.print((float)data.voltage / 100, 1);
        Serial.print("V ");
      }
      Serial.println();      
      sendData(&data);
    }
  }
}

void loop() {
	static uint32_t last = 0;
	byte state = wifi.getState();
	switch (state) {
        case WIFI_NEW_MESSAGE: 
          wifi.debugPrintln("Received:");
	  wifi.sendMessage(wifi.getMessage());		//send the message to TCP server what it has received
	  wifi.setState(WIFI_IDLE);
	  break;
	case WIFI_CLOSED :							//reconnet to the TCP server 
	  wifi.debugPrintln("Connecting...");
	  if (wifi.connect(serverIP, serverPort)) {
	    wifi.debugPrintln("OK");
	    wifi.setState(WIFI_IDLE);
	  } else {
	    wifi.debugPrintln("Failed");
	    wifi.setState(WIFI_CLOSED);
	  }
	  break;
	case WIFI_IDLE :							
          receiveRemoteSensors();
          state = wifi.checkMessage(); 
          wifi.setState(state);
          break;
        }
}

