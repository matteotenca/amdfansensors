// ReSharper disable CppDFANullDereference
#include "GpuManager.h"
#include "ADLXHelper.h"
#include <IPerformanceMonitoring2.h>
#include <IPerformanceMonitoring3.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <algorithm>

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
    std::cout << gpuName << " is selected." << std::endl;

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
    if (m_perfMonitoringServices) return;
    ADLX_RESULT res = g_ADLX.GetSystemServices()->GetPerformanceMonitoringServices(&m_perfMonitoringServices);
    cleanUp(res, "Get performance monitoring services");
    res = m_perfMonitoringServices->GetSupportedGPUMetrics(m_oneGPU, &m_gpuMetricsSupport);
    cleanUp(res, "Get supported GPU metrics support");
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

IADLXGPUMetricsPtr GpuManager::GetCurrentGpuMetrics() const {
    IADLXAllMetricsPtr allMetrics = GetCurrentAllMetrics();
    IADLXGPUMetricsPtr gpuMetrics;
    ADLX_RESULT res = allMetrics->GetGPUMetrics(m_oneGPU, &gpuMetrics);
    cleanUp(res, "Get GPU metrics snapshot");
    return gpuMetrics;
}

IADLXSystemMetricsPtr GpuManager::GetCurrentSystemMetrics() const {
    // if (m_systemMetrics) return;
    EnsurePerfServices();
    IADLXAllMetricsPtr allMetrics = GetCurrentAllMetrics();
    // ADLX_RESULT res = m_perfMonitoringServices->GetCurrentAllMetrics(&allMetrics);
    // cleanUp(res, "Get all metrics for system snapshot");
    IADLXSystemMetricsPtr systemMetrics;
    ADLX_RESULT res = allMetrics->GetSystemMetrics(&systemMetrics);
    cleanUp(res, "Get system metrics snapshot");
    return systemMetrics;
}

adlx_int GpuManager::GetFanPercent() const {
    EnsurePerfServices();
    IADLXGPUMetricsPtr gpuMetrics = GetCurrentGpuMetrics();
    IADLXGPUMetricsSupport3Ptr gpuMetricsSupport3(m_gpuMetricsSupport);
    IADLXGPUMetrics3Ptr gpuMetrics3(gpuMetrics);
    adlx_int speedPercent = 0;
    ADLX_RESULT res = gpuMetrics3->GPUFanDuty(&speedPercent);
    cleanUp(res, "Get Fan Percent");
    return speedPercent;
}

adlx_double GpuManager::GetGPUUsage() const {
    IADLXGPUMetricsPtr gpuMetrics = GetCurrentGpuMetrics();
    adlx_double usage = 0;
    ADLX_RESULT res = gpuMetrics->GPUUsage(&usage);
    cleanUp(res, "Get GPU usage");
    return usage;
}

GpuManager::GpuStatusInfo GpuManager::GetStatusInfo() const {
    return m_statusInfo;
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
    std::cout << "Current GPU clock voltage offset: " << currentVolt << " mV" << std::endl;

    if (voltage != 0) {
        res = m_manualGFXTuning2->SetGPUVoltage(voltage);
        if (ADLX_RESET_NEEDED == res) {
            res = m_gpuTuningService->ResetToFactory(m_oneGPU);
            if (ADLX_SUCCEEDED(res)) {
                res = m_manualGFXTuning2->SetGPUVoltage(voltage);
            }
        }
        cleanUp(res);
        std::cout << "Set GPU voltage offset " << voltage << " mV: " << (ADLX_SUCCEEDED(res) ? "succeeded" : "failed")
            << std::endl;

        res = m_manualGFXTuning2->GetGPUVoltage(&currentVolt);
        cleanUp(res);
        std::cout << "Current GPU clock voltage offset (after set): " << currentVolt << " mV" << std::endl;
        m_statusInfo.voltageOffsetMV = currentVolt;
    }
}

