// tst_accessspeccatalog - the shared iocore access-spec naming + default catalog.
#include <QtTest>
#include "AccessSpecCatalog.h"

using namespace iocore;

class TestAccessSpecCatalog : public QObject {
    Q_OBJECT
private slots:
    void formatSize_units();
    void accessSpecLabel_singleLine();
    void accessSpecLabel_randomIsInverseOfSeq();
    void accessSpecLabel_multiLineUsesName();
    void defaultSpecs_haveIdleDefaultAndAllInOne();
    void defaultSpecs_allInOneDistributesToHundred();
    void fillWireAccess_mapsFriendlyToWire();
    void smartNameText_formats();
};

// A stand-in for a wire access-spec row (canonical Access_Spec / Qt DyAccessSpec):
// any struct with these field names works with iocore::fillWireAccess.
namespace { struct FakeWire { int of_size, reads, random, delay, burst;
                              unsigned reply, size, align; }; }

void TestAccessSpecCatalog::formatSize_units() {
    QCOMPARE(formatSize(0),       std::string("0 B"));
    QCOMPARE(formatSize(-5),      std::string("0 B"));
    QCOMPARE(formatSize(512),     std::string("512 B"));
    QCOMPARE(formatSize(1024),    std::string("1 KiB"));
    QCOMPARE(formatSize(65536),   std::string("64 KiB"));
    QCOMPARE(formatSize(1048576), std::string("1 MiB"));
}

void TestAccessSpecCatalog::accessSpecLabel_singleLine() {
    AccessSpec s; s.name = "whatever";
    AccessSpecLine l; l.sizeBytes = 65536; l.readPercent = 100; l.seqPercent = 100;
    s.lines.push_back(l);
    QCOMPARE(accessSpecLabel(s), std::string("64 KiB; 100% Read; 0% random"));
}

void TestAccessSpecCatalog::accessSpecLabel_randomIsInverseOfSeq() {
    AccessSpec s;
    AccessSpecLine l; l.sizeBytes = 4096; l.readPercent = 50; l.seqPercent = 0; // fully random
    s.lines.push_back(l);
    QCOMPARE(accessSpecLabel(s), std::string("4 KiB; 50% Read; 100% random"));
}

void TestAccessSpecCatalog::accessSpecLabel_multiLineUsesName() {
    AccessSpec s; s.name = "All in one";
    s.lines.push_back(AccessSpecLine{});
    s.lines.push_back(AccessSpecLine{});
    QCOMPARE(accessSpecLabel(s), std::string("All in one"));
}

void TestAccessSpecCatalog::defaultSpecs_haveIdleDefaultAndAllInOne() {
    auto specs = defaultAccessSpecs();
    QVERIFY(specs.size() > 3);
    QCOMPARE(specs.front().name, std::string("Idle"));
    bool hasDefault = false, hasAllInOne = false;
    for (const auto &s : specs) {
        if (s.name == "Default") {
            hasDefault = true;  QVERIFY(s.defaultSpec); QCOMPARE(s.lines.size(), size_t(1));
            // The Default spec must match the canonical MFC InitAccessSpecLine
            // values (2 KiB, 67% read, 100% random, request-size aligned), so the
            // two GUIs agree on the default workload. (Saved ICF row would be
            // 2048,100,67,100,0,1,2048,0.)
            const AccessSpecLine &l = s.lines[0];
            QCOMPARE(l.sizeBytes,   2048);
            QCOMPARE(l.readPercent, 67);
            QCOMPARE(l.seqPercent,  0);      // 100% random
            QCOMPARE(l.alignBytes,  2048);   // aligned to the request size
            QCOMPARE(l.ofSize,      100);
        }
        if (s.name == "All in one")  { hasAllInOne = true; QVERIFY(s.lines.size() > 1); }
    }
    QVERIFY(hasDefault);
    QVERIFY(hasAllInOne);
}

void TestAccessSpecCatalog::defaultSpecs_allInOneDistributesToHundred() {
    for (const auto &s : defaultAccessSpecs()) {
        if (s.name != "All in one") continue;
        int total = 0;
        for (const auto &l : s.lines) total += l.ofSize;
        QCOMPARE(total, 100);   // rounding distributes exactly 100%
        return;
    }
    QFAIL("no 'All in one' spec");
}

void TestAccessSpecCatalog::fillWireAccess_mapsFriendlyToWire() {
    AccessSpecLine l;
    l.ofSize = 40; l.readPercent = 67; l.seqPercent = 0;   // 0% seq => 100% random
    l.delayMs = 0; l.burstLength = 1; l.replyBytes = 0;
    l.sizeBytes = 4096; l.alignBytes = 0;                   // sector alignment
    FakeWire a{};
    fillWireAccess(a, l);
    QCOMPARE(a.of_size, 40);
    QCOMPARE(a.reads, 67);
    QCOMPARE(a.random, 100);
    QCOMPARE((int)a.size, 4096);
    QCOMPARE((int)a.align, 512);          // alignBytes 0 -> sector (512)

    l.alignBytes = -1;                    // request-size alignment -> == size
    fillWireAccess(a, l);
    QCOMPARE((int)a.align, 4096);

    l.sizeBytes = 0; l.alignBytes = 8192; // size 0 -> default 65536; explicit align
    fillWireAccess(a, l);
    QCOMPARE((int)a.size, 65536);
    QCOMPARE((int)a.align, 8192);
}

void TestAccessSpecCatalog::smartNameText_formats() {
    QCOMPARE(smartNameText(4096, 100, 100), std::string("4 KiB random reads"));
    QCOMPARE(smartNameText(512,    0,   0), std::string("512 B sequential writes"));
    QCOMPARE(smartNameText(1048576, 0, 100), std::string("1 MiB sequential reads"));
    QCOMPARE(smartNameText(8192,  30,  67), std::string("8 KiB 30% random 67% reads"));
}

QTEST_APPLESS_MAIN(TestAccessSpecCatalog)
#include "tst_accessspeccatalog.moc"
