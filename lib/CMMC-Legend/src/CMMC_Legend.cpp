#include "CMMC_Legend.h"

extern uint8_t SCREEN;


CMMC_Legend::CMMC_Legend(HardwareSerial *s) {
  this->_serial_legend = s;
  this->_loop_ms = millis();
};

void CMMC_Legend::addModule(CMMC_Module* module) {
  _modules.push_back(module);
  this->_serial_legend->printf("addModule.. size = %d\r\n", _modules.size());
}


void CMMC_Legend::scanWiFi() {
    this->wifi_scanning = true;
    int n = WiFi.scanNetworks();
    this->_num_wifi = n;
    this->_serial_legend->println("scan done");
    // StaticJsonBuffer<400> jsonBuffer;
    // StaticJsonBuffer<400> jsonDBuffer;
    // JsonArray& array1 = jsonBuffer.createArray();
    // int sizeToUse = 0;
    if (n == 0) {
        this->_serial_legend->println("no networks found");
    } else {
        this->_serial_legend->print(n);
        this->_serial_legend->println(" networks found");
    }
    this->wifi_scanning = false;
}
// being called by os
void CMMC_Legend::run() {
  // static CMMC_Legend *that;
  // that = this;
  isLongPressed();
  int size = _modules.size();
  for (int i = 0 ; i < size; i++) {
    _modules[i]->loop();
    this->_loop_ms = millis();
  }
  // if (must_scan) {
  //
  // }
  yield();
  // vTaskDelay(10)
}

bool CMMC_Legend::enable_run_mode(bool status) {
  if (status) {
    File f = SPIFFS.open("/enabled", "a+");
    return f;
  }
  else {
      return SPIFFS.remove("/enabled");
  }
}

void CMMC_Legend::isLongPressed() {
  uint32_t prev = millis();
  if (digitalRead(this->button_gpio) == this->SWITCH_PRESSED_LOGIC) {
    this->_hook_button_pressed();
    while (digitalRead(this->button_gpio) == this->SWITCH_PRESSED_LOGIC) {
      this->_hook_button_pressing();
      blinker->high();
      if ( (millis() - prev) > 3L * 1000L) {
        _serial_legend->println("LONG PRESSED.");
        this->_hook_button_long_pressed();
        blinker->blink(50);
        while (digitalRead(this->button_gpio) == this->SWITCH_PRESSED_LOGIC) {
          delay(10);
        }
        enable_run_mode(false);
        _serial_legend->println("being restarted.");
        delay(1000);
        ESP.restart();
      }
    }
    this->_hook_button_released();
  }
  blinker->low();
  blinker->blink(4000);
}

void CMMC_Legend::setup(os_config_t *config) {
    this->BLINKER_PIN = config->BLINKER_PIN;
    this->button_gpio = config->BUTTON_MODE_PIN;
    this->SWITCH_PRESSED_LOGIC = config->SWITCH_PRESSED_LOGIC;
    this->SWITCH_PIN_MODE = config->SWITCH_PIN_MODE;
    this->_hook_init_ap = config->hook_init_ap;
    this->_hook_button_pressed = config->hook_button_pressed;
    this->_hook_button_released = config->hook_button_released;
    this->_hook_button_pressing = config->hook_button_pressing;
    this->_hook_button_long_pressed = config->hook_button_long_pressed;

    this->_hook_config_loaded= config->hook_config_loaded;
    this->_hook_ready = config->hook_ready;

    init_gpio();

    pinMode(this->button_gpio, this->SWITCH_PIN_MODE);
    blinker = new xCMMC_LED;
    blinker->init();
    blinker->setPin(config->BLINKER_PIN);
    this->_serial_legend->println();

    init_fs();

    init_user_config();

    init_user_sensor();
    init_network();

    _serial_legend->println("--------- setup -----------");
    for (int i = 0 ; i < _modules.size(); i++) {
      _serial_legend->printf("calling %s.setup()\r\n", _modules[i]->name());
      _modules[i]->setup();
      // _modules[i]->loop();
    }

    _serial_legend->println("---------------------------");

    if(this->_hook_ready) {
      this->_hook_ready();
    }

    this->_loop_ms = millis();
}

void CMMC_Legend::init_gpio() {
  _serial_legend->println("OS::Init GPIO..");
  delay(10);
}

void CMMC_Legend::init_fs() {
  _serial_legend->println("OS::Init FS..");
  // _serial_legend->println("starting SPIFFS..");
  SPIFFS.begin();
  // Dir dir = SPIFFS.openDir("/");
  // isLongPressed();
  // this->_serial_legend->println("--------------------------");
  // while (dir.next()) {
  //   File f = dir.openFile("r");
  //   that->_serial_legend->printf("> %s \r\n", dir.fileName().c_str());
  // }
  /*******************************************
     Boot Mode Selection
   *******************************************/
  this->_serial_legend->println("--------------------------");
  if (!SPIFFS.exists("/enabled")) {
    _serial_legend->println("no /enabled file.");
    mode = CONFIG;
  }
  else {
    _serial_legend->println("found /enabled file.");
    mode = RUN;
  }
}

