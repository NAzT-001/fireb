#include <CMMC_Module.h>
#include <WiFi.h>
#include <WiFiGeneric.h>
#ifndef CMMC_WIFI_MODULE_H
#define CMMC_WIFI_MODULE_H

// #DEFINE
class WiFiModule: public CMMC_Module {
  public:
    void config(CMMC_System *os, AsyncWebServer* server);
    void configSetup();
    void setup();
    void loop();
    void isLongPressed();
    void configLoop();
    char sta_ssid[30] = "";
    char sta_pwd[30] = "";
    char ap_pwd[30] = "";
    bool __wifi_connected = false;
    const char* name() {
      return "WiFiModule";
    }
  protected:
    void configWebServer();
    void on_event(system_event_id_t event);
  private:
    void _init_sta();
};

#endif
