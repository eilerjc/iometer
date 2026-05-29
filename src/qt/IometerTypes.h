// IometerTypes.h -- Shared data types for the Iometer Qt GUI
#pragma once
#include <QString>
#include <QList>
#include <QStringList>

// ---- Result types (match IDR_POPUP_DISPLAY_LIST order) ----------------------

enum ResultType {
    RESULT_IOPS = 0,
    RESULT_MBPS_DEC,
    RESULT_MBPS_BIN,
    RESULT_READ_IOPS,
    RESULT_WRITE_IOPS,
    RESULT_READ_MBPS_DEC,
    RESULT_WRITE_MBPS_DEC,
    RESULT_AVG_LATENCY_MS,
    RESULT_MAX_LATENCY_MS,
    RESULT_CPU_UTIL,
    RESULT_CPU_USER,
    RESULT_CPU_KERNEL,
    RESULT_ERRORS,
    NUM_RESULT_TYPES
};

inline QStringList resultTypeNames() {
    return {
        "IOps",
        "MBps (Decimal)",
        "MBps (Binary)",
        "Read IOps",
        "Write IOps",
        "Read MBps (Dec)",
        "Write MBps (Dec)",
        "Avg Latency (ms)",
        "Max Latency (ms)",
        "CPU Util (%)",
        "CPU User (%)",
        "CPU Kernel (%)",
        "Errors",
    };
}

// ---- Per-worker live result snapshot ----------------------------------------

struct WorkerResult {
    QString managerName;
    QString workerName;
    bool    isAggregate  = false;

    double  iops          = 0.0;
    double  readIops      = 0.0;
    double  writeIops     = 0.0;
    double  mbpsDec       = 0.0;
    double  readMbpsDec   = 0.0;
    double  writeMbpsDec  = 0.0;
    double  mbpsBin       = 0.0;
    double  avgLatencyMs  = 0.0;
    double  maxLatencyMs  = 0.0;
    double  cpuUtil       = 0.0;
    double  cpuUser       = 0.0;
    double  cpuKernel     = 0.0;
    int     errors        = 0;

    double get(int resultType) const {
        switch (resultType) {
            case RESULT_IOPS:           return iops;
            case RESULT_MBPS_DEC:       return mbpsDec;
            case RESULT_MBPS_BIN:       return mbpsBin;
            case RESULT_READ_IOPS:      return readIops;
            case RESULT_WRITE_IOPS:     return writeIops;
            case RESULT_READ_MBPS_DEC:  return readMbpsDec;
            case RESULT_WRITE_MBPS_DEC: return writeMbpsDec;
            case RESULT_AVG_LATENCY_MS: return avgLatencyMs;
            case RESULT_MAX_LATENCY_MS: return maxLatencyMs;
            case RESULT_CPU_UTIL:       return cpuUtil;
            case RESULT_CPU_USER:       return cpuUser;
            case RESULT_CPU_KERNEL:     return cpuKernel;
            case RESULT_ERRORS:         return errors;
            default:                    return 0.0;
        }
    }
};

// ---- Access specification ---------------------------------------------------

struct AccessSpec {
    QString name           = "Untitled";
    int     xferSizeBytes  = 65536;
    int     alignBytes     = 65536;
    int     readPercent    = 100;
    int     seqPercent     = 100;
    int     burstLength    = 1;
    double  delayMs        = 0.0;
    int     iterations     = 0;      // 0 = run until stopped
    bool    defaultSpec    = false;

    // Format as the original shows in the Global list:
    // "64 KiB; 100% Read; 0% random"
    QString displayLabel() const {
        // Format size
        QString sizeStr;
        if      (xferSizeBytes >= 1048576) sizeStr = QString("%1 MiB").arg(xferSizeBytes / 1048576);
        else if (xferSizeBytes >= 1024)    sizeStr = QString("%1 KiB").arg(xferSizeBytes / 1024);
        else                               sizeStr = QString("%1 B").arg(xferSizeBytes);
        const int randomPct = 100 - seqPercent;
        return QString("%1; %2% Read; %3% random").arg(sizeStr).arg(readPercent).arg(randomPct);
    }
};

// ---- Worker descriptor ------------------------------------------------------

struct WorkerInfo {
    QString     id;
    QString     name;
    QString     type;          // "Disk" or "Network"
    QString     managerName;
    QStringList targets;       // assigned disk/network targets
    QStringList assignedSpecs; // access spec names assigned (empty = use default)
    int         queueDepth     = 1;
    int         activeSpec     = 0;

    // Disk Targets tab parameters (match original's right-panel fields):
    qint64      maxDiskSize    = 0;       // sectors; 0 = use entire disk
    qint64      startingSector = 0;
    bool        useFixedSeed   = false;
    qint64      fixedSeedValue = 0;
    bool        testConnRate   = false;
    int         transPerConn   = 1;
    int         dataPattern    = 0;       // 0=Repeating bytes, 1=Pseudo-random, 2=Full random

    // Network Targets tab parameters:
    QString     networkInterface = "";   // local NIC to use for outgoing connections
    int         maxOutstandingSends = 1; // "Max # Outstanding Sends" (network queue depth)
};

// ---- Manager descriptor -----------------------------------------------------

struct ManagerInfo {
    QString           name;
    QString           address;
    bool              connected     = false;
    QList<WorkerInfo> workers;
    QStringList       availableTargets;      // disk targets reported by Dynamo
    QStringList       availableNetInterfaces;// NIC addresses reported by Dynamo
};

// ---- Test Setup tab configuration -------------------------------------------

struct TestConfig {
    QString description    = "";
    int     runHours       = 0;
    int     runMinutes     = 0;
    int     runSeconds     = 0;
    int     rampSeconds    = 0;
    int     recordResults  = 0;   // 0=All, 1=First Run, 2=Last Run
    // Cycling options
    int     cyclingMode    = 0;   // 0=Normal
    int     workerStart    = 1;
    int     workerStep     = 1;
    int     workerStepping = 0;   // 0=Linear Stepping
    int     targetStart    = 1;
    int     targetStep     = 1;
    int     targetStepping = 0;
    int     ioqStart       = 1;
    int     ioqEnd         = 32;
    int     ioqPower       = 2;
    int     ioqStepping    = 1;   // 1=Exponential Stepping
};
