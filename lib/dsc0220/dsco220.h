#ifndef _DSCO220_H_
#define _DSCO220_H_

#include <FreeRTOS.h>
#include <Stream.h>
#include <Wire.h>

namespace dsco220 {

struct Data {
  uint16_t co2_ppm;
  uint16_t calibration_param1;
  uint16_t calibration_param2;
};

struct TaskData {
  Data* data;
  TwoWire* i2c;
  SemaphoreHandle_t i2c_mutex;
};

bool Read(TwoWire* serial, Data* data);

bool Read(Stream* serial, Data* data);

void TaskPollDsCo2(void* task_data);

}  // namespace dsco220

#endif  // _DSCO220_H_
