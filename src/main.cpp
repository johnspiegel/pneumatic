#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>

#include "bme.h"
#include "constants.h"
#include "dsco220.h"
#include "dump.h"
#include "html.h"
#include "mhz19.h"
#include "net_manager.h"
#include "ota.h"
#include "pmsx003.h"
#include "sensor_community.h"
#include "ui.h"

#ifndef LED_BUILTIN
// ESP32 Dev Kit
#define LED_BUILTIN 2
// Heltec wifi kit 32
// #define LED_BUILTIN 25
#endif

#define WS2812B_PIN 25

// NodeMCU 32-s
// #define PMSX003_RX_PIN 16
// #define PMSX003_TX_PIN 17

// TTGO T-Display
#define PMSX003_RX_PIN 26
#define PMSX003_TX_PIN 27

#define MHZ19_RX_PIN 33
#define MHZ19_TX_PIN 32

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
// My BME680 seems unstable at 100khz.
#define I2C_FREQ 10000

#define BME280_I2C_ADDRESS 0x76
// TODO: acutally use ds-co2-20 address
#define DSCO220_I2C_ADDRESS 0x08

static const char* TAG = "main";

SemaphoreHandle_t i2c_mutex = nullptr;

HardwareSerial pms_serial(2);
pmsx003::TaskData pmsx003_data = {0};

// HardwareSerial mhz19_serial(1);
mhz19::TaskData mhz19_data = {0};

bme::Data bme_data = {0};

dsco220::Data dsco220_data = {0};
dsco220::TaskData dsco220_task_data = {0};

ui::TaskData ui_task_data = {0};
Adafruit_NeoPixel pixels(/*num_pixels=*/1, /*pin=*/WS2812B_PIN,
                         NEO_GRB + NEO_KHZ800);

void i2cScan() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("done\n");
  }
}

