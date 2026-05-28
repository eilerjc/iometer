// PageDisplay.h — "Display" tab: 8 live bar charts + BigMeter button.
// Equivalent to CPageDisplay in the MFC GUI.
#pragma once

#include "IometerTypes.h"
#include <QWidget>
#include <QVector>

class BarChartWidget;
class BigMeterWidget;
class IometerEngine;
class QPushButton;
class QLabel;

class PageDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit PageDisplay(IometerEngine *engine, QWidget *parent = nullptr);

    static constexpr int NUM_CHARTS = 8;

public slots:
    void updateResults(const QVector<WorkerResult> &results);
    void onTestStarted();
    void onTestStopped();
    void refreshWorkerAssignments();

public slots:
    void onBigMeterClicked();
    void onChartBigMeterRequested(int slotIndex);

private:
    void setupUi();
    void populateCharts();

    IometerEngine   *m_engine  = nullptr;
    BarChartWidget  *m_charts[NUM_CHARTS] = {};
    BigMeterWidget  *m_bigMeter = nullptr;
    QPushButton     *m_bigMeterBtn = nullptr;
    QLabel          *m_statusLabel = nullptr;

    bool m_running = false;
};
