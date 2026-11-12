# AmdFanSensors

A small Windows desktop app (Qt 6 Widgets, C++17) that monitors an AMD GPU and lets you tune its fan curve, Zero RPM mode and voltage offset. It talks to the GPU through AMD's [ADLX SDK](https://github.com/GPUOpen-LibrariesAndSDKs/ADLX).

It is a pure GUI application: it takes no command-line arguments and opens no console window.

## Contents

- [Features](#features)
- [Requirements](#requirements)
- [Using the app](#using-the-app)
  - [Main window](#main-window)
  - [Settings window](#settings-window)
  - [Fan curve files (`.afcc`)](#fan-curve-files-afcc)
  - [Where settings are stored](#where-settings-are-stored)
- [Building](#building)
  - [Prerequisites](#prerequisites)
  - [Configure and build with CMake](#configure-and-build-with-cmake)
  - [IDEs](#ides)
- [Project layout](#project-layout)
- [Troubleshooting](#troubleshooting)
- [Notes and limitations](#notes-and-limitations)

## Features

- Live hardware metrics: GPU, hotspot and memory temperatures, fan speed (RPM and %), GPU usage, VRAM usage and system RAM usage.
- FPS monitor: current, average, minimum and maximum FPS, with a reset button.
- Editable fan curve: drag points on a graph, then apply the curve to the GPU.
- Zero RPM toggle (on GPUs that support it).
- Manual GPU voltage offset (negative values only).
- Export and import of fan curves as plain-text files.
- Dark and light themes, "always on top" option, adjustable refresh rates.
- The window layout, theme, refresh rates and fan curve are remembered between runs.

## Requirements

To run the app:

- Windows 10 or newer, 64-bit.
- An AMD GPU with the AMD Radeon driver (Adrenalin) installed. ADLX ships with the driver; the app fails to start without it.
- Qt 6 runtime DLLs next to the executable (or on `PATH`), e.g. produced with `windeployqt`.

Which features work depends on the GPU and driver. If a feature is not supported (for example Zero RPM, voltage tuning or memory temperature), the matching control is disabled or shows `--`.

> **Tuning warning.** Fan curve, Zero RPM and voltage offset changes are written straight to the GPU. A bad fan curve can cause overheating. Make small changes, watch the temperatures, and keep an eye on stability. Use at your own risk.

## Using the app

On start the app initialises ADLX, selects the first GPU, restores your saved window position and opens the main window. If initialisation fails, an error dialog is shown and the app exits.

### Main window

The window is split into collapsible sections. Click a section's title bar to collapse or expand it; the state is remembered.

**Hardware Metrics**

- Tick **Enable Hardware Stats Gathering** to start polling the GPU. Unticked, the values show `--` and no polling happens.
- Shows GPU, Hotspot and Memory temperatures, VRAM used, system memory (with a bar), fan speed (RPM and %), GPU memory and GPU usage.
- Polling interval is set in the Settings window (Update Frequency).

**GPU Tuning / Controls**

- A fan curve graph of temperature (°C) against fan speed (%). The point currently in use is marked with the live temperature and fan value.
- When manual fan curve mode is enabled (see [Settings window](#settings-window)), the curve points can be **dragged with the mouse**. Points can only move between their neighbours' temperatures, so the curve always stays non-decreasing in temperature, and speeds are limited to the range the GPU accepts.
- **Apply Fan Curve** sends the edited curve to the GPU. The button is enabled only when manual mode is on and there are pending edits. Errors are shown in a dialog.

**FPS**

- Tick **Enable FPS/Stats Monitoring** to collect FPS data. Shows the current FPS, the average, and min / max.
- **Reset FPS Data** clears the statistics.
- FPS comes from ADLX's performance monitoring and reflects the application currently rendering on the GPU.

**Menu**

- **File → Settings** opens the Settings window.

### Settings window

**Window**

- **Always on Top**: keep the main window above other windows.
- **Light Theme**: switch between the dark (default) and light theme.

**Update Frequency**

- **Hardware Data** and **FPS Monitor** sliders set the polling interval for each, from 100 ms to 5000 ms (default 1000 ms). Changes apply immediately.

**Tuning**

- **Enable Zero RPM**: lets the fans stop below a temperature threshold. Disabled if the GPU does not support it.
- **Manual Fan Curve Profile**: switches between the GPU's default curve and your own custom curve. Turning it on immediately applies your last custom curve to the GPU and lets you edit it in the main window; turning it off immediately applies the default curve. Your custom curve is kept so it can be restored when you turn it on again.
- **Manual Voltage Offset**: enables the slider and spin box (in mV) to set a GPU voltage offset. Only negative values (undervolting) are accepted; the range comes from the GPU. The value is saved, and at start-up the saved value is re-applied to the GPU if it differs.
- **Export Curve** / **Import Curve**: save the current custom curve to a file or load one from a file (see below). An imported curve becomes your custom curve; press **Apply Fan Curve** to send it to the GPU.

### Fan curve files (`.afcc`)

Curves are exchanged as plain UTF-8 text, one point per line: temperature in °C, then fan speed in %, separated by spaces or tabs. Blank lines and lines starting with `#` are ignored.

```text
# AmdFanSensors fan curve (tempC speedPercent)
30 30
50 44
56 58
63 72
71 100
```

On import the file is validated and rejected with a message if:

- a line does not contain exactly two integers,
- temperatures decrease from one line to the next,
- a temperature or fan speed is outside the range the GPU supports,
- the file has no points, or more points than the GPU supports.

### Where settings are stored

Settings are saved with `QSettings` under the organisation/application name `AmdFanSensors` / `AmdFanSensors`, which on Windows is the registry key `HKEY_CURRENT_USER\Software\AmdFanSensors\AmdFanSensors`. Stored values include window and settings-window geometry, collapsed sections, theme, always-on-top, refresh rates, the voltage offset and the custom fan curve. Delete that key to reset the app to defaults.

## Building

### Prerequisites

| Tool | Notes |
| --- | --- |
| Windows 10/11 x64 | The app is Windows-only. |
| MSVC (Visual Studio 2022 or newer, "Desktop development with C++") | Run the build from an environment where `cl.exe` is available (e.g. a "x64 Native Tools" prompt, or after `vcvarsall.bat x64`). |
| CMake 4.2 or newer | Required by `cmake_minimum_required`. |
| Ninja (optional) | Used by the examples below; any CMake generator works. |
| Qt 6 (developed with 6.7.2, `msvc2019_64` kit) | Needs the `Core`, `Gui`, `Widgets` modules. `EntryPointPrivate` ships with Qt and is used for the Windows entry point. |
| ADLX SDK | **Not included in this repository.** Download it from the [ADLX GitHub repository](https://github.com/GPUOpen-LibrariesAndSDKs/ADLX). The build uses the SDK's headers (`Include/`) and two of its source files (`ADLXHelper/Windows/Cpp/ADLXHelper.cpp`, `Platform/Windows/WinAPIs.cpp`) directly from the SDK folder; they are not copied into this repository. |

No package manager is used: there is no vcpkg or Conan dependency.

### Configure and build with CMake

`CMakeLists.txt` contains no hard-coded paths. Tell CMake where Qt and the ADLX headers are:

| What | CMake variable | Environment variable alternative |
| --- | --- | --- |
| Qt 6 | `-DCMAKE_PREFIX_PATH=<Qt>/6.7.2/msvc2019_64` | `Qt6_ROOT` |
| ADLX SDK | `-DADLX_SDK_DIR=<ADLX SDK folder>` (the `SDK` folder containing `Include/`, `ADLXHelper/` and `Platform/`) | `ADLX_SDK_DIR` |

If the SDK cannot be found, configuration stops with `ADLX SDK not found. Set ADLX_SDK_DIR ...`.

From a developer command prompt (so MSVC is on the path):

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH=D:/Qt/6.7.2/msvc2019_64 ^
      -DADLX_SDK_DIR=C:/adlx/sdk
cmake --build build --parallel
```

Or with environment variables set once:

```bat
set Qt6_ROOT=D:\Qt\6.7.2\msvc2019_64
set ADLX_SDK_DIR=C:\adlx\sdk
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The result is `build\AmdFanSensors.exe`. The target is a Windows-subsystem (GUI) executable, and Qt's moc, uic and rcc steps run automatically (`AUTOMOC`, `AUTOUIC`, `AUTORCC`).

To run it, make the Qt DLLs available. The simplest way is `windeployqt`:

```bat
D:\Qt\6.7.2\msvc2019_64\bin\windeployqt.exe build\AmdFanSensors.exe
```

### Stale build directories

If Visual Studio or the MSVC toolset is updated, the cached compiler path in an old build directory can go stale (`... cl.exe is not a full path to an existing compiler tool`). Delete `CMakeCache.txt` and the `CMakeFiles` folder in the build directory and configure again.

### IDEs

- **CLion / Qt Creator / VS Code with CMake Tools / Visual Studio ("Open Folder")**: open the project folder as a CMake project, set `CMAKE_PREFIX_PATH` and `ADLX_SDK_DIR` in the IDE's CMake settings (or environment), and build.
- Use an x64 MSVC toolchain. MinGW is not supported (the project uses MSVC-targeted Qt and ADLX).
- CMake is the only build system; there are no Visual Studio `.sln`/`.vcxproj` files.

## Project layout

| File | Purpose |
| --- | --- |
| `AmdFanSensors.cpp` | `main`: creates the Qt application, the `GpuManager` and the main window. |
| `Gui.cpp`, `Gui.h` | Main window, settings window, fan curve widget and collapsible group boxes. |
| `Gui.ui`, `SettingsWindow.ui` | Qt Designer layouts for the main and settings windows. |
| `GpuManager.cpp`, `GpuManager.h` | ADLX wrapper: metrics, FPS statistics, fan curve, Zero RPM, voltage offset. |
| `resources.qrc`, `resources.rc`, `icon.ico` | Window icon (Qt resource) and executable icon (Windows resource). |
| `CMakeLists.txt` | Build definition. |

Diagnostic messages use Qt logging (`qInfo` / `qWarning`). Because the app has no console, they are only visible in a debugger output window (e.g. Visual Studio's Output pane or DebugView), or on stderr if you start the app with `QT_ASSUME_STDERR_HAS_CONSOLE=1` from a console.

## Troubleshooting

| Symptom | Likely cause / fix |
| --- | --- |
| Configure fails with `ADLX SDK not found` | Set `ADLX_SDK_DIR` (variable or environment) to the SDK folder containing `Include/ADLX.h`. |
| `Could not find a package configuration file provided by "Qt6"` | Set `CMAKE_PREFIX_PATH` (or `Qt6_ROOT`) to the Qt kit that matches your compiler, e.g. `.../6.7.2/msvc2019_64`. |
| `cl.exe is not a full path to an existing compiler tool` | Stale CMake cache after a Visual Studio update; clear the cache and configure from an MSVC environment. |
| App exits immediately with an error dialog | ADLX could not initialise: check that an AMD GPU is present and the AMD driver is installed and up to date. |
| "The code execution cannot proceed because Qt6*.dll was not found" | Run `windeployqt` on the executable or add the Qt `bin` folder to `PATH`. |
| A tuning control is greyed out | The GPU or driver does not support it (e.g. Zero RPM, manual fan tuning, voltage tuning). |
| Metrics show `--` | Hardware stats gathering is off, or the GPU does not report that sensor. |

## Notes and limitations

- Only the first GPU reported by ADLX is used.
- Voltage offsets are limited to zero or negative values.
- Tuning writes to the GPU; behaviour after a reboot or driver restart depends on the driver, and the app re-applies the saved voltage offset at start-up.
- The ADLX SDK is subject to its own licence; it is not redistributed with this project: its headers and helper sources (`ADLXHelper.cpp`, `WinAPIs.cpp`) are compiled straight from your local SDK copy.
