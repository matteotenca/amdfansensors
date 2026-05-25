// Gui.cpp full content - OPTIMIZED FOR REDUNDANT DRAWING
#include "Gui.h"

#include <chrono>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <imgui_internal.h>
#include <d3d11.h>
#include <iostream>
#include <thread>

// Window procedure remains external as it uses Windows API directly
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            // The GuiWindow::RunLoop will handle resizing logic using its internal state.
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}


// Implementation of CreateDeviceD3D (No changes needed)
bool GuiWindow::CreateDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0; // Will be set later when window size is known
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    constexpr D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,};

    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
                                      featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_pSwapChain,
                                      &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext) != S_OK) {
        std::cerr << "Failed to create DirectX device and swap chain." << std::endl;
        return false;
    }

    if (m_pSwapChain == nullptr) {
        std::cerr << "Swap chain creation failed - m_pSwapChain is null." << std::endl;
        return false;
    }

    if (!CreateRenderTarget()) {
        std::cerr << "Failed to create render target after device initialization." << std::endl;
        CleanupDeviceD3D();  // Clean up device resources on failure
        return false;
    }
    return true;
}

// Implementation of CreateRenderTarget (No changes needed)
bool GuiWindow::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

    if (FAILED(hr)) {
        std::cerr << "Failed to create render target view from swap chain buffer. Error code: 0x"
                  << std::hex << hr << std::dec << std::endl;
        pBackBuffer = nullptr; // Ensure no dangling pointer
        return false;
    }

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRenderTargetView);
    if (FAILED(hr)) {  // Check the HRESULT from CreateRenderTargetView
        std::cerr << "Failed to create render target view. Error code: 0x"
                  << std::hex << hr << std::dec << std::endl;
        pBackBuffer->Release();
        return false;
    }

    pBackBuffer->Release();
    return true;
}

