/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include "pmsx003.h"

#include "Arduino.h"

namespace pmsx003 {

bool VerifyPacket(uint8_t* packet, int size) {
	if (size < 6) {
		Serial.print("ERROR Invalid Packet Size!\n");
		return false;
	}

	// First two magic bytes: 0x42 0x4d
	if (packet[0] != 0x42 || packet[1] != 0x4d) {
		Serial.print("Invalid packet, unxpected magic bytes: ");
		Serial.print(packet[0], HEX);
		Serial.print(" ");
		Serial.print(packet[1], HEX);
		Serial.print("\n");
		return false;
	}

	// remaining bytes (does not includ magic, or length)
	uint16_t frameSize = (packet[2] << 8) | packet[3];
	if ((frameSize + 4) > size) {
		Serial.print("Invalid packet, too long! Frame length: ");
		Serial.print(frameSize);
		Serial.print("\n");
		return false;
	} else if ((frameSize + 4) < size) {
		Serial.print("info: frame length+6 < buffer size: ");
		Serial.print(frameSize);
		Serial.print("\n");
	}

	uint16_t checksumOffset = 4 + frameSize - 2;
	uint16_t checksum = 0;
	for (int i = 0; i < checksumOffset; ++i) {
		checksum += packet[i];
	}
	if (checksum != ((packet[checksumOffset] << 8) | packet[checksumOffset+1])) {
		Serial.print("Invalid packet, unxpected checksum, calculated: ");
		Serial.print(checksum >> 8, HEX);
		Serial.print(" ");
		Serial.print(checksum & 0xff, HEX);
		Serial.print(", got: ");
		Serial.print(packet[checksumOffset], HEX);
		Serial.print(" ");
		Serial.print(packet[checksumOffset+1], HEX);
		Serial.print("\n");
		return false;
	}

	// All good!
	return true;
}

bool Read(Stream* serial, PmsData* data, unsigned long timeout_ms) {
    unsigned long start_time_ms = millis();

    // Get to packet start byte
    while (true) {
        int c = serial->peek();
        if (c == 0x42) {
            break;
        }
        if (millis() - start_time_ms >= timeout_ms) {
            Serial.print("ERROR: timed out waiting for PMSx003 data\n");
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

    const int kPacketSize = 32;
    uint8_t buffer[kPacketSize] = {0};
    while (serial->available() < kPacketSize) {
        if (millis() - start_time_ms >= timeout_ms) {
            Serial.print("ERROR: timed out waiting for PMSx003 data\n");
            return false;
        }
        delay(10);
    }
    if (serial->readBytes(buffer, kPacketSize) < kPacketSize) {
        Serial.print("WTF: did not read enough bytes");
        return false;
    }

	/*
    Serial.print("\nPMSx003 data:\n");
    Serial.print("  Raw: ");
    int val;
    for (int i = 0; i+1 < kPacketSize; i += 2) {
        // 16-bit big-endian values.
        val = (buffer[i] << 8) | buffer[i+1];
        Serial.print(val);
        Serial.print(" ");
    }
    Serial.println("");
	*/

    if (!VerifyPacket(buffer, sizeof(buffer))) {
        Serial.print("ERROR: Packet error!\n");
        return false;
    }

    data->pm1Raw = (buffer[4] << 8) | buffer[5];
    data->pm25Raw = (buffer[6] << 8) | buffer[7];
    data->pm10Raw = (buffer[8] << 8) | buffer[9];

    data->pm1 = (buffer[10] << 8) | buffer[11];
    data->pm25 = (buffer[12] << 8) | buffer[13];
    data->pm10 = (buffer[14] << 8) | buffer[15];

    data->particles_gt_0_3 = (buffer[16] << 8) | buffer[17];
    data->particles_gt_0_5 = (buffer[18] << 8) | buffer[19];
    data->particles_gt_1_0 = (buffer[20] << 8) | buffer[21];
    data->particles_gt_2_5 = (buffer[22] << 8) | buffer[23];
    data->particles_gt_5_0 = (buffer[24] << 8) | buffer[25];
    data->particles_gt_10_0 = (buffer[26] << 8) | buffer[27];

	/*
    Serial.print("  [ug/m^3] PM1.0: ");
    Serial.print(data->pm1);
    Serial.print("  PM2.5: ");
    Serial.print(data->pm25);
    Serial.print("  PM10: ");
    Serial.print(data->pm10);
    Serial.println();
	*/

    return true;
}

} // namespace pmsx003
