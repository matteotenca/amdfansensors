#ifndef AMDFANSENSORS_WINDOW_H
#define AMDFANSENSORS_WINDOW_H
#pragma once
#define UNICODE
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h> // Added for ComPtr
#include <imgui.h>
#include "GpuManager.h"

using Microsoft::WRL::ComPtr; // Shortcut for cleaner code

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Window procedure - main message handler for the application window
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/**
 * @brief Class responsible for managing the rendering context and game loop.
 * Encapsulates DirectX resources (ID3D11Device*, etc.) and the main GUI logic.
 */
class GuiWindow {
public:
    // Initialize DirectX, ImGui and the main loop resources.
    bool Initialize(int refreshRateMs, int refreshRateFPS = 1000);

    // Run the main application loop (handles messages and rendering).
    // void RunLoop() const;
    int OpenWin(GpuManager& manager, int refreshRateMs, int refreshRateFPS = 1000, bool fpsOnly = false);

    // Clean up all allocated DirectX/ImGui resources.
    ~GuiWindow();
    GuiWindow() = default;
    // Prevent copying to avoid double-release of DirectX resources
    GuiWindow(const GuiWindow&) = delete;
    GuiWindow& operator=(const GuiWindow&) = delete;

    int refreshRateMs = 1000;
    int refreshRateFPSMs = 1000;

private:
    HWND m_hWnd = nullptr;
    // Changed raw pointers to ComPtr for automatic lifecycle management
    ComPtr<ID3D11Device> m_pd3dDevice;
    ComPtr<ID3D11DeviceContext> m_pd3dDeviceContext;
    ComPtr<IDXGISwapChain> m_pSwapChain;
    ComPtr<ID3D11RenderTargetView> m_mainRenderTargetView;


    // Internal methods for DX setup/cleanup
    bool CreateDeviceD3D();
    bool CreateRenderTarget();
    void CleanupRenderTarget();
    void CleanupDeviceD3D();
};


#endif //AMDFANSENSORS_WINDOW_H
