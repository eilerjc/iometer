#pragma once

#include <string>
#include <vector>
#include "IometerTypes.h"

// CSV output generation (platform-agnostic)
// Shared interface for the GUI front-ends
class ResultsWriter {
public:
    // Write Iometer results to CSV file (ICF 1.1.0 output format)
    // Both GUI front-ends use this interface for consistent CSV generation
    static bool writeBatchResultsCsv(const std::string &filepath,
                                     const std::vector<WorkerResult> &results,
                                     const TestConfig &cfg);

    // CSV column indices - shared with MFC equivalent
    static constexpr int COL_TARGET_TYPE = 0;           // "ALL" or "DISK"
    static constexpr int COL_TARGET_NAME = 1;           // "All" or worker name
    static constexpr int COL_MANAGER = 2;               // Manager name
    static constexpr int COL_WORKERS = 3;               // Worker count
    static constexpr int COL_MANAGERS = 4;              // Manager count
    static constexpr int COL_TARGETS = 5;               // Target count
    static constexpr int COL_IOPS = 6;                  // IOps (I/O ops per second)
    static constexpr int COL_READ_IOPS = 7;             // Read IOps
    static constexpr int COL_WRITE_IOPS = 8;            // Write IOps
    static constexpr int COL_MBPS_BIN = 9;              // MBps (Binary)
    static constexpr int COL_READ_MBPS_BIN = 10;        // Read MBps (Binary)
    static constexpr int COL_WRITE_MBPS_BIN = 11;       // Write MBps (Binary)
    static constexpr int COL_MBPS_DEC = 12;             // MBps (Decimal) - KEY FIELD
    static constexpr int COL_READ_MBPS_DEC = 13;        // Read MBps (Decimal)
    static constexpr int COL_WRITE_MBPS_DEC = 14;       // Write MBps (Decimal)
    static constexpr int COL_TRANS_PER_SEC = 15;        // Transactions per second
    static constexpr int COL_CONN_PER_SEC = 16;         // Connections per second
    static constexpr int COL_AVG_LATENCY_MS = 17;       // Average I/O Response Time (ms)
    static constexpr int COL_AVG_READ_LATENCY = 18;     // Average Read Response Time (ms)
    static constexpr int COL_AVG_WRITE_LATENCY = 19;    // Average Write Response Time (ms)
    static constexpr int COL_AVG_TRANS_TIME = 20;       // Average Transaction Time (ms)
    static constexpr int COL_AVG_CONN_TIME = 21;        // Average Connection Time (ms)
    static constexpr int COL_MAX_LATENCY_MS = 22;       // Maximum I/O Response Time (ms)
    static constexpr int COL_MAX_READ_LATENCY = 23;     // Maximum Read Response Time (ms)
    static constexpr int COL_MAX_WRITE_LATENCY = 24;    // Maximum Write Response Time (ms)
    static constexpr int COL_MAX_TRANS_TIME = 25;       // Maximum Transaction Time (ms)
    static constexpr int COL_MAX_CONN_TIME = 26;        // Maximum Connection Time (ms)
    static constexpr int COL_ERRORS = 27;               // Errors (error count) - KEY FIELD
    static constexpr int COL_READ_ERRORS = 28;          // Read Errors
    static constexpr int COL_WRITE_ERRORS = 29;         // Write Errors
    static constexpr int COL_CPU_TOTAL = 30;            // CPU % Utilization (total)
    static constexpr int COL_CPU_USER = 31;             // CPU % User
    static constexpr int COL_CPU_PRIVILEGED = 32;       // CPU % Privileged
    static constexpr int COL_CPU_DPC = 33;              // CPU % DPC
    static constexpr int COL_CPU_INTERRUPT = 34;        // CPU % Interrupt
    static constexpr int COL_CPU_INTERRUPTS_SEC = 35;   // CPU Interrupts/sec
    static constexpr int COL_CPU_EFFECTIVENESS = 36;    // CPU Effectiveness

    // Total expected columns in output
    static constexpr int TOTAL_COLUMNS = 37;

    // CSV format version (shared)
    static constexpr const char *VERSION = "1.1.0";

private:
    // Formatting helpers
    static std::string formatDouble(double value, int precision = 2);
    static std::string escapeCsvField(const std::string &field);
};
