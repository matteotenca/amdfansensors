#ifndef GPU_MANAGER_H
#define GPU_MANAGER_H
#pragma once
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
    RingBuffer() : m_head(0), m_tail(0), m_count(0), m_completedRings(0) {
    }

    /// Retrieves an element at the specified logical position relative to the head.
    /// @param pos The logical index to retrieve, where 0 is the oldest element (head).
    /// @param value Reference where the retrieved value will be stored if the index is valid.
    /// @return True if the position is within bounds (pos < size()) and the value was retrieved, false otherwise.
    /// This operation is O(1).
    bool get(size_t pos, T& value) {
        if (pos >= m_count) return false;
        // Calculate physical index based on head offset
        value = m_buffer[(m_head + pos) % Capacity];
        return true;
    }

    /// Adds a new element to the end of the buffer (tail).
    /// If the buffer is at capacity, the oldest element (head) is overwritten and the head pointer advances.
    /// @param value The element to be added to the buffer.
    /// @return Always returns true, as the buffer overwrites oldest data rather than failing.
    /// This operation is O(1).
    bool push_back(const T& value) {
        m_buffer[m_tail] = value;
        size_t current_tail = m_tail;

        if (m_incFullCounter) {
            m_completedRings++;
            m_incFullCounter = false;
        }

        if (m_count == Capacity) {
            // Buffer is full: advance head to overwrite the oldest element
            m_head = (m_head + 1) % Capacity;
        }
        else {
            ++m_count;
        }

        m_tail = (m_tail + 1) % Capacity;
        if (current_tail > m_tail && m_tail == 0) {
            m_incFullCounter = true;
        }
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
        m_completedRings = 0;
        m_incFullCounter = false;
    }

    /// Gets the number of times the buffer has wrapped around (overflowed) since its creation.
    /// This is useful for tracking how much data was discarded or how many full cycles were completed in a long-running stream.
    /// @return The total count of completed rings as a size_t.
    /// This operation is O(1).
    size_t getCompletedRings() const {
        return m_completedRings;
    }

    /// Gets the current number of elements stored in the ring buffer.
    /// @return The number of valid elements currently in the buffer (0 to Capacity).
    /// This operation is O(1).
    size_t size() const { return m_count; }

    /// Checks if the ring buffer contains no elements at all. Can be used as a boolean condition (e.g., while(!buffer.isEmpty())). Returns true when isEmpty(). Returns false even when the buffer is empty due to push_back failure, or full but not exceeding capacity).
    bool isEmpty() const { return m_count == 0; }

    /// Checks if the ring buffer has reached its maximum capacity.
    /// When the buffer is full, subsequent calls to push_back will begin overwriting the oldest elements.
    /// @return True if the number of elements has reached Capacity, false otherwise.
    /// This operation is O(1).
    bool isFull() const { return m_count >= Capacity; }


private:
    T m_buffer[Capacity];
    size_t m_head; // Index of oldest element
    size_t m_tail; // Index where next element will be written
    size_t m_count; // Current number of valid elements
    size_t m_completedRings;
    bool m_incFullCounter = false;
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

    struct GlobalStatusInfo {
        mutable adlx_double gpuTemperatureC = 0.0; // GPU temperature in Celsius
        mutable adlx_double gpuMemoryTemperatureC = 0.0; // GPU memory temperature in Celsius
        mutable adlx_double gpuHotspotTemperatureC = 0.0; // GPU hotspot temperature in Celsius
        mutable adlx_int fanSpeed = 0; // GPU fan speed in RPM
        mutable adlx_int fanPercent = 0;
        mutable adlx_int vRam = 0; // GPU VRAM used in MB
        mutable adlx_int vRamp = 0;
        mutable adlx_int instantFPS = 0;
        mutable adlx_int maxFPS = 0;
        mutable adlx_int minFPS = 0;
        mutable adlx_int avgFPS = 0;
        mutable adlx_int totalFPS = 0;
        mutable adlx_double gpuUsage = 0.0;
        mutable adlx_double gpuPower = 0.0;
        mutable adlx_int usedSystemRam = 0;
        mutable adlx_int totalUnattended = 0;
        mutable int fullCounter = 0;
        // Ring buffer for FPS history - O(1) operations, no reallocations
        RingBuffer<adlx_int, 500> fpsHistory;
    };

    GpuManager(const GpuManager&) = delete;
    GpuManager& operator=(const GpuManager&) = delete;

    GpuStatusInfo GetStatusInfo() const;
    adlx_int GetUsedSystemRam() const;
    int GetUnattended() const;
    // Monitoring methods - marked const as they don't modify state
    void ShowGPUVRAM() const;
    void ShowGPUMemoryTemperature() const;
    void UpdateGpuTempStruct(bool fpsOnly = false);
    void UpdateGpuFpsMetrics();
    void UpdateGpuHardwareMetrics();
    void ShowGPUVoltage() const;
    void ShowSystemRAM() const;
    void ShowTotalSystemRam() const;

    // ZeroRPM specific methods - split into read and toggle
    void ShowZeroRPMStatus(); // Reads status from struct
    void ToggleZeroRPM(); // Toggles ZeroRPM state
    bool IsZeroRPMSupported();
    bool GetZeroRPMState();
    void SetZeroRPM(bool enable);
    bool IsFanCurveDefaultOne(); // Added: Compares current states to default values

    // Tuning methods
    void ShowGetAndSetFan(bool setRamp, bool defaultRamp);
    void SetVoltageOffset(adlx_int voltage) const; // UPDATED: Added voltage parameter
    void ShowFanPercent() const;
    void ShowGPUUsage() const;
    adlx_int GetVoltageOffset() const; // NEW: Getter for voltage offset
    bool GetVoltageOffsetRange(adlx_int& minVolt, adlx_int& maxVolt) const;

    // Pointers are now mutable to allow lazy-loading initialization within const methods
    mutable IADLXManualGraphicsTuning2Ptr m_manualGFXTuning2 = nullptr;
    mutable IADLXGPUMetricsSupportPtr m_gpuMetricsSupport = nullptr;
    mutable IADLXSystemMetricsSupportPtr m_systemMetricsSupport = nullptr;
    mutable IADLXGPUTuningServicesPtr m_gpuTuningService = nullptr;
    mutable IADLXManualFanTuningPtr m_manualFanTuning = nullptr;
    mutable IADLXPerformanceMonitoringServicesPtr m_perfMonitoringServices = nullptr;

    IADLXGPUPtr m_oneGPU = nullptr;
    GpuStatusInfo m_statusInfo; // Status information struct
    GlobalStatusInfo m_temperatureInfo; // Temperature information struct
private:
    // Lazy loading helpers
    // void EnsureAllMetrics() const;
    void EnsurePerfServices() const;
    void EnsureTuningService() const;
    void EnsureFanTuning();
    void EnsureGfxTuning() const;
    IADLXGPUMetricsPtr GetCurrentGpuMetrics() const;
    void EnsureSystemMetricsSupport() const;
    IADLXSystemMetricsPtr GetCurrentSystemMetrics() const;
    adlx_int GetFanPercent() const;
    adlx_double GetGPUUsage() const;
    IADLXAllMetricsPtr GetCurrentAllMetrics() const;

    static constexpr int m_defaultSpeeds[5] = { 30, 44, 58, 72, 100 };
    static constexpr int m_defaultTemps[5] = { 30, 50, 56, 63, 71 };

    static void cleanUp(ADLX_RESULT res, const char* operation_name = nullptr);
};
#endif // GPU_MANAGER_H
