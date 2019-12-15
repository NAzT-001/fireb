
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <CMMC_Legend.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

bool doing_ota = false;;
bool doing_scanwifi = false;
float calibrate_temp = 0.0;
uint32_t seq = 0;

extern String DB_MINI_APP_VERSION = "v1.0.0";


#include <Preferences.h>
Preferences preferences;

#include <rom/rtc.h>

#define TFT_CS 27
#define TFT_DC 32
#define TFT_BL 4

/*** initial button ***/
#define BT1 38
#define BT2 37
#define BT3 39

bool sponsoring = true;
char ap_name[40];
char full_mac[40];
uint8_t SCREEN = 0;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
/*** Global Variable ***/
uint32_t pevTime = 0;
const long interval = 1 * 1000;

#define RUN 1
#define CONFIG 2

int mode = CONFIG;

/*** Serial1 for dust sensor ***/
#define RXD2 4
#define TXD2 5


#include "modules/WiFiModule.h"
#include "modules/SharedPoolModule.h"

#define SCREEN_MAIN_PAGE (1)
#define SCREEN_CONNECTING_WIFI (0)
#define SCREEN_PRESSED (2)
#define SCREEN_LONG_PRESSED (4)
#define SCREEN_CONFIG (3)

int first_published = true;


WiFiModule *wifiModule;
SharedPoolModule pool;

CMMC_Legend *os;

#include "bme280.h"
#include "display.h"

bool no_internet_link = 0;
String pressed_text = "";
unsigned int boot_count = 0;

void lcd_task(void *parameter);
void sensorTask(void *parameter);

int LOCKING = false;

void hook_init_ap(char *name, char* fullmac, IPAddress ip)
{
  strcpy(ap_name, name);
  strcpy(full_mac, fullmac);
  Serial.println("----------- hook_init_ap -----------");
  Serial.println(name);
  Serial.println(ip);
  Serial.println("/----------- hook_init_ap -----------");
  SCREEN = 0;
  mode = CONFIG;
}

void hook_button_pressing()
{
      tft.setTextSize(2);
      tft.fillRect(0, 0, 480, 320, ILI9341_BLACK);
      tft.setCursor(0, 0);
      tft.print(pressed_text);
      Serial.println("pressing..");
      delay(50);

}
void hook_button_pressed()
{
  tft.fillScreen(ILI9341_BLACK);
  Serial.println("[user] hook_button_pressed");
  pressed_text = "Waiting...";
  LOCKING = true;
  SCREEN = SCREEN_PRESSED;
  Serial.println("[user] set SCREEN = SCREEN_PRESSED.");
}

void hook_button_released()
{
  // pressed_text = "xxxxxxxx";
  Serial.println("[user] hook_button_released");
  tft.fillScreen(ILI9341_BLACK);
  SCREEN = SCREEN_MAIN_PAGE;
  LOCKING = false;
}

