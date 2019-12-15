#include <CMMC_Module.h>

#ifndef CMMC_SHARED_POOL_H
#define CMMC_SHARED_POOL_H

#include <WiFi.h>
#include <CMMC_Interval.h>
#include <PMS.h>

// #DEFINE
class SharedPoolModule: public CMMC_Module {
  public:
    void config(CMMC_System *os, AsyncWebServer* server);
    void configSetup();
    void setup();
    void loop();
    void isLongPressed();
    void configLoop();
    const char* name() {
      return "SharedPoolModule";
    }

    String getCurrentDateTime();

    uint16_t __pm10 = 0;
    uint16_t __pm2_5 = 0;

    float __temperature = 0;
    float __humidity = 0;
    float __pressure = 0;

    char __device_name[30];
    bool __wifi_connected = false;

    uint16_t __dust_PM_SP_UG_1_0 = 0;
    uint16_t __dust_PM_SP_UG_2_5 = 0;
    uint16_t __dust_PM_SP_UG_10_0 = 0;

    // Atmospheric environment
    uint16_t __dust_PM_AE_UG_1_0 = 0;
    uint16_t __dust_PM_AE_UG_2_5 = 0;
    uint16_t __dust_PM_AE_UG_10_0 = 0;

  protected:
    void configWebServer();
  private:
    CMMC_Interval ti;
};

#endif
