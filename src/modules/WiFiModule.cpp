#include "WiFiModule.h"
#define WIFI_CONFIG_FILE "/wifi.json"

// extern LCDModule* lcdModule;

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

extern Adafruit_ILI9341 tft;
extern int LOCKING;
extern bool sponsoring;
void WiFiModule::isLongPressed() {
  if (digitalRead(37) == LOW) {
    uint32_t pressed_start = millis();
    while(digitalRead(37) == LOW) {
      if (millis() - pressed_start > 2000) {
          LOCKING = true;
          tft.fillScreen(0x0000);
          tft.setTextSize(2);
          tft.fillRect(0, 0, 480, 320, ILI9341_BLACK);
          tft.setCursor(0, 0);
          tft.print("Release the button.");
        while(digitalRead(37) == LOW) {
          delay(10);
        }
        Serial.println("remove /enabled");
        SPIFFS.remove("/enabled");
        // digitalWrite(0, HIGH);
        delay(1000);
        ESP.deepSleep(10e5);
      }
    }
  }
}

void WiFiModule::configLoop() {
  // if (digitalRead(15) == HIGH) {
  //   // lcdModule->displayLogoWaitReboot();
  //   while(digitalRead(15) == HIGH) {
  //     delay(10);
  //   }
  //   File f = SPIFFS.open("/enabled", "a+");
  //   f.close();
  //   digitalWrite(0, HIGH);
  //   delay(100);
  //   ESP.restart();
  // }
}

void WiFiModule::configSetup() {
  // lcdModule->displayConfigWiFi();
}

void WiFiModule::config(CMMC_System *os, AsyncWebServer *server)
{
  strcpy(this->path, "/api/wifi/sta");
  static WiFiModule *that = this;
  this->_serverPtr = server;
  this->_managerPtr = new CMMC_ConfigManager(WIFI_CONFIG_FILE);
  this->_managerPtr->init();
  this->_managerPtr->load_config([](JsonObject *root, const char *content) {
    if (root == NULL)
    {
      Serial.print("wifi.json failed. >");
      Serial.println(content);
      return;
    }
    Serial.println("[user] wifi config json loaded..");
    const char *sta_config[2];
    sta_config[0] = (*root)["sta_ssid"];
    sta_config[1] = (*root)["sta_password"];

    if ((sta_config[0] == NULL) || (sta_config[1] == NULL))
    {
      Serial.println("NO CREDENTIAL!!");
      SPIFFS.remove("/enabled");
      return;
    };
    strcpy(that->sta_ssid, sta_config[0]);
    strcpy(that->sta_pwd, sta_config[1]);
    Serial.println(that->sta_ssid);
    Serial.println(that->sta_pwd);
  });
  this->configWebServer();
}

void WiFiModule::configWebServer()
{
  static WiFiModule *that = this;
  _serverPtr->on(this->path, HTTP_POST, [&](AsyncWebServerRequest *request) {

    String output = that->saveConfig(request, this->_managerPtr);
    AsyncWebParameter* p = request->getParam("sta_ssid", true);
    String ssid = p->value();

    p = request->getParam("sta_password", true);
    String password = p->value();

    WiFi.begin(ssid.c_str(), password.c_str());
  //   WiFi.setAutoConnect(true);
    uint32_t t = millis();
  //
  //
    while(WiFi.status() != WL_CONNECTED) {
      Serial.println("Connecting WiFi..");
      delay(100);
      yield();
      Serial.println(millis() - t);
      if (millis() - t > 4000) {
        break;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
          Serial1.print(ssid);
          Serial1.print(",,,");
          Serial1.println(password);
          request->send(200, "application/json", "{\"status\": \"WiFi Connected.\"}");
          File f = SPIFFS.open("/enabled", "a+");
          f.close();
          ESP.deepSleep(10e5);
    }
    else {
          request->send(200, "application/json", "{\"status\": \"WiFi connect failed.\"}");
    }

  //     tft.fillScreen(ILI9341_BLACK);
  //     tft.setCursor(10, 50);
  //     tft.setTextSize(2);
  //     tft.println(" ");
  //     tft.println("Connecting to WiFi..");
  //     tft.println();
  //     tft.print("SSID: ");
  //     tft.println(ssid);
  //     tft.println();
  //     tft.print("Pass: ");
  //     tft.println(password);
  //     tft.println();
  //
   WiFiEventId_t eventID = WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info){
     tft.setCursor(8, 200);
    tft.println("WiFi Connected.");
    Serial.println("WiFi Connected.");
    // File f = SPIFFS.open("/enabled", "a+");
    // f.close()
    // ESP.deepSleep(10e5);
  }, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
  //
  //  WiFiEventId_t eventID2 = WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info){
  //      Serial.print("[Y]");
  //      Serial.println(info.disconnected.reason);
  //    tft.setCursor(8, 200);
  //    tft.println("Incorrect WiFi password.");
  // }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
  //
  //    WiFiEventId_t eventID3 = WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info){
  //      Serial.print("[X]");
  //      Serial.println(info.disconnected.reason);
  //    });


  });
}

void WiFiModule::setup()
{
  _init_sta();
}

void WiFiModule::loop() {

}

void WiFiModule::_init_sta()
{
  WiFi.persistent(false);
  WiFi.disconnect(true);
  WiFi.softAPdisconnect();
  delay(100);
  WiFi.mode(WIFI_STA);
  int i = 0;
  delay(10);
  // WiFi.onEvent(this->on_event);
  WiFi.enableSTA(false);
  delay(10);
  WiFi.enableSTA(true);
  delay(10);
  WiFi.setAutoReconnect(true);
  delay(10);
  Serial.println(sta_ssid);
  Serial.println(sta_pwd);
  WiFi.begin(sta_ssid, sta_pwd);

  int counter = 0;
  int prev_ms = millis();
    if (!sponsoring) {
      tft.setTextSize(2);
      tft.fillRect(3, 4, 310, 40, ILI9341_BLUE);
      Serial.printf("Connecting to %s:%s, status=%d\r\n", sta_ssid, sta_pwd, WiFi.status());
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
      tft.print(sta_ssid);
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    isLongPressed();
    // delay(200);
    // if (millis() - prev_ms > 240L*1000) {
    //   ESP.deepSleep(10e5);
    // }
  }
  __wifi_connected = true;
  Serial.println("WiFi Connected.");
  __wifi_connected = true;
  tft.fillRect(3, 4, 310, 40, ILI9341_DARKGREEN);
}


void WiFiModule::on_event(system_event_id_t event) {
}
