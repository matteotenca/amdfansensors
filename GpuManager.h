#ifndef GPU_MANAGER_H
#define GPU_MANAGER_H
#pragma once
#define UNICODE
#include <ADLX.h>
#include <IGPUManualFanTuning.h>
#include <IGPUTuning.h>
#include <IPerformanceMonitoring.h>
#include <IGPUManualGFXTuning.h>
#include <stdexcept>

// Ring buffer template for efficient circular storage
template <typename T, size_t Capacity>
class RingBuffer {
public:
    RingBuffer() : m_head(0), m_tail(0), m_count(0) {
    }

    bool get(size_t pos, T& value) {
        if (pos >= m_count) return false;
        // Calculate physical index based on head offset
        value = m_buffer[(m_head + pos) % Capacity];
        return true;
    }

    bool push_back(const T& value) {
        m_buffer[m_tail] = value;

        if (m_count == Capacity) {
            // Buffer is full: advance head to overwrite the oldest element
            m_head = (m_head + 1) % Capacity;
        }
        else {
            ++m_count;
        }

        m_tail = (m_tail + 1) % Capacity;
        return true;
    }

    /// Retrieves the front element without removing it from the buffer.
    /// @return Reference to the first element in the ring buffer, or throws an exception if the buffer is empty. Throws std::runtime_error with message "RingBuffer: empty buffer" on failure. Only call when you are certain the buffer contains at least one element (e.g., after checking isEmpty() == false).
    T& front() {
        if (m_count == 0) throw std::runtime_error("RingBuffer: empty buffer");
        return m_buffer[m_head];
    }

    /// Retrieves a const reference to the front element without removing it from the buffer.
    /// @return Const reference to the first element in the ring buffer, or throws an exception if the buffer is empty. Throws std::runtime_error with message "RingBuffer: empty buffer" on failure. Only call when you are certain the buffer contains at least one element (e.g., after checking isEmpty() == false).
    const T& front() const {
        if (m_count == 0) throw std::runtime_error("RingBuffer: empty buffer");
        return m_buffer[m_head];
    }

    /// Retrieves a non-const reference to the last element without removing it from the buffer.
    /// @return Non-const reference to the last element in the ring buffer, or throws an exception if the buffer is empty. Throws std::runtime_error with message "RingBuffer: empty buffer" on failure. Only call when you are certain the buffer contains at least one element (e.g., after checking isEmpty() == false).
    T& back() {
        if (m_count == 0) throw std::runtime_error("RingBuffer: empty buffer");
        int idx = (m_tail - 1 + Capacity) % Capacity;
        return m_buffer[idx];
    }

    /// Retrieves a const reference to the last element without removing it from the buffer.
    /// @return Const reference to the last element in the ring buffer, or throws an exception if the buffer is empty. Throws std::runtime_error with message "RingBuffer: empty buffer" on failure. Only call when you are certain the buffer contains at least one element (e.g., after checking isEmpty() == false).
    const T& back() const {
        if (m_count == 0) throw std::runtime_error("RingBuffer: empty buffer");
        int idx = (m_tail - 1 + Capacity) % Capacity;
        return m_buffer[idx];
    }

    /// Clears the entire ring buffer to its initial state, effectively resetting all elements.
    /// This operation is O(1). After reset(), isEmpty() will always be true until new data is added via push_back(). Be cautious: this doesn't reallocate memory - it only resets m_head, m_tail, and m_count fields in the internal structure that are not stored but implicit to a ring buffer (in-place)
    void reset() {
        m_head = 0;
        m_tail = 0;
        m_count = 0;
    }

    /// Returns the current number of elements currently stored in the buffer.
    /// @return The count of active elements (between 0 and Capacity-1). O(1) operation. Always less than or equal to capacity, since push_back returns false when full prevents overflow beyond this threshold.
    size_t size() const { return m_count; }

    /// Checks if the ring buffer contains no elements at all. Can be used as a boolean condition (e.g., while(!buffer.isEmpty())). Returns true when isEmpty(). Returns false even when the buffer is empty due to push_back failure, or full but not exceeding capacity).
    bool isEmpty() const { return m_count == 0; }

