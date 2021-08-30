#ifndef _OTA_H_
#define _OTA_H_

#include <ArduinoJson.h>

namespace ota {

DynamicJsonDocument FetchReleases();

void TaskOta(void* unused);

}  // namespace ota

#endif  // _OTA_H_
