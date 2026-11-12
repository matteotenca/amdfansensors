#include "GpuManager.h"
#include "ADLXHelper.h"
#include <IPerformanceMonitoring2.h>
#include <IPerformanceMonitoring3.h>
#include <stdexcept>
#include <QDebug>
#include <string>
#include <algorithm>
#include <vector>
#include <utility>

GpuManager::GpuManager() {
    ADLX_RESULT res = g_ADLX.Initialize();
    if (ADLX_FAILED(res)) throw std::runtime_error("Failed to initialize ADLX");

    IADLXGPUListPtr gpus;
    res = g_ADLX.GetSystemServices()->GetGPUs(&gpus);
    cleanUp(res);

    res = gpus->At(gpus->Begin(), &m_oneGPU);
    cleanUp(res);

    const char* gpuName;
    res = m_oneGPU->Name(&gpuName);
    cleanUp(res);
    qInfo().noquote() << gpuName << " is selected.";

    // Basic system info is still fetched here as it's cheap and non-ADLX specific
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m_statusInfo.totalSystemRamMB = memInfo.ullTotalPhys / (1024 * 1024);
    }

    adlx_uint totalVramMB = 0;
    res = m_oneGPU->TotalVRAM(&totalVramMB);
    if (ADLX_SUCCEEDED(res)) {
        m_statusInfo.totalGPUMemoryMB = totalVramMB;
    }
}

GpuManager::~GpuManager() = default;

void GpuManager::cleanUp(ADLX_RESULT res, const char* msg) {
    if (ADLX_FAILED(res)) {
        throw std::runtime_error(std::string("ADLX Error: ") + std::to_string(res) + " - " +
            (msg ? msg : "Operation") + " failed");
    }
}

void GpuManager::EnsureFanTuning() {
    if (m_manualFanTuning) return;
    EnsureTuningService();
    IADLXInterfacePtr manualFanTuningIfc;
    ADLX_RESULT res = m_gpuTuningService->GetManualFanTuning(m_oneGPU, &manualFanTuningIfc);
    cleanUp(res, "Get manual fan tuning interface");
    m_manualFanTuning = static_cast<IADLXManualFanTuningPtr>(manualFanTuningIfc);

    adlx_bool isZeroRPM = false;
    if (ADLX_SUCCEEDED(m_manualFanTuning->GetZeroRPMState(&isZeroRPM))) {
        m_statusInfo.zeroRPMEnabled = isZeroRPM;
    }
}

void GpuManager::EnsureGfxTuning() const {
    if (m_manualGFXTuning2) return;
    EnsureTuningService();
    IADLXInterfacePtr manualGFXTuningIfc;
    ADLX_RESULT res = m_gpuTuningService->GetManualGFXTuning(m_oneGPU, &manualGFXTuningIfc);
    cleanUp(res, "Get manual graphics tuning interface");
    m_manualGFXTuning2 = static_cast<IADLXManualGraphicsTuning2Ptr>(manualGFXTuningIfc);

    adlx_int voltage = 0;
    if (ADLX_SUCCEEDED(m_manualGFXTuning2->GetGPUVoltage(&voltage))) {
        m_statusInfo.voltageOffsetMV = voltage;
    }
}

void GpuManager::EnsurePerfServices() const {
    ADLX_RESULT res;
    if (m_perfMonitoringServices == nullptr) {
        res = g_ADLX.GetSystemServices()->GetPerformanceMonitoringServices(&m_perfMonitoringServices);
        cleanUp(res, "Get performance monitoring services");
    }
    if (m_gpuMetricsSupport == nullptr) {
        res = m_perfMonitoringServices->GetSupportedGPUMetrics(m_oneGPU, &m_gpuMetricsSupport);
        cleanUp(res, "Get supported GPU metrics support");
    }
}

void GpuManager::EnsureSystemMetricsSupport() const {
    if (m_systemMetricsSupport) return;
    EnsurePerfServices();
    ADLX_RESULT res = m_perfMonitoringServices->GetSupportedSystemMetrics(&m_systemMetricsSupport);
    cleanUp(res, "Get system metrics support");
}

void GpuManager::EnsureTuningService() const {
    if (m_gpuTuningService) return;
    ADLX_RESULT res = g_ADLX.GetSystemServices()->GetGPUTuningServices(&m_gpuTuningService);
    cleanUp(res, "Get GPU tuning services");
}

