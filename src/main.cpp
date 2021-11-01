#include <Arduino.h>
#include <Metriful_sensor.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include "settings.h"
#ifndef ESP8266
#include <WiFi101.h>
#endif

// Define the display attributes of data sent to Home Assistant. 
// The chosen name, unit and icon will appear in on the overview 
// dashboard in Home Assistant. The icons can be chosen from 
// https://cdn.materialdesignicons.com/5.3.45/    
// (remove the "mdi-" part from the icon name).
// The attribute fields are: {name, device_class, unit, icon, decimal places}
HA_Attributes_t pressure =       {"Pressure", "pressure", "hPa", "weather-cloudy", 1};
HA_Attributes_t humidity =       {"Humidity", "humidity", "%", "water-percent", 1};
HA_Attributes_t illuminance =    {"Illuminance", "illuminance", "lx", "white-balance-sunny", 2};
HA_Attributes_t soundLevel =     {"Sound level", "None", "dBA", "microphone", 1};
HA_Attributes_t peakAmplitude =  {"Sound peak", "None", "mPa", "waveform", 2};
HA_Attributes_t AQI =            {"Air Quality Index", "aqi", "", "thought-bubble-outline", 1};
HA_Attributes_t AQ_assessment =  {"Air quality assessment", "None", "", "flower-tulip", 0};
HA_Attributes_t CO2 =            {"CO2 concentration", "carbon_dioxide", "ppm", "periodic-table-co2", 0};
#if (PARTICLE_SENSOR == PARTICLE_SENSOR_PPD42)
  HA_Attributes_t particulates = {"Particle concentration", "pm10", "ppL", "chart-bubble", 0};
#else
  HA_Attributes_t particulates = {"Particle concentration", "pm25", SDS011_UNIT_SYMBOL, "chart-bubble", 2};
#endif
#ifdef USE_FAHRENHEIT
  HA_Attributes_t temperature =  {"Temperature", "temperature", FAHRENHEIT_SYMBOL, "thermometer", 1};
#else
  HA_Attributes_t temperature =  {"Temperature", "temperature", CELSIUS_SYMBOL, "thermometer", 1};
#endif

WiFiClient client;
SensirionI2CScd4x scd4x;

char postBuffer[2048] = {};
char fieldBuffer[1024] = {};

AirData_t airData = {};
AirQualityData_t airQualityData = {};
LightData_t lightData = {}; 
ParticleData_t particleData = {};
SoundData_t soundData = {};

void sendNumericData(const HA_Attributes_t *, uint32_t, uint8_t, bool);
void sendTextData(const HA_Attributes_t *, const char *);
void http_POST_Home_Assistant(const HA_Attributes_t *, const char *);
void printWiFiStatus();
void connectToWiFi();

