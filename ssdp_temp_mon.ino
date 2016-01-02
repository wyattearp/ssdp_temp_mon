#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "LocalCredentials.h"

#ifndef CREDS_SSID
#error "You need to define CREDS_SSID in LocalCredentials.h"
#endif

#ifndef CREDS_PASS
#error "You need to define CREDS_PASS in LocalCredentials.h"
#endif

const char* ssid = CREDS_SSID;
const char* pass = CREDS_PASS;

#define SSDP_MULTICAST_TTL 2
IPAddress ssdp_addr(239, 255, 255, 250);
const uint16_t ssdp_port = 1900;
WiFiUDP ssdp_server;

#define M_SEARCH_PATTERN "M-SEARCH"
#define SSDP_DISCOVERY "ssdp:discover"
// as defined by TemperatureSensor:1 UPNP Template from http://upnp.org/specs/ha/UPnP-ha-TemperatureSensor-v1-Service.pdf
#define URN_TEMP_SENSOR "urn:schemas-upnp-org:service:TemperatureSensor:1"
#define URN_RESPONSE "HTTP/1.1 200 OK\r\nCACHE-CONTROL: max-age=1200\r\nEXT:\r\nSERVER:Arduino\r\nST: upnp:rootdevice\r\n"
#define URN_USN_RESPONSE "USN: uuid:abcdefgh-7dec-11d0-a765-7499692d3040\r\n"
#define URN_LOCATION "LOCATION: http://";

//dont forget our mac adress USN: uuid:abcdefgh-7dec-11d0-a765-Mac addr "
// the place to store our data during discovery
unsigned char ssdp_packet[UDP_TX_PACKET_MAX_SIZE];

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


void setup() {
  pinMode(0, OUTPUT);

  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println();
  Serial.println("Connecting to: ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected.");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  // now that we've got an address, start listening for mcast
  ssdp_server.beginMulticast(WiFi.localIP(), ssdp_addr, ssdp_port);

}

int value = 0;
void loop() {
  delay(5000);
  ++value;

  if (ssdp_server.available() > 0) {
    // let's turn on the LED so we know something is working
    digitalWrite(0, LOW);
    while (ssdp_server.available() > 0) {
      int len = ssdp_server.peek();
      Serial.printf("Data available: 0x%08x\n", len);
      Serial.printf("Data will be stored at 0x%08x\n", &ssdp_packet);
      memset(ssdp_packet, 0, sizeof(ssdp_packet));
      len = ssdp_server.parsePacket();
      Serial.printf("Data available: 0x%08x\n", len);
      ssdp_server.read(ssdp_packet, len);
      // is this a packet we're interested
      if (memcmp(M_SEARCH_PATTERN, ssdp_packet, sizeof(M_SEARCH_PATTERN) - 1) == 0) {
        // we're interested, is it a discovery we should respond to?
        char* needle;
        if ((needle = strstr((const char*)ssdp_packet, (const char*)SSDP_DISCOVERY)) != NULL) {
          // this is a legit discovery message, is it our kind?
          if ((needle = strstr((const char*)ssdp_packet, (const char*)URN_TEMP_SENSOR)) != NULL) {
            // excellent, this is something we should respond to
            // TODO: perform appropriate response
            // TODO: provide details about how to do callback
            hexdump_mem(ssdp_packet, len);
          }
        }
        Serial.println("Done.");
      }
    }
    digitalWrite(0, HIGH);
    // restart the listening, for some reason ... ?
    ssdp_server.beginMulticast(WiFi.localIP(), ssdp_addr, ssdp_port);
  } else {
    Serial.println("Waiting for packet...");
  }
}
