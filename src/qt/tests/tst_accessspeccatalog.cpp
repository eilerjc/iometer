// tst_accessspeccatalog - the shared iocore access-spec naming + default catalog.
#include <QtTest>
#include "AccessSpecCatalog.h"
#include "SizeUnits.h"

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
    void mkb_composeDecomposeRoundTrip();
    void validateAccessSpec_rules();
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

void TestAccessSpecCatalog::mkb_composeDecomposeRoundTrip() {
    // Compose: MiB*1048576 + KiB*1024 + B.
    QCOMPARE(mkbToBytes(0, 64, 0), 65536);
    QCOMPARE(mkbToBytes(0,  2, 0), 2048);
    QCOMPARE(mkbToBytes(1,  2, 3), 1050627);

    // Decompose: each level's remainder, exactly as both editors display it.
    auto eq = [](const Mkb &m, int mb, int kb, int b) {
        return m.megabytes == mb && m.kilobytes == kb && m.bytes == b;
    };
    QVERIFY(eq(bytesToMkb(65536),      0,   64,   0));
    QVERIFY(eq(bytesToMkb(2048),       0,    2,   0));
    QVERIFY(eq(bytesToMkb(1050627),    1,    2,   3));
    QVERIFY(eq(bytesToMkb(1073741823), 1023, 1023, 1023)); // 1023 MiB + 1023 KiB + 1023 B

    // Round-trip a spread of sizes.
    for (int bytes : {0, 1, 512, 4096, 65536, 262144, 1048576, 1050627, 999999}) {
        const Mkb m = bytesToMkb(bytes);
        QCOMPARE(mkbToBytes(m.megabytes, m.kilobytes, m.bytes), bytes);
    }
}

void TestAccessSpecCatalog::validateAccessSpec_rules() {
    auto line = [](int size, int ofSize) {
        AccessSpecLine l; l.sizeBytes = size; l.ofSize = ofSize; return l;
    };
    auto specOf = [&](const std::string &name, std::vector<AccessSpecLine> lines) {
        AccessSpec s; s.name = name; s.lines = std::move(lines); return s;
    };

    // Valid single-line spec, no name clash.
    QVERIFY(validateAccessSpec(specOf("Foo", {line(4096, 100)}), {}).ok);

    // Size 0 on any line.
    auto zero = validateAccessSpec(specOf("Foo", {line(0, 100)}), {});
    QVERIFY(!zero.ok);
    QCOMPARE(zero.error, std::string("A line in the access specification is for 0 bytes.  "
                                     "All sizes must be greater than 0."));

    // % of access must sum to exactly 100.
    auto sum = validateAccessSpec(specOf("Foo", {line(512, 50), line(512, 40)}), {});
    QVERIFY(!sum.ok);
    QCOMPARE(sum.error, std::string("Percent of Access Specification values must sum to exactly 100."));
    QVERIFY(validateAccessSpec(specOf("Foo", {line(512, 60), line(512, 40)}), {}).ok);

    // Empty name.
    auto blank = validateAccessSpec(specOf("", {line(4096, 100)}), {});
    QVERIFY(!blank.ok);
    QCOMPARE(blank.error, std::string("You must assign a name to this access specification."));

    // Comma in name.
    auto comma = validateAccessSpec(specOf("a,b", {line(4096, 100)}), {});
    QVERIFY(!comma.ok);
    QCOMPARE(comma.error, std::string("Commas are not allowed in access specification names."));

    // Duplicate name (case-insensitive, like MFC strcasecmp); message embeds the name.
    auto dup = validateAccessSpec(specOf("Foo", {line(4096, 100)}), {"bar", "FOO"});
    QVERIFY(!dup.ok);
    QCOMPARE(dup.error, std::string("An access specification named \"Foo\" already exists.  "
                                    "Access specification names must be unique."));
    // A different name against the same others is fine.
    QVERIFY(validateAccessSpec(specOf("Baz", {line(4096, 100)}), {"bar", "FOO"}).ok);
}

QTEST_APPLESS_MAIN(TestAccessSpecCatalog)
#include "tst_accessspeccatalog.moc"
