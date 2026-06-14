// IometerTypes.h -- Shared data types for Iometer (platform-agnostic)
#pragma once

#include <string>
#include <vector>
#include <cstdint>

// ---- Result types (match IDR_POPUP_DISPLAY_LIST order) ----------------------

enum ResultType {
    // Operations per Second submenu
    RESULT_IOPS = 0,
    RESULT_READ_IOPS,
    RESULT_WRITE_IOPS,
    RESULT_TRANS_PS,            // Transactions per Second (network)
    RESULT_CONN_PS,             // Connections per Second (network)
    // Megabytes per Second submenu
    RESULT_MBPS_DEC,
    RESULT_READ_MBPS_DEC,
    RESULT_WRITE_MBPS_DEC,
    RESULT_MBPS_BIN,
    RESULT_READ_MBPS_BIN,
    RESULT_WRITE_MBPS_BIN,
    // Average Latency submenu
    RESULT_AVG_LATENCY_MS,
    RESULT_AVG_READ_LATENCY_MS,
    RESULT_AVG_WRITE_LATENCY_MS,
    RESULT_AVG_TRANS_LATENCY_MS,
    RESULT_AVG_CONN_LATENCY_MS,
    // Maximum Latency submenu
    RESULT_MAX_LATENCY_MS,
    RESULT_MAX_READ_LATENCY_MS,
    RESULT_MAX_WRITE_LATENCY_MS,
    RESULT_MAX_TRANS_LATENCY_MS,
    RESULT_MAX_CONN_LATENCY_MS,
    // CPU submenu
    RESULT_CPU_UTIL,
    RESULT_CPU_USER,
    RESULT_CPU_KERNEL,          // % Privileged Time in original
    RESULT_CPU_DPC,
    RESULT_CPU_INT_TIME,
    RESULT_CPU_INTERRUPTS_PS,
    RESULT_CPU_EFFECTIVENESS,
    // Network submenu
    RESULT_NET_PACKETS_PS,
    RESULT_NET_PACKET_ERRORS,
    RESULT_NET_RETRANS_PS,
    // Errors submenu
    RESULT_ERRORS,
    RESULT_READ_ERRORS,
    RESULT_WRITE_ERRORS,

    NUM_RESULT_TYPES
};

// ---- Disk target descriptor (for the Disk Targets tab target list) ----------

enum class TargetKind {
    PhysicalDisk,   // raw device (\\.\PhysicalDriveN) - always directly testable
    LogicalDisk,    // mounted volume - needs iobw.tst to be "prepared"
    TcpServer,
    TcpClient
};

struct TargetInfo {
    std::string name;                          // display name sent by Dynamo
    TargetKind  kind  = TargetKind::LogicalDisk;
    bool        ready = false;                 // logical only: iobw.tst present
};

// ---- Per-worker live result snapshot ----------------------------------------

struct WorkerResult {
    std::string managerName;
    std::string workerName;
    bool        isAggregate  = false;

    double      iops          = 0.0;
    double      readIops      = 0.0;
    double      writeIops     = 0.0;
    double      mbpsDec       = 0.0;
    double      readMbpsDec   = 0.0;
    double      writeMbpsDec  = 0.0;
    double      mbpsBin       = 0.0;
    double      avgLatencyMs  = 0.0;
    double      maxLatencyMs  = 0.0;
    double      cpuUtil       = 0.0;
    double      cpuUser       = 0.0;
    double      cpuKernel     = 0.0;
    int         errors        = 0;

