#ifndef PMSX003_H
#define PMSX003_H

/**
 */
#include "Stream.h"

namespace pmsx003 {

struct TaskData {
  Stream* serial;

  uint16_t pm1Raw;
  uint16_t pm25Raw;
  uint16_t pm10Raw;

  float pm_1_0;
  float pm_2_5;
  float pm_10_0;

  float particles_gt_0_3;
  float particles_gt_0_5;
  float particles_gt_1_0;
  float particles_gt_2_5;
  float particles_gt_5_0;
  float particles_gt_10_0;
};

bool VerifyPacket(uint8_t* packet, int size);

bool Read(Stream* serial, TaskData* data, unsigned long timeout_ms = 5000);

void TaskPoll(void* task_data);

}  // namespace pmsx003

#endif  // PMSX003_H
