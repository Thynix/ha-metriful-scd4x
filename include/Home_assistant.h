#include <settings.h>
#include <Metriful_sensor.h>

// Define the display attributes of data sent to Home Assistant.
// The chosen name, unit and icon will appear in on the overview
// dashboard in Home Assistant. The icons can be chosen from
// https://cdn.materialdesignicons.com/5.3.45/
// (remove the "mdi-" part from the icon name).
// The attribute fields are: {name, device_class, unit, icon, decimal places}
const HA_Attributes_t pressure =       {"Pressure", "pressure", "hPa", "weather-cloudy", 1};
const HA_Attributes_t humidity =       {"Humidity", "humidity", "%", "water-percent", 1};
const HA_Attributes_t illuminance =    {"Illuminance", "illuminance", "lx", "white-balance-sunny", 2};
const HA_Attributes_t soundLevel =     {"Sound level", "None", "dBA", "microphone", 1};
const HA_Attributes_t peakAmplitude =  {"Sound peak", "None", "mPa", "waveform", 2};
const HA_Attributes_t AQI =            {"Air Quality Index", "aqi", "", "thought-bubble-outline", 1};
const HA_Attributes_t AQ_assessment =  {"Air quality assessment", "None", "", "flower-tulip", 0};
#if CO2_SENSOR != CO2_SENSOR_OFF
  const HA_Attributes_t CO2 =            {"CO2 concentration", "carbon_dioxide", "ppm", "molecule-co2", 0};
#endif
#if PARTICLE_SENSOR == PARTICLE_SENSOR_PPD42
  const HA_Attributes_t particulates = {"Particle concentration", "pm10", "ppL", "chart-bubble", 0};
#elif PARTICLE_SENSOR == PARTICLE_SENSOR_SDS011
  const HA_Attributes_t particulates = {"Particle concentration", "pm25", SDS011_UNIT_SYMBOL, "chart-bubble", 2};
#endif
#ifdef USE_FAHRENHEIT
  const HA_Attributes_t temperature =  {"Temperature", "temperature", FAHRENHEIT_SYMBOL, "thermometer", 1};
#else
  const HA_Attributes_t temperature =  {"Temperature", "temperature", CELSIUS_SYMBOL, "thermometer", 1};
#endif

int sendNumericData(const HA_Attributes_t *, uint32_t, uint8_t, bool);
int sendTextData(const HA_Attributes_t *, const char *);
