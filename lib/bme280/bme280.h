#ifndef _BME280_H_
#define _BME280_H_

#include <Adafruit_BME280.h>

namespace bme280 {

struct Data {
  Adafruit_BME280* bme280;
  SemaphoreHandle_t i2c_mutex;

  float temp_c;
  float pressure_pa;
  float humidity_pct;
};

void TaskPoll(void* task_data_param);

}  // namespace bme280

#endif  // _BME280_H_
