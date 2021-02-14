#include "net_manager.h"

#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <vector>

#include <ui.h>

namespace net_manager {

WiFiManager wifi_manager;

wifi_ap_record_t* ScanFor(const char* ssid, unsigned long timeout_ms) {
    Serial.println("WiFi: scan start");
    unsigned long start_time_ms = millis();

    WiFi.mode(WIFI_STA);
    // WiFi.scanNetworks will return the number of networks found
    int n = 0;
    do  {
        unsigned long scan_start_time_ms = millis();
        n = WiFi.scanNetworks();
        Serial.print("ScanFor(): scan done in ");
        Serial.print(millis() - scan_start_time_ms);
        Serial.print(" ms\n");

        if (n < 0) {
            Serial.print("ScanFor(): Scan failed: ");
            Serial.println(n);
        } else if (n == 0) {
            Serial.println("ScanFor(): no networks found");
        }
        delay(100);
    } while (n <= 0 && (millis() - start_time_ms) < timeout_ms);

    wifi_ap_record_t* ret = nullptr;
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.print((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
        Serial.print("  channel: ");
        Serial.print(WiFi.channel(i));
        Serial.print("  BSSID: ");
        Serial.print(WiFi.BSSIDstr(i));
        Serial.print("\n");

        if (ret == nullptr && !strcmp(ssid, WiFi.SSID(i).c_str())) {
            ret = reinterpret_cast<wifi_ap_record_t*>(WiFi.getScanInfoByIndex(i));
        }
    }
    Serial.println("");

	return ret;
}

enum Status {
    STATUS_UNKNOWN = 0,
    STATUS_DISCONNECTED,
    STATUS_CONNECTING,
    STATUS_CONNECTED,
};

Status status = STATUS_DISCONNECTED;

bool Init() {
    // Triggers low-level esp wifi init
    WiFi.mode(WIFI_STA);
    // Persist wifi config to flash (NVS)
    WiFi.persistent(true);
    WiFi.setSleep(false);

    // Set up some event handlers
    WiFi.onEvent(
        [](WiFiEvent_t event, WiFiEventInfo_t info) {
            Serial.print("WiFi connected. IP: ");
            Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
            status = STATUS_CONNECTED;
        },
        WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);

    WiFi.onEvent(
        [](WiFiEvent_t event, WiFiEventInfo_t info) {
            Serial.print("WiFi lost connection. Reason: ");
            Serial.println(info.disconnected.reason);
            Serial.print("WiFi not connected; status: ");
            Serial.println(WiFi.status());
            // WiFi.persistent(false);
            // WiFi.disconnect(true);
            status = STATUS_DISCONNECTED;
            // ESP.restart();
        },
        WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

    return true;
}

bool Connect(unsigned long timeout_ms) {
    wifi_config_t config = {0};
    char ssid[sizeof(config.sta.ssid) + 1] = {0};
    char password[sizeof(config.sta.password) + 1] = {0};
    int connect_failures = 0;
    for (;;) {
        if (status == STATUS_CONNECTED) {
            Serial.print("Wifi: connected!");
            break;
        }

        esp_wifi_get_config(WIFI_IF_STA, &config);
        // config.sta.ssid[0] = '\0';  // force captive portal
        if (connect_failures > 3 || config.sta.ssid[0] == '\0') {
            Serial.print("Wifi: no saved SSID, starting captive portal...");

            // wifi_manager.setConfigPortalTimeout(/*seconds=*/ 180);  // blocking
            // wifi_manager.setConnectRetries(3);  // blocking
            // wifi_manager.setSaveConnectTimeout(/*seconds=*/ 20);  // blocking
            // wifi_manager.setConnectTimeout(/*seconds=*/ 20);  // blocking
            auto s = wifi_manager.startConfigPortal();  // blocking
            Serial.print("WifiManager done, result: ");
            Serial.println(s);
            wifi_manager.stopConfigPortal();
            esp_wifi_get_config(WIFI_IF_STA, &config);
            if (config.sta.ssid[0] == '\0') {
                Serial.print("Wifi: WifiManager failed no ssid...");
            }

            memcpy(ssid, config.sta.ssid, sizeof(config.sta.ssid));
            memcpy(password, config.sta.password, sizeof(config.sta.password));
            Serial.print("Wifi: WiFiManager SSID: ");
            Serial.println(ssid);
            Serial.print("Wifi: WiFiManager password: ");
            Serial.println(password);
            continue;
        }

        memcpy(ssid, config.sta.ssid, sizeof(config.sta.ssid));
        memcpy(password, config.sta.password, sizeof(config.sta.password));
        Serial.print("Wifi: saved SSID: ");
        Serial.println(ssid);
        Serial.print("Wifi: saved password: ");
        Serial.println(password);

        unsigned long start_time_ms = millis();
        config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        esp_wifi_set_config(WIFI_IF_STA, &config);

#if 0
        // WiFi.disconnect(/*wifioff=*/ true);
        WiFi.disconnect();
        status = STATUS_DISCONNECTED;
        auto* ap_info = ScanFor(ssid, timeout_ms);
        if (!ap_info) {
            Serial.print("WiFi: Failed to find SSID: ");
            Serial.println(ssid);
            // ++connect_failures;
            // continue;
        } else {
            Serial.print(" channel: ");
            Serial.print(ap_info->primary);
            Serial.print(" BSSID: ");
            for (int i = 0; i < sizeof(ap_info->bssid); ++i) {
                if (i > 0) {
                    Serial.print(":");
                }
                Serial.print(ap_info->bssid[i] >> 4, HEX);
                Serial.print(ap_info->bssid[i] & 0x0f, HEX);
            }
            Serial.print(" RSSI: ");
            Serial.print(ap_info->rssi);
            Serial.print("\n");

            /*
            config.sta.bssid_set = true;
            memcpy(config.sta.bssid, ap_info->bssid, 6);
            config.sta.channel = ap_info->primary;
            */
            config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
            config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
            config.sta.bssid_set = false;
            memset(config.sta.bssid, 0, 6);
            config.sta.channel = 0;
            esp_wifi_set_config(WIFI_IF_STA, &config);
        }
#endif

        Serial.print("WiFi: Connecting to ");
        Serial.println(ssid);
        // WiFi.disconnect(/*wifioff=*/ true);
        // WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        // WiFi.setHostname("esp32dev");

        Serial.print("WiFi: status: ");
        Serial.println(status);
        status = STATUS_CONNECTING;
        if (WiFi.begin() == WL_CONNECT_FAILED) {
            auto err = esp_wifi_connect();
            Serial.print("esp_wifi_connect: ");
            Serial.println(err);
            status = STATUS_DISCONNECTED;
        }
        unsigned long backoff_ms = 10;
        while (status != STATUS_CONNECTED && (millis() - start_time_ms) < timeout_ms) {
            Serial.print("WiFi connecting... Status: ");
            Serial.println(status);
            if (status == STATUS_DISCONNECTED) {
                // WiFi.disconnect(/*wifioff=*/ false, /*eraseap=*/ false);
                // WiFi.disconnect(/*wifioff=*/ true);
                Serial.print("WiFi: sleep for ");
                Serial.println(ui::MillisHumanReadable(backoff_ms));
                delay(backoff_ms);
                backoff_ms *= 2;
                status = STATUS_CONNECTING;
                if (WiFi.begin() == WL_CONNECT_FAILED) {
                    auto err = esp_wifi_connect();
                    Serial.print("esp_wifi_connect: ");
                    Serial.println(err);
                    status = STATUS_DISCONNECTED;
                }
            }
            delay(1000);
        }

        if (status != STATUS_CONNECTED) {
            Serial.print("WiFi failed to connect; status: ");
            Serial.println(status);
            Serial.print("; elapsed: ");
            Serial.print((millis() - start_time_ms));
            Serial.print(" ms\n");
            // WiFi.printDiag(Serial);
            // return false;
            ++connect_failures;
            continue;
        }

        // Connected!
        connect_failures = 0;
        Serial.print("WiFi connected in ");
        Serial.print((millis() - start_time_ms));
        Serial.print(" ms\n");
        WiFi.printDiag(Serial);

        Serial.print("My MAC ADDRESS:  ");
        Serial.println(WiFi.macAddress());
        Serial.print("My IP:           ");
        Serial.println(WiFi.localIP().toString());
        Serial.print("My hostname:     ");
        Serial.println(WiFi.getHostname());
        Serial.print("SSID:            ");
        Serial.println(WiFi.SSID());
        Serial.print("BSSID:           ");
        Serial.println(WiFi.BSSIDstr());
        Serial.print("Channel:         ");
        Serial.println(WiFi.channel());
        Serial.print("RSSI:            ");
        Serial.println(WiFi.RSSI());
        Serial.print("TxPower:         ");
        Serial.println(WiFi.getTxPower());
        Serial.print("Gateway:         ");
        Serial.println(WiFi.gatewayIP().toString());
        Serial.print("Network:         ");
        Serial.println(WiFi.networkID().toString());
        Serial.print("Netmask:         ");
        Serial.println(WiFi.subnetMask().toString());
        Serial.print("Broadcast:       ");
        Serial.println(WiFi.broadcastIP().toString());
        Serial.print("Subnet CIDR:     ");
        Serial.println(WiFi.subnetCIDR());

        return true;
    }

    return false;
}

void DoTask(void* unused) {
    Init();

    const unsigned long kResetIntervalMs = 48 * 60 * 60 * 1000;
    unsigned long last_connect_time = millis();
    unsigned long last_print_time_ms = 0;
    for (;;) {
        if ((millis() - last_print_time_ms) > 10 * 60 * 1000 || !last_print_time_ms) {
            Serial.print("net_manager::DoTask(): core: ");
            Serial.println(xPortGetCoreID());
            last_print_time_ms = millis();
        }
        if (status == STATUS_CONNECTED && millis() - last_connect_time > kResetIntervalMs) {
            Serial.print("WiFi: Resetting...");
            // WiFi.disconnect(/*wifioff=*/ true, /*eraseap=*/ false);
            WiFi.disconnect(/*wifioff=*/ true);
            status = STATUS_DISCONNECTED;
            delay(10);
        }
        if (status == STATUS_CONNECTED && WiFi.status() == WL_CONNECTED) {
            delay(5000);
            continue;
        }

        Serial.print("WiFi not connected; status: ");
        Serial.println(WiFi.status());
        Serial.println(status);
        if (Connect(/*timeout_ms=*/ 60000)) {
            Serial.print("WiFi: Connected");
        } else {
            Serial.print("WiFi: Resetting...");
            // WiFi.disconnect(/*wifioff=*/ true);
        }
        last_connect_time = millis();
    }
    vTaskDelete(nullptr);
}

} // namespace net_manager
