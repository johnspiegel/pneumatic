#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "sensor_community.h"

namespace sensor_community {
namespace {
WiFiClient wifi_client;
HTTPClient http_client;
}  // namespace

bool Push(const ui::TaskData* data) {
    const char kSoftwareVersion[] = "JBS-2021-001";
    String macAddress = WiFi.macAddress();
    macAddress.replace(":", "");
    macAddress.toLowerCase();

    Serial.print("sensor-id: esp32-");
    Serial.println(macAddress);

    http_client.setReuse(false);
    http_client.setConnectTimeout(20000);
    http_client.setTimeout(20000);
    http_client.setUserAgent(String(kSoftwareVersion) + '/' + macAddress);

    String payload = R"({
            "software_version": "{software_version}",
            "sensordatavalues": [
                { "value_type": "P0", "value": "{pm1_0}" },
                { "value_type": "P1", "value": "{pm2_5}" },
                { "value_type": "P2", "value": "{pm10_0}" }
            ]
        })";
    payload.replace("{software_version}", kSoftwareVersion);
    payload.replace("{pm1_0}", String(data->pmsx003_data->pm1));
    payload.replace("{pm2_5}", String(data->pmsx003_data->pm25));
    payload.replace("{pm10_0}", String(data->pmsx003_data->pm10));

    Serial.print("SensorCommunity: Payload:\n");
    Serial.print(payload);

    if (!http_client.begin(wifi_client, "api.sensor.community", 80,
                           "/v1/push-sensor-data/", /*https=*/ false)) {
        return false;
    }

    http_client.addHeader("Content-Type", "application/json");
    http_client.addHeader("X-Sensor", "esp32-" + macAddress);
    http_client.addHeader("X-MAC-ID", "esp32-" + macAddress);
    http_client.addHeader("X-PIN", "1");  // PMSX003
    int err = http_client.POST(payload);

    if (err >= HTTP_CODE_OK && err <= HTTP_CODE_ALREADY_REPORTED) {
        Serial.print("SensorCommunity: Data pushed: ");
        Serial.println(err);
        Serial.println(http_client.getString());
    } else {
        Serial.print("ERROR: SensorCommunity: http return: ");
        Serial.println(err);
        Serial.println(http_client.getString());
    }

    http_client.end();
    return true;
}

void TaskSensorCommunity(void* task_param) {
    const ui::TaskData* ui_task_data = reinterpret_cast<ui::TaskData*>(task_param);

    delay(1000);
    Serial.print("SensorCommunity: waiting for 1m...\n");
    delay(60000);

    for (;;) {
        Serial.print("TaskSensorCommunity(): core: ");
        Serial.println(xPortGetCoreID());
        Serial.print("SensorCommunity: Pushing data...\n");
        Push(ui_task_data);
        Serial.print("SensorCommunity: waiting for 1m...\n");
        delay(60000);
    }
    vTaskDelete(NULL);
}

}  // namespace sensor_community
