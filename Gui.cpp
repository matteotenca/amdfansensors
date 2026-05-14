#include "Gui.h"

#include <chrono>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <imgui_internal.h>
#include <d3d11.h>
#include <iostream>
#include <thread>

// --- Global DirectX variables removed: Now members of GuiWindow ---

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
            // For now, we keep the placeholder return.
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// --- GuiWindow Implementation ---

// /**
//  * @brief Constructor initializes necessary pointers to null.
//  */
// GuiWindow::GuiWindow(HWND hWnd) : m_hWnd(hWnd),
//                                  m_pd3dDevice(nullptr),
//                                  m_pd3dDeviceContext(nullptr),
//                                  m_pSwapChain(nullptr),
//                                  m_mainRenderTargetView(nullptr) {}


// Implementation of CreateDeviceD3D, now a member function
bool GuiWindow::CreateDeviceD3D() {
    // Setup DirectX
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

    // Use the class member pointers for initialization
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
                                      featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_pSwapChain,
                                      &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext) != S_OK)
        return false;

    // Now that m_pSwapChain is initialized, we can create the render target view
    // We must first resize the swap chain to the window dimensions (if possible).
    // For simplicity in this refactor, we assume CreateRenderTarget handles initial sizing correctly.
    CreateRenderTarget();
    return true;
}

void GuiWindow::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    // Get the buffer from the swap chain
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

    // Create and assign the render target view to the class member
    m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRenderTargetView);
    pBackBuffer->Release();
}

// Implementation of CleanupRenderTarget (Uses class members)
void GuiWindow::CleanupRenderTarget() {
    if (m_mainRenderTargetView) {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}

// Implementation of CleanupDeviceD3D (Uses class members)
void GuiWindow::CleanupDeviceD3D() {
    CleanupRenderTarget(); // Clean RTV first
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

// Destructor ensures proper cleanup when the object goes out of scope.
GuiWindow::~GuiWindow() {
    CleanupDeviceD3D();
}

/**
 * @brief Initializes all graphics resources and sets up ImGui context.
 */
bool GuiWindow::Initialize(int refreshRate) {
    // Step 1: Initialize DirectX using the member function
    if (!CreateDeviceD3D()) {
        std::cerr << "Failed to create DirectX device." << std::endl;
        return false;
    }

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(m_hWnd);
    // Use class members for initialization
    ImGui_ImplDX11_Init(m_pd3dDevice, m_pd3dDeviceContext);
    refreshRateMs = refreshRate;
    return true;
}

/**
 * @brief Runs the main event and rendering loop. This replaces the previous procedural loop body.
 */
void GuiWindow::RunLoop() const {
    // Main loop message handling setup
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    std::cout << "Finestra aperta. Premere ESC per uscire." << std::endl;
    constexpr float clear_color_with_alpha[4] = {0.45f, 0.55f, 0.60f, 1.00f};

    std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
    // std::chrono::milliseconds updateInterval(refreshRateMs); // Assuming a fixed interval for the loop structure demonstration

    // Main message handling and rendering loop
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Demo window (will be replaced with GPU temperature display)
        ImGui::Begin("GPU Temperature Monitor");

        ImGui::Text("Monitor Temperatura GPU");
        ImGui::Separator();

        // Display refresh rate info - Note: The actual refreshRateMs parameter was consumed during initialization.
        // We use the constant defined earlier for demonstration purposes, or ideally pass it into RunLoop if needed.
        // Assuming we know the desired interval here (e.g., 100ms update).
        ImGui::Text("Refresh Rate: %d ms", refreshRateMs); // Placeholder value

        // Only update metrics if the interval has passed
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastUpdateTime) >= std::chrono::milliseconds(refreshRateMs)) {
            // Use the manager object passed to OpenWin, which should be accessible here.
            // Since RunLoop is a member function and doesn't take `GpuManager`,
            // we must adjust OpenWin to manage this state flow. (See final implementation below).
            lastUpdateTime = std::chrono::high_resolution_clock::now();
        }

        // Display metrics every frame (using cached values)
        // Placeholder access: We rely on the manager object existing in scope during the main loop execution.
        /* ImGui::Text("Temperatura GPU: %0.1f °C", g_manager.m_temperatureInfo.gpuTemperatureC); */
        ImGui::End();

        // Rendering
        ImGui::Render();
        m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, nullptr);
        m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_pSwapChain->Present(1, 0);
    }
}


