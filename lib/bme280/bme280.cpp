#include "bme280.h"

#include <dump.h>
#include <esp_log.h>

namespace bme280 {
namespace {

using dump::Ewma;

const char TAG[] = "bme280";
const int kPollPeriodMs = 1000;

}  // namespace

void TaskPoll(void* task_data_param) {
  auto* data = reinterpret_cast<Data*>(task_data_param);

  unsigned long last_print_time_ms = 0;
  unsigned long last_poll_time_ms = 0;
  for (;;) {
    unsigned long now_ms = millis();
    if (last_poll_time_ms && now_ms - last_poll_time_ms < kPollPeriodMs) {
      // TODO: use vTaskDelayUntil()
      delay(kPollPeriodMs - (now_ms - last_poll_time_ms));
      last_poll_time_ms += kPollPeriodMs;
    } else {
      last_poll_time_ms = now_ms;
    }

    if (xSemaphoreTake(data->i2c_mutex, kPollPeriodMs / portTICK_PERIOD_MS) ==
        pdTRUE) {
      float temp_c = data->bme280->readTemperature();
      if (temp_c > -60 && temp_c < 75) {
        data->temp_c = Ewma(temp_c, data->temp_c, 10000 / kPollPeriodMs);
      } else {
        ESP_LOGW(TAG, "Outlier temperature: %.2f", temp_c);
      }
      float pressure_pa = data->bme280->readPressure();
      if (pressure_pa > 33000 && pressure_pa < 110000) {
        data->pressure_pa =
            Ewma(pressure_pa, data->pressure_pa, 10000 / kPollPeriodMs);
      } else {
        ESP_LOGW(TAG, "Outlier pressure: %.2f", pressure_pa);
      }
      float humidity_pct = data->bme280->readHumidity();
      if (humidity_pct < 99.99) {
        data->humidity_pct =
            Ewma(humidity_pct, data->humidity_pct, 10000 / kPollPeriodMs);
      } else {
        ESP_LOGW(TAG, "Outlier humidity: %.2f", humidity_pct);
      }
      xSemaphoreGive(data->i2c_mutex);
    } else {
      Serial.print("ERROR: BME280 failed to acquire i2c mutex\n");
      continue;
    }

    if ((millis() - last_print_time_ms) < 10 * 60 * 1000 &&
        last_print_time_ms) {
      continue;
    }
    last_print_time_ms = millis();

    Serial.print("PollBme(): core: ");
    Serial.println(xPortGetCoreID());
    Serial.print("BME280");
    Serial.print("  Temp: ");
    Serial.print(data->temp_c);
    Serial.print(" °C ");
    Serial.print(dump::CToF(data->temp_c));
    Serial.print(" °F");
    Serial.print("  Pressure: ");
    Serial.print(data->pressure_pa / 100.0);
    Serial.print(" hPa");
    Serial.print("  Humidity: ");
    Serial.print(data->humidity_pct);
    Serial.print("%\n");
  }

  vTaskDelete(NULL);
}

}  // namespace bme280
