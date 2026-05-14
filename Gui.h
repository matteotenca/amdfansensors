#ifndef AMDFANSENSORS_WINDOW_H
#define AMDFANSENSORS_WINDOW_H

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

// DirectX device creation and cleanup functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

// Main window application entry point
int OpenWin(GpuManager& manager, int refreshRateMs = 1000);
#endif //AMDFANSENSORS_WINDOW_H
