
#include "dsco220.h"

#include <Arduino.h>

#include "pmsx003.h"

namespace dsco220 {

bool Read(TwoWire* i2c, Data* data) {

    while (i2c->available()) {
        Serial.print("WTF DS-CO2-20 available before: ");
        Serial.println(i2c->read(), HEX);
    }

    const int kPacketSize = 12;

    i2c->requestFrom(0x08, kPacketSize);

    unsigned long timeout_ms = 2000;
    unsigned long start_time_ms = millis();
    while (i2c->available() < kPacketSize && (millis() - start_time_ms) < timeout_ms) {
        delay(10);
    }
    if (i2c->available() < kPacketSize) {
        Serial.print("DS ERROR, expect 12 bytes available, have: ");
        Serial.println(i2c->available());
        return false;
    }

    uint8_t buffer[kPacketSize] = {0};
    i2c->readBytes(buffer, sizeof(buffer));

    if (!pmsx003::VerifyPacket(buffer, sizeof(buffer))) {
        Serial.print("ERROR Packet error!\n");
        Serial.print("  Raw: ");
        for (int i = 0; i < 12; ++i) {
            Serial.print(buffer[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        return false;
    }

    data->co2_ppm = (buffer[4] << 8) | buffer[5];
    data->calibration_param1 = (buffer[6] << 8) | buffer[7];
    data->calibration_param2 = (buffer[8] << 8) | buffer[9];

    return true;
}

bool Read(Stream* serial, Data* data) {
    unsigned long start_time_ms = millis();
    unsigned long timeout_ms = 2000;

    while (serial->available()) {
        Serial.print("WTF DS-CO2-20 available before: ");
        Serial.println(serial->read(), HEX);
    }

    uint8_t cmd[7] = {0x42, 0x4d, 0xe3, 0x00, 0x00, 0x01, 0x72};
    serial->write(cmd, sizeof(cmd));

    // Get to packet start byte
    while (true) {
        int c = serial->peek();
        if (c == 0x42) {
            break;
        }
        if (millis() - start_time_ms >= timeout_ms) {
            Serial.print("ERROR: timed out waiting for DS-CO2-20 data\n");
            return false;
        }
        if (c == -1) {
            // Nothing there.
            delay(10);
            continue;
        } else {
            Serial.print("Waiting for 0x42, got: 0x");
            Serial.println(c, HEX);
            Serial.println(serial->read(), HEX);
            continue;
        }
    }

    const int kPacketSize = 12;
    uint8_t buffer[kPacketSize] = {0};
    while (serial->available() < kPacketSize) {
        if (millis() - start_time_ms >= timeout_ms) {
            Serial.print("ERROR: timed out waiting for DS-CO2-20 data\n");
            return false;
        }
        delay(10);
    }
    if (serial->readBytes(buffer, kPacketSize) < kPacketSize) {
        Serial.print("WTF: did not read enough bytes");
        return false;
    }

    if (!pmsx003::VerifyPacket(buffer, sizeof(buffer))) {
        Serial.print("ERROR Packet error!\n");
        Serial.print("  Raw: ");
        for (int i = 0; i < 12; ++i) {
            Serial.print(buffer[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        return false;
    }

    data->co2_ppm = (buffer[4] << 8) | buffer[5];
    data->calibration_param1 = (buffer[6] << 8) | buffer[7];
    data->calibration_param2 = (buffer[8] << 8) | buffer[9];

    return true;
}

void TaskPollDsCo2(void* task_data_arg) {
    SemaphoreHandle_t i2c_mutex = reinterpret_cast<TaskData*>(task_data_arg)->i2c_mutex;
    TwoWire* i2c = reinterpret_cast<TaskData*>(task_data_arg)->i2c;
    Data* data = reinterpret_cast<TaskData*>(task_data_arg)->data;

    // int dummy_co2ppm = 0;
    for (;;) {
        bool success = false;
        if (xSemaphoreTake(i2c_mutex, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            success = Read(i2c, data);
            xSemaphoreGive(i2c_mutex);
        } else {
            Serial.print("ERROR: DS-CO2-20 failed to acquire i2c mutex\n");
            continue;
        }
        /*
        data->co2_ppm = dummy_co2ppm;
        if (dummy_co2ppm < 1000) {
            dummy_co2ppm += 100;
        } else {
            dummy_co2ppm += 500;
        }
        dummy_co2ppm %= 8000;
        */

        if (!success) {
            Serial.print("DS ERROR");
            delay(5000);
            continue;
        }

        Serial.print("TaskPollDsCo2(): core: ");
        Serial.println(xPortGetCoreID());
        Serial.print("DS-CO2-20 Results:");
        Serial.print("  CO2: ");
        Serial.print(data->co2_ppm);
        Serial.print("ppm ");
        Serial.print("\n");


        delay(1000);
    }

    vTaskDelete(NULL);
}

}  // namespace dsco220
