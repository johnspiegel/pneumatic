#include "ota.h"

#include <WiFi.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_ota_ops.h>

#include <vector>

#include "dump.h"

namespace ota {
namespace {

const char TAG[] = "ota";
const char kPneumaticVersion[] = "0.0.1";
const char kReleasesUrl[] = "https://dev.jspiegel.net/pneumatic/releases.json";
const char kDeviceConfig[] = "tdisplay";

const char kCaPem[] = R"(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----

-----BEGIN CERTIFICATE-----
MIIFFjCCAv6gAwIBAgIRAJErCErPDBinU/bWLiWnX1owDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjAwOTA0MDAwMDAw
WhcNMjUwOTE1MTYwMDAwWjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDELMAkGA1UEAxMCUjMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQC7AhUozPaglNMPEuyNVZLD+ILxmaZ6QoinXSaqtSu5xUyxr45r+XXIo9cP
R5QUVTVXjJ6oojkZ9YI8QqlObvU7wy7bjcCwXPNZOOftz2nwWgsbvsCUJCWH+jdx
sxPnHKzhm+/b5DtFUkWWqcFTzjTIUu61ru2P3mBw4qVUq7ZtDpelQDRrK9O8Zutm
NHz6a4uPVymZ+DAXXbpyb/uBxa3Shlg9F8fnCbvxK/eG3MHacV3URuPMrSXBiLxg
Z3Vms/EY96Jc5lP/Ooi2R6X/ExjqmAl3P51T+c8B5fWmcBcUr2Ok/5mzk53cU6cG
/kiFHaFpriV1uxPMUgP17VGhi9sVAgMBAAGjggEIMIIBBDAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0lBBYwFAYIKwYBBQUHAwIGCCsGAQUFBwMBMBIGA1UdEwEB/wQIMAYB
Af8CAQAwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYfr52LFMLGMB8GA1UdIwQYMBaA
FHm0WeZ7tuXkAXOACIjIGlj26ZtuMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcw
AoYWaHR0cDovL3gxLmkubGVuY3Iub3JnLzAnBgNVHR8EIDAeMBygGqAYhhZodHRw
Oi8veDEuYy5sZW5jci5vcmcvMCIGA1UdIAQbMBkwCAYGZ4EMAQIBMA0GCysGAQQB
gt8TAQEBMA0GCSqGSIb3DQEBCwUAA4ICAQCFyk5HPqP3hUSFvNVneLKYY611TR6W
PTNlclQtgaDqw+34IL9fzLdwALduO/ZelN7kIJ+m74uyA+eitRY8kc607TkC53wl
ikfmZW4/RvTZ8M6UK+5UzhK8jCdLuMGYL6KvzXGRSgi3yLgjewQtCPkIVz6D2QQz
CkcheAmCJ8MqyJu5zlzyZMjAvnnAT45tRAxekrsu94sQ4egdRCnbWSDtY7kh+BIm
lJNXoB1lBMEKIq4QDUOXoRgffuDghje1WrG9ML+Hbisq/yFOGwXD9RiX8F6sw6W4
avAuvDszue5L3sz85K+EC4Y/wFVDNvZo4TYXao6Z0f+lQKc0t8DQYzk1OXVu8rp2
yJMC6alLbBfODALZvYH7n7do1AZls4I9d1P4jnkDrQoxB3UqQ9hVl3LEKQ73xF1O
yK5GhDDX8oVfGKF5u+decIsH4YaTw7mP3GFxJSqv3+0lUFJoi5Lc5da149p90Ids
hCExroL1+7mryIkXPeFM5TgO9r0rvZaBFOvV2z0gp35Z0+L4WPlbuEjN/lxPFin+
HlUjr8gRsI3qfJOQFy/9rKIJR0Y/8Omwt/8oTWgy1mdeHmmjk7j1nYsvC9JSQ6Zv
MldlTTKB3zhThV1+XWYp6rjd5JW1zbVWEkLNxE7GJThEUG3szgBVGP7pSWTUTsqX
nLRbwHOoq7hHwg==
-----END CERTIFICATE-----
)";

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
