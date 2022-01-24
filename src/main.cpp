#include <Arduino.h>

#include <settings.h>

#include <State.h>
#include <Home_assistant.h>
#include <Update_CO2.h>
#include <Update_display.h>
#include <Update_metriful.h>

void printWiFiStatus();
void connectToWiFi();

void setup()
{
#ifdef M0WiFi // TODO: Is there already a flag for this?
  // Configure pins for Adafruit ATWINC1500 Feather
  WiFi.setPins(8, 7, 4, 2);
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(SERIAL_BAUD_RATE);
  long start = millis();
  while (!Serial) {
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);

    // Give up waiting for serial after 10 seconds.
    if (millis() - start > 10000) break;
  }

  Serial.println("Starting up");

  setupDisplay();
  setupMetriful();

  Serial.println("Reporting environment data for " SENSOR_NAME);

  connectToWiFi();

  setupCO2();
}

void loop()
{
  // Ensure WiFi is still connected.
  connectToWiFi();

  updateCO2();
  updateMetriful();
  updateDisplay();

  // Run TinyUSB tasks.
  yield();
}

void connectToWiFi()
{
  int status = WiFi.status();
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
      WiFi.disconnect();
      delay(5000);
    }
  }

  Serial.println(" Connected.");
  printWiFiStatus();
  digitalWrite(LED_BUILTIN, LOW);
}

void printWiFiStatus()
{
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
