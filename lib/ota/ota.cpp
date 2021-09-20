#include "ota.h"

#include <WiFi.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_ota_ops.h>

#include <vector>

#include "constants.h"
#include "dump.h"

namespace ota {
namespace {
const char TAG[] = "ota";

std::vector<char> http_buf;
int http_buf_offset = 0;

esp_err_t HandleHttpEvent(esp_http_client_event_t* evt) {
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
      printf("%.*s", evt->data_len, (char*)evt->data);
      break;
    case HTTP_EVENT_ON_DATA:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (esp_http_client_is_chunked_response(evt->client)) {
        ESP_LOGW(TAG, "chunked response!");
      }
      http_buf.resize(http_buf_offset + evt->data_len + 1);
      memcpy(&http_buf[http_buf_offset], evt->data, evt->data_len);
      http_buf_offset += evt->data_len;
      http_buf[http_buf_offset] = '\0';
      break;
    case HTTP_EVENT_ON_FINISH:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
};

}  // namespace

void TaskOta(void* unused) {
  ESP_LOGI(TAG, "TaskOta: Starting task...");

  unsigned long last_print_time_ms = 0;
  unsigned long next_update_time_ms = 0;
  int long_delay_ms = 1 * 60 * 60 * 1000; // 1 hour
  int delay_ms = 30 * 1000;
  bool first_since_boot = true;
  for (;; delay(delay_ms)) {
    if ((millis() - last_print_time_ms) > 60 * 1000 || !last_print_time_ms) {
      ESP_LOGI(
          TAG, "TaskOta(): uptime: %s next update in: %s core: %d",
          dump::MillisHumanReadable(millis()).c_str(),
          dump::MillisHumanReadable(next_update_time_ms - millis()).c_str(),
          xPortGetCoreID());
      if (!WiFi.isConnected()) {
        ESP_LOGW(TAG, "TaskOta: Waiting for WiFi...");
      }
      last_print_time_ms = millis();
    }
    while (!WiFi.isConnected() && last_print_time_ms &&
           ((millis() - last_print_time_ms) <= 60 * 1000)) {
      delay(1000);
    }
    if (!WiFi.isConnected()) {
      continue;
    }
    if (millis() < next_update_time_ms) {
      continue;
    }

    ESP_LOGI(TAG, "Fetching Releases...");
    bool do_update = true;

    http_buf.resize(0);
    http_buf_offset = 0;
    esp_http_client_config_t http_config{
        .url = kReleasesUrl,
        .cert_pem = kCaPem,
        // .event_handler = http_event_handler,
        .event_handler = HandleHttpEvent,
    };
    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    ESP_LOGE(TAG, "http buffer size: %d", http_config.buffer_size);
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "http error = %s", esp_err_to_name(err));
      esp_http_client_cleanup(client);
      continue;
    }
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "http_buf.size() = %d", http_buf.size());
    ESP_LOGI(TAG, "http_buf =\n%s", &http_buf.front());

    StaticJsonDocument<128> json_filter;
    json_filter["releases"][kDeviceConfig] = true;
    StaticJsonDocument<256> json_doc;
    auto json_err = deserializeJson(json_doc, &http_buf.front(), DeserializationOption::Filter(json_filter));
    if (json_err) {
      ESP_LOGE(TAG, "deserializeJson() failed: %s", json_err.c_str());
      continue;
    }

    const char* url = json_doc["releases"][kDeviceConfig]["url"];
    if (!url) {
      ESP_LOGE(TAG, "no url found for device: %s", kDeviceConfig);
      continue;
    }
    ESP_LOGI(TAG, "Firmware url: %s (kDeviceConfig: %s)", url, kDeviceConfig);
    bool update_at_boot = json_doc["releases"][kDeviceConfig]["update_at_boot"];
    ESP_LOGI(TAG, "update_at_boot: %s", update_at_boot ? "true" : "false");

    if (first_since_boot) {
      first_since_boot = false;
      if (update_at_boot) {
        ESP_LOGI(TAG, "update_at_boot=true; attempting OTA");
      } else if (!update_at_boot) {
        ESP_LOGI(TAG,
                 "first_since_boot==true && update_at_boot==false; "
                 "will not update (this time)");
        do_update = false;
      }
    }

    const char* version = json_doc["releases"][kDeviceConfig]["version"];
    if (!version) {
      ESP_LOGE(TAG, "no version found! (proceeding)");
    } else if (strcmp(version, kPneumaticVersion)) {
      ESP_LOGI(
          TAG,
          "proceeding with new/different version. running: %s available: %s",
          kPneumaticVersion, version);
    } else {
      ESP_LOGI(TAG, "Already up-to-date with version: %s == %s; will not update",
               version, kPneumaticVersion);
      do_update = false;
    }

    http_config.url = url;

    // TODO(jbs): check if it's the factory partition before marking?
    err = esp_ota_mark_app_valid_cancel_rollback();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "failed to mark running OTA app valid. err = %s",
               esp_err_to_name(err));
    } else {
      ESP_LOGI(TAG, "marked running OTA valid (will not rollback on boot)");
    }

    if (!do_update) {
      ESP_LOGI(TAG, "Not updating, will check again in: %s",
               dump::MillisHumanReadable(long_delay_ms).c_str());
      next_update_time_ms = millis() + long_delay_ms;
      continue;
    }

    ESP_LOGW(TAG, "Attempting to install OTA: %s", url);
    err = esp_https_ota(&http_config);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_http_ota() error = %s", esp_err_to_name(err));
      continue;
    }

    ESP_LOGW(TAG, "Successfully updated OTA. Restarting...");
    delay(100);
    esp_restart();
  }
}

}  // namespace ota
