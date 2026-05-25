#ifndef AMDFANSENSORS_WINDOW_H
#define AMDFANSENSORS_WINDOW_H
#pragma once

// Windows headers
#include <windows.h>

// DirectX headers
#include <d3d11.h>

// ImGui headers
#include <imgui.h>

#include "GpuManager.h"

// Forward declarations
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
    bool Initialize(int refreshRateMs);

    // Run the main application loop (handles messages and rendering).
    // void RunLoop() const;
    int OpenWin(GpuManager& manager, int refreshRateMs, bool fpsOnly = false);

    // Clean up all allocated DirectX/ImGui resources.
    ~GuiWindow();

private:
    HWND m_hWnd = nullptr;
    ID3D11Device* m_pd3dDevice = nullptr;
    ID3D11DeviceContext* m_pd3dDeviceContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    ID3D11RenderTargetView* m_mainRenderTargetView = nullptr;
    int refreshRateMs = 1000;

    // Internal methods for DX setup/cleanup
    bool CreateDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void CleanupDeviceD3D();
};



#endif //AMDFANSENSORS_WINDOW_H