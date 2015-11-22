WiFiClient espClient;
PubSubClient client(espClient);

class CThing {
public:
  void setup_wifi(const char* ssid, const char* password) {
  
   delay(10);
   // We start by connecting to a WiFi network
   Serial.println();
   Serial.print("Connecting to ");
   Serial.println(ssid);
  
   WiFi.begin(ssid, password);
  
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
   }
  
   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());
  }
  bool setup_mqtt(const char* host, int port, MQTT_CALLBACK_SIGNATURE)
  {
    client.setServer(host, port);
    client.setCallback(callback);
  }
  void connect(const char* user, const char* passwd, const char* topic) {
   // Loop until we're reconnected
   do {
     Serial.print("Attempting MQTT connection...");
     // Attempt to connect
     if (client.connect("ESP8266Client", user, passwd)) {
       Serial.println("connected");
       client.subscribe(topic);
     } else {
       Serial.print("failed, rc=");
       Serial.print(client.state());
       Serial.println(" try again in 5 seconds");
       // Wait 5 seconds before retrying
       delay(5000);
     }
   } while (!client.connected());
  }
  void loop()
  {
    if (!client.connected()) {
      connect(MQTT_USER, MQTT_PASSWD, MQTT_TOPIC);
    }
    client.loop();
  }
};

CThing thing;
