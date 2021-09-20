#ifndef _BME_H_
#define _BME_H_

// TODO: rename this BMEx80

#include <memory>

#include <Adafruit_BME280.h>
#include <bsec.h>

#define BME280_I2C_ADDRESS 0x76

namespace bme {

struct Data {
  SemaphoreHandle_t i2c_mutex;

  std::unique_ptr<Adafruit_BME280> bme280;
  std::unique_ptr<Bsec> bsec;

  const char* sensor_name;
  float temp_c;
  float pressure_pa;
  float humidity_pct;
};

bool Init(Data* data);

void TaskPoll(void* task_data_param);

}  // namespace bme

#endif  // _BME_H_
