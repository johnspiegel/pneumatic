#ifndef _UI_H_
#define _UI_H_

#include <stdint.h>
#include <Adafruit_NeoPixel.h>

#include "bme280.h"
#include "dsco220.h"
#include "mhz19.h"
#include "pmsx003.h"

namespace ui {

struct TaskData {
    pmsx003::TaskData*  pmsx003_data;
    mhz19::TaskData*    mhz19_data;
    dsco220::Data*      dsco220_data;
    bme280::Data*       bme280_data;
    Adafruit_NeoPixel*  pixels;
};

String MillisHumanReadable(unsigned long ms);

float CToF(float temp_C);

float usAQI(int val);

const char* aqiClass(int aqi);

const char* aqiMessage(int aqi);

const char* co2Class(int co2_ppm);

void TaskServeWeb(void* unused);

void TaskDoPixels(void* unused);

void TaskStrobePixels(void* pixels_arg);

}  // namespace ui

#endif  // _UI_H_
