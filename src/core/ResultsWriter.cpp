#include "ResultsWriter.h"
#include "ResultsAggregator.h"
#include "ResultsCsv.h"      // canonical results-CSV schema + row writer (shared with MFC)
#include <fstream>
#include <string>
#include <ctime>

// Format a timestamp matching the original Iometer output ("yyyy/MM/dd hh:mm:ss")
static std::string currentTimestamp()
{
    std::time_t now = std::time(nullptr);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &tmv);
    return std::string(buf);
}

// Replace commas / CR / LF with spaces so a name can't break the CSV column
// layout. The canonical writer emits names raw (matching the MFC GUI, which
// sanitizes rather than quotes), so do the same sanitization here.
static std::string sanitizeName(const std::string &s)
{
    std::string out = s;
    for (char &c : out)
        if (c == ',' || c == '\n' || c == '\r') c = ' ';
    return out;
}

// Copy the measured columns Qt actually computes into a canonical row. Columns
// Qt does not track (read/write binary MBps split, transactions/connections,
// the read/write/transaction/connection latency variants, raw counters, network)
// stay at their zero defaults.
static void fillFromWorker(iocore::ResultRow &row, const WorkerResult &wr)
{
    row.iops = wr.iops; row.readIops = wr.readIops; row.writeIops = wr.writeIops;
    row.mbpsBin = wr.mbpsBin;
    row.mbpsDec = wr.mbpsDec; row.readMbpsDec = wr.readMbpsDec; row.writeMbpsDec = wr.writeMbpsDec;
    row.aveLatency = wr.avgLatencyMs;
    row.maxLatency = wr.maxLatencyMs;
    row.totalErrors = static_cast<unsigned long>(wr.errors < 0 ? 0 : wr.errors);
}

bool ResultsWriter::writeBatchResultsCsv(const std::string &filepath,
                                         const std::vector<WorkerResult> &results,
                                         const TestConfig &cfg)
{
    std::ofstream out(filepath);
    if (!out) return false;

    // -- File preamble -----------------------------------------------------
    out << "'Iometer Output File\n";
    out << "Version 1.1.0 \n";
    out << "'Test Description\n\t" << cfg.description << "\n";
    out << "'Time Stamp\n\t" << currentTimestamp() << "\n";
    out << "'Run Time\n'\thours      minutes    seconds\n";
    out << "\t" << cfg.runHours
        << "          " << cfg.runMinutes
        << "          " << cfg.runSeconds << "\n";
    out << "'Ramp Up Time (s)\n\t" << cfg.rampSeconds << "\n";

    // -- Canonical results section (the same 80-column schema + row writer the
    //    MFC GUI uses). Qt fills the columns it computes; the rest stay 0/blank.
    iocore::writeResultsHeader(out);

    // ALL aggregate row, built from the non-aggregate worker rows (shared core).
    const iocore::AggregateResult aggRes = iocore::aggregateResults(results);
    const WorkerResult &agg = aggRes.all;
    const int workerCount   = aggRes.workerCount;
    const int mgCount       = aggRes.managerCount;
    {
        iocore::ResultRow row;
        fillFromWorker(row, agg);
        row.targetType = "ALL";
        row.targetName = "All";
        row.managers = std::to_string(mgCount);
        row.workers  = std::to_string(workerCount);
        row.disks    = std::to_string(workerCount);   // Qt: one disk target per worker
        row.rawBlock = false;                         // ALL row blanks the raw block (like MFC)
        row.cpuNetBlock = true;                       // ALL row carries the aggregate CPU
        row.cpuUtil[0] = agg.cpuUtil;                 // % CPU Utilization (total)
        row.cpuUtil[1] = agg.cpuUser;                 // % User Time
        row.cpuUtil[2] = agg.cpuKernel;               // % Privileged Time
        row.procSpeedPresent = false;
        iocore::writeResultRow(out, row);
    }

    // Per-worker rows (canonical WORKER rows; CPU/network blank, like the MFC GUI).
    for (const auto &wr : results) {
        if (wr.isAggregate) continue;
        iocore::ResultRow row;
        fillFromWorker(row, wr);
        row.targetType = "WORKER";
        row.targetName = sanitizeName(wr.workerName);
        row.disks = "1";                              // one disk target per Qt worker
        row.rawBlock = true;                          // worker rows fill the raw block (0; Qt has no raw counters)
        row.cpuNetBlock = false;
        row.procSpeedPresent = false;
        iocore::writeResultRow(out, row);
    }

    out.close();
    return true;
}