void setup() {
  ESP_LOGI(TAG, "setup(): version: [%s] core: %d", kPneumaticVersion,
           xPortGetCoreID());

  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Arduino Serial online");

  // This chipid is the mac address, but reversed? I'll just use the actual mac
  // address.
  String esp_chipid;
  uint64_t chipid_num;
  chipid_num = ESP.getEfuseMac();
  esp_chipid = String((uint16_t)(chipid_num >> 32), HEX);
  esp_chipid += String((uint32_t)chipid_num, HEX);
  Serial.print("setup(): esp_chip_id: ");
  Serial.println(esp_chipid);
  char mac[17] = {0};
  for (int i = 0; i < 6; ++i) {
    // uint8_t val = chipid_num >> (8 * (7-i)) & 0xff;
    snprintf(mac + i * 2, 3, "%02x",
             reinterpret_cast<uint8_t*>(&chipid_num)[i]);
  }
  Serial.print("setup(): e_fuse_mac: ");
  Serial.println(mac);

  // ui::InitTft();

  // pixels.begin();
  // pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  // pixels.show();

  // Serial.println("Setting up WiFi...");
  // net_manager::Setup();
  // net_manager::Connect(/*timeout_ms=*/ 60000);

  Serial.println("Setting up PMSx003 Serial port...");
  pms_serial.begin(9600, SERIAL_8N1, /*rx=*/PMSX003_RX_PIN,
                   /*tx=*/PMSX003_TX_PIN);
  while (!pms_serial) {
    Serial.println("    ...");
    delay(100);
  }
  pmsx003_data.serial = &pms_serial;
  Serial.println("PMSx003 Serial online");

  // Serial.println("Setting up MH-Z19 Serial port...");
  // mhz19_serial.begin(9600, SERIAL_8N1, /*rx=*/MHZ19_RX_PIN,
  //                    /*tx=*/MHZ19_TX_PIN);
  // while (!mhz19_serial) {
  //   Serial.println("    ...");
  //   delay(100);
  // }
  // Serial.println("MH-Z19 Serial online");
  // mhz19_data.serial = &mhz19_serial;
  // mhz19::SetAutoBackgroundCalibration(&mhz19_serial, /*abc_on=*/true);

  ESP_LOGI(TAG, "Initializing I2C bus...");
  i2c_mutex = xSemaphoreCreateMutex();
  Wire.begin(/* sda= */ I2C_SDA_PIN,
             /* scl= */ I2C_SCL_PIN,
             /* frequencey= */ I2C_FREQ);
  i2cScan();

  ESP_LOGI(TAG, "Setting up Plantower DS CO2...");
  dsco220_task_data.i2c_mutex = i2c_mutex;
  dsco220_task_data.i2c = &Wire;
  dsco220_task_data.data = &dsco220_data;

  ESP_LOGI(TAG, "Setting up BMEx8x...");
  bme_data.i2c_mutex = i2c_mutex;
  bme::Init(&bme_data);

  ESP_LOGI(TAG, "Initializing NTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_servermode_dhcp(true);
  // sntp_setservername(0, "pool.ntp.org");
  sntp_setservername(0, "time.google.com");
  sntp_init();
  setenv("TZ", "PST8PDT,M3.2.0,M11.1.0", 1);
  tzset();

  // Apparently ESP32 FreeRTOS can't elegantly handle different tasks at the
  // same priority without the possibility of starvation.
  int next_priority = 2;
  xTaskCreate(pmsx003::TaskPoll, "pmsx003",
              /*stack_size=*/3 * 1024,
              /*param=*/&pmsx003_data,
              /*priority=*/next_priority++,
              /*handle=*/nullptr);
  // xTaskCreate(mhz19::TaskPoll, "PollMhz19",
  //             /*stack_size=*/10000,
  //             /*param=*/&mhz19_data,
  //             /*priority=*/next_priority++,
  //             /*handle=*/nullptr);
  xTaskCreate(dsco220::TaskPollDsCo2, "dsco220",
              /*stack_size=*/3 * 1024,
              /*param=*/&dsco220_task_data,
              /*priority=*/next_priority++,
              /*handle=*/nullptr);
  xTaskCreate(bme::TaskPoll, "bme",
              /*stack_size=*/4 * 1024,
              /*param=*/&bme_data,
              /*priority=*/next_priority++,
              /*handle=*/nullptr);

  ui_task_data.pmsx003_data = &pmsx003_data;
  ui_task_data.mhz19_data = &mhz19_data;
  ui_task_data.dsco220_data = &dsco220_data;
  ui_task_data.bme_data = &bme_data;
  ui_task_data.pixels = &pixels;
  // xTaskCreate(ui::TaskDoPixels, "TaskDoPixels",
  //             /*stack_size=*/1024,
  //             /*param=*/&ui_task_data,
  //             /*priority=*/next_priority++,
  //             /*handle=*/nullptr);
  // xTaskCreate(sensor_community::TaskSensorCommunity, "TaskSensorCommunity",
  //             /*stack_size=*/4 * 1024,
  //             /*param=*/&ui_task_data,
  //             /*priority=*/next_priority++,
  //             /*handle=*/nullptr);
  xTaskCreate(ui::TaskDisplay, "TaskDisplay",
              /*stack_size=*/3 * 1024,
              /*param=*/&ui_task_data,
              /*priority=*/next_priority++,
              /*handle=*/nullptr);
  xTaskCreate(ui::TaskServeWeb, "TaskServeWeb",
              /*stack_size=*/8 * 1024,
              /*param=*/&ui_task_data,
              /*priority=*/next_priority++,
              /*handle=*/nullptr);
  xTaskCreate(ota::TaskOta, "TaskOta",
              // Note: 4484 bytes of stack used to do an OTA on v0.2.0
              /*stack_size=*/8 * 1024,
              /*param=*/nullptr,
              /*priority=*/next_priority++,
              /*handle=*/nullptr);
  xTaskCreate(net_manager::DoTask, "NetManager",
              /*stack_size=*/8 * 1024,
              /*param=*/nullptr,
              /*priority=*/next_priority++,
              /*handle=*/nullptr);
  xTaskCreate(ui::TaskButtons, "TaskButtons",
              /*stack_size=*/3 * 1024,
              /*param=*/&ui_task_data,
              /*priority=*/next_priority++,
              /*handle=*/nullptr);

  Serial.print("setup(): core: ");
  Serial.println(xPortGetCoreID());
  Serial.println("Setup done");
}

// Don't do anything in loop() so Arduino can do its stuff without delay.
unsigned long last_print_time_ms = 0;
void loop() {
  delay(10);  // delay so idle task can feed watchdog
  if ((millis() - last_print_time_ms) < 1 * 60 * 1000 && last_print_time_ms) {
    return;
  }
  last_print_time_ms = millis();

  ESP_LOGI(TAG, "loop(): uptime: %s core: %d stackHighWater: %d",
           dump::MillisHumanReadable(millis()).c_str(), xPortGetCoreID(),
           uxTaskGetStackHighWaterMark(nullptr));
  for (int i = 0; (1 << i) <= MALLOC_CAP_DEFAULT; ++i) {
    ESP_LOGI(TAG, "loop(): cap_bit: %d minimum_free_size: %d", i,
             heap_caps_get_minimum_free_size(1 << i));
  }

  ESP_LOGI(TAG, "NTP sync status: %d", sntp_get_sync_status());
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "Time is: %s", strftime_buf);

#if 0
  // Reset once per hour cause we suck
  if (millis() > 60 * 60 * 1000) {
    ESP_LOGW(TAG, "Resetting just cuz");
    delay(1000);
    esp_restart();
  }
#endif
}
