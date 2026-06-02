#ifndef AMDFANSENSORS_AMDFANSENSORS_H
#define AMDFANSENSORS_AMDFANSENSORS_H
#pragma once
#include "GpuManager.h"
bool validateRefreshRate(int& refreshRateMs, std::string arg);
bool validateVoltageOffset(adlx_int& voltageOffset, std::string arg);
#endif //AMDFANSENSORS_AMDFANSENSORS_H