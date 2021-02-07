#ifndef PMSX003_H
#define PMSX003_H

/**
 */
#include "Stream.h"

namespace pmsx003 {

struct PmsData {
	uint16_t pm1Raw;
	uint16_t pm25Raw;
	uint16_t pm10Raw;

	uint16_t pm1;
	uint16_t pm25;
	uint16_t pm10;

	uint16_t particles_gt_0_3;
	uint16_t particles_gt_0_5;
	uint16_t particles_gt_1_0;
	uint16_t particles_gt_2_5;
	uint16_t particles_gt_5_0;
	uint16_t particles_gt_10_0;
};

bool VerifyPacket(uint8_t* packet, int size);

bool Read(Stream* serial, PmsData* data, unsigned long timeout_ms=5000);

} // namespace pmsx003

#endif  // PMSX003_H