// Main window application entry point. Now manages the object lifecycle.
int GuiWindow::OpenWin(GpuManager& manager, int refreshRate) {
    // Create window context (Standard boilerplate remains)
    refreshRateMs = refreshRate;
    WNDCLASSEX wc;
    wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr,
        nullptr,
        static_cast<LPCSTR>("ImGuiWindow"), nullptr
    };
    RegisterClassEx(&wc);

    // Use the original dimensions for window creation
    m_hWnd = CreateWindow(wc.lpszClassName, reinterpret_cast<LPCSTR>(L"GPU Temperature Monitor"),
                             WS_OVERLAPPEDWINDOW, 100, 100, 640, 480, nullptr, nullptr, wc.hInstance, nullptr);

    // --- Refactoring Point: Use the GuiWindow object ---
    // GuiWindow gui(hwnd);

    std::cout << "Initializing GUI resources..." << std::endl;
    if (!Initialize(refreshRateMs)) {
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(m_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hWnd);


    // --- Main loop execution (Need to capture the scope of OpenWin to pass manager state) ---
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    std::cout << "Finestra aperta. Premere ESC per uscire." << std::endl;
    constexpr float clear_color_with_alpha[4] = {0.45f, 0.55f, 0.60f, 1.00f};

    std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds updateInterval(refreshRateMs);


    // The main loop logic must be contained within OpenWin scope to access 'manager'
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Demo window (will be replaced with GPU temperature display)
        ImGui::Begin("GPU Temperature Monitor");

        ImGui::Text("Monitor Temperatura GPU");
        ImGui::Separator();

        // Display refresh rate info
        ImGui::Text("Refresh Rate: %d ms", refreshRateMs);

        // Only update metrics if the interval has passed (Using manager object in scope)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - lastUpdateTime) >= updateInterval) {
            manager.UpdateGpuTempStruct();
            lastUpdateTime = std::chrono::high_resolution_clock::now();
        }

        // Display metrics every frame (using cached values from the local manager object)
        ImGui::Text("Temperatura GPU: %0.1f °C", manager.m_temperatureInfo.gpuTemperatureC);
        ImGui::Text("Temperatura GPU Memory: %0.1f °C", manager.m_temperatureInfo.gpuMemoryTemperatureC);
        ImGui::Text("Temperatura GPU Hotspot: %0.1f °C", manager.m_temperatureInfo.gpuHotspotTemperatureC);
        ImGui::Text("Velocità Ventola GPU: %d RPM", manager.m_temperatureInfo.fanSpeed);
        ImGui::Text("VRAM GPU occupata: %d MB", manager.m_temperatureInfo.vRam);
        ImGui::Text("VRAM GPU libera: %d MB", manager.m_statusInfo.totalGPUMemoryMB - manager.m_temperatureInfo.vRam);

        ImGui::End();

        // Rendering (Using the internal resources of the 'gui' object)
        ImGui::Render();
        m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, nullptr); // Direct access needed due to scope/design pattern limitation
        m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_pSwapChain->Present(1, 0);
    }

    // --- Cleanup (Destructor handles cleanup of resources) ---
    std::cout << "Cleaning up GUI resources..." << std::endl;
    // The 'gui' object goes out of scope here, triggering the destructor and releasing all DX resources.

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    UnregisterClass(wc.lpszClassName, wc.hInstance);

    std::cout << "Applicazione chiusa." << std::endl;

    return 0;
}