IADLXAllMetricsPtr GpuManager::GetCurrentAllMetrics() const {
    EnsurePerfServices();
    IADLXAllMetricsPtr allMetrics;
    ADLX_RESULT res = m_perfMonitoringServices->GetCurrentAllMetrics(&allMetrics);
    cleanUp(res, "Get all metrics for system snapshot");
    return allMetrics;
}

const GpuManager::GpuStatusInfo& GpuManager::GetStatusInfo() const {
    return m_statusInfo;
}

const GpuManager::GlobalStatusInfo& GpuManager::GetTemperatureInfo() const {
    return m_temperatureInfo;
}

void GpuManager::ResetFpsStats() {
    m_temperatureInfo.fpsHistory.reset();
    m_temperatureInfo.maxFPS = 0;
    m_temperatureInfo.minFPS = 0;
    m_temperatureInfo.totalFPS = 0;
    m_temperatureInfo.instantFPS = 0;
    m_temperatureInfo.avgFPS = 0;
    m_temperatureInfo.fullCounter = 0;
}

adlx_int GpuManager::GetVoltageOffset() const {
    EnsureGfxTuning();
    adlx_int voltage = 0;
    if (ADLX_SUCCEEDED(m_manualGFXTuning2->GetGPUVoltage(&voltage))) {
        m_statusInfo.voltageOffsetMV = voltage;
    }
    return m_statusInfo.voltageOffsetMV;
}

void GpuManager::SetVoltageOffset(adlx_int voltage) const {
    EnsureGfxTuning();
    adlx_int currentVolt = 0;
    ADLX_RESULT res = m_manualGFXTuning2->GetGPUVoltage(&currentVolt);
    cleanUp(res);
    qInfo().noquote() << "Current GPU clock voltage offset:" << currentVolt << "mV";

    res = m_manualGFXTuning2->SetGPUVoltage(voltage);
    if (ADLX_RESET_NEEDED == res) {
        res = m_gpuTuningService->ResetToFactory(m_oneGPU);
        if (ADLX_SUCCEEDED(res)) {
            res = m_manualGFXTuning2->SetGPUVoltage(voltage);
        }
    }
    cleanUp(res);
    qInfo().noquote() << "Set GPU voltage offset " << voltage << "mV:" << (ADLX_SUCCEEDED(res) ? "succeeded" : "failed");

    res = m_manualGFXTuning2->GetGPUVoltage(&currentVolt);
    cleanUp(res);
    qInfo().noquote() << "Current GPU clock voltage offset (after set):" << currentVolt << "mV";
    m_statusInfo.voltageOffsetMV = currentVolt;
}

bool GpuManager::IsFanCurveDefaultOne() {
    EnsureFanTuning();
    IADLXManualFanTuningStateListPtr states;
    ADLX_RESULT res = m_manualFanTuning->GetFanTuningStates(&states);
    if (ADLX_FAILED(res)) {
        qInfo().noquote() << "Failed to retrieve fan tuning states for comparison.";
        return true;
    }

    // m_defaultSpeeds/m_defaultTemps hold exactly 5 points; a curve with a different
    // point count cannot match the default and must not be used to index those arrays.
    if (states->Size() != 5) {
        return false;
    }
    for (adlx_uint crt = states->Begin(); crt != states->End(); ++crt) {
        IADLXManualFanTuningStatePtr oneState;
        res = states->At(crt, &oneState);
        if (ADLX_FAILED(res)) continue;

        adlx_int currentSpeed = 0;
        adlx_int currentTemp = 0;
        res = oneState->GetFanSpeed(&currentSpeed);
        cleanUp(res, "Get fan speed for comparison");
        res = oneState->GetTemperature(&currentTemp);
        cleanUp(res, "Get temperature for comparison");

        bool matches = currentSpeed == m_defaultSpeeds[crt] && currentTemp == m_defaultTemps[crt];
        if (!matches) {
            return false;
        }
    }
    return true;
}

