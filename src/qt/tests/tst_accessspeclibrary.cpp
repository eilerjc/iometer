// tst_accessspeclibrary - the shared iocore access-spec list management
// (ported MFC AccessSpecList semantics: Untitled n / Copy of X (n) / SmartName).
#include <QtTest>
#include "AccessSpecLibrary.h"

using iocore::AccessSpecLibrary;

class TestAccessSpecLibrary : public QObject {
    Q_OBJECT
private slots:
    void startsEmpty_loadDefaultsSeedsCatalog();
    void defaultLine_matchesMfcInit();
    void newSpec_untitledNamingAndDefaults();
    void newSpec_skipsTakenUntitledNames();
    void copySpec_uniqueCopyNaming();
    void copySpec_invalidIndex();
    void removeAt_validAndInvalid();
    void indexByName_caseInsensitive();
    void smartName_sizeRandomReadParts();
    void smartName_midPercentages();
    void smartName_multiLineKeepsName();
    void smartName_uniqueSuffix();
    void smartName_keepsOwnNameWithoutSuffix();
};

void TestAccessSpecLibrary::startsEmpty_loadDefaultsSeedsCatalog() {
    AccessSpecLibrary lib;
    QCOMPARE(lib.count(), 0);
    lib.loadDefaults();
    QVERIFY(lib.count() > 3);
    QCOMPARE(lib.at(0).name, std::string("Idle"));   // idle is index 0 (IDLE_SPEC)
    QVERIFY(lib.indexByName("Default") != -1);
    QVERIFY(lib.indexByName("All in one") != -1);
}

void TestAccessSpecLibrary::defaultLine_matchesMfcInit() {
    const AccessSpecLine l = AccessSpecLibrary::defaultLine();
    QCOMPARE(l.sizeBytes, 2048);
    QCOMPARE(l.ofSize, 100);
    QCOMPARE(l.readPercent, 67);
    QCOMPARE(l.seqPercent, 0);       // random = 100
    QCOMPARE(l.burstLength, 1);
    QCOMPARE(l.alignBytes, 2048);    // align = size
    QCOMPARE(l.replyBytes, 0);
}

void TestAccessSpecLibrary::newSpec_untitledNamingAndDefaults() {
    AccessSpecLibrary lib;
    const int i = lib.newSpec();
    QCOMPARE(i, 0);
    QCOMPARE(lib.at(i).name, std::string("Untitled 1"));
    QVERIFY(!lib.at(i).defaultSpec);
    QCOMPARE(lib.at(i).lines.size(), size_t(1));
    QCOMPARE(lib.at(i).lines[0].sizeBytes, 2048);
    QCOMPARE(lib.newSpec(), 1);
    QCOMPARE(lib.at(1).name, std::string("Untitled 2"));
}

void TestAccessSpecLibrary::newSpec_skipsTakenUntitledNames() {
    AccessSpecLibrary lib;
    lib.newSpec();                        // Untitled 1
    lib.newSpec();                        // Untitled 2
    QVERIFY(lib.removeAt(0));             // remove Untitled 1; one "Untitled*" left
    const int i = lib.newSpec();          // count=1 -> tries "Untitled 2" (taken) -> 3
    QCOMPARE(lib.at(i).name, std::string("Untitled 3"));
}

void TestAccessSpecLibrary::copySpec_uniqueCopyNaming() {
    AccessSpecLibrary lib;
    const int i = lib.newSpec();
    lib.at(i).name = "My Spec";
    const int c1 = lib.copySpec(i);
    QCOMPARE(lib.at(c1).name, std::string("Copy of My Spec (1)"));
    const int c2 = lib.copySpec(i);
    QCOMPARE(lib.at(c2).name, std::string("Copy of My Spec (2)"));
    // Content copied
    QCOMPARE(lib.at(c1).lines[0].sizeBytes, lib.at(i).lines[0].sizeBytes);
}

void TestAccessSpecLibrary::copySpec_invalidIndex() {
    AccessSpecLibrary lib;
    QCOMPARE(lib.copySpec(0), -1);
    QCOMPARE(lib.copySpec(-1), -1);
}

void TestAccessSpecLibrary::removeAt_validAndInvalid() {
    AccessSpecLibrary lib;
    lib.newSpec();
    QVERIFY(!lib.removeAt(5));
    QVERIFY(lib.removeAt(0));
    QCOMPARE(lib.count(), 0);
}

void TestAccessSpecLibrary::indexByName_caseInsensitive() {
    AccessSpecLibrary lib;
    const int i = lib.newSpec();
    lib.at(i).name = "All In One";
    QCOMPARE(lib.indexByName("all in one"), i);
    QCOMPARE(lib.indexByName("ALL IN ONE"), i);
    QCOMPARE(lib.indexByName("nope"), -1);
}

void TestAccessSpecLibrary::smartName_sizeRandomReadParts() {
    AccessSpecLibrary lib;
    const int i = lib.newSpec();
    auto &l = lib.at(i).lines[0];
    l.sizeBytes = 4096; l.seqPercent = 0; l.readPercent = 100;   // 100% random reads
    lib.smartName(i);
    QCOMPARE(lib.at(i).name, std::string("4 KiB random reads"));

    l.sizeBytes = 512; l.seqPercent = 100; l.readPercent = 0;    // sequential writes
    lib.smartName(i);
    QCOMPARE(lib.at(i).name, std::string("512 B sequential writes"));

    l.sizeBytes = 1048576;                                        // 1 MiB
    lib.smartName(i);
    QCOMPARE(lib.at(i).name, std::string("1 MiB sequential writes"));
}

void TestAccessSpecLibrary::smartName_midPercentages() {
    AccessSpecLibrary lib;
    const int i = lib.newSpec();
    auto &l = lib.at(i).lines[0];
    l.sizeBytes = 8192; l.seqPercent = 70; l.readPercent = 67;   // 30% random, 67% reads
    lib.smartName(i);
    QCOMPARE(lib.at(i).name, std::string("8 KiB 30% random 67% reads"));
}

void TestAccessSpecLibrary::smartName_multiLineKeepsName() {
    AccessSpecLibrary lib;
    const int i = lib.newSpec();
    lib.at(i).name = "keep me";
    lib.at(i).lines.push_back(AccessSpecLine{});
    lib.smartName(i);
    QCOMPARE(lib.at(i).name, std::string("keep me"));
}

void TestAccessSpecLibrary::smartName_uniqueSuffix() {
    AccessSpecLibrary lib;
    const int a = lib.newSpec();
    auto &la = lib.at(a).lines[0];
    la.sizeBytes = 4096; la.seqPercent = 0; la.readPercent = 100;
    lib.smartName(a);                       // "4 KiB random reads"
    const int b = lib.newSpec();
    auto &lb = lib.at(b).lines[0];
    lb.sizeBytes = 4096; lb.seqPercent = 0; lb.readPercent = 100;
    lib.smartName(b);                       // collision -> " 2"
    QCOMPARE(lib.at(b).name, std::string("4 KiB random reads 2"));
}

void TestAccessSpecLibrary::smartName_keepsOwnNameWithoutSuffix() {
    AccessSpecLibrary lib;
    const int i = lib.newSpec();
    auto &l = lib.at(i).lines[0];
    l.sizeBytes = 4096; l.seqPercent = 0; l.readPercent = 100;
    lib.smartName(i);
    lib.smartName(i);   // renaming to its own current name must not add " 2"
    QCOMPARE(lib.at(i).name, std::string("4 KiB random reads"));
}

QTEST_APPLESS_MAIN(TestAccessSpecLibrary)
#include "tst_accessspeclibrary.moc"
