/*
  Energy Monitor Sketch for Arduino/Bluno
  Developed by Stanley Huang for Open IoT Project
  Distributed under GPL V3
*/

#include <Arduino.h>
#include <SPI.h>
#include <MultiLCD.h>
#include <RF24.h>

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(9,10);
byte addresses[][6] = {"SNDER","RCVER"};

LCD_ILI9341 lcd;

#define PIN_CURRENT_SENSOR A0
#define PIN_AMBIENT_SENSOR A4
#define PIN_RELAY A2
#define CURRENT_SENSOR_RATIO 0.33
#define AMBIENT_SWITCH_OFF 300
#define AMBIENT_SWITCH_ON 600

#define CHANNELS 6
#define LOCAL_CHANNEL 0

typedef struct {
  byte id;
  unsigned long time;
  int temperature; /* 0.1C */
  unsigned int humidity; /* 0.1% */
  unsigned int voltage; /* 1/100V */
  int current; /* 1/100A */
  unsigned int watt; /* W */
} DATA_BLOCK;

typedef struct {
  const char* caption;
  DATA_BLOCK data;
  float wh; /* calculated watt hour */
  uint32_t lastTime;
} METER_INFO;

METER_INFO meters[CHANNELS] = {
  {"Solar Inverter"},
  {"Home Main AC"},
  {"Solar Panel 1"},
  {"Solar Panel 2"},
  {"Solar Panel 3"},
  {"Solar Panel 4"},
};

byte channel = 0;
bool reinit = false;
int ambientLight = 0;
bool relayState = true;

typedef struct {
  uint16_t left;
  uint16_t right;
  uint16_t bottom;
  uint16_t height;
  uint16_t pos;
} CHART_DATA;

void chartUpdate(CHART_DATA* chart, unsigned int value);

CHART_DATA chartPower = {24, 319, 239, 100};

void chartUpdate(CHART_DATA* chart, unsigned int value)
{
  if (value > chart->height) value = chart->height;
  for (unsigned int n = 0; n < value; n++) {
    uint8_t b = n * 255 / chart->height;
    lcd.setPixel(chart->pos, chart->bottom - n, RGB16(0, 0, b));
  }
  if (chart->pos++ == chart->right) {
    chart->pos = chart->left;
  }
  lcd.fill(chart->pos, chart->pos, 239 - chart->height, chart->bottom);
}

void initScreen()
{
    lcd.clear();
    lcd.setBackLight(255);

    lcd.setFontSize(FONT_SIZE_SMALL);
    lcd.setColor(RGB16_YELLOW);
    lcd.print(channel);
    lcd.print("# ");
    lcd.print(meters[channel].caption);

    lcd.setFontSize(FONT_SIZE_MEDIUM);
    lcd.setColor(RGB16_CYAN);
    lcd.setCursor(20, 2);
    lcd.print("POWER");
    lcd.setCursor(120, 2);
    lcd.print("ENERGY");
    lcd.setCursor(20, 10);
    lcd.print("VOLTAGE");
    lcd.setCursor(120, 10);
    lcd.print("CURRENT");

    lcd.setColor(RGB16_YELLOW);
    lcd.setCursor(70, 6);
    lcd.print("W");
    lcd.setCursor(180, 6);
    lcd.print("Wh");
    lcd.setCursor(70, 14);
    lcd.print("V");
    lcd.setCursor(164, 14);
    lcd.print("A");

    lcd.setFontSize(FONT_SIZE_SMALL);    
    lcd.setColor(RGB16_CYAN);
    lcd.setXY(0, 140);
    lcd.print("2kW");
    lcd.setXY(0, 186);
    lcd.print("1kW");
    lcd.setXY(0, 232);
    lcd.print("0kW");

    lcd.setColor(RGB16_WHITE);
    
    chartPower.pos = chartPower.left;
}

