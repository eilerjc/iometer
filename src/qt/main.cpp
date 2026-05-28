// main.cpp — Iometer Qt GUI entry point
#include "MainWindow.h"
#include "DemoEngine.h"
#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Iometer");
    app.setApplicationDisplayName("Iometer Qt GUI");
    app.setOrganizationName("Iometer Project");
    app.setOrganizationDomain("github.com/eilerjc/iometer");

    // Default font — slightly larger for readability
    QFont defaultFont("Segoe UI", 10);
    app.setFont(defaultFont);

    // Create the engine (demo mode — generates simulated I/O data)
    DemoEngine engine;

    MainWindow win(&engine);
    win.show();

    return app.exec();
}
