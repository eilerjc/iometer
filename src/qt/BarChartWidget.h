// BarChartWidget.h — Horizontal performance bar chart, one worker/metric slot.
// Equivalent to CBargraph in the MFC GUI.
#pragma once

#include "IometerTypes.h"
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QString>

class BarChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BarChartWidget(QWidget *parent = nullptr);

    // Assign a worker to this chart slot
    void setWorker(const QString &managerName, const QString &workerName);

    // Feed in a fresh result set; widget updates if it holds a matching worker
    void updateResults(const QVector<WorkerResult> &results);

    // Force a particular result type (e.g., from BigMeter's "same bar" selection)
    void setResultType(int resultType);
    int  resultType() const { return m_resultType; }

    QString workerName()  const { return m_workerName; }
    QString managerName() const { return m_managerName; }
    double  currentValue() const { return m_value; }

signals:
    void resultTypeChanged(int type);
    void bigMeterRequested(int slotIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void onMetricChanged(int index);

private:
    void updatePeak();

    QComboBox *m_metricCombo = nullptr;
    QLabel    *m_workerLabel = nullptr;
    QLabel    *m_valueLabel  = nullptr;

    QString  m_managerName;
    QString  m_workerName;
    int      m_resultType = RESULT_IOPS;
    double   m_value      = 0.0;
    double   m_peak       = 0.0;
    double   m_maxRange   = 100.0;   // auto-ranging high watermark

    int  m_barTop    = 0;   // geometry cached on resize
    int  m_barHeight = 0;
    int  m_barLeft   = 0;
    int  m_barRight  = 0;

    static constexpr int COMBO_H  = 22;
    static constexpr int LABEL_H  = 18;
    static constexpr int MARGIN   =  4;
};
