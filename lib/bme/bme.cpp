#include "bme.h"

#include <dump.h>
#include <esp_log.h>

#include <vector>

namespace bme {
namespace {

using dump::Ewma;

const char TAG[] = "bme";
const int kPollPeriodMs = 1000;

bool CheckBsecStatus(const Bsec& bsec) {
  if (bsec.status != BSEC_OK) {
    if (bsec.status < BSEC_OK) {
      ESP_LOGE(TAG, "BSEC error code: %d", bsec.status);
      return false;
    } else {
      ESP_LOGW(TAG, "BSEC warning code: %d", bsec.status);
    }
  }

  if (bsec.bme680Status != BME680_OK) {
    if (bsec.bme680Status < BME680_OK) {
      ESP_LOGE(TAG, "BME680 error code: %d", bsec.bme680Status);
      return false;
    } else {
      ESP_LOGW(TAG, "BME680 warning code: %d", bsec.bme680Status);
    }
  }

  return true;
}

void UpdateData(Data* data, float temp_c, float pressure_pa,
                float humidity_pct) {
  if (temp_c > -60 && temp_c < 75) {
    data->temp_c = Ewma(temp_c, data->temp_c, 10000 / kPollPeriodMs);
  } else {
    ESP_LOGW(TAG, "Outlier temperature: %.2f", temp_c);
  }

  if (pressure_pa > 33000 && pressure_pa < 110000) {
    data->pressure_pa =
        Ewma(pressure_pa, data->pressure_pa, 10000 / kPollPeriodMs);
  } else {
    ESP_LOGW(TAG, "Outlier pressure: %.2f", pressure_pa);
  }

  if (humidity_pct < 99.99) {
    data->humidity_pct =
        Ewma(humidity_pct, data->humidity_pct, 10000 / kPollPeriodMs);
  } else {
    ESP_LOGW(TAG, "Outlier humidity: %.2f", humidity_pct);
  }
}

bool PollBme280(Data* data) {
    if (xSemaphoreTake(data->i2c_mutex, kPollPeriodMs / portTICK_PERIOD_MS) !=
        pdTRUE) {
      ESP_LOGE(TAG, "PollBme280: failed to acquire i2c mutex");
      return false;
    }

    UpdateData(data, /* temp_c= */ data->bme280->readTemperature(),
               /* pressure_pa= */ data->bme280->readPressure(),
               /* humdity_pct= */ data->bme280->readHumidity());

    xSemaphoreGive(data->i2c_mutex);
    return true;
}

bool PollBsec(Data* data) {
    if (xSemaphoreTake(data->i2c_mutex, kPollPeriodMs / portTICK_PERIOD_MS) !=
        pdTRUE) {
      ESP_LOGE(TAG, "PollBsec: failed to acquire i2c mutex");
      return false;
    }

    if (data->bsec->run()) {  // If new data is available
      // TODO: update EWMA time period to match BSEC polling
      UpdateData(data, /* temp_c= */ data->bsec->temperature,
                 /* pressure_pa= */ data->bsec->pressure,
                 /* humdity_pct= */ data->bsec->humidity);

      /*
      output += ", " + String(bsec.rawTemperature);
      output += ", " + String(bsec.pressure);
      output += ", " + String(bsec.rawHumidity);
      output += ", " + String(bsec.gasResistance);
      output += ", " + String(bsec.iaq);
      output += ", " + String(bsec.iaqAccuracy);
      output += ", " + String(bsec.temperature);
      output += ", " + String(bsec.humidity);
      output += ", " + String(bsec.staticIaq);
      output += ", " + String(bsec.co2Equivalent);
      output += ", " + String(bsec.breathVocEquivalent);
      Serial.println(output);
      */
    } else {
      CheckBsecStatus(*data->bsec);
    }

    xSemaphoreGive(data->i2c_mutex);
    return true;
}

}  // namespace

bool Init(Data* data) {
  if (xSemaphoreTake(data->i2c_mutex, kPollPeriodMs / portTICK_PERIOD_MS) !=
      pdTRUE) {
    ESP_LOGE(TAG, "Init: failed to acquire i2c mutex");
    return false;
  }

  data->sensor_name = "BME not found";

  // Try initializing BME280
  {
    std::unique_ptr<Adafruit_BME280> bme280(new Adafruit_BME280());
    if (bme280->begin(BME280_I2C_ADDRESS)) {
      data->bme280 = std::move(bme280);
      data->sensor_name = "BME280";

      ESP_LOGI(TAG, "BME280 initialized at i2c: 0x%02x", BME280_I2C_ADDRESS);
      data->bme280->getTemperatureSensor()->printSensorDetails();
      data->bme280->getPressureSensor()->printSensorDetails();
      data->bme280->getHumiditySensor()->printSensorDetails();

      xSemaphoreGive(data->i2c_mutex);
      return true;
    }
    ESP_LOGW(TAG, "BME280 not found at i2c: 0x%02x", BME280_I2C_ADDRESS);
  }

  // Try initializing BME68x
  {
    std::unique_ptr<Bsec> bsec(new Bsec());
    bsec->begin(BME680_I2C_ADDR_PRIMARY, Wire);
    if (CheckBsecStatus(*bsec)) {
      data->bsec = std::move(bsec);
      data->sensor_name = "BME68x";
      ESP_LOGI(TAG,
               "BME680 initialized at i2c: 0x%02x, BSEC library version: "
               "%d.%d.%d.%d",
               BME680_I2C_ADDR_PRIMARY,                               //
               data->bsec->version.major, data->bsec->version.minor,  //
               data->bsec->version.major_bugfix,
               data->bsec->version.minor_bugfix);

      std::vector<bsec_virtual_sensor_t> sensors = {
          BSEC_OUTPUT_RAW_TEMPERATURE,
          BSEC_OUTPUT_RAW_PRESSURE,
          BSEC_OUTPUT_RAW_HUMIDITY,
          BSEC_OUTPUT_RAW_GAS,
          BSEC_OUTPUT_IAQ,
          BSEC_OUTPUT_STATIC_IAQ,
          BSEC_OUTPUT_CO2_EQUIVALENT,
          BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
          BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
          BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
      };
      data->bsec->updateSubscription(&sensors[0], sensors.size(),
                                     BSEC_SAMPLE_RATE_LP);
      if (!CheckBsecStatus(*data->bsec)) {
        ESP_LOGE(TAG, "failed to update BME680 sensor subscription: %0d",
                 data->bsec->status);
      }

      xSemaphoreGive(data->i2c_mutex);
      return true;
    }
    ESP_LOGW(TAG, "BME680 not found at i2c: 0x%02x", BME680_I2C_ADDR_PRIMARY);
  }

  xSemaphoreGive(data->i2c_mutex);
  return false;
}

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

    if (data->bme280 != nullptr) {
      PollBme280(data);
    }
    if (data->bsec != nullptr) {
      PollBsec(data);
    }

    if ((millis() - last_print_time_ms) < 10 * 60 * 1000 &&
        last_print_time_ms) {
      continue;
    }
    last_print_time_ms = millis();

    ESP_LOGI(TAG,
             "bme::TaskPoll(): core: %d sensor: %s "
             "Temp: %.1f °C %.1f °F Pressure: %.3f hPa Humidity: %.1f",
             xPortGetCoreID(), data->sensor_name,  //
             data->temp_c, dump::CToF(data->temp_c), data->pressure_pa / 100.0,
             data->humidity_pct);
  }

  vTaskDelete(NULL);
}

}  // namespace bme