void GpuManager::SetVoltageOffsetFromCLI(adlx_int voltage) const {
    EnsureGfxTuning();
    if (voltage == 0) {
        std::cout << "Voltage offset was not specified or set to 0 (no change)." << std::endl;
        return;
    }

    adlx_int currentVolt = 0;
    ADLX_RESULT res = m_manualGFXTuning2->GetGPUVoltage(&currentVolt);
    if (ADLX_FAILED(res)) {
        std::cout << "Failed to check GPU voltage support." << std::endl;
        return;
    }

    SetVoltageOffset(voltage);
}

void GpuManager::ShowFanPercent() const {
    adlx_int fanSpeedPercent = GetFanPercent();
    std::cout << "CPU usage: " << fanSpeedPercent << "%" << std::endl;
}

void GpuManager::ShowGetAndSetFan(bool setRamp, bool defaultRamp) {
    EnsureFanTuning();
    ADLX_IntRange fanSpeedRange, fanTemperatureRange;
    ADLX_RESULT res = m_manualFanTuning->GetFanTuningRanges(&fanSpeedRange, &fanTemperatureRange);
    cleanUp(res);

    std::cout << "Fan speed range is: (" << fanSpeedRange.minValue << ", " << fanSpeedRange.maxValue << ")" <<
        std::endl;
    std::cout << "Fan temperature range is: (" << fanTemperatureRange.minValue << ", " << fanTemperatureRange.maxValue
        << ")" << std::endl;

    IADLXManualFanTuningStateListPtr states;
    IADLXManualFanTuningStatePtr oneState;
    res = m_manualFanTuning->GetFanTuningStates(&states);
    if (ADLX_SUCCEEDED(res)) {
        for (adlx_uint crt = states->Begin(); crt != states->End(); ++crt) {
            res = states->At(crt, &oneState);
            cleanUp(res);
            adlx_int speed = 0, temperature = 0;
            oneState->GetFanSpeed(&speed);
            oneState->GetTemperature(&temperature);
            std::cout << "The current " << crt << " state: speed is " << speed << " temperature is " << temperature <<
                std::endl;
        }
    }

    const int default_speeds[5] = {30, 44, 58, 72, 100};
    const int default_temps[5] = {30, 50, 56, 63, 71};
    const int my_speeds[5] = {30, 30, 60, 85, 100};
    const int my_temps[5] = {25, 40, 50, 60, 65};
    const int* speeds = defaultRamp ? default_speeds : my_speeds;
    const int* temps = defaultRamp ? default_temps : my_temps;

    if (setRamp) {
        res = m_manualFanTuning->GetEmptyFanTuningStates(&states);
        if (ADLX_SUCCEEDED(res)) {
            for (adlx_uint crt = states->Begin(); crt != states->End(); ++crt) {
                res = states->At(crt, &oneState);
                cleanUp(res);
                adlx_int speed = 0, temperature = 0;
                res = oneState->SetFanSpeed(speeds[crt]);

                if (ADLX_RESET_NEEDED == res) {
                    std::cerr << "RESETTING RAMP TO FACTORY!" << std::endl;
                    res = m_gpuTuningService->ResetToFactory(m_oneGPU);
                    if (ADLX_SUCCEEDED(res)) oneState->SetFanSpeed(speeds[crt]);
                }
                else if (ADLX_FAILED(res)) {
                    std::cout << "State " << crt << ": error setting speed" << std::endl;
                    return;
                }

                oneState->GetFanSpeed(&speed);
                res = oneState->SetTemperature(temps[crt]);
                if (res == ADLX_FAIL) return;
                oneState->GetTemperature(&temperature);
                std::cout << "Set empty " << crt << " state: speed is " << speed << " temperature is " << temperature <<
                    std::endl;
            }
        }

        adlx_int errorIndex;
        res = m_manualFanTuning->IsValidFanTuningStates(states, &errorIndex);
        std::cout << "IsValidGPUTuningStates, errorIndex is: " << errorIndex << std::endl;
        if (ADLX_SUCCEEDED(res)) {
            m_manualFanTuning->SetFanTuningStates(states);
        }

        res = m_manualFanTuning->GetFanTuningStates(&states);
        if (ADLX_SUCCEEDED(res)) {
            std::cout << "After setting:" << std::endl;
            for (adlx_uint crt = states->Begin(); crt != states->End(); ++crt) {
                res = states->At(crt, &oneState);
                cleanUp(res);
                adlx_int speed = 0, temperature = 0;
                oneState->GetFanSpeed(&speed);
                oneState->GetTemperature(&temperature);
                std::cout << "The current " << crt << " state: speed is " << speed << " temperature is " << temperature
                    << std::endl;
            }
        }
    }
}

