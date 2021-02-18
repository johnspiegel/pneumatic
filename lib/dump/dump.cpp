#include "dump.h"

#include <cstdio>

namespace dump {

String SecondsHumanReadable(unsigned long ms) {
    int days = ms / (24 * 60 * 60 * 1000);
    ms %= (24 * 60 * 60 * 1000);
    int hours = ms / (60 * 60 * 1000);
    ms %= (60 * 60 * 1000);
    int minutes = ms / (60 * 1000);
    ms %= (60 * 1000);
    int seconds = ms / (1000);
    ms %= (1000);
    if (ms >= 5000) {
      seconds += 1;
    }

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
        snprintf(buf, sizeof(buf), "%02ds", seconds);
    } else {
        snprintf(buf, sizeof(buf), "%ds", seconds);
    }
    str += buf;

    return str;
}

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

}  // namespace dump
