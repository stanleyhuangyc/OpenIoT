EthernetClient ethClient;
PubSubClient client(ethClient);

class CThing {
public:
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
