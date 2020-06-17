#include <Arduino.h>

/*
  ESP32 (tested on generic DevKit v4 board)

  ESP-NOW based gateway, receives data packet from ESP8266 client node with
  BME280 sensor

  Waits to be sent readings every xx minutes from client with a fixed mac address

  Most of the code for ESPNow on ESP32 doesn't work, finely got it working thanks
  to these articles:
    https://www.reddit.com/r/esp32/comments/9jmkf9/here_is_how_you_set_a_custom_mac_address_on_esp32/
    https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/
*/

#include <esp_now.h> // Note "esp_now.h" for the ESP32 ("espnow.h" for the 8266).
#include <WiFi.h>
#include <esp_wifi.h> // *** FOR REFERENCE *** Important info in this file.

#define WIFI_DEFAULT_CHANNEL 11
uint8_t masterCustomMAC[] = {0x36, 0x35, 0x34, 0x33, 0x32, 0x31};
esp_now_peer_info_t slave;
const uint8_t maxDataFrameSize = 200;
byte cnt = 0;
volatile boolean haveReading = false;
uint8_t remoteMac[6];

// keep in sync with client struct
typedef struct __attribute__((packed)) SENSOR_DATA
{
  uint8_t loc_id;
  float temp;
  float humidity;
  float pressure;
} SENSOR_DATA;

// Create a struct_message called myData
SENSOR_DATA myData;

/* This must be called before setup or at least before esp_now_init() */

void initVariant()
{
  WiFi.mode(WIFI_AP); // OR WHATEVER mode we need.  In master case Station WIFI_STA or Slave case WIFI_AP
  //wifi_set_macaddr(SOFTAP_IF, &customMac[0]); //8688 code
  //wifi_set_macaddr(STATION_IF, &customMac[0]); //8688 code
  esp_wifi_set_mac(ESP_IF_WIFI_AP, &masterCustomMAC[0]); // esp32 code
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int data_len)
{
  Serial.printf("\r\nReceived\t%d Bytes\t%d", data_len, incomingData[0]);
  memcpy(&myData, incomingData, sizeof(myData));
  memcpy(&remoteMac, mac_addr, sizeof(remoteMac));
  Serial.printf("\nlocation=%i, temp=%01f, humidity=%01f, pressure=%01f\n", myData.loc_id, myData.temp, myData.humidity, myData.pressure);
  Serial.printf("target mac: %02x:%02x:%02x:%02x:%02x:%02x\n", remoteMac[0], remoteMac[1], remoteMac[2], remoteMac[3], remoteMac[4], remoteMac[5]);
  haveReading = true;
}

void setup()
{
  Serial.begin(115200);

  initVariant();

  WiFi.mode(WIFI_AP);
  Serial.print("This node AP mac (softAPmacAddress): "); Serial.println(WiFi.softAPmacAddress());
  Serial.print("This node STA mac (macAddress): "); Serial.println(WiFi.macAddress());
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESPNow Init Success!");
  }
  else
  {
    Serial.println("ESPNow Init Failed....");
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  yield();
}
