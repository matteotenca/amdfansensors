#include "Gui.h"
#include <QApplication>
#include <QMessageBox>
#include <exception>

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	try {
		GpuManager manager;
		GuiWindow guiWindow(manager);
		return guiWindow.OpenWin() ? app.exec() : 1;
	}
	catch (const std::exception& e) {
		QMessageBox::critical(nullptr, QStringLiteral("AmdFanSensors"), QString::fromUtf8(e.what()));
		return 1;
	}
}
