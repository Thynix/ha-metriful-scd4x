#include <State.h>

// Between Display and HomeAssistant.
WiFiClient client;

// Between Display and CO2.
long lastCO2;

// Between Display and Metriful.
long lastMetriful;

// Between CO2 and Metriful.
#if CO2_SENSOR == CO2_SENSOR_SCD4x
  #include <SensirionI2CScd4x.h>
  SensirionI2CScd4x scd4x;
#elif CO2_SENSOR == CO2_SENSOR_SCD30
  #include <Adafruit_SCD30.h>
  Adafruit_SCD30 scd30;
#endif
