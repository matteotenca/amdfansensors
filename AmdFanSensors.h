#ifndef AMDFANSENSORS_AMDFANSENSORS_H
#define AMDFANSENSORS_AMDFANSENSORS_H
#pragma once
#include "GpuManager.h"

//#include <ADLX.h>
//#include <IGPUManualFanTuning.h>
//#include <IGPUTuning.h>
//#include <IPerformanceMonitoring.h>
//#include <IGPUManualGFXTuning.h>

//class GpuManager {
//public:
//    GpuManager();
//    ~GpuManager();
//
//    // Struct to hold global GPU status information
//    struct GpuStatusInfo {
//        size_t totalSystemRamMB = 0;      // Total installed system RAM (MB)
//        adlx_uint totalGPUMemoryMB = 0;   // GPU total VRAM (MB)
//        adlx_bool zeroRPMEnabled = false; // Current ZeroRPM status
//        adlx_int voltageOffsetMV = 0;     // Current GPU voltage offset (mV)
//    };
//
//    //GpuManager(const GpuManager&) = delete;
//    //GpuManager& operator=(const GpuManager&) = delete;
//
//    GpuStatusInfo GetStatusInfo() const;
//
//    // Monitoring methods - marked const as they don't modify state
//    void ShowGPUVRAM() const;
//    void ShowGPUMemoryTemperature() const;
//    void ShowGPUVoltage() const;
//    void ShowSystemRAM() const;
//    void ShowTotalSystemMemory() const;
//
//    // ZeroRPM specific methods - split into read and toggle
//    void ShowZeroRPMStatus() const;      // Reads status from struct
//    void ToggleZeroRPM();                // Toggles ZeroRPM state
//
//    // Tuning methods
//    void ShowGetAndSetFan(bool setRamp, bool defaultRamp);
//    void SetGPUStates(adlx_int voltage); // UPDATED: Added voltage parameter
//    void GetGPUStates() const;
//    void SetVoltageOffsetFromCLI(adlx_int voltage); // NEW: Added from CLI
//    adlx_int GetVoltageOffset() const; // NEW: Getter for voltage offset
//    IADLXManualGraphicsTuning2Ptr m_manualGFXTuning2;
//    IADLXGPUMetricsSupportPtr m_gpuMetricsSupport;
//    IADLXGPUMetricsPtr m_gpuMetrics;
//    IADLXSystemMetricsSupportPtr m_systemMetricsSupport;
//    IADLXSystemMetricsPtr m_systemMetrics;
//
//    IADLXGPUTuningServicesPtr m_gpuTuningService;
//    IADLXManualFanTuningPtr m_manualFanTuning;
//    ADLXHelper m_adlxHelp;
//    IADLXGPUPtr m_oneGPU;
//    //adlx_uint m_totalVram;
//    GpuStatusInfo m_statusInfo; // Status information struct
//private:
//    // Updated signature: accepts ADLX_RESULT and optional operation_name name
//    void cleanUp(ADLX_RESULT res, const char* operation_name = nullptr);
//
//    
//};
#endif //AMDFANSENSORS_AMDFANSENSORS_H