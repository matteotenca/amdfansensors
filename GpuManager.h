#ifndef GPU_MANAGER_H
#define GPU_MANAGER_H
#pragma once
#include <ADLX.h>
#include <IGPUManualFanTuning.h>
#include <IGPUTuning.h>
#include <IPerformanceMonitoring.h>
#include <IGPUManualGFXTuning.h>
#include <vector>

class GpuManager {
public:
    GpuManager();
    ~GpuManager();

    // Struct to hold global GPU status information
    struct GpuStatusInfo {
        size_t totalSystemRamMB = 0;      // Total installed system RAM (MB)
        adlx_uint totalGPUMemoryMB = 0;   // GPU total VRAM (MB)
        adlx_bool zeroRPMEnabled = false; // Current ZeroRPM status
        mutable adlx_int voltageOffsetMV = 0;     // Current GPU voltage offset (mV)
    };

    struct GpuTemperatureInfo {
        mutable adlx_double gpuTemperatureC = 0.0; // GPU temperature in Celsius
        mutable adlx_double gpuMemoryTemperatureC = 0.0; // GPU memory temperature in Celsius
        mutable adlx_double gpuHotspotTemperatureC = 0.0; // GPU hotspot temperature in Celsius
        mutable adlx_int fanSpeed = 0; // GPU fan speed in RPM
        mutable adlx_int vRam = 0; // GPU fan speed in RPM
        mutable adlx_int vRamp = 0;
        mutable adlx_int instantFPS = 0;
        mutable adlx_int maxFPS = 0;
        mutable adlx_int minFPS = 0;
        mutable adlx_int avgFPS = 0;
        mutable std::vector<adlx_int> fpsHistory;
    };

    //GpuManager(const GpuManager&) = delete;
    //GpuManager& operator=(const GpuManager&) = delete;

    GpuStatusInfo GetStatusInfo() const;

    // Monitoring methods - marked const as they don't modify state
    void ShowGPUVRAM() const;
    void ShowGPUMemoryTemperature() const;
    void UpdateGpuTempStruct(int refreshRateMs = 1000, bool fpsOnly = false);
    void ShowGPUVoltage() const;
    void ShowSystemRAM() const;
    void ShowTotalSystemMemory() const;

    // ZeroRPM specific methods - split into read and toggle
    void ShowZeroRPMStatus() const;      // Reads status from struct
    void ToggleZeroRPM();                // Toggles ZeroRPM state

    // Tuning methods
    void ShowGetAndSetFan(bool setRamp, bool defaultRamp) const;
    void SetVoltageOffset(adlx_int voltage) const; // UPDATED: Added voltage parameter
    //void GetGPUStates() const;
    void SetVoltageOffsetFromCLI(adlx_int voltage) const; // NEW: Added from CLI
    adlx_int GetVoltageOffset() const; // NEW: Getter for voltage offset
    IADLXManualGraphicsTuning2Ptr m_manualGFXTuning2;
    IADLXGPUMetricsSupportPtr m_gpuMetricsSupport;
    IADLXGPUMetricsPtr m_gpuMetrics;
    IADLXSystemMetricsSupportPtr m_systemMetricsSupport;
    IADLXSystemMetricsPtr m_systemMetrics;

    IADLXGPUTuningServicesPtr m_gpuTuningService;
    IADLXManualFanTuningPtr m_manualFanTuning;
    IADLXPerformanceMonitoringServicesPtr m_perfMonitoringServices;
    //ADLXHelper m_adlxHelp;
    IADLXGPUPtr m_oneGPU;
    //adlx_uint m_totalVram;
    GpuStatusInfo m_statusInfo; // Status information struct
    GpuTemperatureInfo m_temperatureInfo; // Temperature information struct
private:
    // Updated signature: accepts ADLX_RESULT and optional operation_name name
    static void cleanUp(ADLX_RESULT res, const char* operation_name = nullptr);


};
#endif // GPU_MANAGER_H
