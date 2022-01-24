#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Bounce2.h>
#include <settings.h>
#include <wl_definitions.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>

#if DISPLAY_FEATHER == DISPLAY_128x32
  Adafruit_SSD1306 display;
#elif DISPLAY_FEATHER == DISPLAY_128x64
  Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
#elif DISPLAY_FEATHER == DISPLAY_OFF
  void setupDisplay() {}
  void updateDisplay() {}
#else
  #error "Unsupported DISPLAY_FEATHER value"
#endif

#if DISPLAY_FEATHER != DISPLAY_OFF

Bounce2::Button connectivityButton;
Bounce2::Button metrifulButton;
Bounce2::Button scd4XButton;

enum DisplayTab {
  None,
  Connectivity,
  Metriful,
  SCD40,
};

// Feather button pin definitions via
// https://learn.adafruit.com/adafruit-128x64-oled-featherwing/arduino-code#run-example-code-3071194-10
#if defined(ESP8266)
  const uint8_t BUTTON_A =  0;
  const uint8_t BUTTON_B = 16;
  const uint8_t BUTTON_C =  2;
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  const uint8_t BUTTON_A = 15;
  const uint8_t BUTTON_B = 32;
  const uint8_t BUTTON_C = 14;
#elif defined(ARDUINO_STM32_FEATHER)
  const uint8_t BUTTON_A = PA15;
  const uint8_t BUTTON_B = PC7;
  const uint8_t BUTTON_C = PC5;
#elif defined(TEENSYDUINO)
  const uint8_t BUTTON_A =  4;
  const uint8_t BUTTON_B =  3;
  const uint8_t BUTTON_C =  8;
#elif defined(ARDUINO_NRF52832_FEATHER)
  const uint8_t BUTTON_A = 31;
  const uint8_t BUTTON_B = 30;
  const uint8_t BUTTON_C = 27;
#else // 32u4, M0, M4, nrf52840, esp32-s2 and 328p
  const uint8_t BUTTON_A =  9;
  const uint8_t BUTTON_B =  6;
  const uint8_t BUTTON_C =  5;
#endif


void setup_display()
{
  connectivityButton.attach(BUTTON_A, INPUT_PULLUP);
  connectivityButton.setPressedState(LOW);

  metrifulButton.attach(BUTTON_B, INPUT_PULLUP);
  metrifulButton.setPressedState(LOW);

  scd4XButton.attach(BUTTON_C, INPUT_PULLUP);
  scd4XButton.setPressedState(LOW);

  #if DISPLAY_FEATHER == DISPLAY_128x32
  display = Adafruit_SSD1306(128, 32, &Wire);
  #endif

  display.begin();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(2, 0);
  display.print("Starting up");
  display.display();
}

void updateDisplay()
{
#if DISPLAY_FEATHER == DISPLAY_128x32
  static DisplayTab tab = Connectivity;
  static long lastUpdate = millis();
  static bool blank = false;

  connectivityButton.update();
  metrifulButton.update();
  scd4XButton.update();

  // Use a placeholder value to check whether any button was pressed this update.
  DisplayTab previousTab = tab;
  tab = None;

  if (connectivityButton.pressed()) tab = Connectivity;
  if (metrifulButton.pressed()) tab = Metriful;
  if (scd4XButton.pressed()) tab = SCD40;

  if (tab == None) tab = previousTab;
  else if (tab != previousTab) {
    blank = false;
    Serial.printf("Display tab changed to %d\r\n", tab);
  } else {
    // Active tab button was pressed this update. Toggle screen blank.
    blank = !blank;
  }

  long now = millis();
  if (max(now, lastUpdate) - min(now, lastUpdate) < 10) return;
  lastUpdate = now;

  display.clearDisplay();

  if (blank) {
    display.display();
    return;
  }

  switch (tab) {
  case Connectivity: {
    int status = WiFi.status();

    display.setCursor(2, 0);
    display.print(SSID);

    if (status != WL_CONNECTED) {
      display.setCursor(2, 8);
      display.print("not connected");
    } else {
      display.setCursor(2, 8);
      display.print("connected");

      display.setCursor(2, 16);
      display.print(IPAddress(WiFi.localIP()));

      display.setCursor(2, 24);
      #ifndef ESP8266
      display.printf("%ld dBm", WiFi.RSSI());
      #else
      display.printf("%d dBm", WiFi.RSSI());
      #endif
    }
    break;
  }
  case Metriful:
    display.setCursor(2, 0);
    display.print("Metriful updated");

    display.setCursor(2, 8);
    display.print((now - lastMetriful) / 1000);

    display.setCursor(2, 16);
    display.print("secs ago");
    break;
  case SCD40:
    display.setCursor(2, 0);
    display.println("CO2 updated");

    display.setCursor(2, 8);
    display.println((now - lastCO2) / 1000);

    display.setCursor(2, 16);
    display.print("secs ago");
    break;
  default:
    display.setCursor(2, 0);
    display.printf("Unhandled tab %d", tab);
  }

  display.display();
#endif
}
#endif