void updateMeterDisplay()
{
  DATA_BLOCK* data = &meters[channel].data;
  lcd.setFontSize(FONT_SIZE_SMALL);
  lcd.setColor(RGB16_WHITE);
  lcd.setCursor(112, 0);
  
  uint32_t s = data->time / 1000;
  if (s >= 3600) {
    lcd.print(s / 3600);
    lcd.print(':');
    s %= 3600;
  }
  byte m = s / 100;
  s %= 60;
  lcd.setFlags(FLAG_PAD_ZERO);
  lcd.printInt(m, 2);
  lcd.print(':');
  lcd.printInt(s, 2);
  lcd.print(' ');
        
  /*
  lcd.print(data->temperature / 10);
  lcd.print("C ");
  lcd.print(data->humidity / 10);
  lcd.print("% ");
  */

  lcd.setFontSize(FONT_SIZE_XLARGE);
  lcd.setFlags(0);
  lcd.setCursor(0, 5);
  lcd.printInt(data->watt, 4);
  lcd.setCursor(110, 5);
  if (meters[channel].wh >= 10000) {
    lcd.printInt(meters[channel].wh / 1000, 4);
    lcd.setCursor(180, 6);
    lcd.print("kWh");
  } else {
    lcd.printInt(meters[channel].wh, 4);
  }
  lcd.setCursor(0, 13);
  lcd.printInt(data->voltage / 100, 4);
  lcd.setCursor(110, 13);
  unsigned int a = data->current / 10;
  if (a >= 1000) {
    if (a >= 10000) a %= 10000;
    lcd.printInt(a / 10, 3);
  } else {
    lcd.printInt(a / 10, 2);
    lcd.setFontSize(FONT_SIZE_SMALL);
    lcd.write('\n');
    lcd.setFontSize(FONT_SIZE_MEDIUM);
    lcd.write('.');
    lcd.printInt(a % 10);
  }
   
  lcd.setFlags(FLAG_PAD_ZERO);
  for (byte n = 0, ln = 0; n < CHANNELS; n++) {
    if (channel == n) continue;
    data = &meters[n].data;
    lcd.setColor(RGB16_YELLOW);
    lcd.setFontSize(FONT_SIZE_SMALL);
    lcd.setCursor(220, ln);
    lcd.print((int)n);
    lcd.print("# ");
    lcd.setColor(RGB16_WHITE);
    lcd.print((float)data->voltage / 100, 1);
    lcd.print("V ");
    lcd.print((float)data->current / 100, 1);
    lcd.print("A ");
    lcd.setColor(RGB16(160, 160, 160));
    lcd.setCursor(238, ln + 1);
    lcd.setFontSize(FONT_SIZE_MEDIUM);
    lcd.print(data->watt);
    lcd.print("W ");
    /*
    lcd.print(data->temperature / 10);
    lcd.print("C ");
    lcd.print(data->humidity / 10);
    lcd.print("% ");
    */
    ln += 3;
  }
  lcd.setFlags(0);
  chartUpdate(&chartPower, meters[channel].data.watt / 20);
}

void setup() {
  // set analog reference voltage to internal 1.1V
  // as we are going to measure very small voltage
  //analogReference(INTERNAL);

  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
 // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_HIGH);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  
  // Start the radio listening for data
  radio.startListening();

  // initialize LCD module
  lcd.begin();
  
  pinMode(8, INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, HIGH);

  meters[0].data.voltage = 24000;
  
  // illustrate UI
  initScreen();

  Serial.begin(115200);
  
  delay(500);
}

void serialOutput(byte channel)
{
      DATA_BLOCK* data = &meters[channel].data;
      Serial.print('[');
      Serial.print(data->id);
      Serial.print(']');
      uint32_t s = data->time / 1000;
      Serial.print(s);
      if (data->voltage || data->current) {
        Serial.print(' ');
        if (data->watt) {
          Serial.print(data->watt);
          Serial.print("W ");
        }
        Serial.print((float)data->current / 100, 2);
        Serial.print("A ");
        /*
        Serial.print((float)data->voltage / 100, 1);
        Serial.print("V ");
        */
      }
      Serial.println();
}

