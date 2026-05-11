// ReSharper disable CppDFANullDereference
#include "GpuManager.h"
#include "ADLXHelper.h"
#include <IPerformanceMonitoring2.h>
#include <stdexcept>
#include <iostream>
#include <string>

GpuManager::GpuManager() {
    //m_adlxHelp = g_ADLX;
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


    IADLXPerformanceMonitoringServicesPtr perfMonitoringServices;
    res = g_ADLX.GetSystemServices()->GetPerformanceMonitoringServices(&perfMonitoringServices);
    cleanUp(res);

    res = perfMonitoringServices->GetSupportedGPUMetrics(m_oneGPU, &m_gpuMetricsSupport);
    cleanUp(res);

    IADLXAllMetricsPtr allMetrics;
    res = perfMonitoringServices->GetCurrentAllMetrics(&allMetrics);
    cleanUp(res, "Get current all metrics");

    res = allMetrics->GetGPUMetrics(m_oneGPU, &m_gpuMetrics);
    cleanUp(res);

    res = perfMonitoringServices->GetSupportedSystemMetrics(&m_systemMetricsSupport);
    cleanUp(res);

    res = allMetrics->GetSystemMetrics(&m_systemMetrics);
    cleanUp(res);

    res = g_ADLX.GetSystemServices()->GetGPUTuningServices(&m_gpuTuningService);
    cleanUp(res);

    IADLXInterfacePtr manualFanTuningIfc;
    res = m_gpuTuningService->GetManualFanTuning(m_oneGPU, &manualFanTuningIfc);
    cleanUp(res, "Get manual fan tuning interface");
    m_manualFanTuning = static_cast<IADLXManualFanTuningPtr>(manualFanTuningIfc);

    IADLXInterfacePtr manualGFXTuningIfc;
    res = m_gpuTuningService->GetManualGFXTuning(m_oneGPU, &manualGFXTuningIfc);
    cleanUp(res, "Get manual graphics tuning interface");
    m_manualGFXTuning2 = static_cast<IADLXManualGraphicsTuning2Ptr>(manualGFXTuningIfc);

    // Populate GpuStatusInfo struct - read system RAM once during initialization
    // 1. Get system RAM total (populates struct)
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m_statusInfo.totalSystemRamMB = memInfo.ullTotalPhys / (1024 * 1024);
    }

    // 2. ZeroRPM status (already queried via m_manualFanTuning during init)
    adlx_bool isZeroRPM = false;
    ADLX_RESULT resZeroRPM = m_manualFanTuning->GetZeroRPMState(&isZeroRPM);
    if (ADLX_SUCCEEDED(resZeroRPM)) {
        m_statusInfo.zeroRPMEnabled = isZeroRPM;
    }

    // 3. Voltage offset (already queried via m_manualGFXTuning2 during init)
    adlx_int voltage = 0;
    res = m_manualGFXTuning2->GetGPUVoltage(&voltage);
    if (ADLX_SUCCEEDED(res)) {
        m_statusInfo.voltageOffsetMV = voltage;
    }

    // 4.Total VRAM (already queried and stored in m_totalVram during init)
    adlx_uint totalVramMB = 0;
    res = m_oneGPU->TotalVRAM(&totalVramMB);
    if (ADLX_SUCCEEDED(res)) {
        m_statusInfo.totalGPUMemoryMB = totalVramMB; // Store in struct
    }
    //cleanUp(res);
    //m_statusInfo.totalGPUMemoryMB = m_totalVram;

}

GpuManager::~GpuManager() = default;

void GpuManager::cleanUp(ADLX_RESULT res, const char* msg) {
    if (ADLX_FAILED(res)) {
        throw std::runtime_error(std::string("ADLX Error: ") + std::to_string(res) + " - " +
            (msg ? msg : "Operation") + " failed");
    }
}

// Getter for status info - returns populated struct
GpuManager::GpuStatusInfo GpuManager::GetStatusInfo() const {
    return m_statusInfo;
}

// NEW METHOD: Reads ZeroRPM status from struct (no ADLX calls)
void GpuManager::ShowZeroRPMStatus() const {
    if (m_statusInfo.zeroRPMEnabled) {
        std::cout << "ZeroRPM is currently active (enabled)." << std::endl;
    }
    else {
        std::cout << "ZeroRPM is currently inactive (disabled)." << std::endl;
    }
}

// NEW METHOD: Toggles ZeroRPM state (reads from ADLX to query current state)
void GpuManager::ToggleZeroRPM() {
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
    m_statusInfo.zeroRPMEnabled = isZeroRPMStateSet; // Update struct

    std::cout << "Command sent, now " << (isZeroRPMStateSet ? "ZeroRPM is active." : "ZeroRPM is inactive.") << std::endl;
}

void GpuManager::ShowTotalSystemMemory() const {
    // Read from struct (already initialized during construction) instead of re-calling API
    if (m_statusInfo.totalSystemRamMB > 0) {
        std::cout << "Total system RAM: " << m_statusInfo.totalSystemRamMB << " MB" << std::endl;
    }
}

//void GpuManager::GetGPUStates() const {
//    adlx_int volt = 0;
//    ADLX_RESULT res = m_manualGFXTuning2->GetGPUVoltage(&volt);
//    if (ADLX_FAILED(res)) {
//        std::cerr << "Failed to get GPU voltage" << std::endl;
//        return;
//    }
//    std::cout << "Current GPU clock voltage offset: " << volt << " mV" << std::endl;
//}

