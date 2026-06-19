// tst_qtpageresults.cpp - QtPageResults (the "Test Setup" page: description, run
// time, ramp, record-results, cycling mode + worker/target/IOQ cycling fields).
// Covers loadConfig (engine -> widgets, via the ctor) and saveConfig (widgets ->
// engine, triggered by a field change) - the latter never ran without a real edit.
#include <QObject>
#include <QTest>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include "QtPageResults.h"
#include "QtDemoEngine.h"

class PageResultsTest : public QObject
{
    Q_OBJECT

    static TestConfig sampleConfig() {
        TestConfig c;
        c.description = "round trip"; c.runHours = 1; c.runMinutes = 2; c.runSeconds = 42;
        c.rampSeconds = 5; c.recordResults = 1; c.cyclingMode = 2;
        c.workerStart = 3; c.workerStep = 4; c.workerStepping = 1;
        c.targetStart = 5; c.targetStep = 6; c.targetStepping = 0;
        c.ioqStart = 7; c.ioqEnd = 8; c.ioqPower = 2; c.ioqStepping = 1;
        return c;
    }

private slots:
    void load_populatesWidgetsFromEngine() {
        QtDemoEngine eng;
        eng.setTestConfig(sampleConfig());
        QtPageResults page(&eng);                       // ctor -> loadConfig
        QCOMPARE(page.findChild<QLineEdit *>()->text(), QString("round trip"));
    }

    void editField_savesWholeConfigBack() {
        QtDemoEngine eng;
        eng.setTestConfig(sampleConfig());
        QtPageResults page(&eng);
        // Change the first combo (Record Results) -> saveConfig writes the whole
        // page state back, so the untouched fields must round-trip via the widgets.
        auto combos = page.findChildren<QComboBox *>();
        QVERIFY(combos.size() >= 2);
        const int newRec = (1 + 1) % combos[0]->count();
        combos[0]->setCurrentIndex(newRec);             // -> saveConfig
        const TestConfig out = eng.testConfig();
        QCOMPARE(out.recordResults, newRec);
        QCOMPARE(out.runSeconds, 42);                   // round-tripped (proves the reads)
        QCOMPARE(out.cyclingMode, 2);
        QCOMPARE(out.workerStart, 3);
        QCOMPARE(out.ioqEnd, 8);
        QCOMPARE(out.description, std::string("round trip"));
    }

    void cyclingModeChange_saved() {
        QtDemoEngine eng;
        QtPageResults page(&eng);
        auto combos = page.findChildren<QComboBox *>();
        const int mode = (combos[1]->currentIndex() + 1) % combos[1]->count();
        combos[1]->setCurrentIndex(mode);               // cycling mode -> saveConfig
        QCOMPARE(eng.testConfig().cyclingMode, mode);
    }
};

QTEST_MAIN(PageResultsTest)
#include "tst_qtpageresults.moc"
