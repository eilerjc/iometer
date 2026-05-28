// main.cpp — Iometer Qt GUI entry point
//
// Usage:
//   IometerQt.exe          → DynamoEngine (listens on port 1066 for Dynamo.exe)
//   IometerQt.exe --demo   → DemoEngine  (simulated data, no Dynamo needed)
//
#include "MainWindow.h"
#include "DemoEngine.h"
#include "DynamoEngine.h"
#include <QApplication>
#include <QFont>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Iometer");
    app.setApplicationDisplayName("Iometer Qt GUI");
    app.setOrganizationName("Iometer Project");
    app.setOrganizationDomain("github.com/eilerjc/iometer");
    app.setApplicationVersion("1.1.0");

    // Default font — slightly larger for readability
    QFont defaultFont("Segoe UI", 10);
    app.setFont(defaultFont);

    // ── Command-line options ────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription("Iometer Qt GUI — I/O benchmark controller");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption demoOpt("demo",
        "Use simulated DemoEngine instead of connecting to Dynamo.exe");
    parser.addOption(demoOpt);

    parser.process(app);

    // ── Create engine ───────────────────────────────────────────────────────
    IometerEngine *engine = nullptr;
    DemoEngine    *demo   = nullptr;
    DynamoEngine  *dynamo = nullptr;

    if (parser.isSet(demoOpt)) {
        demo   = new DemoEngine;
        engine = demo;
    } else {
        dynamo = new DynamoEngine;
        engine = dynamo;
    }

    MainWindow win(engine);
    win.show();

    const int ret = app.exec();

    delete engine;
    return ret;
}
