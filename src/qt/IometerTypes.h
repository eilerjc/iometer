// IometerTypes.h — Shared data types for the Iometer Qt GUI
#pragma once
#include <QString>
#include <QList>
#include <QStringList>

// ── Result types (match IDR_POPUP_DISPLAY_LIST order) ────────────────────────
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

// ── Per-worker live result snapshot ──────────────────────────────────────────
struct WorkerResult {
    QString managerName;
    QString workerName;
    bool    isAggregate = false;   // true → "ALL" row

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

// ── Access specification ──────────────────────────────────────────────────────
struct AccessSpec {
    QString name           = "Untitled";
    int     xferSizeBytes  = 65536;   // 64 KiB default
    int     alignBytes     = 65536;
    int     readPercent    = 100;     // 0=write-only, 100=read-only
    int     seqPercent     = 100;     // 0=random, 100=sequential
    int     burstLength    = 1;
    double  delayMs        = 0.0;
    int     iterations     = 0;       // 0 = run until stopped
    bool    defaultSpec    = false;
};

// ── Worker descriptor ─────────────────────────────────────────────────────────
struct WorkerInfo {
    QString          id;            // unique within manager
    QString          name;
    QString          type;          // "Disk" or "Network"
    QString          managerName;
    QStringList      targets;       // assigned disk/network targets
    int              queueDepth = 1;
    int              activeSpec  = 0;   // index into access spec list
};

// ── Manager descriptor ────────────────────────────────────────────────────────
struct ManagerInfo {
    QString           name;
    QString           address;      // "127.0.0.1" for local
    bool              connected = false;
    QList<WorkerInfo> workers;
};