void receiveRemoteSensors()
{
  if( radio.available()){
    while (radio.available()) {                                   // While there is data ready
      DATA_BLOCK data = {0};
      radio.read( &data, sizeof(data));
      if (data.id > 0 && data.id < CHANNELS) {
        memcpy(&meters[data.id].data, &data, sizeof(DATA_BLOCK));
        if (meters[data.id].lastTime) {
          meters[data.id].wh  += (float)data.watt * (data.time - meters[data.id].lastTime) / 3600000;
        }
        meters[data.id].lastTime = data.time;
        serialOutput(data.id);
      }
    }
  }
}

bool checkButton()
{
  if (digitalRead(8) == LOW) {  
    channel = (channel + 1) % CHANNELS;
    reinit = true;
    return true;
  } else {
    return false;
  }
}

#define ACS712_SENSITIVITY 0.185 //0.185mV is typical value
#define ADC_RESOLUTION  (float)5/1024 // 5/1024 is eaque 0.0049V per unit

int acsZeroRef = 512;

float getCurrent(byte pin)
{
    return (float)(analogRead(pin) - acsZeroRef) * ADC_RESOLUTION /ACS712_SENSITIVITY * 2;
}

void readLocalSensor(byte mychannel, unsigned int interval)
{
  DATA_BLOCK* data = &meters[mychannel].data;
  float an = 0;
  float vn = 0;
  float pn = 0;
  uint32_t n;
  uint32_t t = millis();
  for (n = 0; millis() - t < interval && !checkButton(); n++) {
    float a = getCurrent(PIN_CURRENT_SENSOR);
    //int v = analogRead(PIN_VOLTAGE_SENSOR);
    an += a * a;
    //vn += (uint32_t)v * v;
    pn += (a >= 0 ? a : -a) * data->voltage / 100;
    receiveRemoteSensors();
  }
  data->current = sqrt((float)an / n) * 100;
  //meters[0].voltage = sqrt((float)vn / 1000) * CURRENT_SENSOR_RATIO * 100;
  data->watt = pn / n;
  // calculations
  t = millis();
  meters[mychannel].data.time = t;
  meters[mychannel].wh += pn / n * (t - meters[mychannel].lastTime) / 3600000;
  meters[mychannel].lastTime = t;
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t lastTime = 0;

  readLocalSensor(LOCAL_CHANNEL, 500);
  receiveRemoteSensors();

  if (millis() - lastTime > 3000) {
    updateMeterDisplay();
    serialOutput(LOCAL_CHANNEL);
    lastTime = millis();
    
    // controls
    ambientLight = analogRead(PIN_AMBIENT_SENSOR);
    if (relayState && ambientLight < AMBIENT_SWITCH_OFF) {
      digitalWrite(PIN_RELAY, LOW);
      lcd.setBackLight(8);
      relayState = false; 
    } else if (!relayState && ambientLight > AMBIENT_SWITCH_ON) {
      digitalWrite(PIN_RELAY, HIGH);
      lcd.setBackLight(255);
      relayState = true; 
    }
    Serial.print("L:");
    Serial.println(ambientLight);
  }
  
  if (reinit) {
    initScreen();
    reinit = false;   
  }
  
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
    case '-':
      digitalWrite(PIN_RELAY, LOW);
      relayState = false;
      break;
    case '=':
    case '+':
      digitalWrite(PIN_RELAY, HIGH);
      relayState = true;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
      channel = c - '0';
      reinit = true;
      break;
    case '/':
      Serial.print("ACS REF:");
      acsZeroRef = 0;
      for (byte n = 0; n < 16; n++) {
        acsZeroRef += analogRead(PIN_CURRENT_SENSOR);
        delay(60);
      }
      acsZeroRef /= 16;
      Serial.println(acsZeroRef);
      break;
    }
  }
}