void CMMC_Legend::init_user_sensor() {
  _serial_legend->printf("Initializing Sensor.. MODE=%s\r\n", mode == CONFIG ? "CONFIG" : "RUN");
  if (mode == CONFIG) {
    return;
  }
}

void CMMC_Legend::init_user_config() {
  _serial_legend->println("init_user_config");
}

void CMMC_Legend::init_network() {
  _serial_legend->println("Initializing network.");
    _serial_legend->println("------- config ----------");
  for (int i = 0 ; i < _modules.size(); i++) {
    _serial_legend->printf("calling %s.config()\r\n", _modules[i]->name());
    _modules[i]->config(this, &server);
  }
  if(this->_hook_config_loaded) {
    this->_hook_config_loaded();
  }
  _serial_legend->println("---------------------------");

  // mode = CONFIG;
  _serial_legend->print("Mode=");
  _serial_legend->println(mode);
  if (mode == CONFIG) {
    _serial_legend->println("------ configSetup --------");
    for (int i = 0 ; i < _modules.size(); i++) {
    _serial_legend->printf("calling %s.configSetup()\r\n", _modules[i]->name());
      _modules[i]->configSetup();
    }

    _serial_legend->println("---------------------------");
    _init_ap();

    setupWebServer(&server, &ws, &events);

    _serial_legend->printf("after setupWebserver\r\n");
    if (blinker) {
      blinker->blink(50);
    }
    else {
      _serial_legend->println("no blinker");
    }

    uint32_t startConfigLoopAtMs = millis();
    while (1 && !stopFlag) {
      if (digitalRead(this->button_gpio) == this->SWITCH_PRESSED_LOGIC) {
          blinker->blink(1000);
          _serial_legend->println("=== button setuped to enabled.");
          enable_run_mode(true);
          delay(300);
          ESP.restart();
      }

      for (int i = 0 ; i < _modules.size(); i++) {
        _modules[i]->configLoop();

        yield();
      }

      if ( (millis() - startConfigLoopAtMs) > 20L*60*1000) {
          _serial_legend->println("3.");
          _serial_legend->println("[Config Timeout]...");
          _serial_legend->println("[Config Timeout]...");
          _serial_legend->println("[Config Timeout]...");
          _serial_legend->println("[Config Timeout]...");
          // enable_run_mode(true);
          // delay(100);
          // ESP.restart();
      }
    }


    File f = SPIFFS.open("/enabled", "a+");
    blinker->blink(50);
    ESP.restart();
  }
  else if (mode == RUN) {
    blinker->blink(4000);
    _serial_legend->println("------ configSetup --------");
    for (int i = 0 ; i < _modules.size(); i++) {
      _serial_legend->printf("calling %s.configSetup()\r\n", _modules[i]->name());
      _modules[i]->configSetup();
    }
    _serial_legend->println("---------------------------");
  }
}

xCMMC_LED *CMMC_Legend::getBlinker() {
  return blinker;
}

