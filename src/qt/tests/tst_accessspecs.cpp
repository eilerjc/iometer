// tst_accessspecs.cpp — Tests for the built-in access specification library
// Verifies count, ordering, specific parameter values, and "All in one" distribution.
#include <QObject>
#include <QTest>
#include "IometerEngine.h"

class AccessSpecsTest : public QObject
{
    Q_OBJECT
private slots:

    // ── Count and structure ──────────────────────────────────────────────────
    void builtinCount() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        // Idle + Default + 29 patterns + "All in one" = 32
        QCOMPARE(specs.size(), 32);
    }
    void firstSpecIsIdle() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        QVERIFY(!specs.isEmpty());
        QCOMPARE(specs[0].name, QString("Idle"));
        QCOMPARE(specs[0].lines[0].sizeBytes, 0);
    }
    void secondSpecIsDefault() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        QVERIFY(specs.size() > 1);
        QCOMPARE(specs[1].name, QString("Default"));
        QVERIFY(specs[1].defaultSpec);
    }
    void lastSpecIsAllInOne() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        QVERIFY(!specs.isEmpty());
        QCOMPARE(specs.last().name, QString("All in one"));
    }

    // ── Each spec has at least one line ─────────────────────────────────────
    void allSpecsHaveLines() {
        for (const auto &spec : IometerEngine::builtinAccessSpecs())
            QVERIFY2(!spec.lines.empty(), (spec.name + " has no lines").c_str());
    }

    // ── Specific spec parameters ─────────────────────────────────────────────
    void spec_512B_read100() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        const auto it = std::find_if(specs.begin(), specs.end(),
            [](const AccessSpec &s){ return s.name == "512 B; 100% Read; 0% random"; });
        QVERIFY(it != specs.end());
        QCOMPARE(it->lines[0].sizeBytes,   512);
        QCOMPARE(it->lines[0].readPercent, 100);
        QCOMPARE(it->lines[0].seqPercent,  100); // 0% random = 100% sequential
        QCOMPARE(it->lines[0].ofSize,      100);
    }
    void spec_4KiB_aligned_random() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        const auto it = std::find_if(specs.begin(), specs.end(),
            [](const AccessSpec &s){ return s.name.rfind("4 KiB aligned; 100%", 0) == 0; });
        QVERIFY(it != specs.end());
        QCOMPARE(it->lines[0].sizeBytes,   4096);
        QCOMPARE(it->lines[0].alignBytes,  4096);
        QCOMPARE(it->lines[0].seqPercent,  0);   // 100% random
        QCOMPARE(it->lines[0].readPercent, 100);
    }
    void spec_64KiB_read100() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        const auto it = std::find_if(specs.begin(), specs.end(),
            [](const AccessSpec &s){ return s.name == "64 KiB; 100% Read; 0% random"; });
        QVERIFY(it != specs.end());
        QCOMPARE(it->lines[0].sizeBytes,   65536);
        QCOMPARE(it->lines[0].readPercent, 100);
        QCOMPARE(it->lines[0].seqPercent,  100);
    }
    void spec_256KiB_write() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        const auto it = std::find_if(specs.begin(), specs.end(),
            [](const AccessSpec &s){ return s.name == "256 KiB; 0% Read; 0% random"; });
        QVERIFY(it != specs.end());
        QCOMPARE(it->lines[0].sizeBytes,   262144);
        QCOMPARE(it->lines[0].readPercent, 0);
        QCOMPARE(it->lines[0].seqPercent,  100);
    }

    // ── "All in one" multi-line distribution ─────────────────────────────────
    void allInOne_lineCount() {
        const auto &spec = IometerEngine::builtinAccessSpecs().last();
        QCOMPARE(QString::fromStdString(spec.name), QString("All in one"));
        QCOMPARE(spec.lines.size(), 29); // 29 patterns (excluding Idle and Default)
    }
    void allInOne_ofSizeSumsTo100() {
        const auto &spec = IometerEngine::builtinAccessSpecs().last();
        int total = 0;
        for (const auto &l : spec.lines) total += l.ofSize;
        QCOMPARE(total, 100);
    }
    void allInOne_noZeroSizes() {
        const auto &spec = IometerEngine::builtinAccessSpecs().last();
        for (const auto &l : spec.lines)
            QVERIFY2(l.sizeBytes > 0, "All-in-one line has zero transfer size");
    }

    // ── No duplicate names ────────────────────────────────────────────────────
    void noDuplicateNames() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        QSet<QString> seen;
        for (const auto &s : specs) {
            QVERIFY2(!seen.contains(QString::fromStdString(s.name)), ("Duplicate spec: " + s.name).c_str());
            seen.insert(QString::fromStdString(s.name));
        }
    }

    // ── Single-line specs have ofSize == 100 ─────────────────────────────────
    void singleLineSpecs_ofSize100() {
        for (const auto &spec : IometerEngine::builtinAccessSpecs()) {
            if (spec.name == "All in one") continue;
            if (spec.lines.size() == 1)
                QVERIFY2(spec.lines[0].ofSize == 100,
                         (spec.name + " single-line ofSize != 100").c_str());
        }
    }

    // ── Smoke-test spec is present (used by smoke_test.icf) ─────────────────
    void smokeTestSpec_present() {
        const auto specs = IometerEngine::builtinAccessSpecs();
        const bool found = std::any_of(specs.begin(), specs.end(),
            [](const AccessSpec &s){ return s.name == "64 KiB; 100% Read; 0% random"; });
        QVERIFY2(found, "Smoke test spec '64 KiB; 100% Read; 0% random' missing from library");
    }
};

QTEST_GUILESS_MAIN(AccessSpecsTest)
#include "tst_accessspecs.moc"
