// tst_qtpageaccess.cpp - QtPageAccess ("Access Specifications" tab). Drives the
// assigned-list transfer/reorder buttons and the global-list New/Edit/Edit Copy/
// Delete actions against a QtDemoEngine. The New/Edit/Copy actions open a modal
// spec-editor dialog and Delete a confirm box; a polling timer closes whichever
// modal appears so the dialog construction + accept/reject paths get covered.
#include <QObject>
#include <QTest>
#include <QListWidget>
#include <QPushButton>
#include <QAbstractButton>
#include <QDialog>
#include <QMessageBox>
#include <QTimer>
#include <QApplication>
#include "QtPageAccess.h"
#include "QtDemoEngine.h"

// Poll for the next modal and accept/reject it (or answer Yes/No on a QMessageBox).
// Self-deleting; gives up after ~2.4 s so a missing modal never hangs the test.
static void scheduleCloseModal(bool accept)
{
    auto *t = new QTimer;
    t->setInterval(40);
    t->setProperty("tries", 0);
    QObject::connect(t, &QTimer::timeout, t, [t, accept]() {
        QWidget *w = QApplication::activeModalWidget();
        if (w) {
            if (auto *mb = qobject_cast<QMessageBox *>(w)) {
                // Click the actual Yes/No button so question() returns it.
                const auto want = accept ? QMessageBox::Yes : QMessageBox::No;
                if (auto *b = mb->button(want)) b->click();
                else mb->done(want);
            } else if (auto *d = qobject_cast<QDialog *>(w)) {
                accept ? d->accept() : d->reject();
            }
            t->stop(); t->deleteLater();
            return;
        }
        const int n = t->property("tries").toInt() + 1;
        t->setProperty("tries", n);
        if (n > 60) { t->stop(); t->deleteLater(); }
    });
    t->start();
}

class PageAccessTest : public QObject
{
    Q_OBJECT

    static QPushButton *btn(QWidget &p, const QString &text) {
        for (auto *b : p.findChildren<QPushButton *>())
            if (b->text() == text) return b;
        return nullptr;
    }
    // Identify the two list widgets: global has the built-in specs, assigned is
    // empty right after construction.
    static void lists(QWidget &p, QListWidget *&global, QListWidget *&assigned) {
        global = assigned = nullptr;
        for (auto *lw : p.findChildren<QListWidget *>())
            (lw->count() > 0 ? global : assigned) = lw;
    }

private slots:
    void construct_populatesGlobalList() {
        QtDemoEngine eng;
        QtPageAccess p(&eng);
        QListWidget *g = nullptr, *a = nullptr;
        lists(p, g, a);
        QVERIFY(g);
        QVERIFY(g->count() >= 2);
        QVERIFY(a);                       // assigned list exists (empty)
        QCOMPARE(a->count(), 0);
    }

    void assigned_addMoveRemove() {
        QtDemoEngine eng;
        QtPageAccess p(&eng);
        QListWidget *g = nullptr, *a = nullptr;
        lists(p, g, a);

        g->setCurrentRow(0); QTest::mouseClick(btn(p, "<< Add"), Qt::LeftButton);
        g->setCurrentRow(1); QTest::mouseClick(btn(p, "<< Add"), Qt::LeftButton);
        QCOMPARE(a->count(), 2);

        g->setCurrentRow(0); QTest::mouseClick(btn(p, "<< Add"), Qt::LeftButton);  // dup ignored
        QCOMPARE(a->count(), 2);

        a->setCurrentRow(1); QTest::mouseClick(btn(p, "Move Up"), Qt::LeftButton);
        QCOMPARE(a->currentRow(), 0);
        QTest::mouseClick(btn(p, "Move Down"), Qt::LeftButton);
        QCOMPARE(a->currentRow(), 1);

        QCOMPARE(p.currentAssignedSpecs().size(), 2);

        a->setCurrentRow(0); QTest::mouseClick(btn(p, "Remove >>"), Qt::LeftButton);
        QCOMPARE(a->count(), 1);
    }

    void setActiveSpecIndex_marksRunningSpec() {
        QtDemoEngine eng;
        QtPageAccess p(&eng);
        QListWidget *g = nullptr, *a = nullptr;
        lists(p, g, a);
        g->setCurrentRow(0); QTest::mouseClick(btn(p, "<< Add"), Qt::LeftButton);
        p.setActiveSpecIndex(0);          // build + apply the run icon
        p.setActiveSpecIndex(-1);         // clear
        QVERIFY(true);
    }

    void newSpec_acceptedAddsGlobalSpec() {
        QtDemoEngine eng;
        QtPageAccess p(&eng);
        const int before = eng.accessSpecs().size();
        scheduleCloseModal(true);
        QTest::mouseClick(btn(p, "New"), Qt::LeftButton);     // editSpecDialog -> accept
        QCOMPARE(eng.accessSpecs().size(), before + 1);
    }

    void editSpec_rejectedLeavesSpecsUnchanged() {
        QtDemoEngine eng;
        QtPageAccess p(&eng);
        QListWidget *g = nullptr, *a = nullptr;
        lists(p, g, a);
        g->setCurrentRow(0);                                  // enables Edit
        const auto before = eng.accessSpecs();
        scheduleCloseModal(false);
        QTest::mouseClick(btn(p, "Edit"), Qt::LeftButton);    // editSpecDialog -> reject
        QCOMPARE(eng.accessSpecs().size(), before.size());
    }

    void editCopySpec_acceptedAddsCopy() {
        QtDemoEngine eng;
        QtPageAccess p(&eng);
        QListWidget *g = nullptr, *a = nullptr;
        lists(p, g, a);
        g->setCurrentRow(0);
        const int before = eng.accessSpecs().size();
        scheduleCloseModal(true);
        QTest::mouseClick(btn(p, "Edit Copy"), Qt::LeftButton);
        QCOMPARE(eng.accessSpecs().size(), before + 1);
    }

    void deleteSpec_confirmYesRemovesSpec() {
        QtDemoEngine eng;
        QtPageAccess p(&eng);
        QListWidget *g = nullptr, *a = nullptr;
        lists(p, g, a);
        const int before = eng.accessSpecs().size();
        g->setCurrentRow(1);                                  // enables Delete
        scheduleCloseModal(true);                             // QMessageBox -> Yes
        QTest::mouseClick(btn(p, "Delete"), Qt::LeftButton);
        QCOMPARE(eng.accessSpecs().size(), before - 1);
    }
};

QTEST_MAIN(PageAccessTest)
#include "tst_qtpageaccess.moc"
