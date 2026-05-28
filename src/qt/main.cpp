// main.cpp — Standalone demo for the Iometer Qt speedometer widget.
//
// Simulates a benchmark run: the meter auto-ranges from 0 up through
// tens of thousands of IOps, showing the watermark arc accumulating
// the min/max envelope, and the needle sweeping in real time.
//
// Build: see CMakeLists.txt
// Run:   ./IometerQtMeterDemo

#include "BigMeterWidget.h"
#include "MeterWidget.h"

#include <QApplication>
#include <QTimer>
#include <cmath>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Iometer Qt Meter Demo");
    app.setOrganizationName("Iometer Project");

    // ── Build the presentation meter ────────────────────────────────────
    BigMeterWidget w;
    w.setTitle("Sequential Read  64 KiB  Q=32");
    w.setWorkerResult("Local System", "IOps");
    w.setButtonState(/*canStart=*/false, /*canStop=*/true, /*canStopAll=*/true);

    // Enable watermark so the red arc tracks the min/max envelope
    w.meter()->showWatermark = true;

    w.show();

    // ── Animation loop ───────────────────────────────────────────────────
    // Simulates a storage benchmark: IOps starts low, ramps up, oscillates,
    // occasionally spikes, and slowly scales to demonstrate auto-ranging.
    QTimer timer;
    double t     = 0.0;
    double phase = 0.0;

    QObject::connect(&timer, &QTimer::timeout, [&]() {
        t     += 0.08;
        phase += 0.13;

        // Slowly growing baseline (demonstrates auto-ranging)
        const double baseline = 5000.0 + 25000.0 * (1.0 - std::exp(-t * 0.12));

        // Oscillation around baseline
        const double wave1 = baseline * 0.30 * std::sin(phase);
        const double wave2 = baseline * 0.12 * std::sin(phase * 3.7);

        // Occasional spike (every ~12 s)
        const double spike = (std::fmod(t, 12.0) < 0.25) ? baseline * 0.90 : 0.0;

        const double iops = std::max(0.0, baseline + wave1 + wave2 + spike);

        // Format as "NN,NNN IOps"
        const auto ival = static_cast<long long>(iops);
        QString text;
        if (ival >= 1000)
            text = QString("%1,%2 IOps")
                   .arg(ival / 1000)
                   .arg(ival % 1000, 3, 10, QChar('0'));
        else
            text = QString("%1 IOps").arg(ival);

        w.updateDisplay(iops, text);
    });
    timer.start(80);   // ~12.5 fps — matches Iometer's ~1 Hz update in a smoother demo

    return app.exec();
}
