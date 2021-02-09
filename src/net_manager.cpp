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

    // WiFi.scanNetworks will return the number of networks found
    int n = 0;
    do  {
        unsigned long scan_start_time_ms = millis();
        n = WiFi.scanNetworks();
        Serial.print("WiFi: scan done in ");
        Serial.print(millis() - scan_start_time_ms);
        Serial.print(" ms\n");

        if (n < 0) {
            Serial.println("Scan failed");
            delay(100);
        } else if (n == 0) {
            Serial.println("no networks found");
            delay(10);
        }
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

bool Connect(unsigned long timeout_ms) {
    // don't persist to flash
    WiFi.persistent(true);
    // Triggers low-level esp wifi init
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    // Serial.print("WiFi connecting... Status: ");
    // Serial.println(WiFi.status(););

    // Set up some event handlers
    WiFi.onEvent(
        [](WiFiEvent_t event, WiFiEventInfo_t info) {
            Serial.print("WiFi connected. IP: ");
            Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
            // WiFiConnected = true;
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
            //WiFiConnected = false;
            status = STATUS_DISCONNECTED;
            // ESP.restart();
        },
        WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

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
        // config.sta.ssid[0] = '\0';
        if (connect_failures > 3 || config.sta.ssid[0] == '\0') {
            Serial.print("Wifi: no saved SSID, starting captive portal...");
            // wifi_manager.setShowPassword(true);

            wifi_manager.setConfigPortalTimeout(3);  // blocking
            wifi_manager.setConnectRetries(3);  // blocking
            wifi_manager.setSaveConnectTimeout(/*seconds=*/ 20);  // blocking
            wifi_manager.setConnectTimeout(/*seconds=*/ 20);  // blocking
            auto s = wifi_manager.startConfigPortal();  // blocking
            Serial.print("WifiManager done, result: ");
            Serial.println(s);
            wifi_manager.stopConfigPortal();
            esp_wifi_get_config(WIFI_IF_STA, &config);
            /*
            if (!wifi_manager.startConfigPortal()) {
                Serial.print("Wifi: WifiManager failed...");
                wifi_manager.stopConfigPortal();
                continue;
            }
            */
            if (config.sta.ssid[0] == '\0') {
                Serial.print("Wifi: WifiManager failed no ssid...");
                continue;
            }
            memcpy(ssid, config.sta.ssid, sizeof(config.sta.ssid));
            memcpy(password, config.sta.password, sizeof(config.sta.password));
            Serial.print("Wifi: WiFiManager SSID: ");
            Serial.println(ssid);
            Serial.print("Wifi: WiFiManager password: ");
            Serial.println(password);

        } else {
            memcpy(ssid, config.sta.ssid, sizeof(config.sta.ssid));
            memcpy(password, config.sta.password, sizeof(config.sta.password));
            // TODO: use string_view to avoid buffer overflow
            Serial.print("Wifi: saved SSID: ");
            Serial.println(ssid);
            Serial.print("Wifi: saved password: ");
            Serial.println(password);
        }


        unsigned long start_time_ms = millis();
        // Do our own config so we can set WIFI_ALL_CHANNEL_SCAN
        // strcpy(reinterpret_cast<char*>(config.sta.ssid), kSsid);
        // strcpy(reinterpret_cast<char*>(config.sta.password), kPass);
        config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        // config.sta.scan_threshold_t = WIFI_CONNECT_AP_BY_SIGNAL;
        esp_wifi_set_config(WIFI_IF_STA, &config);

        WiFi.disconnect(/*wifioff=*/ true);
        auto* ap_info = ScanFor(ssid, timeout_ms);
        if (!ap_info) {
            Serial.print("WiFi: Failed to find SSID: ");
            Serial.println(ssid);
            ++connect_failures;
            continue;
        }

        Serial.print("WiFi: Connecting to ");
        Serial.print(ssid);
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

        config.sta.bssid_set = true;
        memcpy(config.sta.bssid, ap_info->bssid, 6);
        config.sta.channel = ap_info->primary;
        esp_wifi_set_config(WIFI_IF_STA, &config);

        Serial.print("WiFi: Connecting...\n");
        WiFi.disconnect(/*wifioff=*/ true);
        // WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);

        // WiFi.setHostname("esp32dev");
        WiFi.setSleep(false);
        // int status = WiFi.begin(/*ssid=*/ kSsid, /*password=*/ kPass);
        status = STATUS_CONNECTING;

        WiFi.begin();
        unsigned long backoff_ms = 10;
        while (status != STATUS_CONNECTED && (millis() - start_time_ms) < timeout_ms) {
            Serial.print("WiFi connecting... Status: ");
            Serial.println(status);
            if (status == STATUS_DISCONNECTED) {
                // WiFi.disconnect(/*wifioff=*/ false, /*eraseap=*/ false);
                WiFi.disconnect(/*wifioff=*/ true);
                Serial.print("WiFi: sleep for ");
                Serial.println(ui::MillisHumanReadable(backoff_ms));
                delay(backoff_ms);
                backoff_ms *= 2;
                WiFi.begin();
                status = STATUS_CONNECTING;
            }
            delay(1000);
            // status = WiFi.status();
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

        WiFi.onEvent(
            [](WiFiEvent_t event, WiFiEventInfo_t info) {
                Serial.print("WiFi lost connection. Reason: ");
                Serial.println(info.disconnected.reason);
                Serial.print("WiFi not connected; status: ");
                Serial.println(WiFi.status());
                // WiFi.persistent(false);
                WiFi.disconnect(true);
                //WiFiConnected = false;
                status = STATUS_DISCONNECTED;
                ESP.restart();
            },
            WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

        return true;
    }

    return false;
}

void DoTask(void* unused) {
    const unsigned long kResetIntervalMs = 60 * 60 * 1000;  // 1 hr
    unsigned long last_connect_time = millis();
    unsigned long last_print_time_ms = 0;
    for (;;) {
        if ((millis() - last_print_time_ms) > 10 * 60 * 1000 || !last_print_time_ms) {
            Serial.print("net_manager::DoTask(): core: ");
            Serial.println(xPortGetCoreID());
            last_print_time_ms = millis();
        }
        if (millis() - last_connect_time > kResetIntervalMs) {
            Serial.print("WiFi: Resetting...");
            // WiFi.disconnect(/*wifioff=*/ true, /*eraseap=*/ false);
            WiFi.disconnect(/*wifioff=*/ true);
        }
        if (WiFi.status() == WL_CONNECTED) {
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
