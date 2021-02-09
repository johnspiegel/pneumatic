/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include "mhz19.h"

#include "Arduino.h"

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

bool Read(Stream* serial, Data* data, unsigned long timeout_ms) {
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

} // namespace mhz19
