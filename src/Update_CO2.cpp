#include <Update_CO2.h>
#include <Arduino.h>
#include <State.h>
#include <Home_assistant.h>

void setupCO2() {
#if CO2_SENSOR == CO2_SENSOR_SCD4x
  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);

  // Stop potentially previously started measurement.
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

  scd4x.setAutomaticSelfCalibration(false);
#elif CO2_SENSOR == CO2_SENSOR_SCD30
  while (!scd30.begin()) delay(10);
  scd30.selfCalibrationEnabled(false);

  // Match SCD4x's periodic measurement of 5 seconds.
  scd30.setMeasurementInterval(5);
#endif
}

void updateCO2()
{
#if CO2_SENSOR == CO2_SENSOR_SCD4x
  static long lastUpdate = millis();
  char errorMessage[256];
  uint16_t dataReady;
  int ret;

  // Don't poll the SCD4x too often.
  long now = millis();
  if (max(now, lastUpdate) - min(now, lastUpdate) < 10) return;

  // Wait for the next SCD4x data release.
  ret = scd4x.getDataReadyStatus(dataReady);
  if (ret) {
    errorToString(ret, errorMessage, sizeof(errorMessage));
    Serial.printf("Failed to check if SCD4x has data ready: %s\r\n", errorMessage);
  } else if ((dataReady & ((1 << 11) - 1)) != 0) {
    // Data is ready. From datasheet:
    // > If the least significant 11 bits of
    // > word[0] are 0 → data not ready
    // > else → data ready for read-out
    uint16_t co2;
    float temperature;
    float humidity;
    ret = scd4x.readMeasurement(co2, temperature, humidity);
    if (ret) {
      errorToString(ret, errorMessage, sizeof(errorMessage));
      Serial.printf("Failed to read SCD4x measurement: %s\r\n", errorMessage);
    } else {
      if (!sendNumericData(&CO2, co2, 0, true)) {
        lastCO2 = millis();
      }
    }
  }
#elif CO2_SENSOR == CO2_SENSOR_SCD30
  if (scd30.dataReady()) {
    if (scd30.read()) {
      if (!sendNumericData(&CO2, scd30.CO2, 0, true)) {
        lastCO2 = millis();
      }
    } else {
      Serial.println("Error reading CO2 sensor data");
    }
  }
#endif
}