void GpuManager::ShowGPUMemoryTemperature() const {
    EnsurePerfServices();
    const IADLXGPUMetricsSupport1Ptr gpuMetricsSupport1(m_gpuMetricsSupport);
    if (!gpuMetricsSupport1) return;

    adlx_bool supported = false;
    ADLX_RESULT res = gpuMetricsSupport1->IsSupportedGPUMemoryTemperature(&supported);
    if (ADLX_SUCCEEDED(res) && supported) {
        const IADLXGPUMetrics1Ptr gpuMetrics1(GetCurrentGpuMetrics());
        if (!gpuMetrics1) return;

        adlx_double temperature = 0;
        adlx_int fanSpeed = 0;
        if (ADLX_SUCCEEDED(gpuMetrics1->GPUMemoryTemperature(&temperature)))
            std::cout << "The GPU memory temperature is: " << temperature << "°C" << std::endl;

        if (ADLX_SUCCEEDED(gpuMetrics1->GPUHotspotTemperature(&temperature)))
            std::cout << "The GPU hotspot temperature is: " << temperature << "°C" << std::endl;

        if (ADLX_SUCCEEDED(gpuMetrics1->GPUTemperature(&temperature)))
            std::cout << "The GPU temperature is: " << temperature << "°C" << std::endl;

        if (ADLX_SUCCEEDED(gpuMetrics1->GPUFanSpeed(&fanSpeed)))
            std::cout << "The GPU fan speed is: " << fanSpeed << std::endl;
    }
    else {
        std::cout << "GPU memory temperature reading not supported." << std::endl;
    }
}

void GpuManager::ShowGPUUsage() const {
    adlx_double cpuUsage = GetGPUUsage();
    std::cout << "CPU usage: " << cpuUsage << "%" << std::endl;
}

void GpuManager::ShowGPUVoltage() const {
    EnsurePerfServices();
    // GetCurrentGpuMetrics();
    adlx_bool supported = false;
    ADLX_RESULT res = m_gpuMetricsSupport->IsSupportedGPUVoltage(&supported);
    if (ADLX_SUCCEEDED(res) && supported) {
        adlx_int voltage = 0;
        res = GetCurrentGpuMetrics()->GPUVoltage(&voltage);
        if (ADLX_SUCCEEDED(res))
            std::cout << "The GPU voltage is: " << voltage << "mV" << std::endl;
    }
    else {
        std::cout << "Reading GPU voltage is not supported." << std::endl;
    }
}

void GpuManager::ShowGPUVRAM() const {
    EnsurePerfServices();
    adlx_bool supported = false;
    ADLX_RESULT res = m_gpuMetricsSupport->IsSupportedGPUVRAM(&supported);
    if (ADLX_SUCCEEDED(res) && supported) {
        adlx_int VRAM = 0;
        IADLXGPUMetricsPtr gpuMetrics = GetCurrentGpuMetrics();
        res = gpuMetrics->GPUVRAM(&VRAM);
        if (ADLX_SUCCEEDED(res)) {
            std::cout << "The used GPU VRAM is: " << VRAM << " MB/" << m_statusInfo.totalGPUMemoryMB << " MB" <<
                std::endl;
            std::cout << "Free VRAM: " << m_statusInfo.totalGPUMemoryMB - VRAM << " MB" << std::endl;
        }
    }
    else {
        std::cout << "The GPU VRAM reading is not supported." << std::endl;
    }
}

