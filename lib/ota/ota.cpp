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

    esp_http_client_config_t http_config{
        .url = kReleasesUrl,
        .cert_pem = kCaPem,
    };
    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "http error = %s", esp_err_to_name(err));
      esp_http_client_cleanup(client);
      continue;
    }

    int content_length = esp_http_client_get_content_length(client);
    std::vector<char> buf(content_length + 1, '\0');
    int offset = 0;
    while (offset < content_length) {
      int bytes_read = esp_http_client_read(client, &buf[offset], buf.size() - offset);
      ESP_LOGI(TAG, "content_length = %d offset = %d bytes_read = %d",
               content_length, offset, bytes_read);
      offset += bytes_read;
    }
    ESP_LOGI(TAG, "content_length = %d, total bytes_read = %d, content =\n%s",
             content_length, offset, &buf.front());
    esp_http_client_cleanup(client);

    StaticJsonDocument<128> json_filter;
    json_filter["releases"][kDeviceConfig] = true;
    StaticJsonDocument<256> json_doc;
    auto json_err = deserializeJson(json_doc, &buf.front(), DeserializationOption::Filter(json_filter));
    if (json_err) {
      ESP_LOGE(TAG, "deserializeJson() failed: %s", json_err.c_str());
      continue;
    }

    const char* url = json_doc["releases"][kDeviceConfig]["url"];
    if (!url) {
      ESP_LOGE(TAG, "no url found!");
      continue;
    }
    ESP_LOGI(TAG, "Firmware url: %s", url);
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
