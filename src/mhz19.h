#ifndef MHZ19_H
#define MHZ19_H

/**
 */
#include "Stream.h"

namespace mhz19 {

struct TaskData {
    Stream*     serial;

    uint16_t    co2_ppm;
    int8_t      temp_c;
};

uint8_t Checksum(uint8_t *buf, int size);

bool SetAutoBackgroundCalibration(Stream* serial, bool abc_on, unsigned long timeout_ms = 5000);

bool Read(Stream* serial, TaskData* data, unsigned long timeout_ms = 5000);

void TaskPoll(void* task_data_param);

} // namespace mhz19

#endif  // MHZ19_H