std::vector<std::pair<int, int>> GpuManager::GetFanCurvePoints() {
    std::vector<std::pair<int, int>> points;
    try {
        EnsureFanTuning();
        if (!m_manualFanTuning) return points;
        
        IADLXManualFanTuningStateListPtr states;
        ADLX_RESULT res = m_manualFanTuning->GetFanTuningStates(&states);
        if (ADLX_FAILED(res)) return points;
        
        IADLXManualFanTuningStatePtr oneState;
        for (adlx_uint crt = states->Begin(); crt != states->End(); ++crt) {
            res = states->At(crt, &oneState);
            if (ADLX_FAILED(res)) continue;
            
            adlx_int speed = 0, temperature = 0;
            res = oneState->GetFanSpeed(&speed);
            if (ADLX_FAILED(res)) continue;
            res = oneState->GetTemperature(&temperature);
            if (ADLX_FAILED(res)) continue;
            
            points.emplace_back((int)temperature, (int)speed);
        }
    }
    catch (const std::exception& e) {
        qWarning().noquote() << "[GpuManager] GetFanCurvePoints:" << e.what();
    }
    catch (...) {
        qWarning().noquote() << "[GpuManager] GetFanCurvePoints: unknown error";
    }
    return points;
}

bool GpuManager::GetFanRanges(int& minTemp, int& maxTemp, int& minSpeed, int& maxSpeed) const {
    try {
        if (!m_manualFanTuning) {
            GpuManager* self = const_cast<GpuManager*>(this);
            self->EnsureFanTuning();
        }
        if (!m_manualFanTuning) return false;

        ADLX_IntRange fanSpeedRange, fanTemperatureRange;
        ADLX_RESULT res = m_manualFanTuning->GetFanTuningRanges(&fanSpeedRange, &fanTemperatureRange);
        if (ADLX_FAILED(res)) return false;

        minTemp = (int)fanTemperatureRange.minValue;
        maxTemp = (int)fanTemperatureRange.maxValue;
        minSpeed = (int)fanSpeedRange.minValue;
        maxSpeed = (int)fanSpeedRange.maxValue;
        return true;
    }
    catch (const std::exception& e) {
        qWarning().noquote() << "[GpuManager] GetFanRanges:" << e.what();
    }
    catch (...) {
        qWarning().noquote() << "[GpuManager] GetFanRanges: unknown error";
    }
    return false;
}

bool GpuManager::ApplyFanTuningStates(const adlx_int* speeds, const adlx_int* temps, adlx_uint count) {
    EnsureFanTuning();
    ADLX_IntRange fanSpeedRange, fanTemperatureRange;
    ADLX_RESULT res = m_manualFanTuning->GetFanTuningRanges(&fanSpeedRange, &fanTemperatureRange);
    cleanUp(res);

    IADLXManualFanTuningStateListPtr states;
    res = m_manualFanTuning->GetEmptyFanTuningStates(&states);
    if (ADLX_FAILED(res)) {
        qWarning().noquote() << "Failed to get empty fan tuning states";
        return false;
    }

    adlx_uint total = states->Size();
    adlx_uint n = (count < total ? count : total);
    for (adlx_uint crt = 0; crt < n; ++crt) {
        IADLXManualFanTuningStatePtr oneState;
        res = states->At(crt, &oneState);
        if (ADLX_FAILED(res)) continue;

        adlx_int speed = speeds[crt];
        adlx_int temperature = temps[crt];
        if (speed < fanSpeedRange.minValue) speed = fanSpeedRange.minValue;
        if (speed > fanSpeedRange.maxValue) speed = fanSpeedRange.maxValue;
        if (temperature < fanTemperatureRange.minValue) temperature = fanTemperatureRange.minValue;
        if (temperature > fanTemperatureRange.maxValue) temperature = fanTemperatureRange.maxValue;

        res = oneState->SetFanSpeed(speed);
        if (ADLX_RESET_NEEDED == res) {
            qWarning().noquote() << "RESETTING RAMP TO FACTORY!";
            res = m_gpuTuningService->ResetToFactory(m_oneGPU);
            if (ADLX_SUCCEEDED(res)) oneState->SetFanSpeed(speed);
        }
        else if (ADLX_FAILED(res)) {
            qInfo().noquote() << "State " << crt << ": error setting speed";
            return false;
        }

        res = oneState->SetTemperature(temperature);
        if (res == ADLX_FAIL) return false;
    }

    adlx_int errorIndex;
    res = m_manualFanTuning->IsValidFanTuningStates(states, &errorIndex);
    qInfo().noquote() << "IsValidFanTuningStates, errorIndex is:" << errorIndex;
    if (ADLX_SUCCEEDED(res)) {
        res = m_manualFanTuning->SetFanTuningStates(states);
        return ADLX_SUCCEEDED(res);
    }
    return false;
}

