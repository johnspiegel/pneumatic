#include "mhz19.h"

#include "Arduino.h"

#include "ui.h"

namespace mhz19 {

uint8_t Checksum(uint8_t *buf, int size) {
    int8_t sum = 0;
    // Skip first byte (0xff) and last byte (checksum)
    for (int i = 1; i < size-1; ++i) {
        sum += buf[i];
    }
    return 0xff - sum + 1;
}

bool ReadPacket(Stream* serial, uint8_t* buffer, int size, uint8_t cmd, unsigned long timeout_ms) {
    unsigned long start_ms = millis();

    while (serial->available() < size) {
        if (millis() - start_ms > timeout_ms) {
            Serial.print("ERROR: MH-Z19 timed out waiting for response");
            Serial.print("ERROR: MH-Z19 expect 9 bytes available, have: ");
            Serial.println(serial->available());
            return false;
        }
        delay(10);
    }

    if (serial->readBytes(buffer, size) != size) {
        Serial.print("ERROR: MH-Z19: didn't read enough bytes\n");
        return false;
    }


    if (buffer[0] != 0xff || buffer[1] != cmd || buffer[size-1] != Checksum(buffer, size)) {
        Serial.print("ERROR: MH-Z19: invalid response\n");

        Serial.print("  Raw: ");
        for (int i = 0; i < size; ++i) {
            Serial.print(buffer[i], HEX);
            Serial.print(" ");
        }
        Serial.println();

        Serial.print("Expected Checksum: ");
        Serial.println(Checksum(buffer, size), HEX);
        return false;
    }

    return true;
}

bool SetAutoBackgroundCalibration(Stream* serial, bool abc_on, unsigned long timeout_ms) {
    unsigned long start_ms = millis();

    while (serial->available()) {
        Serial.print("WARNING: MH-Z19 ABC wtf byte: ");
        Serial.println(serial->read(), HEX);
    }

    uint8_t buffer[9] = {0xff, 0x01, 0x79,
                      static_cast<uint8_t>(abc_on ? 0xa0 : 0x00),
                      0x00, 0x00, 0x00, 0x00, 0x00};
    buffer[8] = Checksum(buffer, sizeof(buffer));
    for (int written = 0; written < sizeof(buffer); ) {
        written += serial->write(buffer + written, sizeof(buffer) - written);
        if (millis() - start_ms > timeout_ms) {
            Serial.print("ERROR: MH-Z19: ABC timed out writing command\n");
            return false;
        }
    }

    unsigned long elapsed = millis() - start_ms;
    if (timeout_ms > elapsed) {
        timeout_ms = timeout_ms - elapsed;
    } else {
        timeout_ms = 0;
    }
    return ReadPacket(serial, buffer, sizeof(buffer), buffer[2], timeout_ms);
}

bool Read(Stream* serial, TaskData* data, unsigned long timeout_ms) {
    unsigned long start_ms = millis();

    while (serial->available()) {
        Serial.print("WARNING: MH-Z19 Read wtf byte: ");
        Serial.println(static_cast<uint8_t>(serial->read()), HEX);
    }

    uint8_t buffer[9] = {0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    for (int written = 0; written < sizeof(buffer); ) {
        written += serial->write(buffer + written, sizeof(buffer) - written);
        if (millis() - start_ms > timeout_ms) {
            Serial.print("ERROR: MH-Z19 Read timed out writing command\n");
            return false;
        }
    }

    if (!ReadPacket(serial, buffer, sizeof(buffer), buffer[2], timeout_ms)) {
        Serial.print("ERROR: MH-Z19: error reading packet\n");
        return false;
    }

    data->co2_ppm = (buffer[2] << 8) | buffer[3];
    data->temp_c = buffer[4] - 40;

    return true;
}

void TaskPoll(void* task_data_param) {
    auto* task_data = reinterpret_cast<TaskData*>(task_data_param);

    unsigned long last_print_time_ms = 0;
    for (;;) {
        if (!mhz19::Read(task_data->serial, task_data, /*timeout_ms=*/ 5000)) {
            Serial.print("ERROR: failed to read MHZ-19c sensor");
            delay(1000);
            continue;
        }

        if ((millis() - last_print_time_ms) < 10 * 60 * 1000 && last_print_time_ms) {
            delay(2000);
            continue;
        }
        last_print_time_ms = millis();


        Serial.print("PollMhz19(): core: ");
        Serial.println(xPortGetCoreID());
        Serial.print("MH-Z19c Results: ");
        /*
        long now_ms = millis();
        if (last_read_time_ms != -1) {
            Serial.print("  [Elapsed: ");
            Serial.print(now_ms - last_read_time_ms);
            Serial.print(" ms]\n");
        }
        last_read_time_ms = now_ms;
        */
        Serial.print("  CO2: ");
        Serial.print(task_data->co2_ppm);
        Serial.print("ppm");
        Serial.print("  Temp: ");
        Serial.print(task_data->temp_c);
        Serial.print(" °C ");
        Serial.print(ui::CToF(task_data->temp_c));
        Serial.print(" °F\n");
        delay(2000);
    }

    vTaskDelete(NULL);
}


} // namespace mhz19
