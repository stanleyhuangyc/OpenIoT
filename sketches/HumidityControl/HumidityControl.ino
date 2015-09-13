#include "blunoAccessory.h"
#include "U8glib.h"

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);
blunoAccessory myAccessory;

#define THRESHOLD_OFF 35
#define THRESHOLD_ON 33

bool on = false;

void setup() {             
  Serial.begin(115200);   
  myAccessory.begin(); 
  u8g.setFont(u8g_font_unifont);
}

// the loop routine runs over and over again forever:
void loop() {
  float h = myAccessory.readHumidity();
  float t = myAccessory.readTemperature();
  Serial.print("Humidity: "); 
  Serial.print(h);
  Serial.print("%\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.println("C");
  
  u8g.firstPage();
  do {
    char buf[32];
    sprintf(buf, "Humidity:%d%%", (int)h);
    u8g.drawStr(0,22,buf);
    sprintf(buf, "Temperature:%dC", (int)t);
    u8g.drawStr(2,22,buf);
  } while(u8g.nextPage());

  if (on) {
    if (h > THRESHOLD_OFF) {
      myAccessory.setRelay(false);
      on = false; 
    }
  } else {
    if (h < THRESHOLD_ON) {
      myAccessory.setRelay(true);
      on = true; 
    }    
  }

  delay(1000);                   // wait for a second 
}