bool GpuManager::SetFanCurvePoints(const std::vector<std::pair<int, int>>& points) {
    try {
        if (points.empty()) return false;

        std::vector<adlx_int> speeds(points.size());
        std::vector<adlx_int> temps(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            speeds[i] = (adlx_int)points[i].second;
            temps[i] = (adlx_int)points[i].first;
        }
        return ApplyFanTuningStates(speeds.data(), temps.data(), (adlx_uint)points.size());
    }
    catch (const std::exception& e) {
        qWarning().noquote() << "[GpuManager] SetFanCurvePoints:" << e.what();
    }
    catch (...) {
        qWarning().noquote() << "[GpuManager] SetFanCurvePoints: unknown error";
    }
    return false;
}

int GpuManager::GetFanTuningSlotCount() {
    EnsureFanTuning();
    if (!m_manualFanTuning) return 0;

    IADLXManualFanTuningStateListPtr states;
    ADLX_RESULT res = m_manualFanTuning->GetEmptyFanTuningStates(&states);
    if (ADLX_FAILED(res)) {
        qWarning().noquote() << "[GpuManager] GetFanTuningSlotCount: failed to get empty fan tuning states";
        return 0;
    }
    return states->Size();
}

bool GpuManager::IsDefaultFanCurve(const std::vector<std::pair<int, int>>& points) const {
    constexpr int pointCount = sizeof(m_defaultSpeeds) / sizeof(m_defaultSpeeds[0]);
    if ((int)points.size() != pointCount) return false;
    for (int i = 0; i < pointCount; ++i) {
        if (points[i].first != m_defaultTemps[i] || points[i].second != m_defaultSpeeds[i])
            return false;
    }
    return true;
}

std::vector<std::pair<int, int>> GpuManager::GetDefaultFanCurvePoints() const {
    constexpr int pointCount = sizeof(m_defaultSpeeds) / sizeof(m_defaultSpeeds[0]);
    std::vector<std::pair<int, int>> points;
    points.reserve(pointCount);
    for (int i = 0; i < pointCount; ++i) {
        points.emplace_back(m_defaultTemps[i], m_defaultSpeeds[i]);
    }
    return points;
}

void GpuManager::UpdateGpuFpsMetrics() {
    IADLXFPSPtr oneFPS;
    adlx_int fpsData = 0;

    EnsurePerfServices();
    IADLXAllMetricsPtr allMetrics = GetCurrentAllMetrics();
    try {
        ADLX_RESULT res = allMetrics->GetFPS(&oneFPS);
        if (ADLX_SUCCEEDED(res)) {
            res = oneFPS->FPS(&fpsData);
            if (ADLX_SUCCEEDED(res)) {
                if (fpsData < 9999) {
                    if (m_temperatureInfo.fpsHistory.isFull()) {
                        adlx_int oldestFPS = 0;
                        oldestFPS = m_temperatureInfo.fpsHistory.front();
                        m_temperatureInfo.totalFPS -= oldestFPS;
                    }
                    m_temperatureInfo.fpsHistory.push_back(fpsData);
                    m_temperatureInfo.totalFPS += fpsData;
                    m_temperatureInfo.instantFPS = fpsData;

                    if (m_temperatureInfo.fpsHistory.size() == 1) {
                        m_temperatureInfo.minFPS = fpsData;
                        m_temperatureInfo.maxFPS = fpsData;
                    }
                    else {
                        m_temperatureInfo.minFPS = std::min(m_temperatureInfo.minFPS, fpsData);
                        m_temperatureInfo.maxFPS = std::max(m_temperatureInfo.maxFPS, fpsData);
                    }
                    m_temperatureInfo.avgFPS =
                        m_temperatureInfo.fpsHistory.size() > 0
                                                   ? m_temperatureInfo.totalFPS / m_temperatureInfo.fpsHistory. size()
                                                   : 0;
                    m_temperatureInfo.fullCounter = m_temperatureInfo.fpsHistory.getCompletedRings();
                }
            } else if (res == ADLX_NOT_SUPPORTED) {
                if (m_temperatureInfo.totalFPS > 0) {
                    m_temperatureInfo.totalUnattended += 1;
                    m_temperatureInfo.fpsHistory.reset();
                    m_temperatureInfo.totalFPS = 0;
                    m_temperatureInfo.instantFPS = 0;
                    m_temperatureInfo.minFPS = 0;
                    m_temperatureInfo.maxFPS = 0;
                    m_temperatureInfo.fullCounter = 0;
                }
            }
        }

    }
    catch (const std::exception& e) {
        qWarning().noquote() << "Fatal Error in UpdateGpuFpsMetrics():" << e.what();
        throw;
    }
}