    double get(int resultType) const {
        switch (resultType) {
            case RESULT_IOPS:               return iops;
            case RESULT_READ_IOPS:          return readIops;
            case RESULT_WRITE_IOPS:         return writeIops;
            case RESULT_MBPS_DEC:           return mbpsDec;
            case RESULT_READ_MBPS_DEC:      return readMbpsDec;
            case RESULT_WRITE_MBPS_DEC:     return writeMbpsDec;
            case RESULT_MBPS_BIN:           return mbpsBin;
            case RESULT_AVG_LATENCY_MS:     return avgLatencyMs;
            case RESULT_MAX_LATENCY_MS:     return maxLatencyMs;
            case RESULT_CPU_UTIL:           return cpuUtil;
            case RESULT_CPU_USER:           return cpuUser;
            case RESULT_CPU_KERNEL:         return cpuKernel;
            case RESULT_ERRORS:             return errors;
            // Fields not collected yet - return 0 so the row still works
            default:                        return 0.0;
        }
    }
};

// ---- Access specification ---------------------------------------------------

// One I/O pattern row inside an AccessSpec (mirrors Test_Spec::access[i])
struct AccessSpecLine {
    int    sizeBytes   = 65536;  // transfer size in bytes
    int    ofSize      = 100;    // % of access specification (% Access column)
    int    readPercent = 100;    // % reads (0-100)
    int    seqPercent  = 100;    // % sequential; randomPercent = 100 - seqPercent
    int    burstLength = 1;      // burst length (I/Os)
    double delayMs     = 0.0;    // transfer delay in ms
    // Alignment: 0 = sector boundaries (512 B on wire),
    //           -1 = request-size boundaries (size on wire),
    //           >0 = explicit bytes
    int    alignBytes  = 0;
    int    replyBytes  = 0;      // 0 = no reply (network specs only)
};

struct AccessSpec {
    std::string              name        = "Untitled";
    bool                     defaultSpec = false;
    int                      iterations  = 0;         // 0 = run until stopped
    std::vector<AccessSpecLine> lines;                 // I/O pattern rows (≥1 for valid specs)
};

// ---- Worker descriptor ------------------------------------------------------

struct WorkerInfo {
    std::string            id;
    std::string            name;
    std::string            type;          // "Disk" or "Network"
    std::string            managerName;
    std::vector<std::string> targets;       // assigned disk/network targets
    std::vector<std::string> assignedSpecs; // access spec names assigned (empty = use default)
    int                    queueDepth     = 1;
    int                    activeSpec     = 0;

    // Disk Targets tab parameters (match original's right-panel fields):
    int64_t     maxDiskSize    = 0;       // sectors; 0 = use entire disk
    int64_t     startingSector = 0;
    bool        useFixedSeed   = false;
    int64_t     fixedSeedValue = 0;
    bool        testConnRate   = false;
    int         transPerConn   = 1;
    int         dataPattern    = 0;       // 0=Repeating bytes, 1=Pseudo-random, 2=Full random

    // Network Targets tab parameters:
    std::string networkInterface = "";   // local NIC to use for outgoing connections
    int         maxOutstandingSends = 1; // "Max # Outstanding Sends" (network queue depth)
};

// ---- Manager descriptor -----------------------------------------------------

struct ManagerInfo {
    std::string             name;
    std::string             address;
    bool                    connected        = false;
    int                     processorCount   = 1;  // CPU count reported by Dynamo
    std::vector<WorkerInfo> workers;
    std::vector<TargetInfo> availableTargets;      // disk targets reported by Dynamo
    std::vector<std::string> availableNetInterfaces;// NIC addresses reported by Dynamo
};

// ---- Test Setup tab configuration -------------------------------------------

struct TestConfig {
    std::string description    = "";
    int         runHours       = 0;
    int         runMinutes     = 0;
    int         runSeconds     = 0;
    int         rampSeconds    = 0;
    int         recordResults  = 0;   // 0=All, 1=First Run, 2=Last Run
    // Cycling options
    int         cyclingMode    = 0;   // 0=Normal
    int         workerStart    = 1;
    int         workerStep     = 1;
    int         workerStepping = 0;   // 0=Linear Stepping
    int         targetStart    = 1;
    int         targetStep     = 1;
    int         targetStepping = 0;
    int         ioqStart       = 1;
    int         ioqEnd         = 32;
    int         ioqPower       = 2;
    int         ioqStepping    = 1;   // 1=Exponential Stepping
};
