cmake_minimum_required(VERSION 4.2)
set(VCINSTALLDIR "$ENV{VCINSTALLDIR}")
set(CMAKE_TOOLCHAIN_FILE "${VCINSTALLDIR}vcpkg/scripts/buildsystems/vcpkg.cmake")
project(AmdFanSensors VERSION 1.0
		DESCRIPTION "AmdFanSensors is a tool to monitor AMD GPU fan speeds and temperatures using the ADLX SDK."
		LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_SOURCE_DIR}")
find_package(imgui CONFIG REQUIRED)
#set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES};g:\\adlx\\sdk\\include")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ig:\\adlx\\sdk\\include")
include_directories("g:\\adlx\\sdk\\include")
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
)

target_link_libraries(AmdFanSensors PRIVATE imgui::imgui)
target_link_libraries(AmdFanSensors PRIVATE
		d3d11
)
#		d3dcompiler
#		dxgi
#)