void hook_button_long_pressed()
{
  Serial.println("[user] hook_button_long_pressed");
  pressed_text = "Release the button...";
  tft.setTextSize(2);
  tft.fillRect(0, 0, 480, 320, ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.print(pressed_text);
  SCREEN = SCREEN_LONG_PRESSED;
}

void hook_config_loaded() {}
void hook_ready() {}


RESET_REASON cpu0_reset_reason;
RESET_REASON cpu1_reset_reason;

void print_reset_reason(RESET_REASON reason) {
  switch ( reason)
  {
    case 1:
      Serial.println ("POWERON_RESET");break;          /**<1,  Vbat power on reset*/
    case 3:
      Serial.println ("SW_RESET");break;               /**<3,  Software reset digital core*/
    case 4:
      Serial.println ("OWDT_RESET");break;             /**<4,  Legacy watch dog reset digital core*/
    case 5:
      Serial.println ("DEEPSLEEP_RESET");break;        /**<5,  Deep Sleep reset digital core*/
    case 6:
      Serial.println ("SDIO_RESET");break;             /**<6,  Reset by SLC module, reset digital core*/
    case 7:
      Serial.println ("TG0WDT_SYS_RESET");break;       /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8:
      Serial.println ("TG1WDT_SYS_RESET");break;       /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9:
      Serial.println ("RTCWDT_SYS_RESET");break;       /**<9,  RTC Watch dog Reset digital core*/
    case 10:
      Serial.println ("INTRUSION_RESET");break;       /**<10, Instrusion tested to reset CPU*/
    case 11:
      Serial.println ("TGWDT_CPU_RESET");break;       /**<11, Time Group reset CPU*/
    case 12:
      Serial.println ("SW_CPU_RESET");break;          /**<12, Software reset CPU*/
    case 13:
      Serial.println ("RTCWDT_CPU_RESET");break;      /**<13, RTC Watch dog Reset CPU*/
    case 14:
      Serial.println ("EXT_CPU_RESET");break;         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15:
      Serial.println ("RTCWDT_BROWN_OUT_RESET");break;/**<15, Reset when the vdd voltage is not stable*/
    case 16:
      Serial.println ("RTCWDT_RTC_RESET");break;      /**<16, RTC Watch dog reset digital core and rtc module*/
    default:
      Serial.println ("NO_MEAN");
  }
}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println(">>>>>>>>>>>>>>>>>>");
  Serial.printf("[xWiFi-event] event: %d\n", event);
  Serial.println(">>>>>>>>>>>>>>>>>>");

  switch (event) {
    case SYSTEM_EVENT_WIFI_READY:
      Serial.println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      Serial.println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("WiFi client started");
      wifiModule->__wifi_connected = false;
      if (!sponsoring) {
        tft.fillRect(3, 4, 310, 40, ILI9341_BLUE);
        if (mode==RUN) {
          SCREEN = SCREEN_CONNECTING_WIFI;
        }
      }
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("Connected to access point");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi access point");
      // WiFi.reconnect();
      Serial.print("WiFi lost connection. Reason: ");
      Serial.println(info.disconnected.reason);
      wifiModule->__wifi_connected = false;
      tft.fillRect(3, 4, 310, 40, ILI9341_RED);
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      Serial.println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      if (!sponsoring) {
        no_internet_link = false;
        tft.fillRect(3, 4, 310, 40, ILI9341_DARKGREEN);
      }
      wifiModule->__wifi_connected = true;
      SCREEN = SCREEN_MAIN_PAGE;
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println("Lost IP address and IP address is reset to 0");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case SYSTEM_EVENT_AP_START:
      Serial.println("WiFi access point started");
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("WiFi access point  stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("Client connected");
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("Client disconnected");
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      Serial.println("Assigned IP address to client");
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      Serial.println("Received probe request");
      break;
    case SYSTEM_EVENT_GOT_IP6:
      Serial.println("IPv6 is preferred");
      break;
    case SYSTEM_EVENT_ETH_START:
      Serial.println("Ethernet started");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet connected");
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.println("Obtained IP address");
      break;
  }
}
void setup()
{
  esp_log_level_set("*", ESP_LOG_VERBOSE);        // set all components to ERROR level
  cpu0_reset_reason = rtc_get_reset_reason(0);
  cpu1_reset_reason = rtc_get_reset_reason(1);
  Serial.begin(115200);
  preferences.begin("x-mini", false);
  boot_count = preferences.getUInt("boot_count", 0);
  boot_count++;
  preferences.putUInt("boot_count", boot_count);
  preferences.end();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  Wire.begin(19, 26);
  WiFi.onEvent(WiFiEvent);

  preferences.begin("db-conf-mqtt", false);
  String USER = preferences.getString("MQTT_USERNAME");
  String MQTT_HOST = preferences.getString("MQTT_HOST");
  String MQTT_USERNAME = preferences.getString("MQTT_USERNAME");
  String MQTT_PASSWORD = preferences.getString("MQTT_PASSWORD");
  String MQTT_CLIENT_ID = preferences.getString("MQTT_CLIENT_ID");
  String MQTT_PREFIX = preferences.getString("MQTT_PREFIX");
  int MQTT_PORT  = preferences.getUInt("MQTT_PORT");
  String DEVICE_NAME = preferences.getString("DEVICE_NAME");
  int PUBLISH_EVERY_S = preferences.getUInt("PUBLISH_EVERY_S");
    // int MQTT_CONNECT_TIMEOUT = 10 =
    // bool MQTT_LWT = false =

  preferences.end();
  static os_config_t config = {
      .BLINKER_PIN = 21,
      .BUTTON_MODE_PIN = 37,
      .SWITCH_PIN_MODE = INPUT_PULLUP,
      .SWITCH_PRESSED_LOGIC = LOW,
      .delay_after_init_ms = 200,
      .hook_init_ap = hook_init_ap,
      .hook_button_pressed = hook_button_pressed,
      .hook_button_pressing = hook_button_pressing,
      .hook_button_long_pressed = hook_button_long_pressed,
      .hook_button_released = hook_button_released,
      .hook_ready = hook_ready,
      .hook_config_loaded = hook_config_loaded,
  };
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial1.println("HELLO");
  // dustModule = new DustModule(&Serial1);

  wifiModule = new WiFiModule();

  xTaskCreate(lcd_task, "lcd_task", 4096, NULL, 2, NULL);
  // xTaskCreate(sensorTask, "sensorTask", 4096, NULL, 1, NULL);

  Serial.println("Starting Legend.");
    // String MQTT_HOST = preferences.getString("MQTT_HOST");
    // String MQTT_CLIENT_ID = preferences.getString("MQTT_CLIENT_ID");
    // String MQTT_PREFIX = preferences.getString("MQTT_PREFIX");
    // int MQTT_PORT  = preferences.getUInt("MQTT_PORT");
    // String DEVICE_NAME = preferences.getString("DEVICE_NAME");
    // int PUBLISH_EVERY_S = preferences.getUInt("PUBLISH_EVERY_S");

  os = new CMMC_Legend(&Serial);

  // os->addModule(dustModule);
  os->addModule(wifiModule);
  // os->addModule(ntpModule);
  // os->addModule(mqttModule);
  // SPIFFS.remove("/enabled");

  Serial.println("add wifi");
  os->setup(&config);
  // os->setE
  print_reset_reason(cpu0_reset_reason);
  print_reset_reason(cpu1_reset_reason);

}

#include <FirebaseESP32.h>
FirebaseData firebaseData;
FirebaseJson json;
void printResult(FirebaseData &data);

#define FIREBASE_HOST "fxxxx-b2530.firebaseio.com" //Do not include https:// in FIREBASE_HOST
#define FIREBASE_AUTH "xftv9Dkn7BA1Ehg6oR2yHCEDBUgl6bdMMJmdTJDz"
int _setup = true;
void loop()
{
  yield();
  if (!doing_ota) {
    os->run();
  }
  if (wifiModule->__wifi_connected) {
    // Serial.println("WiFi Connected!.");
    if (_setup) {
        Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
        Firebase.reconnectWiFi(true);

        //Set database read timeout to 1 minute (max 15 minutes)
        Firebase.setReadTimeout(firebaseData, 1000 * 60);
        //tiny, small, medium, large and unlimited.
        //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
        Firebase.setwriteSizeLimit(firebaseData, "tiny");
        _setup = false;
    }
    else {
        String path = "/Test";

        Serial.println("------------------------------------");
        Serial.println("Set double test...");

        for (uint8_t i = 0; i < 10; i++)
        {
          //Also can use Firebase.set instead of Firebase.setDouble
          if (Firebase.setDouble(firebaseData, path + "/Double/Data" + (i + 1), ((i + 1) * 10) + 0.123456789))
          {
            Serial.println("PASSED");
            Serial.println("PATH: " + firebaseData.dataPath());
            Serial.println("TYPE: " + firebaseData.dataType());
            Serial.println("ETag: " + firebaseData.ETag());
            Serial.print("VALUE: ");
            printResult(firebaseData);
            Serial.println("------------------------------------");
            Serial.println();
          }
          else
          {
            Serial.println("FAILED");
            Serial.println("REASON: " + firebaseData.errorReason());
            Serial.println("------------------------------------");
            Serial.println();
          }
        }

        Serial.println("------------------------------------");
        Serial.println("Get double test...");

        for (uint8_t i = 0; i < 10; i++)
        {
          //Also can use Firebase.get instead of Firebase.setInt
          if (Firebase.getInt(firebaseData, path + "/Double/Data" + (i + 1)))
          {
            Serial.println("PASSED");
            Serial.println("PATH: " + firebaseData.dataPath());
            Serial.println("TYPE: " + firebaseData.dataType());
            Serial.println("ETag: " + firebaseData.ETag());
            Serial.print("VALUE: ");
            printResult(firebaseData);
            Serial.println("------------------------------------");
            Serial.println();
          }
          else
          {
            Serial.println("FAILED");
            Serial.println("REASON: " + firebaseData.errorReason());
            Serial.println("------------------------------------");
            Serial.println();
          }
        }

        Serial.println("------------------------------------");
        Serial.println("Push integer test...");

        for (uint8_t i = 0; i < 5; i++)
        {
          //Also can use Firebase.push instead of Firebase.pushInt
          if (Firebase.pushInt(firebaseData, path + "/Push/Int", (i + 1)))
          {
            Serial.println("PASSED");
            Serial.println("PATH: " + firebaseData.dataPath());
            Serial.print("PUSH NAME: ");
            Serial.println(firebaseData.pushName());
            Serial.println("ETag: " + firebaseData.ETag());
            Serial.println("------------------------------------");
            Serial.println();
          }
          else
          {
            Serial.println("FAILED");
            Serial.println("REASON: " + firebaseData.errorReason());
            Serial.println("------------------------------------");
            Serial.println();
          }
        }

    }
  }
}

#include <CMMC_Interval.h>
int dirty = 0;

void lcd_task(void *parameter)
{
  tft_init();
  if (SCREEN != SCREEN_CONFIG && cpu0_reset_reason == 1) { // case 1: power on;
    sponsoring = true;
    tft_display_logo();
  }

  sponsoring = false;
  bool first = true;
  bool first_no_internet_connection = true;
  CMMC_Interval ti;
  bool first_connected = true;

  pool.setup();
  while (1)
  {
    if (doing_ota) {
      break;
    }
    pool.loop();
    ti.every_ms(5000, [&]() {
      Serial.println("==========================");
      Serial.printf("LCD TASK with os diff = %lu\r\n", millis() - os->_loop_ms);
      Serial.printf("first_no_internet_connection = %d\r\n", first_no_internet_connection);
      Serial.printf("no_internet_link = %d\r\n", no_internet_link);
      Serial.println("==========================");
      if (doing_scanwifi) {
        return;
      }
      if (SCREEN != SCREEN_CONFIG) {
        if (doing_ota) {
          return;
        }
        if (!doing_ota) {
          if (millis() - os->_loop_ms > 240L*1000) {
            ESP.deepSleep(10e5);
          }
        }
        else {
          if (millis() - os->_loop_ms > 600L*1000) {
            ESP.deepSleep(10e5);
          }
        }
      }
      else {
        // if (millis() - os->_loop_ms > 15*60L*1000) {
        //   ESP.deepSleep(10e5);
        // }
      }
    });

    Serial.print("SCREEN=");
    Serial.println(SCREEN);
    if (SCREEN == SCREEN_CONNECTING_WIFI) // Connecting WiFi
    {
      if (!LOCKING) {
        screen2(pool.__pm2_5, pool.__pm10,  pool.__temperature, pool.__humidity, pool.__wifi_connected);
        mode = RUN;
        tft.setTextSize(2);
        if (first_no_internet_connection) {
          tft.fillRect(3, 4, 310, 40, ILI9341_BLUE);
          first_no_internet_connection = false;
        }
        tft.setCursor(5, 5);
        tft.setTextColor(ILI9341_WHITE);
        tft.print("Connecting to (WiFi)");
        tft.print(" ");
        // tft.fillRect(250, 4, 60, 26, ILI9341_RED); // for time
        // tft.print(millis()/1000);
        // tft.print("s");
        tft.setCursor(5, 25);
        // tft.setTextColor(ILI9341_RED);
        tft.print(" > ");
        tft.print(wifiModule->sta_ssid);
      }
    }
    else if (SCREEN == SCREEN_MAIN_PAGE) // MAIN PAGE
    {
      if (!LOCKING) {
        // Serial.println("MAIN PAGE!");
        // screen2(pool.__pm2_5, pool.__pm10, pool.__temperature, pool.__humidity, pool.__wifi_connected);
        // screen2(pool.__pm2_5, pool.__pm10, pool.__temperature, pool.__humidity, pool.__wifi_connected);
        tft.setTextColor(ILI9341_WHITE);
        tft.setCursor(6, 20);
        tft.setTextSize(2);
        tft.print(" WiFi Connected. ");
        // tft.print(pool.__device_name);
        if (first_connected) {
          // tft.fillScreen(0x0000);
          tft.fillRect(3, 4, 310, 40, ILI9341_DARKGREEN);
          first_connected = false;
        }
        if (no_internet_link) {
          tft.fillRect(3, 4, 310, 40, ILI9341_RED);
        }
      }
    }
    else if (SCREEN == SCREEN_PRESSED || SCREEN == SCREEN_LONG_PRESSED) // LONG PRESSED PAGE
    {
      Serial.println("set screen to pressed..");
      tft.setTextSize(2);
      tft.fillRect(0, 0, 480, 320, ILI9341_BLACK);
      tft.setCursor(0, 0);
      tft.print(pressed_text);
    }
    else if (SCREEN == SCREEN_CONFIG) // CONFIG PAGE
    {
      if (first) {
        tft.fillRect(0, 0, 480, 320, ILI9341_BLACK);
        // tft.fillRect(12, 70, 170, 200, ILI9341_WHITE);
        // tft.setCursor(50, 50);
        tft.setTextSize(2);
        tft.setCursor(10, 50);
        tft.println("192.168.4.1");
        // tft.drawRGBBitmap(20, 80, qr, 150, 150);
      }
      tft.setTextSize(2);
      tft.setCursor(10, 20);
      tft.print("Name:");
      tft.println(String(ap_name));
      // tft.setCursor(10, 25);
      // tft.print("LINE2");
      tft.setCursor(200, 60+172);
      tft.setTextSize(1);
      tft.print(full_mac);
      tft.setCursor(30+250, 60+172);
      // tft.print(millis() / 1000);
      tft.setTextSize(1);
      tft.print(DB_MINI_APP_VERSION);
      // tft.fillScreen(0xFFFF)''
      // tft.fillRect(236, 15, 66, 25, ILI9341_RED);
      tft.setTextSize(2);
      tft.setCursor(240, 20);
      tft.print(pool.__device_name);
      first = false;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  vTaskDelete( NULL );
}


void sensorTask(void *parameter)
{
  while (1)
  {
    if (doing_ota) {
      break;
    }

    // if (doing_scanwifi) {
    //   continue;
    // }
    if (sponsoring) {
      Serial.println("sponsoring..");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      print_reset_reason(cpu0_reset_reason);
      print_reset_reason(cpu1_reset_reason);
    }
    if (mode == CONFIG) {
      vTaskDelay(10*1000/ portTICK_PERIOD_MS);
      // os->scanWiFi();
      Serial.println("*");
      continue;
    }

    // Serial.println("bmeModule->loop()");
    // Serial.println("dustModule->loop()");
    // dustModule->loop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  vTaskDelete( NULL );
}
void printResult(FirebaseData &data)
{

  if (data.dataType() == "int")
    Serial.println(data.intData());
  else if (data.dataType() == "float")
    Serial.println(data.floatData(), 5);
  else if (data.dataType() == "double")
    printf("%.9lf\n", data.doubleData());
  else if (data.dataType() == "boolean")
    Serial.println(data.boolData() == 1 ? "true" : "false");
  else if (data.dataType() == "string")
    Serial.println(data.stringData());
  else if (data.dataType() == "json")
  {
    Serial.println();
    FirebaseJson &json = data.jsonObject();
    //Print all object data
    Serial.println("Pretty printed JSON data:");
    String jsonStr;
    json.toString(jsonStr, true);
    Serial.println(jsonStr);
    Serial.println();
    Serial.println("Iterate JSON data:");
    Serial.println();
    size_t len = json.iteratorBegin();
    String key, value = "";
    int type = 0;
    for (size_t i = 0; i < len; i++)
    {
      json.iteratorGet(i, type, key, value);
      Serial.print(i);
      Serial.print(", ");
      Serial.print("Type: ");
      Serial.print(type == JSON_OBJECT ? "object" : "array");
      if (type == JSON_OBJECT)
      {
        Serial.print(", Key: ");
        Serial.print(key);
      }
      Serial.print(", Value: ");
      Serial.println(value);
    }
    json.iteratorEnd();
  }
  else if (data.dataType() == "array")
  {
    Serial.println();
    //get array data from FirebaseData using FirebaseJsonArray object
    FirebaseJsonArray &arr = data.jsonArray();
    //Print all array values
    Serial.println("Pretty printed Array:");
    String arrStr;
    arr.toString(arrStr, true);
    Serial.println(arrStr);
    Serial.println();
    Serial.println("Iterate array values:");
    Serial.println();
    for (size_t i = 0; i < arr.size(); i++)
    {
      Serial.print(i);
      Serial.print(", Value: ");

      FirebaseJsonData &jsonData = data.jsonData();
      //Get the result data from FirebaseJsonArray object
      arr.get(jsonData, i);
      if (jsonData.typeNum == JSON_BOOL)
        Serial.println(jsonData.boolValue ? "true" : "false");
      else if (jsonData.typeNum == JSON_INT)
        Serial.println(jsonData.intValue);
      else if (jsonData.typeNum == JSON_DOUBLE)
        printf("%.9lf\n", jsonData.doubleValue);
      else if (jsonData.typeNum == JSON_STRING ||
               jsonData.typeNum == JSON_NULL ||
               jsonData.typeNum == JSON_OBJECT ||
               jsonData.typeNum == JSON_ARRAY)
        Serial.println(jsonData.stringValue);
    }
  }
}
