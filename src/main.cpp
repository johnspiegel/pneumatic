/**
 */
#include <Adafruit_BME280.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <FreeRTOS.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <Wire.h>

#include "bme280.h"
#include "dsco220.h"
#include "html.h"
#include "mhz19.h"
#include "net_manager.h"
#include "pmsx003.h"
#include "sensor_community.h"
#include "ui.h"

#ifndef LED_BUILTIN
// Arduino Nano 33 IOT
// #define LED_BUILTIN 13
// ESP32 Dev Kit
#define LED_BUILTIN 2
// Heltec wifi kit 32
// #define LED_BUILTIN 25
#endif

#define WS2812B_PIN 25
#define PMSX003_RX_PIN 16
#define PMSX003_TX_PIN 17
#define MHZ19_RX_PIN 33
#define MHZ19_TX_PIN 32

SemaphoreHandle_t i2c_mutex = nullptr;

HardwareSerial mhz19_serial(1);
HardwareSerial pms_serial(2);
Adafruit_BME280 bme;

pmsx003::PmsData pmsx003_data = {0};
mhz19::Data mhz19_data = {0};

dsco220::Data dsco220_data = {0};
dsco220::TaskData dsco220_task_data = {0};

ui::TaskData ui_task_data = {0};

Adafruit_NeoPixel pixels(/*num_pixels=*/1, /*pin=*/ WS2812B_PIN,  NEO_GRB + NEO_KHZ800);

void BlinkIt(void* unused) {
    for (;;) {
        // turn the LED on (HIGH is the voltage level)
        digitalWrite(LED_BUILTIN, HIGH);
        // wait for a second
        delay(2000);
        // turn the LED off by making the voltage LOW
        digitalWrite(LED_BUILTIN, LOW);
        // wait for a second
        delay(100);
    }

    vTaskDelete(NULL);
}

void PollPmsx003(void* unused) {
    unsigned long last_print_time_ms = 0;
    for (;;) {
        if (!pmsx003::Read(&pms_serial, &pmsx003_data, /*timeout_ms=*/ 5000)) {
            Serial.print("ERROR: failed to read PMSX003 sensor");
            delay(1000);
            continue;
        }

        if ((millis() - last_print_time_ms) < 10 * 60 * 1000 && last_print_time_ms) {
            continue;
        }
        last_print_time_ms = millis();

        Serial.print("PollPmsx003(): core: ");
        Serial.println(xPortGetCoreID());

        Serial.print("PMSx003 data:");
        Serial.print("  [ug/m^3] PM1.0: ");
        Serial.print(pmsx003_data.pm1);
        Serial.print("  PM2.5: ");
        Serial.print(pmsx003_data.pm25);
        Serial.print("  PM10: ");
        Serial.print(pmsx003_data.pm10);
        Serial.println();
    }

    vTaskDelete(NULL);
}

void PollMhz19(void* unused) {
    unsigned long last_print_time_ms = 0;
    for (;;) {
        if (!mhz19::Read(&mhz19_serial, &mhz19_data, /*timeout_ms=*/ 5000)) {
            Serial.print("ERROR: failed to read MHZ-19c sensor");
            delay(1000);
            continue;
        }

        if ((millis() - last_print_time_ms) < 10 * 60 * 1000 && last_print_time_ms) {
            delay(2000);
            continue;
        }
        last_print_time_ms = millis();


        Serial.print("PollMhz19(): core: ");
        Serial.println(xPortGetCoreID());
        Serial.print("MH-Z19c Results: ");
        /*
        long now_ms = millis();
        if (last_read_time_ms != -1) {
            Serial.print("  [Elapsed: ");
            Serial.print(now_ms - last_read_time_ms);
            Serial.print(" ms]\n");
        }
        last_read_time_ms = now_ms;
        */
        Serial.print("  CO2: ");
        Serial.print(mhz19_data.co2_ppm);
        Serial.print("ppm");
        Serial.print("  Temp: ");
        Serial.print(mhz19_data.temp_c);
        Serial.print(" °C ");
        Serial.print(ui::CToF(mhz19_data.temp_c));
        Serial.print(" °F\n");
        delay(2000);
    }

    vTaskDelete(NULL);
}

bme280::Data bme_data = {0};

void PollBme(void* unused) {
    unsigned long last_print_time_ms = 0;
    for (;;) {
        if (xSemaphoreTake(i2c_mutex, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            bme_data.temp_c = bme.readTemperature();
            bme_data.pressurePa = bme.readPressure();
            bme_data.humidityPercent = bme.readHumidity();
            xSemaphoreGive(i2c_mutex);
        } else {
            Serial.print("ERROR: BME280 failed to acquire i2c mutex\n");
            continue;
        }

        if ((millis() - last_print_time_ms) < 10 * 60 * 1000 && last_print_time_ms) {
            delay(1000);
            continue;
        }
        last_print_time_ms = millis();

        Serial.print("PollBme(): core: ");
        Serial.println(xPortGetCoreID());
        Serial.print("BME280");
        Serial.print("  Temp: ");
        Serial.print(bme_data.temp_c);
        Serial.print(" °C ");
        Serial.print(ui::CToF(bme_data.temp_c));
        Serial.print(" °F");
        Serial.print("  Pressure: ");
        Serial.print(bme_data.pressurePa / 100.0);
        Serial.print(" hPa");
        Serial.print("  Humidity: ");
        Serial.print(bme_data.humidityPercent);
        Serial.print("%\n");

        delay(1000);
    }

    vTaskDelete(NULL);
}

void i2cScan() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknown error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
}


