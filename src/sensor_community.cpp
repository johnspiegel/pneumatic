#include <WiFiClient.h>
#include <HTTPClient.h>

#include "sensor_community.h"

namespace sensor_community {
namespace {
WiFiClient wifi_client;
HTTPClient http_client;
}  // namespace

bool Push(const ui::TaskData* data) {
    http_client.setReuse(false);
    http_client.setConnectTimeout(20000);
    http_client.setTimeout(20000);
    http_client.setUserAgent("TODO");
    http_client.addHeader("TODO", "value");
    http_client.begin(wifi_client, "api.sensor.community", 443, "/v1/push-sensor-data/", /*https=*/ true);
    return false;
}

void TaskSensorCommunity(void* task_param) {
    const ui::TaskData* ui_task_data = reinterpret_cast<ui::TaskData*>(task_param);

    for (;;) {
        Push(ui_task_data);
        delay(60000);
    }
    vTaskDelete(NULL);
}
}  // namespace sensor_community
