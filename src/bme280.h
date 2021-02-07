#ifndef _BME280_H_
#define _BME280_H_

namespace bme280 {

struct Data {
    float temp_c;
    float pressurePa;
    float humidityPercent;
};

}  // namespace bme280
#endif  // _BME280_H_