void setup() {
  // Initialize the host pins, set up the serial port and reset:
  SensorHardwareSetup(I2C_ADDRESS); 

  Serial.println("Reporting environment data for " SENSOR_NAME);

  connectToWiFi();

  // Apply settings to the MS430 and enter cycle mode
  uint8_t particleSensorCode = PARTICLE_SENSOR;
  TransmitI2C(I2C_ADDRESS, PARTICLE_SENSOR_SELECT_REG, &particleSensorCode, 1);
  TransmitI2C(I2C_ADDRESS, CYCLE_TIME_PERIOD_REG, &cycle_period, 1);
  ready_assertion_event = false;
  TransmitI2C(I2C_ADDRESS, CYCLE_MODE_CMD, 0, 0);

  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
      Serial.print("stopPeriodicMeasurement() failed: ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error) {
      Serial.print("getSerialNumber() failed: ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
  } else {
    Serial.printf("SCD4x serial number: 0x%04x%04x%04x\r\n", serial0, serial1, serial2);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
      Serial.print("startPeriodicMeasurement() failed: ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
  }
}

void loop() {
  uint16_t dataReady;
  int ret;

  // Ensure WiFi is still connected.
  connectToWiFi();

  // Wait for the next SCD4x data release.
  ret = scd4x.getDataReadyStatus(dataReady);
  if (ret) {
    Serial.printf("Failed to check if SCD4x has data ready: %d\r\n", ret);
  } else if (dataReady & 0x7ff) {
    // Data is ready. From datasheet:
    // > If the least significant 11 bits of
    // > word[0] are 0 → data not ready 
    // > else → data ready for read-out
    uint16_t co2;
    float temperature;
    float humidity;
    ret = scd4x.readMeasurement(co2, temperature, humidity);
    if (ret) {
      Serial.printf("Failed to read SCD4x measurement: %d\r\n", ret);
    } else {
      sendNumericData(&CO2, co2, 0, true);
    }
  }

  // Wait for the next Metriful data release.
  if (!ready_assertion_event) {
    yield();
    return;
  }
  ready_assertion_event = false;

  /* Read data from the MS430 into the data structs. 
  For each category of data (air, sound, etc.) a pointer to the data struct is 
  passed to the ReceiveI2C() function. The received byte sequence fills the data 
  struct in the correct order so that each field within the struct receives
  the value of an environmental quantity (temperature, sound level, etc.)
  */ 
  
  // Air data
  ReceiveI2C(I2C_ADDRESS, AIR_DATA_READ, (uint8_t *) &airData, AIR_DATA_BYTES);
  
  /* Air quality data
  The initial self-calibration of the air quality data may take several
  minutes to complete. During this time the accuracy parameter is zero 
  and the data values are not valid.
  */ 
  ReceiveI2C(I2C_ADDRESS, AIR_QUALITY_DATA_READ, (uint8_t *) &airQualityData, AIR_QUALITY_DATA_BYTES);
  
  // Light data
  ReceiveI2C(I2C_ADDRESS, LIGHT_DATA_READ, (uint8_t *) &lightData, LIGHT_DATA_BYTES);
  
  // Sound data
  ReceiveI2C(I2C_ADDRESS, SOUND_DATA_READ, (uint8_t *) &soundData, SOUND_DATA_BYTES);
  
  /* Particle data
  This requires the connection of a particulate sensor (invalid 
  values will be obtained if this sensor is not present).
  Specify your sensor model (PPD42 or SDS011) in Metriful_sensor.h
  Also note that, due to the low pass filtering used, the 
  particle data become valid after an initial initialization 
  period of approximately one minute.
  */ 
  ReceiveI2C(I2C_ADDRESS, PARTICLE_DATA_READ, (uint8_t *) &particleData, PARTICLE_DATA_BYTES);

  uint8_t T_intPart = 0;
  uint8_t T_fractionalPart = 0;
  bool isPositive = true;
  getTemperature(&airData, &T_intPart, &T_fractionalPart, &isPositive);
  
  // Update pressure used by SCD4x.
  scd4x.setAmbientPressure(roundf(airData.P_Pa / 100.0));

  // Send data to Home Assistant
  sendNumericData(&temperature, (uint32_t) T_intPart, T_fractionalPart, isPositive);
  sendNumericData(&pressure, (uint32_t) roundf(airData.P_Pa / 100.0), 0, true);
  sendNumericData(&humidity, (uint32_t) airData.H_pc_int, airData.H_pc_fr_1dp, true);
  sendNumericData(&illuminance, (uint32_t) lightData.illum_lux_int, lightData.illum_lux_fr_2dp, true);
  sendNumericData(&soundLevel, (uint32_t) soundData.SPL_dBA_int, soundData.SPL_dBA_fr_1dp, true);
  sendNumericData(&peakAmplitude, (uint32_t) soundData.peak_amp_mPa_int, 
                  soundData.peak_amp_mPa_fr_2dp, true);
  sendNumericData(&AQI, (uint32_t) airQualityData.AQI_int, airQualityData.AQI_fr_1dp, true);
  if (PARTICLE_SENSOR != PARTICLE_SENSOR_OFF) {
    sendNumericData(&particulates, (uint32_t) particleData.concentration_int, 
                    particleData.concentration_fr_2dp, true);
  }
  sendTextData(&AQ_assessment, interpret_AQI_value(airQualityData.AQI_int));
}

// Send numeric data with specified sign, integer and fractional parts
void sendNumericData(const HA_Attributes_t * attributes, uint32_t valueInteger, 
                             uint8_t valueDecimal, bool isPositive) {
  char valueText[20] = {0};
  const char * sign = isPositive ? "" : "-";
  switch (attributes->decimalPlaces) {
    case 0:
    default:
      sprintf(valueText,"%s%" PRIu32, sign, valueInteger);
      break;
    case 1:
      sprintf(valueText,"%s%" PRIu32 ".%u", sign, valueInteger, valueDecimal);
      break;
    case 2:
      sprintf(valueText,"%s%" PRIu32 ".%02u", sign, valueInteger, valueDecimal);
      break;
  }
  http_POST_Home_Assistant(attributes, valueText);
}

// Send a text string: must have quotation marks added
void sendTextData(const HA_Attributes_t * attributes, const char * valueText) {
  char quotedText[20] = {0};
  sprintf(quotedText,"\"%s\"", valueText);
  http_POST_Home_Assistant(attributes, quotedText);
}

// Send the data to Home Assistant as an HTTP POST request.
void http_POST_Home_Assistant(const HA_Attributes_t * attributes, const char * valueText) {
  int ret;

  client.stop();
  ret = client.connect(HOME_ASSISTANT_IP, 8123);
  if (ret == 1) {
    // Form the URL from the name but replace spaces with underscores
    strcpy(fieldBuffer,attributes->name);
    for (uint8_t i=0; i<strlen(fieldBuffer); i++) {
      if (fieldBuffer[i] == ' ') {
        fieldBuffer[i] = '_';
      }
    }

    // See https://developers.home-assistant.io/docs/api/rest#actions
    sprintf(postBuffer,"POST /api/states/sensor." SENSOR_NAME "_%s HTTP/1.1", fieldBuffer);
    client.println(postBuffer);
    client.println("Host: " HOME_ASSISTANT_IP ":8123");
    client.println("Content-Type: application/json");
    client.println("Authorization: Bearer " LONG_LIVED_ACCESS_TOKEN);

    // Assemble the JSON content string:
    sprintf(postBuffer, "{\"state\":%s,\"attributes\":{"
                          "\"unique_id\":\"" SENSOR_NAME "\","
                          "\"device_class\":\"%s\","
                          "\"unit_of_measurement\":\"%s\","
                          "\"friendly_name\":\"%s\","
                          "\"icon\":\"mdi:%s\"}"
                        "}",
                       valueText,
                       attributes->device_class, attributes->unit, attributes->name, attributes->icon);
    
    sprintf(fieldBuffer,"Content-Length: %u", strlen(postBuffer));  
    client.println(fieldBuffer);
    client.println();
    client.print(postBuffer);

    // Handle expected entity update codes and dump anything else.
    String responseCode = client.readStringUntil('\n');
    if (responseCode.startsWith("HTTP/1.1 2")) {
      if (responseCode.startsWith("HTTP/1.1 200")) {
        Serial.print("Updated state of ");
        Serial.println(attributes->name);
        return;
      } else if (responseCode.startsWith("HTTP/1.1 201")) {
        Serial.println("Created new entity for ");
        Serial.println(attributes->name);
        return;
      }

      Serial.println("Undocumented successful response:");
    } else {
      Serial.println("Error response: ");
    }

    Serial.println(responseCode);
    while (client.available()) {
      Serial.write(client.read());
    }
  }
  else {
    Serial.printf("Client connection failed: %d\n", ret);
  }
}

void connectToWiFi() {
  wl_status_t status = WiFi.begin(SSID, password);
  if (status == WL_CONNECTED) return;

  // Keep LED on while trying to connect.
  digitalWrite(LED_BUILTIN, HIGH);
  while (status != WL_CONNECTED) {
    Serial.printf("Connecting to SSID: %s", SSID);

    status = WiFi.begin(SSID, password);

    for (int statusChecks = 0; (status != WL_CONNECTED) && (statusChecks < 8); statusChecks++) {
      delay(1000);
      Serial.print(".");
      status = WiFi.status();
    }

    if (status != WL_CONNECTED) {
      Serial.println(" Failed.");
      if (status == WL_WRONG_PASSWORD) {
        Serial.println(" Wrong password.");
      } else {
        Serial.println();
      }

      WiFi.disconnect();
      delay(5000);
    }
  }

  Serial.println(" Connected.");
  printWiFiStatus();
  digitalWrite(LED_BUILTIN, LOW);
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
