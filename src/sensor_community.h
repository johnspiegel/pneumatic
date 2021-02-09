#ifndef _SENSOR_COMMUNITY_H_
#define _SENSOR_COMMUNITY_H_

#include "ui.h"

namespace sensor_community {

bool Push(const ui::TaskData* data);

void TaskSensorCommunity(void* data);

}  // namespace sensor_community

#endif  // _SENSOR_COMMUNITY_H_
