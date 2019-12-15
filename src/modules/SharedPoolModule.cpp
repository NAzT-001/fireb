#include "SharedPoolModule.h"
#include "WiFiModule.h"

// extern DustModule *dustModule;
extern WiFiModule *wifiModule;
extern uint8_t SCREEN;
extern float calibrate_temp;

// extern LCDModule* lcdModule;
void SharedPoolModule::isLongPressed() {

}

void SharedPoolModule::configLoop() {
}

void SharedPoolModule::configSetup() {
}

void SharedPoolModule::config(CMMC_System *os, AsyncWebServer *server) {
}

void SharedPoolModule::configWebServer() {
  static SharedPoolModule *that = this;
}

void SharedPoolModule::setup() {
}

void SharedPoolModule::loop() {
  ti.every_ms(1000, [&]() { });
}


String SharedPoolModule::getCurrentDateTime() {
  // return ntpModule->getTimeString();
}
