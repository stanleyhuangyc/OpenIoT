#include <Arduino.h>
#include <SPI.h>
#include <MultiLCD.h>

LCD_ILI9341 lcd;

#define PV_INPUTS 6
#define CURRENT_SENSOR_RATIO 0.067

float amp = 0; /* A */
uint16_t voltage = 240; /* V */
float watt = 0;
float wh = 0; /* watt hour */
float pvv[PV_INPUTS] = {28.9, 27.5, 28.1, 26.9};
float pva[PV_INPUTS] = {1.5, 2.1, 3.1, 2.9};
int pvt[PV_INPUTS] = {35, 34, 36, 35};
int pvh[PV_INPUTS] = {66, 67, 65, 64};
uint16_t pvs[PV_INPUTS] = {0};

typedef struct {
  uint16_t left;
  uint16_t right;
  uint16_t bottom;
  uint16_t height;
  uint16_t pos;
} CHART_DATA;

void chartUpdate(CHART_DATA* chart, unsigned int value);

CHART_DATA chartPower = {24, 319, 239, 100, 24};

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
    lcd.setFontSize(FONT_SIZE_MEDIUM);
    lcd.setColor(RGB16_CYAN);
    lcd.setCursor(20, 0);
    lcd.print("POWER");
    lcd.setCursor(120, 0);
    lcd.print("ENERGY");
    lcd.setCursor(20, 8);
    lcd.print("VOLTAGE");
    lcd.setCursor(120, 8);
    lcd.print("CURRENT");

    lcd.setFontSize(FONT_SIZE_SMALL);
    for (byte n = 0; n < PV_INPUTS; n++) {
      lcd.setCursor(228, n * 3);
      lcd.print("PV");
      lcd.print(n);
      lcd.print(':');
    }

    lcd.setFontSize(FONT_SIZE_MEDIUM);
    lcd.setColor(RGB16_YELLOW);
    lcd.setCursor(70, 4);
    lcd.print("W");
    lcd.setCursor(180, 4);
    lcd.print("Wh");
    lcd.setCursor(70, 12);
    lcd.print("V");
    lcd.setCursor(164, 12);
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
}

void updateMeters()
{
  lcd.setFontSize(FONT_SIZE_XLARGE);
  lcd.setColor(RGB16_WHITE);
  lcd.setCursor(0, 3);
  lcd.printInt(watt, 4);
  lcd.setCursor(110, 3);
  lcd.printInt(wh, 4);
  lcd.setCursor(0, 11);
  lcd.printInt(voltage, 4);
  lcd.setCursor(110, 11);
  int a = (int)(amp * 10);
  lcd.printInt(a / 10, 2);
  lcd.setFontSize(FONT_SIZE_SMALL);
  lcd.write('\n');
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.write('.');
  lcd.printInt(a % 10);
   
  lcd.setFontSize(FONT_SIZE_SMALL);
  lcd.setFlags(FLAG_PAD_ZERO);
  for (byte n = 0; n < PV_INPUTS; n++) {
    lcd.setColor(RGB16_WHITE);
    lcd.setCursor(252, n * 3);
    lcd.print(pvv[n], 1);
    lcd.print("V ");
    lcd.print(pva[n], 1);
    lcd.print("A ");
    lcd.setColor(RGB16(160, 160, 160));
    lcd.setCursor(252, n * 3 + 1);
    lcd.print((int)(pvv[n] * pva[n]));
    lcd.print("W ");
    lcd.print(pvt[n]);
    lcd.print("C ");
    lcd.print(pvh[n]);
    lcd.print("% ");
    lcd.setColor(RGB16(92, 92, 92));
    lcd.setCursor(252, n * 3 + 2);
    lcd.printInt(pvs[n] / 60, 2);
    lcd.print(':');
    lcd.printInt(pvs[n] % 60, 2);
  }
  lcd.setFlags(0);
  chartUpdate(&chartPower, watt / 20);
}

void setup() {
  analogReference(INTERNAL);

  lcd.begin();
  initScreen();
  updateMeters();
  
  Serial.begin(57600);
}

void calculate()
{
  static uint32_t lastTime = 0;
  
  uint32_t t = millis();
  wh += watt * (t - lastTime) / 3600000;
  lastTime = t;
  for (byte n = 0; n < 4; n++) {
    pvs[n] = millis() / 1000;
  }
}

void sensorPower()
{
  uint32_t an = 0;
  float pn = 0;
  for (int m = 0; m < 1000; m++) {
    int a = analogRead(A2);
    an += (uint32_t)a * a;
    pn += (a * CURRENT_SENSOR_RATIO) * voltage;
    delay(1);
  }
  amp = sqrt((float)an / 500) * CURRENT_SENSOR_RATIO;
  watt = amp <= 0.05 ? 0 : pn / 500;
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t lastTime = 0;
  if (millis() - lastTime > 1000) {
    updateMeters();
    lastTime = millis();
  }
  if (Serial.available()) {
     // amp adjuster
     char c = Serial.read();
     if (c == ',') {
       amp -= 0.1;
       watt -= voltage / 10;
     } else if (c == '.') {
       amp += 0.1;
       watt += voltage / 10;
     }
  }
  sensorPower();
  calculate();
}
