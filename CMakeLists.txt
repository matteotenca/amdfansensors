cmake_minimum_required(VERSION 4.2)
project(AmdFanSensors)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ig:\\adlx\\sdk\\include")
add_executable(AmdFanSensors
        AmdFanSensors.cpp
        AmdFanSensors.h
		GpuManager.cpp
		GpuManager.h
		WinApis.cpp
		ADLXHelper.cpp
		ADLXHelper.h)