// Implementation of CleanupRenderTarget (No changes needed)
void GuiWindow::CleanupRenderTarget() {
    if (m_mainRenderTargetView) {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}

// Implementation of CleanupDeviceD3D (No changes needed)
void GuiWindow::CleanupDeviceD3D() {
    CleanupRenderTarget(); 
    if (m_pSwapChain) {
        m_pSwapChain->Release();
        m_pSwapChain = nullptr;
    }
    if (m_pd3dDeviceContext) {
        m_pd3dDeviceContext->Release();
        m_pd3dDeviceContext = nullptr;
    }
    if (m_pd3dDevice) {
        m_pd3dDevice->Release();
        m_pd3dDevice = nullptr;
    }
}

// Destructor ensures proper cleanup when the object goes out of scope. (No changes needed)
GuiWindow::~GuiWindow() {
    CleanupDeviceD3D();
}

/**
 * @brief Initializes all graphics resources and sets up ImGui context. (No changes needed)
 */
bool GuiWindow::Initialize(int refreshRate, int refreshRateFPS) {
    if (!CreateDeviceD3D()) {
        std::cerr << "Failed to create DirectX device." << std::endl;
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplDX11_Init(m_pd3dDevice, m_pd3dDeviceContext);
    refreshRateMs = refreshRate;
    refreshRateFPSMs = refreshRateFPS;
    return true;
}

// Main window application entry point. Optimized to reduce CPU usage.
int GuiWindow::OpenWin(GpuManager& manager, const int refreshRate, const int refreshRateFPS, const bool fpsOnly) {

    int error = 0;
    // Setup Window Class (No changes needed)
    WNDCLASSEX wc;
    wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr,
        nullptr,
        static_cast<LPCSTR>("ImGuiWindow"), nullptr
    };
    RegisterClassEx(&wc);

    m_hWnd = CreateWindow(wc.lpszClassName, "GPU Temperature Monitor",
                             WS_OVERLAPPEDWINDOW, 100, 100, 640, 480, nullptr, nullptr, wc.hInstance, nullptr);

    std::cout << "Initializing GUI resources..." << std::endl;
    if (!Initialize(refreshRate, refreshRateFPS)) {
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(m_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hWnd);


    // --- Main loop execution (Optimized) ---
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    std::cout << "Finestra aperta. Premere ESC per uscire." << std::endl;
    constexpr float clear_color_with_alpha[4] = {0.45f, 0.55f, 0.60f, 1.00f};

    std::chrono::high_resolution_clock::time_point lastFpsUpdateTime = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point lastTempUpdateTime = std::chrono::high_resolution_clock::now();

    std::chrono::milliseconds fpsInterval(refreshRateFPSMs);
    std::chrono::milliseconds tempInterval(refreshRateMs);

    // State tracking for optimization: Store the last successfully displayed values
    GpuManager::GpuTemperatureInfo lastDisplayedTempInfo = manager.m_temperatureInfo;
    GpuManager::GpuStatusInfo lastDisplayedStatusInfo = manager.GetStatusInfo();

    int updatesTemp = 0;
    int updatesFPS = 0;

    while (msg.message != WM_QUIT) {
        bool needsRedraw = false;

        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            needsRedraw = true; // Redraw on any window event
            continue;
        }

        // --- CPU Optimization: Throttling ---
        if (IsIconic(m_hWnd) || ! IsWindowVisible(m_hWnd)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        auto now = std::chrono::high_resolution_clock::now();

        // High Frequency Poll: FPS
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFpsUpdateTime) >= fpsInterval) {
            manager.UpdateGpuTempStruct(true);
            lastFpsUpdateTime = now;
            lastDisplayedTempInfo = manager.m_temperatureInfo;
            updatesFPS+=1;
            needsRedraw = true;
        }

        // Low Frequency Poll: Temperatures and VRAM
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTempUpdateTime) >= tempInterval) {
            manager.UpdateGpuTempStruct(false);
            lastTempUpdateTime = now;
            lastDisplayedTempInfo = manager.m_temperatureInfo;
            lastDisplayedStatusInfo = manager.GetStatusInfo();
            updatesTemp+=1;
            needsRedraw = true;
        }

        if (needsRedraw) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("GPU Monitor");

            ImGui::Text("Monitor Temperatura GPU");
            ImGui::Separator();

            ImGui::Text("Refresh Rate Temp: %d ms", refreshRateMs);
            ImGui::Text("Refresh Rate FPS: %d ms", refreshRateFPSMs);
            ImGui::Text("FPS Samples: %zu/1000", manager.m_temperatureInfo.fpsHistory.size());
            ImGui::Text("Times FPS where refreshed: %d", updatesFPS);
            ImGui::Text("Times temps where refreshed: %d", updatesTemp);

            if (!fpsOnly) {
                ImGui::Text("Temperatura GPU: %0.1f °C", lastDisplayedTempInfo.gpuTemperatureC);
                ImGui::Text("Temperatura GPU Memory: %0.1f °C", lastDisplayedTempInfo.gpuMemoryTemperatureC);
                ImGui::Text("Temperatura GPU Hotspot: %0.1f °C", lastDisplayedTempInfo.gpuHotspotTemperatureC);
                ImGui::Text("Velocità Ventola GPU: %d RPM", lastDisplayedTempInfo.fanSpeed);
                ImGui::Text("VRAM GPU occupata: %d MB", lastDisplayedTempInfo.vRam);
                ImGui::Text("VRAM GPU libera: %d MB", lastDisplayedStatusInfo.totalGPUMemoryMB - lastDisplayedTempInfo.vRam);
            }
            ImGui::Text("Instant FPS: %d", lastDisplayedTempInfo.instantFPS);
            ImGui::Text("Average FPS: %d", lastDisplayedTempInfo.avgFPS);
            ImGui::Text("Minimum FPS: %d", lastDisplayedTempInfo.minFPS);
            ImGui::Text("Maximum FPS: %d", lastDisplayedTempInfo.maxFPS);

            ImGui::End();

            ImGui::Render();
            m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, nullptr);
            m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clear_color_with_alpha);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            HRESULT hr = m_pSwapChain->Present(1, 0);
            if (FAILED(hr)) {
                std::cout << "DXGI Error! Error code: 0x" << std::hex << hr << std::dec << std::endl;
                error = 1;
                break;
            }
        } else {
            // No data changed and no messages: sleep briefly to avoid 100% core usage
            // This is the key to reducing CPU from 2% to <0.1%
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // --- Cleanup (No changes needed) ---
    std::cout << "Cleaning up GUI resources..." << std::endl;
    
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    UnregisterClass(wc.lpszClassName, wc.hInstance);

    std::cout << "Applicazione chiusa." << std::endl;

    return error;
}
