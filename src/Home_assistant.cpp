#include <Home_assistant.h>
#include <State.h>

int http_POST_Home_Assistant(const HA_Attributes_t *, const char *);

// Send numeric data with specified sign, integer and fractional parts
int sendNumericData(const HA_Attributes_t * attributes, uint32_t valueInteger,
                             uint8_t valueDecimal, bool isPositive)
{
  char valueText[64] = {0};
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
  return http_POST_Home_Assistant(attributes, valueText);
}

// Send a text string: must have quotation marks added
int sendTextData(const HA_Attributes_t * attributes, const char * valueText)
{
  char quotedText[64] = {0};
  sprintf(quotedText,"\"%s\"", valueText);
  return http_POST_Home_Assistant(attributes, quotedText);
}

// Send the data to Home Assistant as an HTTP POST request.
int http_POST_Home_Assistant(const HA_Attributes_t * attributes, const char * valueText)
{
  char postBuffer[450] = {};
  char fieldBuffer[70] = {};
  int ret;

  client.stop();
  ret = client.connect(HOME_ASSISTANT_IP, 8123);
  if (ret == 1) {
    // Form the URL from the name but replace spaces with underscores
    strncpy(fieldBuffer, attributes->name, sizeof(fieldBuffer));
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
                          "\"unique_id\":\"" SENSOR_NAME "_%s\","
                          "\"device_class\":\"%s\","
                          "\"unit_of_measurement\":\"%s\","
                          "\"friendly_name\":\"%s\","
                          "\"icon\":\"mdi:%s\"}"
                        "}",
                       valueText,
                       fieldBuffer,
                       attributes->device_class, attributes->unit, attributes->name, attributes->icon);

    sprintf(fieldBuffer,"Content-Length: %u", strlen(postBuffer));
    client.println(fieldBuffer);
    client.println();
    client.print(postBuffer);

    // Handle expected entity update codes and dump anything else.
    String responseCode = client.readStringUntil('\n');
    if (responseCode.startsWith("HTTP/1.1 2")) {
      if (responseCode.startsWith("HTTP/1.1 200") ||
          responseCode.startsWith("HTTP/1.1 201")) {

        if (responseCode.startsWith("HTTP/1.1 200")) Serial.print("Updated state of ");
        if (responseCode.startsWith("HTTP/1.1 201")) Serial.print("Created new entity for ");
        Serial.println(attributes->name);

        // Drain response.
        while (client.available()) {
          client.read();
        }
        return 0;
      }

      Serial.println("Undocumented successful response: ");
    } else {
      Serial.println("Error response: ");
    }

    Serial.println(responseCode);
    while (client.available()) {
      Serial.write(client.read());
    }

    return 1;
  }
  else {
    Serial.printf("Client connection failed: %d\n", ret);
    return ret;
  }
}
