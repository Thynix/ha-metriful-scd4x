#include <WiFiClient.h>

// State that crosses modules goes here.

// Between Display and HomeAssistant.
extern WiFiClient client;

// Between Display and CO2.
extern long lastCO2;

// Between Display and Metriful.
extern long lastMetriful;

// Between CO2 and Metriful.
#if CO2_SENSOR == CO2_SENSOR_SCD4x
  #include <SensirionI2CScd4x.h>
  extern SensirionI2CScd4x scd4x;
#elif CO2_SENSOR == CO2_SENSOR_SCD30
  #include <Adafruit_SCD30.h>
  extern Adafruit_SCD30 scd30;
#elif CO2_SENSOR == CO2_SENSOR_OFF
#else
  #error "Unsupported CO2_SENSOR value"
#endif
