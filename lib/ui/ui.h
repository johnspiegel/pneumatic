#ifndef _UI_H_
#define _UI_H_

#include <Adafruit_NeoPixel.h>
#include <stdint.h>

#include "bme.h"
#include "dsco220.h"
#include "mhz19.h"
#include "pmsx003.h"

namespace ui {

struct TaskData {
  pmsx003::TaskData* pmsx003_data;
  mhz19::TaskData* mhz19_data;
  dsco220::Data* dsco220_data;
  bme::Data* bme_data;
  Adafruit_NeoPixel* pixels;
};

void InitTft(void);

const char* aqiClass(int aqi);

const char* aqiMessage(int aqi);

const char* co2Class(int co2_ppm);

void TaskButtons(void* task_data_arg);

void TaskDisplay(void* task_data_arg);

void TaskServeWeb(void* unused);

void TaskDoPixels(void* unused);

void TaskStrobePixels(void* pixels_arg);

}  // namespace ui

#endif  // _UI_H_
