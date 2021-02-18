#ifndef _DUMP_H_
#define _DUMP_H_

#include <WString.h>

namespace dump {

String SecondsHumanReadable(unsigned long ms);
String MillisHumanReadable(unsigned long ms);

float CToF(float temp_C);

}  // namespace dump

#endif  // _DUMP_H_