void CMMC_Legend::_init_ap() {
  WiFi.disconnect(true);
  WiFi.softAPdisconnect();
  delay(50);
  WiFi.mode(WIFI_AP_STA);
  esp_wifi_set_ps (WIFI_PS_NONE);
  WiFi.setSleep(false);
  delay(50);
  IPAddress Ip(192, 168, 4, 1);
  IPAddress NMask(255, 255, 255, 0);
  this->_serial_legend->println("setting softAPConfig..");
  WiFi.softAPConfig(Ip, Ip, NMask);


  #ifdef ESP8266
    ESP.wdtDisable();
    sprintf(&this->ap_ssid[5], "%08x", ESP.getChipId());
  #else
    uint8_t baseMac[6];
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    Serial.println("XXXXXXX");
    sprintf(this->fullmac, "%02X%02X%02X%02X%02X%02X\r\n",baseMac[0], baseMac[1],  baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    Serial.println(this->fullmac);
    sprintf(this->ap_ssid, "-NAT-%02x%02x%02x%02x", 00, 255, baseMac[4], baseMac[5]);
  #endif
  this->_serial_legend->println(this->ap_ssid);
  this->_serial_legend->println("setting softap..");

  WiFi.softAP(this->ap_ssid, ap_ssid+strlen("DUST-"));
  this->scanWiFi();

  Serial.println("WiFi.softAP called.");
  delay(20);
  IPAddress myIP = WiFi.softAPIP();
  this->_serial_legend->println();
  this->_serial_legend->print("AP IP address: ");
  this->_serial_legend->println(myIP);
  delay(100);
  if (_hook_init_ap != NULL) {
    _hook_init_ap(ap_ssid, fullmac, myIP);
  }
}
void CMMC_Legend::setupWebServer(AsyncWebServer *server, AsyncWebSocket *ws, AsyncEventSource *events) {
  // ws->onEvent(this->onWsEvent);
  static CMMC_Legend *that;
  that = this;
  server->addHandler(ws);
  server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  events->onConnect([](AsyncEventSourceClient * client) {
    client->send("hello!", NULL, millis(), 1000);
  });
  server->addHandler(events);
  // server->addHandler(new SPIFFSEditor(http_username, http_password));

  server->on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", "OK");
    blinker->blink(20);
    ESP.restart();
  });

  server->on("/api/ssid", HTTP_GET, [](AsyncWebServerRequest * request) {
    while(that->wifi_scanning) {
        that->_serial_legend->println("wait scanning wifi.");
        delay(1);
    }
    int n = that->_num_wifi;
    String s;
    // StaticJsonBuffer<400> jsonBuffer;
    // StaticJsonBuffer<400> jsonDBuffer;
    // JsonArray& array1 = jsonBuffer.createArray();
    // int sizeToUse = 0;
    // if (n == 0) {
    //     that->_serial_legend->println("no networks found");
    // } else {
    //     that->_serial_legend->print(n);
    //     that->_serial_legend->println(" networks found");
    //     // if (n > 10) n = 10;
    s += "[";
        char b[200];
        for (int i = 0; i < n-1; ++i) {
          int byteWritten = sprintf(b, "{\"ssid\": \"%s\", \"rssi\": %d},", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
          s+= String(b);
          // sizeToUse += byteWritten;
        }
    int byteWritten = sprintf(b, "{\"ssid\": \"%s\", \"rssi\": %d}", WiFi.SSID(n-1).c_str(), WiFi.RSSI(n-1));
    s+= String(b);
    s += "]";
    //
    //     // char* buffer = (char*)malloc(sizeToUse+1);
    //     bzero(that->buffer, sizeToUse);
    //     sizeToUse = 0;
    //     that->buffer[0] = '[';
    //     for (int i = 0; i < n; ++i) {
    //       char b[200] ;
    //       int byteWritten = sprintf(that->buffer+sizeToUse+1, "{\"ssid\": \"%s\", \"rssi\": %d},", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    //       sizeToUse += byteWritten;
    //     //   that->_serial_legend->println(b);
    //     //   that->_serial_legend->printf("size=%d\r\n", strlen(that->buffer));
    //     //   strcpy(that->buffer+sizeToUse, b);
    //     //   that->_serial_legend->println(that->buffer);
    //       that->_serial_legend->printf("idx=%d\r\n", i);
    //       that->_serial_legend->printf("///idx=%d\r\n", i);
    //     }
    //     that->buffer[sizeToUse] = ']';
    //     // array1.printTo(*that->_serial_legend);
    //     // array1.printTo(x, sizeof(x));
        request->send(200, "text/plain", (s));
    //     free(that->buffer);
  });


  // server->on("/enable", HTTP_GET, [](AsyncWebServerRequest * request) {
  //   File f = SPIFFS.open("/enabled", "a+");
  //   if (!f) {
  //     that->_serial_legend->println("file open failed");
  //   }
  //   request->send(200, "text/plain", String("ENABLING.. ") + String(ESP.getFreeHeap()));
  //   delay(1000);
  //   ESP.restart();
  // });

  static const char* fsServerIndex = "<form method='POST' action='/do-fs' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
  static const char* serverIndex = "<form method='POST' action='/do-firmware' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
  server->on("/firmware", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", serverIndex);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server->on("/fs", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", fsServerIndex);
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server->on("/do-fs", HTTP_POST, [](AsyncWebServerRequest * request) {
    // the request handler is triggered after the upload has finished...
    // create the response, add header, and send response
    bool updateHasError = Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (updateHasError) ? "FAIL" : "OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    // restartRequired = true;  // Tell the main loop to restart the ESP
    request->send(response);
    if (!updateHasError) {
      // delay(1000);
      // ESP.restart();
    }
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    //Upload handler chunks in data
    if (!index) { // if index == 0 then this is the first frame of data
      SPIFFS.end();
      blinker->detach();
      that->_serial_legend->println("upload start...");
      that->_serial_legend->printf("UploadStart: %s\n", filename.c_str());
      that->_serial_legend->setDebugOutput(true);
      // calculate sketch space required for the update
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      bool updateOK = maxSketchSpace < ESP.getFreeSketchSpace();
      if (!Update.begin(maxSketchSpace, U_SPIFFS)) { //start with max available size
        Update.printError(Serial);
      }
      //TODO: ESP32
      // Update.runAsync(true); // tell the updaterClass to run in async mode
    }

    //Write chunked data to the free sketch space
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }

    if (final) { // if the final flag is set then this is the last frame of data
      if (Update.end(true)) { //true to set the size to the current progress
        that->_serial_legend->printf("Update Success: %u B\nRebooting...\n", index + len);
        blinker->blink(1000);
      } else {
        Update.printError(Serial);
      }
      that->_serial_legend->setDebugOutput(false);
    }
  });

  server->on("/update", HTTP_POST, [&](AsyncWebServerRequest *request){
    // the request handler is triggered after the upload has finished...
    // create the response, add header, and send response
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError())?"FAIL":"OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  },[&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    //Upload handler chunks in data
    if(!index){ // if index == 0 then this is the first frame of data
      that->_serial_legend->printf("UploadStart: %s\n", filename.c_str());
      that->_serial_legend->setDebugOutput(true);

      // calculate sketch space required for the update
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if(!Update.begin(maxSketchSpace)){//start with max available size
        Update.printError(Serial);
      }
      // TODO: ESP32
      // Update.runAsync(true); // tell the updaterClass to run in async mode
    }

    //Write chunked data to the free sketch space
    if(Update.write(data, len) != len){
        Update.printError(Serial);
    }

    if(final){ // if the final flag is set then this is the last frame of data
      if(Update.end(true)){ //true to set the size to the current progress
          that->_serial_legend->printf("Update Success: %u B\nRebooting...\n", index+len);
          that->stopFlag = true;
          stopFlag = true;
        } else {
          Update.printError(Serial);
        }
        that->_serial_legend->setDebugOutput(false);
    }
  });


  server->on("/do-firmware", HTTP_POST, [](AsyncWebServerRequest * request) {
    // the request handler is triggered after the upload has finished...
    // create the response, add header, and send response
    bool updateHasError = Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (updateHasError) ? "FAIL" : "OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    // restartRequired = true;  // Tell the main loop to restart the ESP
    request->send(response);
    if (!updateHasError) {
      // delay(1000);
      // ESP.restart();
    }
  }, [&](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    //Upload handler chunks in data
    if (!index) { // if index == 0 then this is the first frame of data
      SPIFFS.end();
      blinker->detach();
      that->_serial_legend->println("upload start...");
      that->_serial_legend->printf("UploadStart: %s\n", filename.c_str());
      that->_serial_legend->setDebugOutput(true);
      // calculate sketch space required for the update
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      bool updateOK = maxSketchSpace < ESP.getFreeSketchSpace();
      if (!Update.begin(maxSketchSpace, U_FLASH)) { //start with max available size
        Update.printError(Serial);
      }
      // TODO: ESP32
      // Update.runAsync(true); // tell the updaterClass to run in async mode
    }

    //Write chunked data to the free sketch space
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }

    if (final) { // if the final flag is set then this is the last frame of data
      if (Update.end(true)) { //true to set the size to the current progress
        that->_serial_legend->printf("Update Success: %u B\nRebooting...\n", index + len);
          that->stopFlag = true;
          stopFlag = true;
        blinker->blink(1000);
      } else {
        Update.printError(Serial);
      }
      that->_serial_legend->setDebugOutput(false);
    }
  });

  server->onNotFound([](AsyncWebServerRequest * request) {
    that->_serial_legend->printf("NOT_FOUND: ");
    if (request->method() == HTTP_GET)
      that->_serial_legend->printf("GET");
    else if (request->method() == HTTP_POST)
      that->_serial_legend->printf("POST");
    else if (request->method() == HTTP_DELETE)
      that->_serial_legend->printf("DELETE");
    else if (request->method() == HTTP_PUT)
      that->_serial_legend->printf("PUT");
    else if (request->method() == HTTP_PATCH)
      that->_serial_legend->printf("PATCH");
    else if (request->method() == HTTP_HEAD)
      that->_serial_legend->printf("HEAD");
    else if (request->method() == HTTP_OPTIONS)
      that->_serial_legend->printf("OPTIONS");
    else
      that->_serial_legend->printf("UNKNOWN");
    that->_serial_legend->printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if (request->contentLength()) {
      that->_serial_legend->printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      that->_serial_legend->printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    // for (i = 0; i < headers; i++) {
    //   // AsyncWebHeader* h = request->getHeader(i);
    //   // that->_serial_legend->printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    // }

    int params = request->params();
    for (i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isFile()) {
        that->_serial_legend->printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if (p->isPost()) {
        that->_serial_legend->printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        that->_serial_legend->printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server->begin();
  that->_serial_legend->println("Starting webserver->...");
  SCREEN = 3;
}
