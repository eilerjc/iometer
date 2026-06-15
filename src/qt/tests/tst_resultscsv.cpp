// Byte-exact tests for the canonical results-CSV row writer (iocore::ResultRow /
// writeResultRow). Expected bytes are derived from, and cross-checked against, a
// real MFC results CSV: doubles are fixed/6, integers plain, and each row variant
// (ALL / MANAGER / WORKER) fills or blanks the raw / CPU+network / proc-speed
// blocks in the pattern MFC emits. The header line is locked separately by the
// load goldens; this pins the data rows (whose measured values vary run to run).

#include <QtTest>
#include <sstream>
#include <string>
#include "core/ResultsCsv.h"

using namespace iocore;

class TestResultsCsv : public QObject {
    Q_OBJECT

    static std::string row(const ResultRow &r) {
        std::ostringstream os;
        writeResultRow(os, r);
        return os.str();
    }
    static std::string binZeros() {           // 21 latency-bin columns, all 0
        std::string s;
        for (int i = 0; i < 21; ++i) s += ",0";
        return s;
    }
    // Fill the shared (non-variant) measured fields with simple values.
    static ResultRow base() {
        ResultRow r;
        r.accessSpec = "S";
        r.iops = 75; r.readIops = 50; r.writeIops = 25;
        r.mbpsBin = 1.5; r.readMbpsBin = 1; r.writeMbpsBin = 0.5;
        r.mbpsDec = 1.5; r.readMbpsDec = 1; r.writeMbpsDec = 0.5;
        r.transPerSec = 15; r.connPerSec = 2.5;
        r.aveLatency = 1; r.aveRead = 1; r.aveWrite = 0; r.aveTrans = 1; r.aveConn = 2;
        r.maxLatency = 5; r.maxRead = 5; r.maxWrite = 0; r.maxTrans = 4; r.maxConn = 2;
        r.totalErrors = 3; r.readErrors = 1; r.writeErrors = 2;
        r.bytesRead = 2097152; r.bytesWritten = 1048576;
        r.readCount = 100; r.writeCount = 50; r.connectionCount = 5;
        r.transPerConn = 10;
        r.startingSector = "0"; r.maxSize = "1024"; r.queueDepth = "8";
        return r;
    }
    static const char *measured() {           // columns [6-35], shared by all variants
        return "75.000000,50.000000,25.000000,"
               "1.500000,1.000000,0.500000,"
               "1.500000,1.000000,0.500000,"
               "15.000000,2.500000,"
               "1.000000,1.000000,0.000000,1.000000,2.000000,"
               "5.000000,5.000000,0.000000,4.000000,2.000000,"
               "3,1,2,"
               "2097152,1048576,100,50,5,"
               "10";
    }

private slots:
    // ALL row: raw block blank, CPU+network filled, proc speed blank.
    void allRow() {
        ResultRow r = base();
        r.targetType = "ALL"; r.targetName = "All";
        r.managers = "2"; r.workers = "3"; r.disks = "4";
        r.rawBlock = false;
        r.cpuNetBlock = true;
        r.cpuUtil[0] = 10; r.cpuUtil[1] = 8; r.cpuUtil[2] = 2;
        r.procSpeedPresent = false;
        r.interrupts = 1.5; r.effectiveness = 7.5;
        r.niPackets = 100; r.niErrors = 0; r.tcpRetrans = 0;

        const std::string expected =
            std::string("ALL,All,S,2,3,4,") + measured() +
            ",,,,,,,,,"                                  // [36-44] raw block blank
            ",0,1024,8"                                  // [45-47]
            ",10.000000,8.000000,2.000000,0.000000,0.000000"  // [48-52] CPU
            ","                                          // [53] proc speed blank
            ",1.500000,7.500000"                         // [54-55]
            ",100.000000,0.000000,0.000000"              // [56-58] network
            + binZeros() + "\n";
        QCOMPARE(QString::fromStdString(row(r)), QString::fromStdString(expected));
    }

    // MANAGER row: raw block filled, CPU+network filled, proc speed = timer res.
    void managerRow() {
        ResultRow r = base();
        r.targetType = "MANAGER"; r.targetName = "TESTMGR";
        r.managers = ""; r.workers = "1"; r.disks = "1";
        r.rawBlock = true;
        r.rawReadLatSum = 1000; r.rawWriteLatSum = 0; r.rawTransLatSum = 1000; r.rawConnLatSum = 2000;
        r.rawMaxRead = 50; r.rawMaxWrite = 0; r.rawMaxTrans = 50; r.rawMaxConn = 60;
        r.rawCounterTime = 99999;
        r.cpuNetBlock = true;
        r.cpuUtil[0] = 10; r.cpuUtil[1] = 8; r.cpuUtil[2] = 2;
        r.procSpeedPresent = true; r.procSpeed = 10000000;
        r.interrupts = 1.5; r.effectiveness = 7.5;
        r.niPackets = 100; r.niErrors = 0; r.tcpRetrans = 0;

        const std::string expected =
            std::string("MANAGER,TESTMGR,S,,1,1,") + measured() +
            ",1000,0,1000,2000,50,0,50,60,99999"         // [36-44] raw block filled
            ",0,1024,8"
            ",10.000000,8.000000,2.000000,0.000000,0.000000"
            ",10000000.000000"                           // [53] proc speed = timer res
            ",1.500000,7.500000"
            ",100.000000,0.000000,0.000000"
            + binZeros() + "\n";
        QCOMPARE(QString::fromStdString(row(r)), QString::fromStdString(expected));
    }

    // WORKER row: raw block filled, CPU+network blank, proc speed = timer res.
    void workerRow() {
        ResultRow r = base();
        r.targetType = "WORKER"; r.targetName = "W1";
        r.managers = ""; r.workers = ""; r.disks = "2";
        r.rawBlock = true;
        r.rawReadLatSum = 1000; r.rawWriteLatSum = 0; r.rawTransLatSum = 1000; r.rawConnLatSum = 2000;
        r.rawMaxRead = 50; r.rawMaxWrite = 0; r.rawMaxTrans = 50; r.rawMaxConn = 60;
        r.rawCounterTime = 99999;
        r.cpuNetBlock = false;
        r.procSpeedPresent = true; r.procSpeed = 10000000;

        const std::string expected =
            std::string("WORKER,W1,S,,,2,") + measured() +
            ",1000,0,1000,2000,50,0,50,60,99999"         // [36-44] raw block filled
            ",0,1024,8"
            ",,,,,"                                      // [48-52] CPU blank
            ",10000000.000000"                           // [53] proc speed
            ",,"                                         // [54-55] blank
            ",,,"                                        // [56-58] network blank
            + binZeros() + "\n";
        QCOMPARE(QString::fromStdString(row(r)), QString::fromStdString(expected));
    }

    // Independent structural check: every variant emits exactly 80 columns
    // (matching the canonical header), regardless of which blocks are blank.
    void everyVariantHas80Columns() {
        for (bool raw : {false, true}) {
            for (bool cpuNet : {false, true}) {
                ResultRow r = base();
                r.targetType = "X"; r.rawBlock = raw; r.cpuNetBlock = cpuNet;
                std::string s = row(r);
                s.pop_back();                            // drop trailing '\n'
                int cols = 1;
                for (char c : s) if (c == ',') ++cols;
                QCOMPARE(cols, 80);
            }
        }
    }
};

QTEST_APPLESS_MAIN(TestResultsCsv)
#include "tst_resultscsv.moc"
