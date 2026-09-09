cmake_minimum_required(VERSION 4.2)
set(VCINSTALLDIR "$ENV{VCINSTALLDIR}")
set(CMAKE_TOOLCHAIN_FILE "${VCINSTALLDIR}vcpkg/scripts/buildsystems/vcpkg.cmake")
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
add_executable(AmdFanSensors
		WinApis.cpp
		ADLXHelper.cpp
		ADLXHelper.h
		AmdFanSensors.cpp
		AmdFanSensors.h
		GpuManager.cpp
		GpuManager.h
		Gui.cpp
		Gui.h
		Gui.ui
		resources.rc
)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /permissive- /std:c++17 ")

# Add the Qt installation path to prefix path so find_package works
list(APPEND CMAKE_PREFIX_PATH "D:/Qt67/6.7.2/msvc2019_64")

include_directories("g:\\adlx\\sdk\\include")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets EntryPointPrivate)
#find_package(imgui CONFIG REQUIRED)

target_link_libraries(AmdFanSensors PRIVATE 
    Qt6::Widgets 
    Qt6::Gui 
    Qt6::Core 
    Qt6::EntryPointPrivate
)

#add_executable(PerfMetrics
#		mainPerfAllMetrics.cpp
#		ADLXHelper.cpp
#		ADLXHelper.h
#		WinAPIs.cpp
#)
#
#add_executable(PerfGPUMetrics
#		PerfGPUMetrics.cpp
#		ADLXHelper.cpp
#		ADLXHelper.h
#		WinAPIs.cpp
#)
#
#add_executable(AutoTune
#		AutoTune.cpp
#		ADLXHelper.cpp
#		ADLXHelper.h
#		WinAPIs.cpp
#)
#
#add_executable(PerfFPSMetrics
#		mainPerfFPSMetrics.cpp
#		ADLXHelper.cpp
#		ADLXHelper.h
#		WinAPIs.cpp
#)