void GpuManager::UpdateGpuHardwareMetrics() const {
    adlx_bool supported = false;

    EnsurePerfServices();
    IADLXAllMetricsPtr allMetrics = GetCurrentAllMetrics();
    try {
        IADLXGPUMetricsPtr gpuMetrics;
        if (ADLX_SUCCEEDED(allMetrics->GetGPUMetrics(m_oneGPU, &gpuMetrics))) {
            const IADLXGPUMetrics1Ptr gpuMetrics1(gpuMetrics);
            const IADLXGPUMetricsSupport1Ptr gpuMetricsSupport1(m_gpuMetricsSupport);
            const IADLXGPUMetricsSupport3Ptr gpuMetricsSupport3(m_gpuMetricsSupport);
            const IADLXGPUMetrics3Ptr gpuMetrics3(gpuMetrics);

            adlx_double temperature = 0, gpuUsage = 0, gpuPower = 0.0;
            adlx_int fanSpeed = 0, vRam = 0, speedPercent = 0;

            // Every metric is checked and read independently, so a GPU lacking one
            // capability still updates all the remaining ones.
            if (ADLX_SUCCEEDED(m_gpuMetricsSupport->IsSupportedGPUTemperature(&supported)) && supported) {
                if (ADLX_SUCCEEDED(gpuMetrics1->GPUTemperature(&temperature)))
                    m_temperatureInfo.gpuTemperatureC = temperature;
            }
            if (ADLX_SUCCEEDED(m_gpuMetricsSupport->IsSupportedGPUHotspotTemperature(&supported)) && supported) {
                if (ADLX_SUCCEEDED(gpuMetrics1->GPUHotspotTemperature(&temperature)))
                    m_temperatureInfo.gpuHotspotTemperatureC = temperature;
            }
            if (ADLX_SUCCEEDED(gpuMetricsSupport1->IsSupportedGPUMemoryTemperature(&supported)) && supported) {
                if (ADLX_SUCCEEDED(gpuMetrics1->GPUMemoryTemperature(&temperature)))
                    m_temperatureInfo.gpuMemoryTemperatureC = temperature;
            }
            if (ADLX_SUCCEEDED(m_gpuMetricsSupport->IsSupportedGPUFanSpeed(&supported)) && supported) {
                if (ADLX_SUCCEEDED(gpuMetrics1->GPUFanSpeed(&fanSpeed)))
                    m_temperatureInfo.fanSpeed = fanSpeed;
            }
            if (ADLX_SUCCEEDED(m_gpuMetricsSupport->IsSupportedGPUVRAM(&supported)) && supported) {
                if (ADLX_SUCCEEDED(gpuMetrics1->GPUVRAM(&vRam)))
                    m_temperatureInfo.vRam = vRam;
            }
            if (ADLX_SUCCEEDED(gpuMetricsSupport3->IsSupportedGPUFanDuty(&supported)) && supported) {
                if (ADLX_SUCCEEDED(gpuMetrics3->GPUFanDuty(&speedPercent)))
                    m_temperatureInfo.fanPercent = speedPercent;
            }
            if (ADLX_SUCCEEDED(m_gpuMetricsSupport->IsSupportedGPUUsage(&supported)) && supported) {
                if (ADLX_SUCCEEDED(gpuMetrics->GPUUsage(&gpuUsage)))
                    m_temperatureInfo.gpuUsage = gpuUsage;
            }
            if (ADLX_SUCCEEDED(m_gpuMetricsSupport->IsSupportedGPUTotalBoardPower(&supported)) && supported) {
                if (ADLX_SUCCEEDED(gpuMetrics->GPUTotalBoardPower(&gpuPower)))
                    m_temperatureInfo.gpuPower = gpuPower;
            }
        }

        // System RAM is read from the same metrics snapshot - no second ADLX fetch
        IADLXSystemMetricsPtr systemMetrics;
        if (ADLX_SUCCEEDED(allMetrics->GetSystemMetrics(&systemMetrics))) {
            EnsureSystemMetricsSupport();
            adlx_bool supported = false;
            adlx_int ramUsage = 0;
            if (ADLX_SUCCEEDED(m_systemMetricsSupport->IsSupportedSystemRAM(&supported)) && supported) {
                if (ADLX_SUCCEEDED(systemMetrics->SystemRAM(&ramUsage)))
                    m_temperatureInfo.usedSystemRam = ramUsage;
            } else {
                static bool warned = false;
                if (!warned) {
                    warned = true;
                    qInfo().noquote() << "Reading system RAM is not supported.";
                }
            }
        }
    }
    catch (const std::exception& e) {
        qWarning().noquote() << "Fatal Error in UpdateGpuHardwareMetrics():" << e.what();
        throw;
    }
}

