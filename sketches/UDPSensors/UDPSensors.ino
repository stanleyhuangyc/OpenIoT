// UDPSensors
// Written by Stanley Huang

#include <esp8266.h>
#include <dht11.h>

#define ssid		"HOMEWIFI"		// you need to change it 
#define password	"PASSWORD"
#define serverIP	"192.168.0.2"
#define	serverPort	"8081"

Esp8266 wifi;
dht11 DHT;

#define DHT11_PIN 14
#define SENSE_INTERVAL 1000 /* ms */

void setup() {
	
	delay(2000);				// it will be better to delay 2s to wait esp8266 module OK
	Serial.begin(115200);
	Serial1.begin(115200);
        while (!Serial);
	wifi.begin(&Serial1, &Serial);
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

void loop() {
        uint32_t last = 0
	byte state = wifi.getState();
	switch (state) {
	    case WIFI_NEW_MESSAGE: 
	      wifi.debugPrintln("new message!");
	      wifi.sendMessage(wifi.getMessage());		//send the message to TCP server what it has received
	      wifi.setState(WIFI_IDLE);
	      break;
	    case WIFI_CLOSED :							//reconnet to the TCP server 
	      wifi.debugPrintln("Connecting...");
	      if (wifi.connect(serverIP, serverPort)) {
	      	wifi.debugPrintln("OK");
	      	wifi.setState(WIFI_IDLE);
	      }
	      else {
	      	wifi.debugPrintln("Failed");
	      	wifi.setState(WIFI_CLOSED);
	      }
	      break;
	    case WIFI_IDLE :							
	    	byte state = wifi.checkMessage(); 
	    	wifi.setState(state);
                if (state != WIFI_IDLE) break;
                
                if (millis() - last > SENSE_INTERVAL) {
                  // read sensors
                  if (DHT.read(DHT11_PIN) == DHTLIB_OK) {
                    char buf[32];
                    sprintf(buf, "%lu,%d,%d", millis(), (int)DHT.temperature, (int)DHT.humidity);
                    wifi.sendMessage(buf);
                    Serial.println(buf);
                  }
                  last = millis();
                }
	    	break;
	}

}

