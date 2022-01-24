#include <Update_metriful.h>
#include <Metriful_sensor.h>
#include <Home_assistant.h>
#include <State.h>

void setupMetriful() {
  // Initialize the host pins, set up the serial port and reset:
  SensorHardwareSetup(I2C_ADDRESS);

  // TODO: before being split out into modules, this went after connecting to WiFi. The delay may be important.
  // Apply settings to the MS430 and enter cycle mode
  uint8_t particleSensorCode = PARTICLE_SENSOR;
  TransmitI2C(I2C_ADDRESS, PARTICLE_SENSOR_SELECT_REG, &particleSensorCode, 1);
  TransmitI2C(I2C_ADDRESS, CYCLE_TIME_PERIOD_REG, &cycle_period, 1);
  ready_assertion_event = false;
  TransmitI2C(I2C_ADDRESS, CYCLE_MODE_CMD, 0, 0);
}


void updateMetriful()
{
  // Wait for the next Metriful data release.
  if (!ready_assertion_event) {
    return;
  }
  ready_assertion_event = false;

  /* Read data from the MS430 into the data structs.
  For each category of data (air, sound, etc.) a pointer to the data struct is
  passed to the ReceiveI2C() function. The received byte sequence fills the data
  struct in the correct order so that each field within the struct receives
  the value of an environmental quantity (temperature, sound level, etc.)
  */
  AirData_t airData = {};
  AirQualityData_t airQualityData = {};
  LightData_t lightData = {};
  ParticleData_t particleData = {};
  SoundData_t soundData = {};

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

#if CO2_SENSOR == CO2_SENSOR_SCD4x
  // Update pressure used by SCD4x.
  scd4x.setAmbientPressure(roundf(airData.P_Pa / 100.0));
#endif

  // Send data to Home Assistant
  sendNumericData(&temperature, (uint32_t) T_intPart, T_fractionalPart, isPositive);
  sendNumericData(&pressure, (uint32_t) roundf(airData.P_Pa / 100.0), 0, true);
  sendNumericData(&humidity, (uint32_t) airData.H_pc_int, airData.H_pc_fr_1dp, true);
  sendNumericData(&illuminance, (uint32_t) lightData.illum_lux_int, lightData.illum_lux_fr_2dp, true);
  sendNumericData(&soundLevel, (uint32_t) soundData.SPL_dBA_int, soundData.SPL_dBA_fr_1dp, true);
  sendNumericData(&peakAmplitude, (uint32_t) soundData.peak_amp_mPa_int,
                  soundData.peak_amp_mPa_fr_2dp, true);
  sendNumericData(&AQI, (uint32_t) airQualityData.AQI_int, airQualityData.AQI_fr_1dp, true);
  sendTextData(&AQ_assessment, interpret_AQI_value(airQualityData.AQI_int));
#if PARTICLE_SENSOR != PARTICLE_SENSOR_OFF
    sendNumericData(&particulates, (uint32_t) particleData.concentration_int,
                    particleData.concentration_fr_2dp, true);
#endif

  lastMetriful = millis();
}
