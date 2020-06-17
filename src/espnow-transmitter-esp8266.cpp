#include <Arduino.h>

/*
  ESP8266 (tested on Wemos D1 mini)

  ESP-NOW based sensor using a BME280 temperature/pressure/humidity sensor

  Sends readings every xx minutes to a server with a fixed mac address

  It takes about 215 milliseconds to wakeup, send a reading and go back to sleep,
  and it uses about 70 milliAmps while awake and about 25 microamps while sleeping,
  so it should last for a good year even AAA alkaline batteries.

  Anthony Elder
  License: Apache License v2

  https://github.com/HarringayMakerSpace/ESP-Now
  MyFolder: HarringayMakerSpace ESP-Now >> built in "test"
  RAM:   [===       ]  34.3% (used 28092 bytes from 81920 bytes)
  Flash: [===       ]  26.6% (used 277312 bytes from 1044464 bytes)

*/
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}
#include "SparkFunBME280.h"

// this is the MAC Address of the remote ESP server which receives these sensor readings
uint8_t remoteMac[] = {0x36, 0x35, 0x34, 0x33, 0x32, 0x31};

#define	MY_LOC_ID	01
#define WIFI_CHANNEL 4
#define SLEEP_SECS 5 * 60 // 5 minutes
#define SEND_TIMEOUT 245  // 245 millis seconds timeout

// keep in sync with gateway struct
struct __attribute__((packed)) SENSOR_DATA {
    uint8_t	loc_id;
    float temp;
    float humidity;
    float pressure;
} sensorData;

BME280 bme280;

volatile boolean callbackCalled;

void readBME280() {
  bme280.settings.commInterface = I2C_MODE;
  bme280.settings.I2CAddress = 0x76;
  bme280.settings.runMode = 2; // Forced mode with deepSleep
  bme280.settings.tempOverSample = 1;
  bme280.settings.pressOverSample = 1;
  bme280.settings.humidOverSample = 1;
  Serial.print("bme280 init="); Serial.println(bme280.begin(), HEX);
  sensorData.loc_id = MY_LOC_ID;
  sensorData.temp = bme280.readTempC();
  sensorData.humidity = bme280.readFloatHumidity();
  sensorData.pressure = bme280.readFloatPressure() / 100.0;
  Serial.printf("location=%i, temp=%01f, humidity=%01f, pressure=%01f\n", sensorData.loc_id, sensorData.temp, sensorData.humidity, sensorData.pressure);
}

void gotoSleep() {
  // add some randomness to avoid collisions with multiple devices
  int sleepSecs = SLEEP_SECS + ((uint8_t)RANDOM_REG32/2);
  Serial.printf("Up for %i ms, going to sleep for %i secs...\n", millis(), sleepSecs);
  ESP.deepSleep(sleepSecs * 1000000, RF_NO_CAL);
}

void setup() {
  Serial.begin(115200); Serial.println();

  // read sensor first before awake generates heat
  readBME280();

  WiFi.mode(WIFI_STA); // Station mode for esp-now sensor node
  WiFi.disconnect();

  Serial.printf("This mac: %s, ", WiFi.macAddress().c_str());
  Serial.printf("target mac: %02x:%02x:%02x:%02x:%02x:%02x", remoteMac[0], remoteMac[1], remoteMac[2], remoteMac[3], remoteMac[4], remoteMac[5]);
  Serial.printf(", channel: %i\n", WIFI_CHANNEL);

  if (esp_now_init() != 0) {
    Serial.println("*** ESP_Now init failed");
    gotoSleep();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(remoteMac, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

  esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) {
    Serial.printf("send_cb, send done, status = %i\n", sendStatus);
    callbackCalled = true;
  });

  callbackCalled = false;

  uint8_t bs[sizeof(sensorData)];
  memcpy(bs, &sensorData, sizeof(sensorData));
  esp_now_send(NULL, bs, sizeof(sensorData)); // NULL means send to all peers
}

void loop() {
  if (callbackCalled || (millis() > SEND_TIMEOUT)) {
    gotoSleep();
  }
}

/*
Typical output:
bme280 init=60
temp=23.530001, humidity=51.628906, pressure=1008.151123
This mac: D8:BF:C0:C7:B4:00, target mac: 363333333333, channel: 4
send_cb, send done, status = 1
Up for 136 ms, going to sleep for 997 secs...

bme280 init=60
temp=23.770000, humidity=49.361328, pressure=1008.143311
This mac: D8:BF:C0:C7:B4:00, target mac: 363333333333, channel: 4
send_cb, send done, status = 1
Up for 138 ms, going to sleep for 1006 secs...

bme280 init=60
temp=23.830000, humidity=49.874023, pressure=1008.109314
This mac: D8:BF:C0:C7:B4:00, target mac: 363333333333, channel: 4
send_cb, send done, status = 1
Up for 135 ms, going to sleep for 949 secs...

bme280 init=60
temp=23.750000, humidity=51.006836, pressure=1008.136108
This mac: D8:BF:C0:C7:B4:00, target mac: 363333333333, channel: 4
send_cb, send done, status = 1
Up for 135 ms, going to sleep for 962 secs...

bme280 init=60
temp=23.740000, humidity=50.060547, pressure=1008.228333
This mac: D8:BF:C0:C7:B4:00, target mac: 363333333333, channel: 4
send_cb, send done, status = 1
Up for 138 ms, going to sleep for 987 secs...

bme280 init=60
temp=23.809999, humidity=50.475586, pressure=1008.266235
This mac: D8:BF:C0:C7:B4:00, target mac: 363333333333, channel: 4
send_cb, send done, status = 1
Up for 135 ms, going to sleep for 925 secs...

bme280 init=60
temp=24.049999, humidity=50.512695, pressure=1008.167603
This mac: D8:BF:C0:C7:B4:00, target mac: 363333333333, channel: 4
send_cb, send done, status = 1
Up for 147 ms, going to sleep for 952 secs...
*/
