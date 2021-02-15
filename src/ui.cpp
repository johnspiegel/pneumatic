#include "ui.h"

#include <Adafruit_Neopixel.h>
#include <WiFi.h>
#include <WiFiServer.h>

#include "html.h"

namespace ui {

String MillisHumanReadable(unsigned long ms) {
    int days = ms / (24 * 60 * 60 * 1000);
    ms %= (24 * 60 * 60 * 1000);
    int hours = ms / (60 * 60 * 1000);
    ms %= (60 * 60 * 1000);
    int minutes = ms / (60 * 1000);
    ms %= (60 * 1000);
    int seconds = ms / (1000);
    ms %= (1000);

    char buf[8] = {0};
    String str;
    if (days) {
        snprintf(buf, sizeof(buf), "%dd", days);
        str += buf;
    }
    if (hours) {
        if (str.length()) {
            snprintf(buf, sizeof(buf), "%02dh", hours);
        } else {
            snprintf(buf, sizeof(buf), "%dh", hours);
        }
        str += buf;
    }
    if (minutes) {
        if (str.length()) {
            snprintf(buf, sizeof(buf), "%02dm", minutes);
        } else {
            snprintf(buf, sizeof(buf), "%dm", minutes);
        }
        str += buf;
    }
    if (str.length()) {
        snprintf(buf, sizeof(buf), "%02d.%03ds", seconds, (int)ms);
    } else {
        snprintf(buf, sizeof(buf), "%d.%03ds", seconds, (int)ms);
    }
    str += buf;

    return str;
}

float CToF(float temp_C) {
    return temp_C * 9 / 5 + 32;
}

float calcAQI(int val, int concL, int concH, int aqiL, int aqiH) {
    return aqiL + float(val - concL) * (aqiH - aqiL)/(concH - concL);
}

struct AqiLevel {
    const char* tag;
    int16_t high_conc;
    int16_t high_aqi;
};

// PM1.0 aqi isn't really defined!
const std::initializer_list<AqiLevel> aqi_pm2_5 = {
    {
        .tag = "good",
        .high_conc = 12,
        .high_aqi = 50,
    },
    {
        .tag = "moderate",
        .high_conc = 35,
        .high_aqi = 100,
    },
    {
        .tag = "unhealthy-for-sensitive-groups",
        .high_conc = 55,
        .high_aqi = 150,
    },
    {
        .tag = "unhealthy",
        .high_conc = 150,
        .high_aqi = 200,
    },
    {
        .tag = "very-unhealthy",
        .high_conc = 250,
        .high_aqi = 300,
    },
    {
        .tag = "hazardous",
        .high_conc = 350,
        .high_aqi = 400,
    },
    {
        .tag = "hazardous",
        .high_conc = 500,
        .high_aqi = 500,
    },
};
const std::initializer_list<AqiLevel> aqi_pm10_0 = {
    {
        .tag = "good",
        .high_conc = 55,
        .high_aqi = 50,
    },
    {
        .tag = "moderate",
        .high_conc = 155,
        .high_aqi = 100,
    },
    {
        .tag = "unhealthy-for-sensitive-groups",
        .high_conc = 255,
        .high_aqi = 150,
    },
    {
        .tag = "unhealthy",
        .high_conc = 355,
        .high_aqi = 200,
    },
    {
        .tag = "very-unhealthy",
        .high_conc = 425,
        .high_aqi = 300,
    },
    {
        .tag = "hazardous",
        .high_conc = 505,
        .high_aqi = 400,
    },
    {
        .tag = "hazardous",
        .high_conc = 604,
        .high_aqi = 500,
    },
};

// TODO: truncate conc to 1 decimal place for PM2.5, integer for PM10.0
// https://www.airnow.gov/aqi/aqi-calculator/
// https://www.airnow.gov/sites/default/files/2020-05/aqi-technical-assistance-document-sept2018.pdf
float Aqi(const std::initializer_list<AqiLevel>& aqi_levels, float conc) {
    int low_conc = 0 , high_conc = 0, low_aqi = 0, high_aqi = 0;
    int max_conc = (aqi_levels.end()-1)->high_conc;
    for (const auto& aqi_level : aqi_levels) {
        if (conc <= aqi_level.high_conc || aqi_level.high_conc == max_conc) {
            high_conc = aqi_level.high_conc;
            high_aqi = aqi_level.high_aqi;
            break;
        }
        low_conc = aqi_level.high_conc;
        low_aqi = aqi_level.high_aqi;
    }
    return low_aqi + float(high_aqi - low_aqi) / float(high_conc - low_conc) * (conc - low_conc);
}

const char* AqiClass(const std::initializer_list<AqiLevel>& aqi_levels, float conc) {
    for (const auto& aqi_level : aqi_levels) {
        if (conc <= aqi_level.high_conc) {
            return aqi_level.tag;
        }
    }
    // Off the charts!
    return (aqi_levels.end()-1)->tag;
}

const char* aqiClass(int aqi) {
    if (aqi < 50) {
        return "good";
    } else if (aqi < 100) {
        return "moderate";
    } else if (aqi < 150) {
        return "unhealthy-for-sensitive-groups";
    } else if (aqi < 200) {
        return "unhealthy";
    } else if (aqi < 300) {
        return "very-unhealthy";
    } else if (aqi < 400) {
        return "hazardous";
    } else {
        return "very-hazardous";
    }
}

const char* aqiMessage(int aqi) {
    if (aqi < 50) {
        return "Good 😀";
    } else if (aqi < 100) {
        return "Moderate 😐";
    } else if (aqi < 150) {
        return "Unhealthy for sensitive groups 🙁";
    } else if (aqi < 200) {
        return "Unhealthy 😷";
    } else if (aqi < 300) {
        return "Very Unhealthy 🤢";
    } else if (aqi < 400) {
        return "Hazardous 😵";
    } else {
        return "Very Hazardous ☠️";
    }
}

const char* co2Class(int co2_ppm) {
    if (co2_ppm < 700) {
        return "good";
    } else if (co2_ppm < 1000) {
        return "moderate";
    } else if (co2_ppm < 1500) {
        return "unhealthy-for-sensitive-groups";
    } else if (co2_ppm < 2000) {
        return "unhealthy";
    } else if (co2_ppm < 3000) {
        return "very-unhealthy";
    } else if (co2_ppm < 4000) {
        return "hazardous";
    } else {
        return "very-hazardous";
    }
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

void DoVarz(WiFiClient* client, const TaskData* task_data) {
    client->print("HTTP/1.1 200 OK\n");
    client->print("Content-Type:text/plain; version=0.0.4; charset=utf-8\n");
    client->print("Connection: close\n");
    client->print("\n");

    String mac = WiFi.macAddress();
    // mac.replace(":", "");
    mac.toLowerCase();
    String ip = WiFi.localIP().toString();
    const char* host = WiFi.getHostname();
    auto MetricLine = [&](const char* name, const char* fields, String value) {
        String line = R"({name}{mac_address="{mac}",ip_address="{ip}",hostname="{host}",{fields}} {value})";
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
    auto MetricLineUint = [&](const char* name, const char* fields, unsigned long value) {
        return MetricLine(name, fields, String(value));
    };
    auto MetricLineDouble = [&](const char* name, const char* fields, double value) {
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
    client->print(MetricLineInt("wifi_txpower", wifi_fields.c_str(), WiFi.getTxPower()));

    client->print(MetricLineInt("pm_ug_m3", R"(sensor="PMSA003",size="pm1.0")",
                                task_data->pmsx003_data->pm1));
    client->print(MetricLineInt("pm_ug_m3", R"(sensor="PMSA003",size="pm2.5")",
                                task_data->pmsx003_data->pm25));
    client->print(MetricLineInt("pm_ug_m3", R"(sensor="PMSA003",size="pm10.0")",
                                task_data->pmsx003_data->pm10));

    // PM1.0 AQI is not a thing!
    int pm1aqi = std::round(Aqi(aqi_pm2_5, task_data->pmsx003_data->pm1));
    int pm25aqi = std::round(Aqi(aqi_pm2_5, task_data->pmsx003_data->pm25));
    int pm10aqi = std::round(Aqi(aqi_pm10_0, task_data->pmsx003_data->pm10));
    client->print(MetricLineInt("us_aqi", R"(sensor="PMSA003",size="pm1.0")", pm1aqi));
    client->print(MetricLineInt("us_aqi", R"(sensor="PMSA003",size="pm2.5")", pm25aqi));
    client->print(MetricLineInt("us_aqi", R"(sensor="PMSA003",size="pm10.0")", pm10aqi));

    client->print(MetricLineInt("co2_ppm", R"(sensor="DS-CO2-20")", 
                                task_data->dsco220_data->co2_ppm));

    client->print(MetricLineInt("co2_ppm", R"(sensor="MH-Z19C")", 
                                task_data->mhz19_data->co2_ppm));
    client->print(MetricLineInt("temp_c", R"(sensor="MH-Z19C")", 
                                task_data->mhz19_data->temp_c));

    client->print(MetricLineDouble("temp_c", R"(sensor="BME280")", 
                                task_data->bme280_data->temp_c));
    client->print(MetricLineDouble("pressure_pa", R"(sensor="BME280")", 
                                task_data->bme280_data->pressurePa));
    client->print(MetricLineDouble("humidity_percent", R"(sensor="BME280")", 
                                task_data->bme280_data->humidityPercent));
    client->stop();
}

void TaskServeWeb(void* task_data_arg) {
    Serial.println("ServeWeb: Starting task...");
    TaskData* task_data = reinterpret_cast<TaskData*>(task_data_arg);
    WiFiServer server(/*port=*/ 80);
    bool server_started = false;

    unsigned long last_print_time_ms = 0;
    for (;;) {
        if ((millis() - last_print_time_ms) > 10 * 60 * 1000 || !last_print_time_ms) {
            Serial.print("ui::TaskServeWeb(): core: ");
            Serial.println(xPortGetCoreID());
            last_print_time_ms = millis();
        }
        // TODO: register for wifi disconnect signal to eliminate restart races.
        if (!WiFi.isConnected() || !server_started) {
            server.end();
            while (!WiFi.isConnected()) {
                Serial.println("ServeWeb: Waiting for WiFi...");
                delay(1000);
                continue;
            }
            Serial.println("----------------------------------------");
            Serial.println("WiFi back online");
            // delay(5000);
            server.begin();
            server_started = true;
            Serial.println("----------------------------------------");
            Serial.println("WiFiServer started");
        }

        // Serial.println("Waiting for HTTP connection...");
        WiFiClient client = server.available();

        if (!client) {
            // Serial.println("ServeWeb: Got no for HTTP connection...");
            client.stop();
            delay(10);
            continue;
        }

        // long connectTime = millis();
        String request;
        Serial.println("New http client");
        unsigned long client_start_time_ms = millis();
        while (client.connected()) {
            if (millis() - client_start_time_ms > 5000) {
                Serial.println("ERROR: TaskServeWeb: Timed out waiting for client");
                client.stop();
                continue;
            }

            if (client.available()) {
                char c = client.read();
                if (c != '\r') {
                    request += c;
                }
                continue;
            }
            if (request[request.length()-1] != '\n') {
                continue;
            }

            Serial.print("HTTP Request:\n----\n");
            Serial.print(request);
            Serial.print("----\n");

            if (request.startsWith("GET /favicon.ico ")) {
                client.print("HTTP/1.1 404 Not Found\n");
                client.print("Connection: close\n");
                client.print("\n");
                client.stop();
                Serial.println("TaskServeWeb: /favicon.ico client finished");
                continue;
            }
            if (request.startsWith("GET /varz ")
                || request.startsWith("GET /metrics ")) {
                Serial.println("TaskServeWeb: /varz");
                DoVarz(&client, task_data);
                continue;
            }

            // PM1.0 AQI is not a thing!
            int pm1aqi = std::round(Aqi(aqi_pm2_5, task_data->pmsx003_data->pm1));
            int pm25aqi = std::round(Aqi(aqi_pm2_5, task_data->pmsx003_data->pm25));
            int pm10aqi = std::round(Aqi(aqi_pm10_0, task_data->pmsx003_data->pm10));

            int max_aqi = pm25aqi;
            const char* max_aqi_class = AqiClass(aqi_pm2_5, task_data->pmsx003_data->pm25);
            if (pm10aqi > pm25aqi) {
                max_aqi = pm10aqi;
                max_aqi_class = AqiClass(aqi_pm10_0, task_data->pmsx003_data->pm10);
            }

            client.print("HTTP/1.1 200 OK\n");
            client.print("Content-Type:text/html charset=utf-8\n");
            client.print("Connection: close\n");
            client.print("\n");

            // int co2Rand = random(0,6000);

            char buf[4*1024] = {0};
            snprintf(buf, sizeof(buf), indexTemplate,
                // Overall AQI
                max_aqi_class,
                max_aqi,
                aqiMessage(max_aqi),
                // PM 1.0/2.5/10.0 AQI
                // PM1.0 AQI is not a thing!
                AqiClass(aqi_pm2_5, task_data->pmsx003_data->pm1),
                pm1aqi,
                task_data->pmsx003_data->pm1,
                AqiClass(aqi_pm2_5, task_data->pmsx003_data->pm25),
                pm25aqi,
                task_data->pmsx003_data->pm25,
                AqiClass(aqi_pm10_0, task_data->pmsx003_data->pm10),
                pm10aqi,
                task_data->pmsx003_data->pm10,
                // CO2
                co2Class(task_data->dsco220_data->co2_ppm),
                task_data->dsco220_data->co2_ppm,
                // Temp/Humidity/Pressure
                task_data->bme280_data->temp_c,
                CToF(task_data->bme280_data->temp_c),
                task_data->bme280_data->humidityPercent,
                task_data->bme280_data->pressurePa / 100.0,

                // Bottom details
                MillisHumanReadable(millis()).c_str(),
                task_data->pmsx003_data->particles_gt_0_3,
                task_data->pmsx003_data->particles_gt_0_5,
                task_data->pmsx003_data->particles_gt_1_0,
                task_data->pmsx003_data->particles_gt_2_5,
                task_data->pmsx003_data->particles_gt_5_0,
                task_data->pmsx003_data->particles_gt_10_0,
                task_data->mhz19_data->co2_ppm,
                task_data->mhz19_data->temp_c,
                CToF(task_data->mhz19_data->temp_c),
                task_data->dsco220_data->co2_ppm,
                task_data->bme280_data->temp_c,
                CToF(task_data->bme280_data->temp_c),
                task_data->bme280_data->pressurePa,
                task_data->bme280_data->humidityPercent
            );
            client.print(buf);

            client.print("\n");
            client.stop();
            Serial.println("TaskServeWeb: client finished");
            continue;
        }
    }
    vTaskDelete(NULL);
}

void TaskDoPixels(void* task_data_arg) {
    TaskData* task_data = reinterpret_cast<TaskData*>(task_data_arg);

    for (;;) {
        task_data->pixels->setBrightness(255);
        task_data->pixels->setPixelColor(0, co2Color(task_data->dsco220_data->co2_ppm));
        task_data->pixels->show();
        delay(1000); // Delay for a period of time (in milliseconds).
    }
    vTaskDelete(NULL);
}

void TaskStrobePixels(void* pixels_arg) {
    auto pixels = reinterpret_cast<Adafruit_NeoPixel*>(pixels_arg);

    for (;;) {

        // white - halogen
        for (int value = 0; value <= 0xff ; value += 1) {
            pixels->setBrightness(value);
            pixels->setPixelColor(0, pixels->Color(255, 147, 41));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        for (int value = 0xff; value ; value -= 1) {
            pixels->setBrightness(value);
            pixels->setPixelColor(0, pixels->Color(255, 147, 41));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        pixels->setBrightness(0xff);
        // white
        for (int value = 0; value <= 0xff ; value += 1) {
            pixels->setPixelColor(0, pixels->ColorHSV(0, 0, value));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        for (int value = 0xff; value ; value -= 1) {
            pixels->setPixelColor(0, pixels->ColorHSV(0, 0, value));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        // white - halogen
        for (int value = 0; value <= 0xff ; value += 1) {
            pixels->setPixelColor(0, pixels->Color(255, 197, 143));
            pixels->setBrightness(value);
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        for (int value = 0xff; value ; value -= 1) {
            pixels->setPixelColor(0, pixels->Color(255, 197, 143));
            pixels->setBrightness(value);
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        pixels->setBrightness(0xff);
        // Red
        for (int value = 0; value <= 0xff ; value += 1) {
            pixels->setPixelColor(0, pixels->ColorHSV(0, 0xff, value));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        for (int value = 0xff; value ; value -= 1) {
            pixels->setPixelColor(0, pixels->ColorHSV(0, 0xff, value));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        // Green
        for (int value = 0; value <= 0xff ; value += 1) {
            pixels->setPixelColor(0, pixels->ColorHSV(0xffff/3, 0xff, value));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        for (int value = 0xff; value ; value -= 1) {
            pixels->setPixelColor(0, pixels->ColorHSV(0xffff/3, 0xff, value));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        // Blue
        for (int value = 0; value <= 0xff ; value += 1) {
            pixels->setPixelColor(0, pixels->ColorHSV(0xffff*2/3, 0xff, value));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
        for (int value = 0xff; value ; value -= 1) {
            pixels->setPixelColor(0, pixels->ColorHSV(0xffff*2/3, 0xff, value));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }

        // Cycle colors
        for (int hue = 0; hue <= 0xffff ; hue += 0xffff * 10 / 3000) {
            pixels->setPixelColor(0, pixels->ColorHSV(hue, 0xff, 64));
            pixels->show();
            delay(10); // Delay for a period of time (in milliseconds).
        }
    }
    vTaskDelete(NULL);
}

}  // namespace ui
