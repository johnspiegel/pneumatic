#include "ui.h"

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiServer.h>
#include <dump.h>
#include <esp_log.h>
#include <esp_wifi.h>

#include "html.h"

#define BUTTON_1 35
#define BUTTON_2 0
#define BUTTONS_MAP \
  { BUTTON_1, BUTTON_2 }

namespace ui {

namespace {
TFT_eSPI tft = TFT_eSPI();  // Invoke custom library
TFT_eSprite spr = TFT_eSprite(&tft);
const char TAG[] = "ui";
}  // namespace

void InitTft(void) {
  tft.init();

  tft.fillScreen(TFT_BLACK);

  // Set "cursor" at top left corner of display (0,0) and select font 4
  tft.setCursor(0, 0, 4);

  // Set the font colour to be white with a black background
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // We can now plot text on screen using the "print" class
  tft.println("Intialised default\n");
  tft.println("White wtext");

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.println("Red rtext");

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Green gtext");

  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.println("Blue btext");
}

void TaskTft() {
  tft.invertDisplay(false);  // Where i is true or false

  tft.fillScreen(TFT_BLACK);

  tft.setCursor(0, 0, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("Invert OFF\n");

  tft.println("White text");

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.println("Red text");

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Green text");

  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.println("Blue text");

  delay(5000);

  // Binary inversion of colours
  tft.invertDisplay(true);  // Where i is true or false

  tft.fillScreen(TFT_BLACK);

  tft.setCursor(0, 0, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("Invert ON\n");

  tft.println("White text");

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.println("Red text");

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("Green text");

  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.println("Blue text");

  delay(5000);
}

float calcAQI(int val, int concL, int concH, int aqiL, int aqiH) {
  return aqiL + float(val - concL) * (aqiH - aqiL) / (concH - concL);
}

uint32_t FgColor(uint32_t bg_color) {
  // https://www.w3.org/TR/AERT/#color-contrast
  // range: [0,255000]
  int brightness = (bg_color >> 16) * 299 + ((bg_color >> 8) & 0xff) * 587 +
                   (bg_color & 0xff) * 114;
  return (brightness > 127500) ? 0x000000 : 0xffffff;
}

struct AqiCategory {
  const char* tag;
  const char* message;

  uint16_t low_aqi;
  uint16_t high_aqi;
  uint32_t color;  // official AQI colors
  uint32_t web_color;
  uint32_t led_color;
};

const std::initializer_list<AqiCategory> aqi_categories = {
    {
        .tag = "good",
        .message = "Good 😀",
        .low_aqi = 0,
        .high_aqi = 50,
        .color = 0x00e400,
        .web_color = 0x68e143,
        .led_color = 0x00ff00,
    },
    {
        .tag = "moderate",
        .message = "Moderate 😐",
        .low_aqi = 51,
        .high_aqi = 100,
        .color = 0xffff00,
        .web_color = 0xffff55,
        .led_color = 0xffff00,
    },
    {
        .tag = "unhealthy-for-sensitive-groups",
        .message = "Unhealthy for sensitive groups 🙁",
        .low_aqi = 101,
        .high_aqi = 150,
        .color = 0xff7e00,
        .web_color = 0xef8533,
        .led_color = 0xff6000,
    },
    {
        .tag = "unhealthy",
        .message = "Unhealthy 😷",
        .low_aqi = 151,
        .high_aqi = 200,
        .color = 0xff0000,
        .web_color = 0xea3324,
        .led_color = 0xff0000,
    },
    {
        .tag = "very-unhealthy",
        .message = "Very Unhealthy 🤢",
        .low_aqi = 201,
        .high_aqi = 300,
        .color = 0x8f3f97,
        .web_color = 0x8c1a4b,
        .led_color = 0xff0020,
    },
    {
        .tag = "hazardous",
        .message = "Hazardous 😵",
        .low_aqi = 301,
        .high_aqi = 400,
        .color = 0x7e0023,
        .web_color = 0x8c1a4b,
        .led_color = 0xff0040,
    },
    {
        // Note: Officially, this is still just "hazardous".
        .tag = "very-hazardous",
        .message = "Very Hazardous ☠️",
        .low_aqi = 401,
        .high_aqi = 500,
        .color = 0x7e0023,
        .web_color = 0x731425,
        .led_color = 0xff0060,
    },
};

struct AqiLevel {
  float low_conc;
  float high_conc;
};

// PM1.0 aqi isn't really defined!
const std::initializer_list<AqiLevel> aqi_pm2_5 = {
    // good
    {
        .low_conc = 0.0,
        .high_conc = 12.0,
    },
    // moderate
    {
        .low_conc = 12.1,
        .high_conc = 35.4,
    },
    // unhealthy for sensitive groups
    {
        .low_conc = 35.5,
        .high_conc = 55.4,
    },
    // unhealthy
    {
        .low_conc = 55.5,
        .high_conc = 150.4,
    },
    // very unhealthy
    {
        .low_conc = 150.5,
        .high_conc = 250.4,
    },
    // hazardous
    {
        .low_conc = 250.5,
        .high_conc = 350.4,
    },
    {
        .low_conc = 350.5,
        .high_conc = 500.4,
    },
};
const std::initializer_list<AqiLevel> aqi_pm10_0 = {
    // good
    {
        .low_conc = 0,
        .high_conc = 54,
    },
    // moderate
    {
        .low_conc = 55,
        .high_conc = 154,
    },
    // unhealthy for sensitive groups
    {
        .low_conc = 155,
        .high_conc = 254,
    },
    // unhealthy
    {
        .low_conc = 255,
        .high_conc = 354,
    },
    // very unhealthy
    {
        .low_conc = 355,
        .high_conc = 424,
    },
    // hazardous
    {
        .low_conc = 425,
        .high_conc = 504,
    },
    {
        .low_conc = 505,
        .high_conc = 604,
    },
};

// CO2 aqi isn't a thing, just doing this for colors.
const std::initializer_list<AqiLevel> aqi_co2 = {
    // good, green
    {
        .low_conc = 0,
        .high_conc = 700,
    },
    // moderate, yellow
    {
        .low_conc = 701,
        .high_conc = 1000,
    },
    // unhealthy for sensitive groups, orange
    {
        .low_conc = 1001,
        .high_conc = 1500,
    },
    // unhealthy, red
    {
        .low_conc = 1501,
        .high_conc = 2000,
    },
    // very unhealthy, purple
    {
        .low_conc = 2001,
        .high_conc = 3000,
    },
    // hazardous, maroon
    {
        .low_conc = 3001,
        .high_conc = 4000,
    },
    {
        .low_conc = 4001,
        .high_conc = 5000,
    },
};

// TODO: truncate conc to 1 decimal place for PM2.5, integer for PM10.0
// https://www.airnow.gov/aqi/aqi-calculator/
// https://www.airnow.gov/sites/default/files/2020-05/aqi-technical-assistance-document-sept2018.pdf
int Aqi(const std::initializer_list<AqiLevel>& aqi_levels,
        int truncate_decimals, float conc) {
  for (int i = 0; i < truncate_decimals; ++i) {
    conc *= 10;
  }
  conc = int(conc);  // truncate
  for (int i = 0; i < truncate_decimals; ++i) {
    conc /= 10;
  }

  auto p_level = aqi_levels.begin();
  auto cat = aqi_categories.begin();
  while (p_level + 1 < aqi_levels.end() && cat + 1 < aqi_categories.end()) {
    if (conc <= p_level->high_conc) {
      break;
    }
    p_level++;
    cat++;
  }

  return std::round(cat->low_aqi +
                    float(cat->high_aqi - cat->low_aqi) /
                        float(p_level->high_conc - p_level->low_conc) *
                        (conc - p_level->low_conc));
}

const AqiCategory& GetAqiCategory(float aqi) {
  const AqiCategory* c = nullptr;
  for (const auto& cat : aqi_categories) {
    c = &cat;
    if (aqi <= cat.high_aqi) {
      break;
    }
  }
  return *c;
}

const char* AqiTag(float aqi) { return GetAqiCategory(aqi).tag; }

const char* AqiMessage(int aqi) { return GetAqiCategory(aqi).message; }

const char* Co2Tag(int co2_ppm) {
  return GetAqiCategory(Aqi(aqi_co2, 0, co2_ppm)).tag;
}

int32_t co2Color(int co2_ppm) {
  if (co2_ppm < 700) {
    // return 0x68e143;
    // return 0x34e000;
    return 0x00ff00;
    // return Adafruit_NeoPixel::Color(0x68, 0xe1, 0x43);
  } else if (co2_ppm < 1000) {
    // return 0xffff55;
    return 0xffff00;
  } else if (co2_ppm < 1500) {
    // return 0xef8533;
    // return 0xf000ec;

    // return 0xffa500;
    return 0xff6000;
  } else if (co2_ppm < 2000) {
    // return 0xea3324;
    // return 0xeb1400;
    return 0xff0000;
  } else if (co2_ppm < 3000) {
    // return 0x8c1a4b;
    // return 0x8c003d;
    return 0xff0020;
  } else if (co2_ppm < 4000) {
    // return 0x8c1a4b;
    // return 0x8c003d;
    return 0xff0040;
  } else {
    // return 0x731425;
    // return 0x730015;
    return 0xff0060;
  }
}

void DoStatusz(WiFiClient* client, const TaskData* task_data) {
  client->print("HTTP/1.1 200 OK\r\n");
  client->print("Content-Type:text/html charset=utf-8\r\n");
  client->print("Connection: close\r\n");
  client->print("\r\n");

  // PM1.0 AQI is not a thing!
  int pm1aqi = Aqi(aqi_pm2_5, 1, task_data->pmsx003_data->pm_1_0);
  int pm25aqi = Aqi(aqi_pm2_5, 1, task_data->pmsx003_data->pm_2_5);
  int pm10aqi = Aqi(aqi_pm10_0, 0, task_data->pmsx003_data->pm_10_0);

  int max_aqi = pm25aqi;
  const char* max_aqi_class = AqiTag(pm25aqi);
  if (pm10aqi > pm25aqi) {
    max_aqi = pm10aqi;
    max_aqi_class = AqiTag(pm10aqi);
  }

  // int co2Rand = random(0,6000);

  char buf[4 * 1024] = {0};
  snprintf(buf, sizeof(buf), indexTemplate,
           // Overall AQI
           max_aqi_class, max_aqi, AqiMessage(max_aqi),
           // PM 1.0/2.5/10.0 AQI
           // PM1.0 AQI is not a thing!
           AqiTag(pm1aqi), pm1aqi, task_data->pmsx003_data->pm_1_0,
           AqiTag(pm25aqi), pm25aqi, task_data->pmsx003_data->pm_2_5,
           AqiTag(pm10aqi), pm10aqi, task_data->pmsx003_data->pm_10_0,
           // CO2
           Co2Tag(task_data->dsco220_data->co2_ppm),
           task_data->dsco220_data->co2_ppm,
           // Temp/Humidity/Pressure
           task_data->bme280_data->temp_c,
           dump::CToF(task_data->bme280_data->temp_c),
           task_data->bme280_data->humidity_pct,
           task_data->bme280_data->pressure_pa / 100.0,

           // Bottom details
           dump::MillisHumanReadable(millis()).c_str(),
           task_data->pmsx003_data->particles_gt_0_3,
           task_data->pmsx003_data->particles_gt_0_5,
           task_data->pmsx003_data->particles_gt_1_0,
           task_data->pmsx003_data->particles_gt_2_5,
           task_data->pmsx003_data->particles_gt_5_0,
           task_data->pmsx003_data->particles_gt_10_0,
           task_data->mhz19_data->co2_ppm, task_data->mhz19_data->temp_c,
           dump::CToF(task_data->mhz19_data->temp_c),
           task_data->dsco220_data->co2_ppm, task_data->bme280_data->temp_c,
           dump::CToF(task_data->bme280_data->temp_c),
           task_data->bme280_data->pressure_pa,
           task_data->bme280_data->humidity_pct);
  client->print(buf);

  client->print("\n");
}

void DoVarz(WiFiClient* client, const TaskData* task_data) {
  client->print("HTTP/1.1 200 OK\r\n");
  client->print("Content-Type:text/plain; version=0.0.4; charset=utf-8\r\n");
  client->print("Connection: close\r\n");
  client->print("\r\n");

  String mac = WiFi.macAddress();
  // mac.replace(":", "");
  mac.toLowerCase();
  String ip = WiFi.localIP().toString();
  const char* host = WiFi.getHostname();
  auto MetricLine = [&](const char* name, const char* fields, String value) {
    String line =
        R"({name}{mac_address="{mac}",ip_address="{ip}",hostname="{host}",{fields}} {value})";
    line.replace("{name}", name);
    line.replace("{mac}", mac);
    line.replace("{ip}", ip);
    line.replace("{host}", host);
    line.replace("{fields}", fields);
    line.replace("{value}", String(value));
    line += '\n';
    return line;
  };
  auto MetricLineInt = [&](const char* name, const char* fields, long value) {
    return MetricLine(name, fields, String(value));
  };
  auto MetricLineUint = [&](const char* name, const char* fields,
                            unsigned long value) {
    return MetricLine(name, fields, String(value));
  };
  auto MetricLineDouble = [&](const char* name, const char* fields,
                              double value) {
    return MetricLine(name, fields, String(value));
  };

  client->print(MetricLineUint("uptime_ms", "", millis()));

  String wifi_fields = R"(ssid="{ssid}",bssid="{bssid}",channel="{channel}")";
  wifi_fields.replace("{ssid}", WiFi.SSID());
  String bssid = WiFi.BSSIDstr();
  bssid.toLowerCase();
  wifi_fields.replace("{bssid}", bssid);
  wifi_fields.replace("{channel}", String(WiFi.channel()));
  client->print(MetricLineInt("wifi_rssi", wifi_fields.c_str(), WiFi.RSSI()));
  client->print(
      MetricLineInt("wifi_txpower", wifi_fields.c_str(), WiFi.getTxPower()));

  if (millis() < 60000) {
    ESP_LOGI(TAG, "Not reporting sensor varz until up for 1m");
    return;
  }

  // Particulate
  client->print(MetricLineDouble("pm_ug_m3", R"(sensor="PMSA003",size="pm1.0")",
                                 task_data->pmsx003_data->pm_1_0));
  client->print(MetricLineDouble("pm_ug_m3", R"(sensor="PMSA003",size="pm2.5")",
                                 task_data->pmsx003_data->pm_2_5));
  client->print(MetricLineDouble("pm_ug_m3",
                                 R"(sensor="PMSA003",size="pm10.0")",
                                 task_data->pmsx003_data->pm_10_0));

  // PM1.0 AQI is not a thing!
  int pm1aqi = Aqi(aqi_pm2_5, 1, task_data->pmsx003_data->pm_1_0);
  int pm25aqi = Aqi(aqi_pm2_5, 1, task_data->pmsx003_data->pm_2_5);
  int pm10aqi = Aqi(aqi_pm10_0, 0, task_data->pmsx003_data->pm_10_0);
  client->print(
      MetricLineInt("us_aqi", R"(sensor="PMSA003",size="pm1.0")", pm1aqi));
  client->print(
      MetricLineInt("us_aqi", R"(sensor="PMSA003",size="pm2.5")", pm25aqi));
  client->print(
      MetricLineInt("us_aqi", R"(sensor="PMSA003",size="pm10.0")", pm10aqi));

  client->print(MetricLineInt("co2_ppm", R"(sensor="DS-CO2-20")",
                              task_data->dsco220_data->co2_ppm));

  client->print(MetricLineInt("co2_ppm", R"(sensor="MH-Z19C")",
                              task_data->mhz19_data->co2_ppm));
  client->print(MetricLineInt("temp_c", R"(sensor="MH-Z19C")",
                              task_data->mhz19_data->temp_c));

  client->print(MetricLineDouble("temp_c", R"(sensor="BME280")",
                                 task_data->bme280_data->temp_c));
  client->print(MetricLineDouble("pressure_pa", R"(sensor="BME280")",
                                 task_data->bme280_data->pressure_pa));
  client->print(MetricLineDouble("humidity_percent", R"(sensor="BME280")",
                                 task_data->bme280_data->humidity_pct));
}

void TaskDisplay(void* task_data_arg) {
  Serial.println("TaskDisplay: Starting task...");
  TaskData* task_data = reinterpret_cast<TaskData*>(task_data_arg);
  tft.init();
  // Clockwise 90 degrees; t-display: USB and buttons to right, antenna to left.
  tft.setRotation(1);
  spr.createSprite(/* width = */ 240, /* height= */ 135);

  unsigned long last_print_time_ms = 0;
  unsigned long last_display_time_ms = 0;
  int delay_ms = 1000;
  for (;; delay(delay_ms)) {
    if ((millis() - last_print_time_ms) > 10 * 1000 || !last_print_time_ms) {
      ESP_LOGI(TAG, "TaskDisplay(): uptime: %s core: %d",
               dump::MillisHumanReadable(millis()).c_str(), xPortGetCoreID());
      last_print_time_ms = millis();
    }

    int pm25aqi = Aqi(aqi_pm2_5, 1, task_data->pmsx003_data->pm_2_5);
    int pm10aqi = Aqi(aqi_pm10_0, 0, task_data->pmsx003_data->pm_10_0);

    int max_aqi = pm25aqi;
    if (pm10aqi > pm25aqi) {
      max_aqi = pm10aqi;
    }
    // static int dummy = 0;
    // max_aqi += dummy;
    // dummy += 10;
    const auto& aqi_cat = GetAqiCategory(max_aqi);

    // Print AQI
    uint16_t bgcolor = tft.color24to16(aqi_cat.color);
    uint16_t fgcolor = tft.color24to16(FgColor(aqi_cat.color));
    spr.fillRect(0, 0, 240, 135, bgcolor);
    spr.setTextColor(fgcolor, bgcolor);
    spr.setTextDatum(TR_DATUM);
    spr.setFreeFont(&FreeMonoBold9pt7b);
    spr.drawString("AQI", 90, 5);
    spr.setFreeFont(&FreeMonoBold24pt7b);
    spr.drawNumber(max_aqi, 100, 30);

    // Print CO2 ppm
    const auto& co2_cat =
        GetAqiCategory(Aqi(aqi_co2, 0, task_data->dsco220_data->co2_ppm));
    spr.fillRect(120, 0, 120, 75, tft.color24to16(co2_cat.color));
    spr.setTextColor(tft.color24to16(FgColor(co2_cat.color)));
    spr.setFreeFont(&FreeMonoBold9pt7b);
    spr.drawString("CO2", 210, 5);
    spr.setFreeFont(&FreeMonoBold24pt7b);
    spr.drawNumber(task_data->dsco220_data->co2_ppm, 235, 30);

    // Print status info at the bottom
    spr.setFreeFont(&FreeMonoBold9pt7b);
    spr.setTextColor(fgcolor, bgcolor);
    spr.setCursor(5, 94);
    spr.print("SSID: ");
    auto ssid = WiFi.SSID();
    auto ip = WiFi.localIP();
    if (ssid.isEmpty() && WiFi.getMode() != WIFI_STA) {
      wifi_config_t config = {0};
      esp_wifi_get_config(WIFI_IF_AP, &config);
      // ssid = WiFi.softAPSSID();
      ssid = reinterpret_cast<const char*>(config.ap.ssid);
      ip = WiFi.softAPIP();
    } else if (ssid.isEmpty()) {
      ssid = "connecting...";
    }
    spr.println(ssid);
    spr.setCursor(5, 112);
    spr.print("IP: ");
    spr.println(WiFi.localIP().toString());
    spr.setTextColor(fgcolor, bgcolor);
    spr.setCursor(5, 129);
    last_display_time_ms = millis();
    spr.print("up: " + dump::SecondsHumanReadable(last_display_time_ms));
    
    // Write to screen
    spr.pushSprite(0, 0);

    delay_ms = std::max<int>(
        0,
        // truncate to previous second
        (last_display_time_ms / 1000) * 1000
            // add one second
            + 1000
            // subract the current time
            - millis()
            // Add one tick to make sure we arrive after the second rolls
            + 10);
    ESP_LOGI(TAG,
             "TaskDisplay(): uptime: %s last_display_time_ms: %ld delay_ms: %d",
             dump::MillisHumanReadable(millis()).c_str(), last_display_time_ms,
             delay_ms);
  }
  vTaskDelete(NULL);
}

void TaskServeWeb(void* task_data_arg) {
  Serial.println("ServeWeb: Starting task...");
  TaskData* task_data = reinterpret_cast<TaskData*>(task_data_arg);
  WiFiServer server(/*port=*/80);

  unsigned long last_print_time_ms = 0;
  unsigned long last_client_time_ms = 0;
  for (;;) {
    delay(10);
    if ((millis() - last_print_time_ms) > 10 * 1000 || !last_print_time_ms) {
      ESP_LOGI(
          TAG,
          "TaskServeWeb(): uptime: %s core: %d server_running: %d "
          "since_last_http_client: %s",
          dump::MillisHumanReadable(millis()).c_str(), xPortGetCoreID(),
          bool(server),
          dump::MillisHumanReadable(millis() - last_client_time_ms).c_str());
      if (!WiFi.isConnected()) {
        ESP_LOGW(TAG, "TaskServeWeb: Waiting for WiFi...");
      }
      last_print_time_ms = millis();
    }
    if (!WiFi.isConnected()) {
      continue;
    }

    // TODO: register for wifi disconnect signal to eliminate restart races.
    if (!server) {
      // server.end();
      ESP_LOGI(TAG, "Starting WiFiServer");
      // delay(5000);
      server.begin();
      if (!server) {
        ESP_LOGE(TAG, "Failed to start WiFiServer");
        // server.end();
        delay(1000);
        continue;
      }
      ESP_LOGI(TAG, "WiFiServer started on port 80");
    }

    // Serial.println("Waiting for HTTP connection...");
    WiFiClient client = server.accept();
    if (!client) {
      // Serial.println("ServeWeb: Got no for HTTP connection...");
      client.stop();
      continue;
    }

    // long connectTime = millis();
    std::string request;
    Serial.println("New http client");
    last_client_time_ms = millis();
    ESP_LOGI(TAG, "ui::TaskServeWeb(): up: %s http request from: %s",
             dump::MillisHumanReadable(last_client_time_ms).c_str(),
             client.remoteIP().toString().c_str());
    while (client.connected()) {
      if (millis() - last_client_time_ms > 5000) {
        ESP_LOGW(
            TAG,
            "TaskServeWeb: Timed out waiting for client, request so far:\n[%s]",
            request.c_str());
        client.stop();
        delay(10);
        continue;
      }

      int available = client.available();
      if (available) {
        int old_size = request.size();
        request.resize(old_size + available);
        int read_count = client.read(
            reinterpret_cast<uint8_t*>(&request[0]) + old_size, available);
        if (read_count > 0) {
          request.resize(old_size + read_count);
        } else {
          request.resize(old_size);
        }
        ESP_LOGI(TAG, "TaskServeWeb: Read %d bytes, old_size: %d new_size: %d",
                 read_count, old_size, request.size());
      }
      if (client.available() || request.find("\r\n\r\n") == std::string::npos) {
        delay(10);
        continue;
      }

      ESP_LOGI(TAG,
               "HTTP Request -- BEGIN --\n"
               "%s"
               "-- END --",
               request.c_str());

      if (request.rfind("GET /favicon.ico ", 0) == 0) {
        Serial.println("TaskServeWeb: /favicon.ico");
        client.print("HTTP/1.1 404 Not Found\r\n");
        client.print("Connection: close\r\n");
        client.print("\r\n");
      } else if (request.rfind("GET /varz ", 0) == 0 ||
                 request.rfind("GET /metrics ", 0) == 0) {
        Serial.println("TaskServeWeb: /varz");
        DoVarz(&client, task_data);
      } else {
        Serial.println("TaskServeWeb: /statusz");
        DoStatusz(&client, task_data);
      }
      client.flush();
      client.stop();
      ESP_LOGI(
          TAG, "TaskServeWeb(): client finished in: %s",
          dump::MillisHumanReadable(millis() - last_client_time_ms).c_str());
      break;
    }
    client.stop();
  }

  vTaskDelete(NULL);
}

void TaskDoPixels(void* task_data_arg) {
  TaskData* task_data = reinterpret_cast<TaskData*>(task_data_arg);

  for (;;) {
    const auto& co2_cat =
        GetAqiCategory(Aqi(aqi_co2, 0, task_data->dsco220_data->co2_ppm));
    task_data->pixels->setBrightness(255);
    task_data->pixels->setPixelColor(0, co2_cat.color);
    task_data->pixels->show();
    delay(1000);  // Delay for a period of time (in milliseconds).
  }
  vTaskDelete(NULL);
}

void TaskStrobePixels(void* pixels_arg) {
  auto pixels = reinterpret_cast<Adafruit_NeoPixel*>(pixels_arg);

  for (;;) {
    // white - halogen
    for (int value = 0; value <= 0xff; value += 1) {
      pixels->setBrightness(value);
      pixels->setPixelColor(0, pixels->Color(255, 147, 41));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    for (int value = 0xff; value; value -= 1) {
      pixels->setBrightness(value);
      pixels->setPixelColor(0, pixels->Color(255, 147, 41));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    pixels->setBrightness(0xff);
    // white
    for (int value = 0; value <= 0xff; value += 1) {
      pixels->setPixelColor(0, pixels->ColorHSV(0, 0, value));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    for (int value = 0xff; value; value -= 1) {
      pixels->setPixelColor(0, pixels->ColorHSV(0, 0, value));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    // white - halogen
    for (int value = 0; value <= 0xff; value += 1) {
      pixels->setPixelColor(0, pixels->Color(255, 197, 143));
      pixels->setBrightness(value);
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    for (int value = 0xff; value; value -= 1) {
      pixels->setPixelColor(0, pixels->Color(255, 197, 143));
      pixels->setBrightness(value);
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    pixels->setBrightness(0xff);
    // Red
    for (int value = 0; value <= 0xff; value += 1) {
      pixels->setPixelColor(0, pixels->ColorHSV(0, 0xff, value));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    for (int value = 0xff; value; value -= 1) {
      pixels->setPixelColor(0, pixels->ColorHSV(0, 0xff, value));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    // Green
    for (int value = 0; value <= 0xff; value += 1) {
      pixels->setPixelColor(0, pixels->ColorHSV(0xffff / 3, 0xff, value));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    for (int value = 0xff; value; value -= 1) {
      pixels->setPixelColor(0, pixels->ColorHSV(0xffff / 3, 0xff, value));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    // Blue
    for (int value = 0; value <= 0xff; value += 1) {
      pixels->setPixelColor(0, pixels->ColorHSV(0xffff * 2 / 3, 0xff, value));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
    for (int value = 0xff; value; value -= 1) {
      pixels->setPixelColor(0, pixels->ColorHSV(0xffff * 2 / 3, 0xff, value));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }

    // Cycle colors
    for (int hue = 0; hue <= 0xffff; hue += 0xffff * 10 / 3000) {
      pixels->setPixelColor(0, pixels->ColorHSV(hue, 0xff, 64));
      pixels->show();
      delay(10);  // Delay for a period of time (in milliseconds).
    }
  }
  vTaskDelete(NULL);
}

}  // namespace ui
