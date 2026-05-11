// ReSharper disable CppDFANullDereference
//#include "ADLXHelper.h"
#include "AmdFanSensors.h"
//#include "IPerformanceMonitoring.h"
//#include "IPerformanceMonitoring2.h"
//#include "IGPUTuning.h"
//#include "IGPUManualFanTuning.h"
//#include "IGPUManualGFXTuning.h"
#include <iostream>
#include <string>
//#include <windows.h>
//#include <vector>

int main(const int argc, char* argv[]) {
	bool showVram = false, showTemp = false, showVoltage = false, showSystemRam = false;
	bool showZeroRPM = false, toggleZeroRPM = false, showRamp = false, setRamp = false;
	bool setDefaultRamp = false, showVoltageOffset = false, setVoltageOffset = false;
	bool help = false, defaultMode = true, showTotalRam = false;
	adlx_int voltageOffset = 0;  // NEW: stores voltage offset from command line
	try {
		GpuManager manager;
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg == "--show-vram" || arg == "-r") { showVram = true; defaultMode = false; }
			else if (arg == "--show-temperature" || arg == "-t") { showTemp = true; defaultMode = false; }
			else if (arg == "--show-voltage" || arg == "-o") { showVoltage = true; defaultMode = false; }
			else if (arg == "--show-system-ram" || arg == "-m") { showSystemRam = true; defaultMode = false; }
			else if (arg == "--show-zero-rpm" || arg == "-z") { showZeroRPM = true; defaultMode = false; }
			else if (arg == "--toggle-zero-rpm" || arg == "-s") { toggleZeroRPM = true; defaultMode = false; }
			else if (arg == "--help" || arg == "-h") { help = true; break; }
			else if (arg == "--show-fan-ramp" || arg == "-f") { showRamp = true; defaultMode = false; }
			else if (arg == "--set-fan-ramp" || arg == "-p") { setRamp = true; defaultMode = false; }
			else if (arg == "--set-default-ramp" || arg == "-d") { setDefaultRamp = true; defaultMode = false; }
			else if (arg == "--show-v-offset" || arg == "-e") { showVoltageOffset = true; defaultMode = false; }
			else if ((arg == "--set-v-offset" || arg == "-v") ||
				(arg.substr(0, 15) == "--set-v-offset=") ||
				(arg.substr(0, 3) == "-v=") && ! showVoltageOffset && ! help) {
				// Extract the voltage value from the argument
				size_t equalsPos = arg.find('=');

				// Extract voltage value after the '=' sign
				std::string voltageStr;
				if (equalsPos != std::string::npos) {
					voltageStr = arg.substr(equalsPos + 1);
				}
				else {
					// No value provided, use default of 0 (or handle as needed)
					voltageStr = "0";
				}

				// Parse and validate the voltage value
				char* endptr = nullptr;
				voltageOffset = static_cast<adlx_int>(strtol(voltageStr.c_str(), &endptr, 10));

				// VALIDATE: Ensure voltage is negative (fan power saving)
				if (voltageOffset >= 0) {
					std::cerr << "Error: Voltage offset must be a negative number (fan power saving). "
						<< "Received: " << voltageOffset << " mV. "
						<< "Using default value of 0 mV." << std::endl;
					voltageOffset = 0;
				}
				else if (endptr == voltageStr.c_str()) {
					// VALIDATE: Ensure parsing was successful
					std::cerr << "Error: Invalid voltage offset value: " << arg << std::endl;
					voltageOffset = 0;
				}
				// If no equals sign and no value provided, keep default (0)

				setVoltageOffset = true; defaultMode = false;
			}
			else if (arg == "--show-total-ram" || arg == "-g") { showTotalRam = true; defaultMode = false; }
		}

		if (help) {
			std::cout << "Usage: AmdFanSensors [OPTIONS]\n"
				<< "Options:\n"
				<< "  -r, --show-vram         Show GPU VRAM usage (MB).\n"
				<< "  -t, --show-temperature  Show GPU memory temperature (°C).\n"
				<< "  -o, --show-voltage      Show GPU voltage (mV).\n"
				<< "  -m, --show-system-ram   Show system RAM usage (MB).\n"
				<< "  -e, --show-v-offset     Show voltage offset.\n"
				<< "  -v, --set-v-offset=N    Set voltage offset to N mV (e.g., --v=-25).\n"
				<< "                          Note: Only negative values are accepted!\n"
				<< "  -f, --show-fan-ramp     Show fan ramp states.\n"
				<< "  -p, --set-fan-ramp      Set fan ramp settings.\n"
				<< "  -d, --set-default-ramp  Set fan to defaults.\n"
				<< "  -z, --show-zero-rpm     Show ZeroRPM status.\n"
				<< "  -s, --toggle-zero-rpm   Toggle ZeroRPM state.\n"
				<< "  -g, --show-total-ram    Show total system RAM (MB).\n"
				<< "  -h, --help              Show this help message." << std::endl;
			return 0;
		}



		// NEW: Apply voltage offset from command line before other operations
		// if (setVoltageOffset) {
		//     manager.SetVoltageOffsetFromCLI(voltageOffset);
		// }
		// GpuManager::GpuStatusInfo status = manager.GetStatusInfo();
		// std::cout << "=== System Status Summary ===" << std::endl;
		// std::cout << "System RAM: " << status.totalSystemRamMB << " MB\n";
		// std::cout << "GPU VRAM: " << status.totalGPUMemoryMB << " MB\n";
		// std::cout << "ZeroRPM Enabled: " << (status.zeroRPMEnabled ? "Yes" : "No") << "\n";
		// std::cout << "Voltage Offset: " << status.voltageOffsetMV << " mV\n";
		// std::cout << "================================" << std::endl;

		if (defaultMode) {
			std::cout << "No specific metrics flag provided. Showing all available metrics." << std::endl;
			manager.ShowGPUVRAM();
			manager.ShowGPUMemoryTemperature();
			manager.ShowGPUVoltage();
			manager.ShowZeroRPMStatus();    // NEW: read from struct
			manager.ShowGetAndSetFan(false, false);
			std::cout << "Current GPU clock voltage offset: " << manager.m_statusInfo.voltageOffsetMV << " mV" << std::endl;
			//manager.GetGPUStates();
		}
		else {
			if (showTotalRam) manager.ShowTotalSystemMemory();
			if (showVram) manager.ShowGPUVRAM();
			if (showTemp) manager.ShowGPUMemoryTemperature();
			if (showVoltage) manager.ShowGPUVoltage();
			if (showSystemRam) manager.ShowSystemRAM();
			if (showZeroRPM) manager.ShowZeroRPMStatus();         // NEW: read from struct
			else if (toggleZeroRPM) manager.ToggleZeroRPM();      // NEW: toggle functionality
			if (showRamp) manager.ShowGetAndSetFan(false, false);
			else if (setRamp || setDefaultRamp) manager.ShowGetAndSetFan(true, setDefaultRamp);
			if (showVoltageOffset) std::cout << "Current GPU clock voltage offset: " << manager.m_statusInfo.voltageOffsetMV << " mV" << std::endl;
			else if (setVoltageOffset) manager.SetVoltageOffsetFromCLI(voltageOffset);  // Apply if not already done in defaultMode
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Fatal Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
