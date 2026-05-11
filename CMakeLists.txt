cmake_minimum_required(VERSION 4.2)
set(CMAKE_TOOLCHAIN_FILE "D:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(AmdFanSensors)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};F:/Documenti/Visual Studio 2026/AmdFanSensors - Copia (3)")
find_package(imgui CONFIG REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ig:\\adlx\\sdk\\include")
add_executable(AmdFanSensors
		WinApis.cpp
		ADLXHelper.cpp
		ADLXHelper.h
        AmdFanSensors.cpp
        AmdFanSensors.h
		GpuManager.cpp
		GpuManager.h
		)

target_link_libraries(AmdFanSensors PRIVATE imgui::imgui)