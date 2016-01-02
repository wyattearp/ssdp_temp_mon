#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>

#include "LocalCredentials.h"

#ifndef CREDS_SSID
#error "You need to define CREDS_SSID in LocalCredentials.h"
#endif

#ifndef CREDS_PASS
#error "You need to define CREDS_PASS in LocalCredentials.h"
#endif

const char* ssid = CREDS_SSID;
const char* pass = CREDS_PASS;

#define LISTEN_PORT 16384

// where we'll handle responses
ESP8266WebServer HTTP(LISTEN_PORT);

void hexdump_mem(const void* data, size_t size) {
  Serial.println("===== BEGIN DUMP  =====");
  char ascii[17];
  size_t i, j;
  ascii[16] = '\0';
  for (i = 0; i < size; ++i) {
    Serial.printf("%02X ", ((unsigned char*)data)[i]);
    if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
      ascii[i % 16] = ((unsigned char*)data)[i];
    } else {
      ascii[i % 16] = '.';
    }
    if ((i + 1) % 8 == 0 || i + 1 == size) {
      Serial.printf(" ");
      if ((i + 1) % 16 == 0) {
        Serial.printf("|  %s \n", ascii);
      } else if (i + 1 == size) {
        ascii[(i + 1) % 16] = '\0';
        if ((i + 1) % 16 <= 8) {
          Serial.printf(" ");
        }
        for (j = (i + 1) % 16; j < 16; ++j) {
          Serial.printf("   ");
        }
        Serial.printf("|  %s \n", ascii);
      }
    }
  }
  Serial.println("=====  END DUMP  =====");
}

String ip_address_to_str(IPAddress address)
{
  return String(address[0]) + "." +
         String(address[1]) + "." +
         String(address[2]) + "." +
         String(address[3]);
}

String getDeviceStatus() {
  // TODO: fill out with sensor data
  int battery_level = 100;
  int temperature = 68;
  char unit = 'f';
  int humidity = 34;

  String data = "{ \"battery\":";
  data += battery_level;
  data += ", \"temperature\": ";
  data += temperature;
  data += ", \"unit\": \"";
  data += unit;
  data += "\"";
  data += ", \"humidity\": ";
  data += humidity;
  data += " }";

  return data;
}

/* ---- Arduino Functions ---- */
void setup() {

  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.print(ssid);
  Serial.println();

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected.");
  Serial.print("IP Address: ");
  Serial.print(WiFi.localIP());
  Serial.println();

  // now that we've got an address, fire up the HTTP server
  Serial.printf("Starting http server on %d: ", LISTEN_PORT);
  HTTP.on("/index.html", HTTP_GET, []() {
    String status = getDeviceStatus();
    HTTP.send(200, "application/json", status);
  });

  HTTP.on("/description.xml", HTTP_GET, []() {
    SSDP.schema(HTTP.client());
  });

  HTTP.begin();
  Serial.print("done.");
  Serial.println();

  // Once HTTP is running, we can start handling SSDP messages
  Serial.println();
  Serial.print("Starting SSDP server on the usual port: ");
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(LISTEN_PORT);
  SSDP.setName("Temperature and Humdity Sensor");
  SSDP.setSerialNumber("8675309");
  SSDP.setURL("index.html");
  SSDP.setModelName("TandH");
  SSDP.setModelNumber("v1");
  SSDP.setModelURL("http://hackerforhire.org/TandHv1");
  SSDP.setManufacturer("Hacker for Hire");
  SSDP.setManufacturerURL("http://hackerforhire.org/");
  SSDP.begin();

  Serial.print("done.");
  Serial.println();
  
}

void loop() {
  HTTP.handleClient();
  delay(1);
}