void GpuManager::SetGPUStates(adlx_int voltage) const {
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
        std::cout << "Set GPU voltage offset " << voltage << " mV: "
            << (ADLX_SUCCEEDED(res) ? "succeeded" : "failed") << std::endl;

        res = m_manualGFXTuning2->GetGPUVoltage(&currentVolt);
        cleanUp(res);
        std::cout << "Current GPU clock voltage offset (after set): " << currentVolt << " mV" << std::endl;
    }
}

void GpuManager::SetVoltageOffsetFromCLI(adlx_int voltage) const {
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

    SetGPUStates(voltage);
}

adlx_int GpuManager::GetVoltageOffset() const {
    return m_statusInfo.voltageOffsetMV;
}

void GpuManager::ShowGetAndSetFan(bool setRamp, bool defaultRamp) const {
    ADLX_IntRange fanSpeedRange, fanTemperatureRange;
    ADLX_RESULT res = m_manualFanTuning->GetFanTuningRanges(&fanSpeedRange, &fanTemperatureRange);
    cleanUp(res);

    std::cout << "Fan speed range is: (" << fanSpeedRange.minValue << ", " << fanSpeedRange.maxValue << ")" << std::endl;
    std::cout << "Fan temperature range is: (" << fanTemperatureRange.minValue << ", " << fanTemperatureRange.maxValue << ")" << std::endl;

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
            std::cout << "The current " << crt << " state: speed is " << speed << " temperature is " << temperature << std::endl;
        }
    }

    const int default_speeds[5] = { 30, 44, 58, 72, 100 };
    const int default_temps[5] = { 30, 50, 56, 63, 71 };
    const int my_speeds[5] = { 30, 30, 60, 85, 100 };
    //const int my_speeds[5] = { 70, 90, 95, 98, 100 };
    const int my_temps[5] = { 25, 40, 50, 60, 65 };
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
                std::cout << "Set empty " << crt << " state: speed is " << speed << " temperature is " << temperature << std::endl;
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
                std::cout << "The current " << crt << " state: speed is " << speed << " temperature is " << temperature << std::endl;
            }
        }
    }
}

void GpuManager::ShowGPUVRAM() const {
    adlx_bool supported = false;
    ADLX_RESULT res = m_gpuMetricsSupport->IsSupportedGPUVRAM(&supported);
    if (ADLX_SUCCEEDED(res) && supported) {
        adlx_int VRAM = 0;
        res = m_gpuMetrics->GPUVRAM(&VRAM);
        if (ADLX_SUCCEEDED(res)) {
            std::cout << "The used GPU VRAM is: " << VRAM << " MB/" << m_statusInfo.totalGPUMemoryMB << " MB" << std::endl;
            std::cout << "Free VRAM: " << m_statusInfo.totalGPUMemoryMB - VRAM << " MB" << std::endl;
        }
    }
    else {
        std::cout << "The GPU VRAM reading is not supported." << std::endl;
    }
}

void GpuManager::ShowGPUMemoryTemperature() const {
    adlx_bool supported = false;
    const IADLXGPUMetricsSupport1Ptr gpuMetricsSupport1(m_gpuMetricsSupport);
    const IADLXGPUMetrics1Ptr gpuMetrics1(m_gpuMetrics);
    if (gpuMetricsSupport1 && gpuMetrics1) {
        ADLX_RESULT res = gpuMetricsSupport1->IsSupportedGPUMemoryTemperature(&supported);
        if (ADLX_SUCCEEDED(res) && supported) {
            adlx_double temperature = 0;
			adlx_int fanSpeed = 0;
            res = gpuMetrics1->GPUMemoryTemperature(&temperature);
            if (ADLX_SUCCEEDED(res))
                std::cout << "The GPU memory temperature is: " << temperature << "°C" << std::endl;
            res = gpuMetrics1->GPUHotspotTemperature(&temperature);
            if (ADLX_SUCCEEDED(res))
                std::cout << "The GPU hotspot temperature is: " << temperature << "°C" << std::endl;
            res = gpuMetrics1->GPUTemperature(&temperature);
            if (ADLX_SUCCEEDED(res))
                std::cout << "The GPU temperature is: " << temperature << "°C" << std::endl;
            res = gpuMetrics1->GPUFanSpeed(&fanSpeed);
            if (ADLX_SUCCEEDED(res))
                std::cout << "The GPU fan speed is: " << fanSpeed << std::endl;
        }
        else {
            std::cout << "GPU memory temperature reading not supported." << std::endl;
        }
    }
}

void GpuManager::ShowGPUVoltage() const {
    adlx_bool supported = false;
    ADLX_RESULT res = m_gpuMetricsSupport->IsSupportedGPUVoltage(&supported);
    if (ADLX_SUCCEEDED(res) && supported) {
        adlx_int voltage = 0;
        res = m_gpuMetrics->GPUVoltage(&voltage);
        if (ADLX_SUCCEEDED(res))
            std::cout << "The GPU voltage is: " << voltage << "mV" << std::endl;
    }
    else {
        std::cout << "Reading GPU voltage is not supported." << std::endl;
    }
}

void GpuManager::ShowSystemRAM() const {
    adlx_bool supported = false;
    ADLX_RESULT res = m_systemMetricsSupport->IsSupportedSystemRAM(&supported);
    if (ADLX_SUCCEEDED(res) && supported) {
        adlx_int ramUsage = 0;
        res = m_systemMetrics->SystemRAM(&ramUsage);
        if (ADLX_SUCCEEDED(res))
            std::cout << "Current system RAM usage: " << ramUsage << " MB" << std::endl;
    }
    else {
        std::cout << "Reading system RAM is not supported." << std::endl;
    }
}
