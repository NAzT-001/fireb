#define ILI9341_BLACK 0x0000       ///<   0,   0,   0
#define ILI9341_NAVY 0x000F        ///<   0,   0, 123
#define ILI9341_DARKGREEN 0x03E0   ///<   0, 125,   0
#define ILI9341_DARKCYAN 0x03EF    ///<   0, 125, 123
#define ILI9341_MAROON 0x7800      ///< 123,   0,   0
#define ILI9341_PURPLE 0x780F      ///< 123,   0, 123
#define ILI9341_OLIVE 0x7BE0       ///< 123, 125,   0
#define ILI9341_LIGHTGREY 0xC618   ///< 198, 195, 198
#define ILI9341_DARKGREY 0x7BEF    ///< 123, 125, 123
#define ILI9341_BLUE 0x001F        ///<   0,   0, 255
#define ILI9341_GREEN 0x07E0       ///<   0, 255,   0
#define ILI9341_CYAN 0x07FF        ///<   0, 255, 255
#define ILI9341_RED 0xF800         ///< 255,   0,   0
#define ILI9341_MAGENTA 0xF81F     ///< 255,   0, 255
#define ILI9341_YELLOW 0xFFE0      ///< 255, 255,   0
#define ILI9341_WHITE 0xFFFF       ///< 255, 255, 255
#define ILI9341_ORANGE 0xFD20      ///< 255, 165,   0
#define ILI9341_GREENYELLOW 0xAFE5 ///< 173, 255,  41
#define ILI9341_PINK 0xFC18        ///< 255, 130, 198

extern SharedPoolModule pool;

// uint32_t pevPM25, pevTemp, pevHumid;
// extern int PM1, PM25, PM10;

const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

void tft_init()
{
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(TFT_BL, ledChannel);
  Serial.println("tft_init.");
  ledcWrite(ledChannel, 255*0.8);
  // pinMode(TFT_BL, OUTPUT);
  // digitalWrite(TFT_BL, HIGH);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(0);
}

#define DELAY_MS (2000)
// main display
void tft_display_logo()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(70, 100);
  tft.setTextSize(6);
  tft.print(String(DB_MINI_APP_VERSION));
  delay(1000);
  tft.fillScreen(ILI9341_BLACK);
}

void drawHeader() {
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(6, 20);
  tft.setTextSize(2);
  tft.print(pool.__device_name);
}


void drawSectionLine() {
  tft.drawLine(0, 0, 320 - 1, 0, ILI9341_WHITE);
  tft.drawLine(0, 0, 0, 240 - 1, ILI9341_WHITE);
  tft.drawLine(320 - 1, 0, 320 - 1, 240 - 1, ILI9341_WHITE);
  tft.drawLine(0, 240 - 1, 320 - 1, 240 - 1, ILI9341_WHITE);

  // line under title
  tft.drawLine(0, 50, 320 - 1, 50, ILI9341_WHITE);
  // multi display
  tft.drawLine(200, 50, 200, 200, ILI9341_WHITE);
  tft.drawLine(200, 120, 320 - 1, 120, ILI9341_WHITE);
  // buttom display
  tft.drawLine(0, 200, 320 - 1, 200, ILI9341_WHITE);
}

// extern int mode;
void drawSectionText() {

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 60);
  tft.print("PM 2.5");
  tft.setCursor(130, 60);
  tft.print("ug/m");
  tft.setTextSize(1);
  tft.setCursor(177, 60);
  tft.print(3);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(210, 130);
  tft.setTextSize(2);
  tft.print("RH");
  tft.setCursor(290, 130);
  tft.print("%");

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(210, 60);
  tft.setTextSize(2);
  tft.print("Temp");
  tft.setCursor(290, 60);
  tft.print("*C");

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 213);
  tft.print("PM 10");
  tft.setCursor(150, 213);
  tft.print("ug/m");

  tft.setTextSize(1);
  tft.setCursor(150+47, 212);
  tft.print(3);


  int x = 5;
  uint32_t s = millis()/1000;
  uint8_t len = String(s).length();
  if (len<=2) {
    // if len == 1 then *4
    x += (3-len)*4;
  }
  tft.fillRect(268, 213, 50, 26, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setCursor(290+x-(len*3.5), 220);
  tft.print(" ");
  tft.print(s);
  tft.print("s");

  tft.setCursor(30+250, 60+170);
  tft.setTextSize(1);
  tft.print(DB_MINI_APP_VERSION);
}

// #define DEV_MODE 1

void drawTemperature(float temp) {
  static float pevTemp = -1;
  if (temp != pevTemp) {
    pevTemp = temp;
    tft.fillRect(210, 80, 100, 35, ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(20, 20);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(215, 87);
    tft.setTextSize(3);
    tft.println(String(temp)); // temp data
  }
}

void drawHumidity(float humid) {
  static float  pevHumid = -1;
    #ifndef DEV_MODE
    #else
        tft.fillRect(210, 150, 100, 40, ILI9341_BLACK);
    #endif
  tft.fillRect(210, 150, 100, 40, ILI9341_CYAN);
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(215, 160);
  tft.setTextSize(3);
  tft.print(String(humid)); // humid data
}

void drawPm2_5(int pm25) {
  static int pevPM25 = -1;
  static uint32_t t = millis();
  // tft.fillRect(10, 80, 180, 110, ILI9341_RED);
  if ( (millis() - t) > (1L*1000)) {
      if (pevPM25 == pm25) {
        return;
      }
      Serial.printf("PAINTING PM2.5 from %d to %d\r\n", pevPM25, pm25);
      tft.fillRect(1, 80, 197, 120, ILI9341_BLACK);
      tft.setTextColor(ILI9341_GREEN);
      if (pm25 < 10) {
        tft.setCursor(60, 115);
      }
      else if (pm25 >= 10) {
        tft.setCursor(55, 115);
      }

      tft.setTextSize(2);
      for (size_t i = 0; i < 3-String(pm25).length(); i++) {
        tft.print(" ");
      }

      tft.setTextSize(6);
      tft.print(String(pm25));
      t = millis();
      pevPM25 = pm25;
    }
}

void drawPm10(int pm10) {
  static int pevPm10 = -1;
  // tft.fillRect(80, 210, 80, 22, ILI9341_RED);
  #ifndef DEV_MODE
  tft.fillRect(80, 210, 60, 22, ILI9341_RED);
  #else
  tft.fillRect(80, 210, 60, 22, ILI9341_RED);
  #endif
  tft.setTextColor(ILI9341_GREEN);

  tft.setTextSize(2);
  tft.setCursor(95, 213);

  tft.print(String(pm10));

}

// value display
void screen2(int pm25, int pm10, float temp, float humid, bool draw_header)
{
  // tft.fillScreen(ILI9341_BLACK);
  if (draw_header == true) {
    // Serial.printf("drawHeader called. flag=%d\r\n", draw_header);
    drawHeader();
  }

  // drawSectionLine();
  // drawSectionText();
  // drawTemperature(temp);
  // drawHumidity(humid);
  // drawPm2_5(pm25);
  // drawPm10(pm10);
  // aqi box quility
}