void GpuManager::ShowSystemRAM() const {
    EnsurePerfServices();
    EnsureSystemMetricsSupport();
    adlx_bool supported = false;
    ADLX_RESULT res = m_systemMetricsSupport->IsSupportedSystemRAM(&supported);
    if (ADLX_SUCCEEDED(res) && supported) {
        adlx_int ramUsage = 0;
        res = GetCurrentSystemMetrics()->SystemRAM(&ramUsage);
        if (ADLX_SUCCEEDED(res))
            std::cout << "Current system RAM usage: " << ramUsage << " MB" << std::endl;
    }
    else {
        std::cout << "Reading system RAM is not supported." << std::endl;
    }
}

void GpuManager::ShowTotalSystemMemory() const {
    if (m_statusInfo.totalSystemRamMB > 0) {
        std::cout << "Total system RAM: " << m_statusInfo.totalSystemRamMB << " MB" << std::endl;
    }
}

void GpuManager::ShowZeroRPMStatus() {
    EnsureFanTuning();
    if (m_statusInfo.zeroRPMEnabled) {
        std::cout << "ZeroRPM is currently active (enabled)." << std::endl;
    }
    else {
        std::cout << "ZeroRPM is currently inactive (disabled)." << std::endl;
    }
}

void GpuManager::ToggleZeroRPM() {
    EnsureFanTuning();
    adlx_bool supported = false;
    ADLX_RESULT res = m_manualFanTuning->IsSupportedZeroRPM(&supported);
    if (ADLX_FAILED(res)) {
        std::cout << "ZeroRPM status is not supported by this GPU." << std::endl;
        return;
    }
    if (!supported) {
        std::cout << "ZeroRPM is not supported by this GPU." << std::endl;
        return;
    }

    adlx_bool isZeroRPMStateSet = false;
    res = m_manualFanTuning->GetZeroRPMState(&isZeroRPMStateSet);
    if (ADLX_FAILED(res)) {
        std::cout << "Failed to read current ZeroRPM state." << std::endl;
        return;
    }

    const adlx_bool newState = !isZeroRPMStateSet;
    res = m_manualFanTuning->SetZeroRPMState(newState);
    if (ADLX_FAILED(res)) {
        std::cout << "Failed to toggle ZeroRPM state." << std::endl;
        return;
    }

    m_manualFanTuning->GetZeroRPMState(&isZeroRPMStateSet);
    m_statusInfo.zeroRPMEnabled = isZeroRPMStateSet;

    std::cout << "Command sent, now " << (isZeroRPMStateSet ? "ZeroRPM is active." : "ZeroRPM is inactive.") <<
        std::endl;
}