void GpuManager::UpdateGpuTempStruct(bool fpsOnly) {
    if (fpsOnly) {
        UpdateGpuFpsMetrics();
    } else {
        UpdateGpuHardwareMetrics();
    }
}

bool GpuManager::IsZeroRPMSupported() {
    try {
        EnsureFanTuning();
        if (!m_manualFanTuning) return false;
        adlx_bool supported = false;
        ADLX_RESULT res = m_manualFanTuning->IsSupportedZeroRPM(&supported);
        return ADLX_SUCCEEDED(res) && supported;
    }
    catch (const std::exception& e) {
        qWarning().noquote() << "[GpuManager] IsZeroRPMSupported:" << e.what();
    }
    catch (...) {
        qWarning().noquote() << "[GpuManager] IsZeroRPMSupported: unknown error";
    }
    return false;
}

bool GpuManager::GetZeroRPMState() {
    try {
        EnsureFanTuning();
        if (!m_manualFanTuning) return false;
        adlx_bool isZeroRPMStateSet = false;
        if (ADLX_SUCCEEDED(m_manualFanTuning->GetZeroRPMState(&isZeroRPMStateSet))) {
            m_statusInfo.zeroRPMEnabled = isZeroRPMStateSet;
        }
        return m_statusInfo.zeroRPMEnabled;
    }
    catch (const std::exception& e) {
        qWarning().noquote() << "[GpuManager] GetZeroRPMState:" << e.what();
    }
    catch (...) {
        qWarning().noquote() << "[GpuManager] GetZeroRPMState: unknown error";
    }
    return false;
}

void GpuManager::SetZeroRPM(bool enable) {
    try {
        EnsureFanTuning();
        if (!m_manualFanTuning) return;
        adlx_bool supported = false;
        if (ADLX_SUCCEEDED(m_manualFanTuning->IsSupportedZeroRPM(&supported)) && supported) {
            if (ADLX_SUCCEEDED(m_manualFanTuning->SetZeroRPMState(enable))) {
                m_statusInfo.zeroRPMEnabled = enable;
            }
        }
    }
    catch (const std::exception& e) {
        qWarning().noquote() << "[GpuManager] SetZeroRPM:" << e.what();
    }
    catch (...) {
        qWarning().noquote() << "[GpuManager] SetZeroRPM: unknown error";
    }
}

bool GpuManager::GetVoltageOffsetRange(adlx_int& minVolt, adlx_int& maxVolt) const {
    try {
        EnsureGfxTuning();
        if (!m_manualGFXTuning2) return false;
        ADLX_IntRange voltRange{};
        ADLX_RESULT res = m_manualGFXTuning2->GetGPUVoltageRange(&voltRange);
        if (ADLX_SUCCEEDED(res)) {
            minVolt = voltRange.minValue;
            maxVolt = voltRange.maxValue;
            return true;
        }
    }
    catch (const std::exception& e) {
        qWarning().noquote() << "[GpuManager] GetVoltageOffsetRange:" << e.what();
    }
    catch (...) {
        qWarning().noquote() << "[GpuManager] GetVoltageOffsetRange: unknown error";
    }
    return false;
}
