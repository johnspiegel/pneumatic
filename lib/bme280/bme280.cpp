#include "bme280.h"

#include <dump.h>

namespace bme280 {

void TaskPoll(void* task_data_param) {
  auto* data = reinterpret_cast<Data*>(task_data_param);

  unsigned long last_print_time_ms = 0;
  for (;;) {
    if (xSemaphoreTake(data->i2c_mutex, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
      data->temp_c = data->bme280->readTemperature();
      data->pressurePa = data->bme280->readPressure();
      data->humidityPercent = data->bme280->readHumidity();
      xSemaphoreGive(data->i2c_mutex);
    } else {
      Serial.print("ERROR: BME280 failed to acquire i2c mutex\n");
      continue;
    }

    if ((millis() - last_print_time_ms) < 10 * 60 * 1000 &&
        last_print_time_ms) {
      delay(1000);
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
    Serial.print(data->pressurePa / 100.0);
    Serial.print(" hPa");
    Serial.print("  Humidity: ");
    Serial.print(data->humidityPercent);
    Serial.print("%\n");

    delay(1000);
  }

  vTaskDelete(NULL);
}

}  // namespace bme280
