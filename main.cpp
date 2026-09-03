#include "trayicon.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Keep running even when all windows are closed
    app.setQuitOnLastWindowClosed(false);

    app.setApplicationName("Fast Paste");
    app.setOrganizationName("qt-fast-copy");

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "Fast Paste",
            "System tray is not available on this desktop.");
        return 1;
    }

    TrayIcon tray;

    return app.exec();
}
