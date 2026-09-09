// ReSharper disable CppDFANullDereference

#include "AmdFanSensors.h"
#include <charconv>
#include "Gui.h"
#include <QApplication>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
	try {
		GpuManager manager;

		bool showVram = false;
		bool showTemp = false;
		bool showVoltage = false;
		bool showSystemRam = false;
		bool showZeroRPM = false;
		bool toggleZeroRPM = false;
		bool showRamp = false;
		bool setRamp = false;
		bool setDefaultRamp = false;
		bool showVoltageOffset = false;
		bool setVoltageOffset = false;
		bool help = false;
		bool defaultMode = true;
		bool showTotalRam = false;
		bool enableGui = false;
		int refreshRateMs = 1000;
		int refreshRateFPSMs = 1000;
		bool fpsOnly = false;

		adlx_int voltageOffset = 0;

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
			else if (arg == "--show-v-offset" || arg == "-e") {
				if (! setVoltageOffset) {
					showVoltageOffset = true;
					defaultMode = false;
				}
			}
			else if (arg == "--set-v-offset" || arg == "-v" ||
				arg.substr(0, 15) == "--set-v-offset=" ||
				arg.substr(0, 3) == "-v=") {
				if (! showVoltageOffset) {
					if (! validateVoltageOffset(voltageOffset, arg)) {
						return 1;
					}
					setVoltageOffset = true;
					defaultMode = false;
				}
			}
			else if (arg == "--show-total-ram" || arg == "-g") { showTotalRam = true; defaultMode = false; }
			else if (arg == "--enable-gui" || arg == "-G") { enableGui = true; defaultMode = false; }
			else if (arg == "--refresh-rate" || arg == "-a=" || arg.substr(0, 3) == "-a=" || arg.substr(0, 15) == "--refresh-rate=") {
				if (! validateRefreshRate(refreshRateMs, arg)) {
					return 1;
				}
				defaultMode = false;
				enableGui = true;
			}
			else if (arg == "--refresh-rate-fps" || arg == "-A=" || arg.substr(0, 3) == "-A=" || arg.substr(0, 19) == "--refresh-rate-fps=") {
				if (! validateRefreshRate(refreshRateFPSMs, arg)) {
					return 1;
				}
				defaultMode = false;
				enableGui = true;
			}
			else if (arg == "--fps-only" || arg == "-u") { fpsOnly = true; defaultMode = false; enableGui = true;}
			else if (arg == "--gui-child") { /* internal marker for the detached GUI relaunch */ }
			else {
				std::cerr << "Warning: unknown argument '" << arg << "' ignored. Use --help for usage." << std::endl;
			}
		}

		if (help) {
			std::cout << "Usage: AmdFanSensors [OPTIONS]\n"
				<< "Options:\n"
				<< "  -r, --show-vram         Show GPU VRAM usage (MB).\n"
				<< "  -t, --show-temperature  Show GPU memory temperature (°C).\n"
				<< "  -o, --show-voltage      Show GPU voltage (mV).\n"
				<< "  -m, --show-system-ram   Show system RAM usage (MB).\n"
				<< "  -z, --show-zero-rpm     Show ZeroRPM status.\n"
				<< "  -s, --toggle-zero-rpm   Toggle ZeroRPM state.\n"
				<< "  -f, --show-fan-ramp     Show fan ramp states.\n"
				<< "  -p, --set-fan-ramp      Set fan ramp settings.\n"
				<< "  -d, --set-default-ramp  Set fan to defaults.\n"
				<< "  -e, --show-v-offset     Show voltage offset.\n"
				<< "  -v=N, --set-v-offset=N  Set voltage offset to N mV (e.g., -v=-25).\n"
				<< "                          Note: Only negative values are accepted!\n"
				<< "  -g, --show-total-ram    Show total system RAM (MB).\n"
				<< "  -G, --enable-gui        Enable the GUI (default: disabled).\n"
				<< "  -a=N, --refresh-rate=N  Set refresh rate in milliseconds (e.g., --refresh-rate=500).\n"
				<< "                          Range: 100ms to 5000ms (default: 1000ms).\n"
				<< "  -A=N, --refresh-rate-fps=N  Set FPS refresh rate in milliseconds (e.g., --refresh-rate-fps=500).\n"
				<< "                          Range: 100ms to 5000ms (default: 1000ms).\n"
				<< "  -u, --fps-only          Show only FPS in GUI (default: false).\n"
				<< "  -h, --help              Show this help message." << std::endl;
			return 0;
		}

		if (defaultMode) {
			std::cout << "No specific metrics flag provided. Showing all available metrics." << std::endl;
			manager.ShowGPUVRAM();
			manager.ShowGPUMemoryTemperature();
			manager.ShowGPUVoltage();
			manager.ShowZeroRPMStatus();
			manager.ShowGetAndSetFan(false, false);
			std::cout << "Current GPU clock voltage offset: " << manager.GetVoltageOffset() << " mV" << std::endl;
		}
		else {
			if (showTotalRam) manager.ShowTotalSystemRam();
			if (showVram) manager.ShowGPUVRAM();
			if (showTemp) manager.ShowGPUMemoryTemperature();
			if (showVoltage) manager.ShowGPUVoltage();
			if (showSystemRam) manager.ShowSystemRAM();
			if (showZeroRPM) manager.ShowZeroRPMStatus();
			else if (toggleZeroRPM) manager.ToggleZeroRPM();
			if (showRamp) manager.ShowGetAndSetFan(false, false);
			else if (setRamp || setDefaultRamp) manager.ShowGetAndSetFan(true, setDefaultRamp);
			if (showVoltageOffset) std::cout << "Current GPU clock voltage offset: " << manager.GetVoltageOffset() << " mV" << std::endl;
			else if (setVoltageOffset) manager.SetVoltageOffset(voltageOffset);
		}
		if (enableGui) {
			// When launched from a shell, relaunch ourselves as a detached process so the
			// GUI survives after the shell returns control to the prompt. The relaunched
			// instance re-enters this block with the internal --gui-child marker set.
			bool guiChildInvoc = false;
			for (int i = 1; i < argc; ++i)
				if (std::string(argv[i]) == "--gui-child") { guiChildInvoc = true; break; }

			if (!guiChildInvoc && GetConsoleWindow() != nullptr) {
				std::wstring cmdline = GetCommandLineW();
				cmdline += L" --gui-child";
				STARTUPINFOW si{};
				si.cb = sizeof(si);
				PROCESS_INFORMATION pi{};
				if (CreateProcessW(nullptr, &cmdline[0], nullptr, nullptr, FALSE,
					DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &si, &pi)) {
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
					return 0; // parent returns immediately; the detached child runs the GUI
				}
				std::cerr << "Warning: could not detach GUI process (error " << GetLastError()
					<< "); running attached." << std::endl;
			}

			QApplication app(argc, argv);
			GuiWindow guiWindow(manager, refreshRateMs, refreshRateFPSMs, fpsOnly);
			if (guiWindow.OpenWin()) {
				int res = app.exec();
				return res;
			}
			return 1;
		}

	}
	catch (const std::exception& e) {
		std::cerr << "Fatal Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

bool validateRefreshRate(int& refreshRateMs, std::string arg) {
	const size_t equalsPos = arg.find('=');
	std::string refreshStr;
	if (equalsPos != std::string::npos) {
		refreshStr = arg.substr(equalsPos + 1);
	} else {
		refreshStr = "1000";
	}

	long newRefreshRate;
	auto result = std::from_chars(refreshStr.data(), refreshStr.data() + refreshStr.size(), newRefreshRate, 10);
	if (result.ec != std::errc{}) {
		std::cerr << "Error: Invalid refresh rate value: " << refreshStr << std::endl;
		return false;
	}

	// VALIDATE: Refresh rate must be between 100ms and 5000ms
	if (newRefreshRate >= 100 && newRefreshRate <= 5000) {
		refreshRateMs = newRefreshRate;
	} else {
		std::cerr << "Error: Refresh rate must be between 100ms and 5000ms. "
			<< "Received: " << newRefreshRate << "ms. "
			<< "Using default value of 1000ms." << std::endl;
		refreshRateMs = 1000;
	}

	return true;
}

bool validateVoltageOffset(adlx_int& voltageOffset, std::string arg) {
	const size_t equalsPos = arg.find('=');

	std::string voltageStr;
	if (equalsPos != std::string::npos) {
		voltageStr = arg.substr(equalsPos + 1);
	}
	else {
		voltageStr = "0";
	}

	auto result = std::from_chars(voltageStr.data(), voltageStr.data() + voltageStr.size(), voltageOffset, 10);
	if (result.ec != std::errc{}) {
		std::cerr << "Error: Invalid voltage offest value: " << voltageStr << std::endl;
		return false;
	}
	// VALIDATE: Ensure voltage is negative (fan power saving)
	if (voltageOffset > 0) {
		std::cerr << "Error: Voltage offset must be a negative number (fan power saving). "
			<< "Received: " << voltageOffset << " mV. "
			<< "Using default value of 0 mV." << std::endl;
		voltageOffset = 0;
	}
	return true;
}