    /// Checks if the ring buffer has reached maximum occupancy limit. When true, any further calls to push_back will fail immediately (returning false) without storing new data and returning an error condition until reset() is called or elements are removed via pop_front().
    bool isFull() const { return m_count >= Capacity; }

private:
    T m_buffer[Capacity];
    size_t m_head; // Index of oldest element
    size_t m_tail; // Index where next element will be written
    size_t m_count; // Current number of valid elements
};

class GpuManager {
public:
    GpuManager();
    ~GpuManager();

    // Struct to hold global GPU status information
    struct GpuStatusInfo {
        size_t totalSystemRamMB = 0; // Total installed system RAM (MB)
        adlx_uint totalGPUMemoryMB = 0; // GPU total VRAM (MB)
        adlx_bool zeroRPMEnabled = false; // Current ZeroRPM status
        mutable adlx_int voltageOffsetMV = 0; // Current GPU voltage offset (mV)
    };

    struct GpuTemperatureInfo {
        mutable adlx_double gpuTemperatureC = 0.0; // GPU temperature in Celsius
        mutable adlx_double gpuMemoryTemperatureC = 0.0; // GPU memory temperature in Celsius
        mutable adlx_double gpuHotspotTemperatureC = 0.0; // GPU hotspot temperature in Celsius
        mutable adlx_int fanSpeed = 0; // GPU fan speed in RPM
        mutable adlx_int vRam = 0; // GPU VRAM used in MB
        mutable adlx_int vRamp = 0;
        mutable adlx_int instantFPS = 0;
        mutable adlx_int maxFPS = 0;
        mutable adlx_int minFPS = 0;
        mutable adlx_int avgFPS = 0;
        // Ring buffer for FPS history - O(1) operations, no reallocations
        RingBuffer<adlx_int, 1000> fpsHistory;
    };

    GpuManager(const GpuManager&) = delete;
    GpuManager& operator=(const GpuManager&) = delete;

    GpuStatusInfo GetStatusInfo() const;

    // Monitoring methods - marked const as they don't modify state
    void ShowGPUVRAM() const;
    void ShowGPUMemoryTemperature() const;
    void UpdateGpuTempStruct(bool fpsOnly = false);
    void ShowGPUVoltage() const;
    void ShowSystemRAM() const;
    void ShowTotalSystemMemory() const;

    // ZeroRPM specific methods - split into read and toggle
    void ShowZeroRPMStatus(); // Reads status from struct
    void ToggleZeroRPM(); // Toggles ZeroRPM state

    // Tuning methods
    void ShowGetAndSetFan(bool setRamp, bool defaultRamp);
    void SetVoltageOffset(adlx_int voltage) const; // UPDATED: Added voltage parameter
    //void GetGPUStates() const;
    void SetVoltageOffsetFromCLI(adlx_int voltage) const; // NEW: Added from CLI
    adlx_int GetVoltageOffset() const; // NEW: Getter for voltage offset

    // Pointers are now mutable to allow lazy-loading initialization within const methods
    mutable IADLXManualGraphicsTuning2Ptr m_manualGFXTuning2 = nullptr;
    mutable IADLXGPUMetricsSupportPtr m_gpuMetricsSupport = nullptr;
    mutable IADLXGPUMetricsPtr m_gpuMetrics = nullptr;
    mutable IADLXSystemMetricsSupportPtr m_systemMetricsSupport = nullptr;
    mutable IADLXSystemMetricsPtr m_systemMetrics = nullptr;

    mutable IADLXGPUTuningServicesPtr m_gpuTuningService = nullptr;
    mutable IADLXManualFanTuningPtr m_manualFanTuning = nullptr;
    mutable IADLXPerformanceMonitoringServicesPtr m_perfMonitoringServices = nullptr;

    IADLXGPUPtr m_oneGPU = nullptr;
    GpuStatusInfo m_statusInfo; // Status information struct
    GpuTemperatureInfo m_temperatureInfo; // Temperature information struct
private:
    // Lazy loading helpers
    void EnsurePerfServices() const;
    void EnsureTuningService() const;
    void EnsureFanTuning();
    void EnsureGfxTuning() const;
    void EnsureGpuMetrics() const;
    void EnsureSystemMetrics() const;

    // Updated signature: accepts ADLX_RESULT and optional operation_name name
    static void cleanUp(ADLX_RESULT res, const char* operation_name = nullptr);
};
#endif // GPU_MANAGER_H
