cmake_minimum_required(VERSION 4.2)
project(AmdFanSensors VERSION 1.0
		DESCRIPTION "AmdFanSensors is a tool to monitor AMD GPU fan speeds and temperatures using the ADLX SDK."
		LANGUAGES CXX)

# --- Qt 6 Configuration ---
# Enable automatic processing of Qt macros (Q_OBJECT, signals, slots)
set(CMAKE_AUTOMOC ON)
# Enable automatic processing of .ui files (if you use them)
set(CMAKE_AUTOUIC ON)
# Enable automatic processing of resources (.qrc)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
add_executable(AmdFanSensors WIN32
		AmdFanSensors.cpp
		GpuManager.cpp
		GpuManager.h
		Gui.cpp
		Gui.h
		Gui.ui
		SettingsWindow.ui
		resources.qrc
		resources.rc
)

# Qt: point CMake at your Qt install with -DCMAKE_PREFIX_PATH=<qt>/msvc2019_64
# (or set the Qt6_ROOT / CMAKE_PREFIX_PATH environment variable).
# ADLX: set -DADLX_SDK_DIR=<SDK folder> (the one containing Include/, ADLXHelper/ and Platform/)
# or the ADLX_SDK_DIR environment variable. Headers and helper sources are used straight from the SDK.
find_path(ADLX_SDK_ROOT NAMES Include/ADLX.h HINTS "${ADLX_SDK_DIR}" "$ENV{ADLX_SDK_DIR}")
if(NOT ADLX_SDK_ROOT)
	message(FATAL_ERROR "ADLX SDK not found. Set ADLX_SDK_DIR to the SDK folder containing Include/ADLX.h (cmake -DADLX_SDK_DIR=... or environment variable).")
endif()
target_sources(AmdFanSensors PRIVATE
	"${ADLX_SDK_ROOT}/ADLXHelper/Windows/Cpp/ADLXHelper.cpp"
	"${ADLX_SDK_ROOT}/Platform/Windows/WinAPIs.cpp")
target_include_directories(AmdFanSensors PRIVATE
	"${ADLX_SDK_ROOT}/Include"
	"${ADLX_SDK_ROOT}/ADLXHelper/Windows/Cpp")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets EntryPointPrivate)

target_link_libraries(AmdFanSensors PRIVATE 
    Qt6::Widgets 
    Qt6::Gui 
    Qt6::Core 
    Qt6::EntryPointPrivate
)