void setup()
{
    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    while(!Serial)
        ;
    Serial.println("Serial online");

    Serial.print("setup(): core: ");
    Serial.println(xPortGetCoreID());

    // This chipid is the mac address, but reversed? I'll just use the actual mac address.
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
        snprintf(mac + i*2, 3, "%02x", reinterpret_cast<uint8_t*>(&chipid_num)[i]);
    }
    Serial.print("setup(): e_fuse_mac: ");
    Serial.println(mac);

    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(0, 0 , 0));
    pixels.show();

    // Serial.println("Setting up WiFi...");
    // net_manager::Setup();
    // net_manager::Connect(/*timeout_ms=*/ 60000);

    Serial.println("Setting up PMSx003 Serial port...");
    pms_serial.begin(9600, SERIAL_8N1, /*rx=*/ PMSX003_RX_PIN, /*tx=*/ PMSX003_TX_PIN);
    while(!pms_serial) {
        Serial.println("    ...");
        delay(100);
    }
    Serial.println("PMSx003 Serial online");

    Serial.println("Setting up MH-Z19 Serial port...");
    mhz19_serial.begin(9600, SERIAL_8N1, /*rx=*/ MHZ19_RX_PIN, /*tx=*/ MHZ19_TX_PIN);
    while(!mhz19_serial) {
        Serial.println("    ...");
        delay(100);
    }
    Serial.println("MH-Z19 Serial online");
    mhz19::SetAutoBackgroundCalibration(&mhz19_serial, /*abc_on=*/ true);

    Serial.println("Setting up Plantower DS CO2..");
    Wire.begin();
    i2cScan();
    i2c_mutex = xSemaphoreCreateMutex();
    dsco220_task_data.i2c_mutex = i2c_mutex;
    dsco220_task_data.i2c = &Wire;
    dsco220_task_data.data = &dsco220_data;
#if 0
    Serial.println("Setting up DS-CO2-20 Serial port...");
    mhz19_serial.begin(9600, SERIAL_8N1, /*rx=*/21, /*tx=*/22);
    while(!mhz19_serial)
        Serial.println("    ...");
        delay(100);
        ;
    Serial.println("DS-CO2-20 Serial online");
#endif

    if (!bme.begin(0x76)) {
        Serial.print("ERROR: no BME280 found\n");
    } else {
        Serial.println("BME280 initialized.");
    }
    bme.getTemperatureSensor()->printSensorDetails();
    bme.getPressureSensor()->printSensorDetails();
    bme.getHumiditySensor()->printSensorDetails();

    // Apparently ESP32 FreeRTOS can't elegantly handle different tasks at the
    // same priority without the possibility of starvation. 
    int next_priority = 2;
#if 0
    xTaskCreate(
        BlinkIt,
        "BlinkIt",
        /*stack_size=*/ 10000,
        /*param=*/ nullptr,
        /*priority=*/ next_priority++,
        /*handle=*/ nullptr);
#endif
    xTaskCreate(
        PollPmsx003,
        "PollPmsx003",
        /*stack_size=*/ 10000,
        /*param=*/ nullptr,
        /*priority=*/ next_priority++,
        /*handle=*/ nullptr);
    xTaskCreate(
        PollMhz19,
        "PollMhz19",
        /*stack_size=*/ 10000,
        /*param=*/ nullptr,
        /*priority=*/ next_priority++,
        /*handle=*/ nullptr);
    xTaskCreate(
        dsco220::TaskPollDsCo2,
        "TaskPollDsCo2",
        /*stack_size=*/ 10000,
        /*param=*/ &dsco220_task_data,
        /*priority=*/ next_priority++,
        /*handle=*/ nullptr);
    xTaskCreate(
        PollBme,
        "pollBme",
        /*stack_size=*/ 10000,
        /*param=*/ nullptr,
        /*priority=*/ next_priority++,
        /*handle=*/ nullptr);

    ui_task_data.pmsx003_data = &pmsx003_data;
    ui_task_data.mhz19_data = &mhz19_data;
    ui_task_data.dsco220_data = &dsco220_data;
    ui_task_data.bme_data = &bme_data;
    ui_task_data.pixels = &pixels;
    xTaskCreate(
        ui::TaskDoPixels,
        "TaskDoPixels",
        /*stack_size=*/ 1024,
        /*param=*/ &ui_task_data,
        /*priority=*/ next_priority++,
        /*handle=*/ nullptr);
    xTaskCreate(
        sensor_community::TaskSensorCommunity,
        "TaskSensorCommunity",
        /*stack_size=*/ 4*1024,
        /*param=*/ &ui_task_data,
        /*priority=*/ next_priority++,
        /*handle=*/ nullptr);
    xTaskCreate(
        ui::TaskServeWeb,
        "TaskServeWeb",
        /*stack_size=*/ 16*1024,
        /*param=*/ &ui_task_data,
        /*priority=*/ next_priority++,
        /*handle=*/ nullptr);
    xTaskCreate(
        net_manager::DoTask,
        "NetManager",
        /*stack_size=*/ 4*1024,
        /*param=*/ nullptr,
        /*priority=*/ next_priority++,
        /*handle=*/ nullptr);

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

    Serial.println("----------------------------------------");
    Serial.print("loop(): Uptime: ");
    Serial.print(ui::MillisHumanReadable(millis()));
    Serial.print("  core: ");
    Serial.println(xPortGetCoreID());
}