void GpuManager::UpdateGpuTempStruct(bool fpsOnly) {
    adlx_bool supported = false;
    IADLXFPSPtr oneFPS;
    adlx_int fpsData = 0;

    EnsurePerfServices();
    IADLXAllMetricsPtr allMetrics = GetCurrentAllMetrics();
    try {
        if (fpsOnly) {
            // 1. Always update FPS if not in a strictly-no-fps mode
            ADLX_RESULT res = allMetrics->GetFPS(&oneFPS);
            if (ADLX_SUCCEEDED(res)) {
                res = oneFPS->FPS(&fpsData);
                if (res == ADLX_NOT_SUPPORTED) {
                    // std::cout << "ADLX_NOT_SUPPORTED" << std::endl;
                    // if (m_temperatureInfo.minFPS != 0) {
                    //     m_temperatureInfo.instantFPS = 0;
                    //     m_temperatureInfo.minFPS = 0;
                    //     m_temperatureInfo.maxFPS = 0;
                    m_temperatureInfo.totalFPS = 0;
                    m_temperatureInfo.fpsHistory.reset();
                    // }
                }
                else if (ADLX_SUCCEEDED(res)) {
                    if (fpsData < 9999) {
                        // if (m_temperatureInfo.minFPS == 0) m_temperatureInfo.minFPS = 9999;
                        if (m_temperatureInfo.fpsHistory.isFull()) {
                            adlx_int oldestFPS = 0;
                            if (m_temperatureInfo.fpsHistory.get(0, oldestFPS)) {
                                m_temperatureInfo.totalFPS -= oldestFPS;
                            }
                        }
                        // push_back now automatically overwrites oldest when capacity (1000) is reached
                        m_temperatureInfo.fpsHistory.push_back(fpsData);
                        m_temperatureInfo.totalFPS += fpsData;
                        m_temperatureInfo.instantFPS = fpsData;

                        if (m_temperatureInfo.fpsHistory.size() == 1) {
                            m_temperatureInfo.minFPS = fpsData;
                            m_temperatureInfo.maxFPS = fpsData;
                        } else {
                            m_temperatureInfo.minFPS = min(m_temperatureInfo.minFPS, fpsData);
                            m_temperatureInfo.maxFPS = max(m_temperatureInfo.maxFPS, fpsData);
                        }
                        m_temperatureInfo.avgFPS = m_temperatureInfo.fpsHistory.size() > 0 ? m_temperatureInfo.totalFPS / m_temperatureInfo.fpsHistory.size() : 0;
                        // std::cout << fpsData << std::endl;
                    }
                }
            }
        }
        // 2. Optimization: Reuse the snapshot to update Temperatures/VRAM
        else {
            if (m_gpuMetricsSupport) {
                IADLXGPUMetricsPtr gpuMetrics;
                if (ADLX_SUCCEEDED(allMetrics->GetGPUMetrics(m_oneGPU, &gpuMetrics))) {
                    IADLXGPUMetrics1Ptr gpuMetrics1(gpuMetrics);
                    IADLXGPUMetricsSupport1Ptr gpuMetricsSupport1(m_gpuMetricsSupport);
                    IADLXGPUMetricsSupport3Ptr gpuMetricsSupport3(m_gpuMetricsSupport);
                    IADLXGPUMetrics3Ptr gpuMetrics3(gpuMetrics);

                    if (gpuMetrics1 && gpuMetricsSupport1 && gpuMetrics3) {
                        ADLX_RESULT res = gpuMetricsSupport1->IsSupportedGPUMemoryTemperature(&supported);
                        if (ADLX_SUCCEEDED(res) && supported) {
                            adlx_double temperature = 0, gpuUsage = 0;
                            adlx_int fanSpeed = 0, vRam = 0, speedPercent = 0;
                            if (ADLX_SUCCEEDED(gpuMetrics1->GPUMemoryTemperature(&temperature))) 
                                m_temperatureInfo.gpuMemoryTemperatureC = temperature;
                            if (ADLX_SUCCEEDED(gpuMetrics1->GPUHotspotTemperature(&temperature))) 
                                m_temperatureInfo.gpuHotspotTemperatureC = temperature;
                            if (ADLX_SUCCEEDED(gpuMetrics1->GPUTemperature(&temperature))) 
                                m_temperatureInfo.gpuTemperatureC = temperature;
                            if (ADLX_SUCCEEDED(gpuMetrics1->GPUFanSpeed(&fanSpeed))) 
                                m_temperatureInfo.fanSpeed = fanSpeed;
                            if (ADLX_SUCCEEDED(gpuMetrics1->GPUVRAM(&vRam))) 
                                m_temperatureInfo.vRam = vRam;
                            if (ADLX_SUCCEEDED(gpuMetrics->GPUUsage(&gpuUsage)))
                                m_temperatureInfo.gpuUsage = gpuUsage;
                            if (ADLX_SUCCEEDED(gpuMetrics3->GPUFanDuty(&speedPercent)))
                                m_temperatureInfo.fanPercent = speedPercent;
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        throw;
    }
}
