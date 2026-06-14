#include "ResultsWriter.h"
#include "ResultsAggregator.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cmath>
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

bool ResultsWriter::writeBatchResultsCsv(const std::string &filepath,
                                         const std::vector<WorkerResult> &results,
                                         const TestConfig &cfg)
{
    std::ofstream out(filepath);
    if (!out) return false;

    // Six-decimal fixed format matches the original Iometer output precision.
    auto fmt = [](double v) { return formatDouble(v, 6); };

    // -- File header (matches original ManagerList output) -----------------
    out << "'Iometer Output File\n";
    out << "Version 1.1.0 \n";
    out << "'Test Description\n\t" << cfg.description << "\n";
    out << "'Time Stamp\n\t" << currentTimestamp() << "\n";
    out << "'Run Time\n'\thours      minutes    seconds\n";
    out << "\t" << cfg.runHours
        << "          " << cfg.runMinutes
        << "          " << cfg.runSeconds << "\n";
    out << "'Ramp Up Time (s)\n\t" << cfg.rampSeconds << "\n";

    // -- Column headers (exact original order, 37 columns) -----------------
    // Smoke test relies on: field[0]=="ALL", [3]=workers, [6]=IOps,
    //                       [12]=MBps(Dec), [27]=Errors
    out << "'Results\n";
    out << "'Target Type,Target Name,Manager,Workers,Managers,Targets,"
           "IOps,Read IOps,Write IOps,"
           "MBps (Binary),Read MBps (Binary),Write MBps (Binary),"
           "MBps (Decimal),Read MBps (Decimal),Write MBps (Decimal),"
           "Transactions per Second,Connections per Second,"
           "Average I/O Response Time (ms),Average Read Response Time (ms),"
           "Average Write Response Time (ms),Average Transaction Time (ms),"
           "Average Connection Time (ms),"
           "Maximum I/O Response Time (ms),Maximum Read Response Time (ms),"
           "Maximum Write Response Time (ms),Maximum Transaction Time (ms),"
           "Maximum Connection Time (ms),"
           "Errors,Read Errors,Write Errors,"
           "CPU % Utilization (total),CPU % User,CPU % Privileged,"
           "CPU % DPC,CPU % Interrupt,CPU Interrupts/sec,CPU Effectiveness\n";

    // Build the ALL aggregate from non-aggregate worker rows (existing ALL rows
    // are skipped) - shared core logic, same as the GUIs' live displays.
    const iocore::AggregateResult aggRes = iocore::aggregateResults(results);
    const WorkerResult &agg   = aggRes.all;
    const int workerCount     = aggRes.workerCount;
    const int mgCount         = aggRes.managerCount;

    // Write the ALL aggregate row - column positions must match the header above.
    // [0]ALL [1]All [2]manager [3]workers [4]managers [5]targets
    // [6]IOps [7]ReadIOps [8]WriteIOps
    // [9]MBpsBin [10]ReadMBpsBin [11]WriteMBpsBin
    // [12]MBpsDec [13]ReadMBpsDec [14]WriteMBpsDec
    // [15]Trans/s [16]Conn/s
    // [17]AvgIO [18]AvgRead [19]AvgWrite [20]AvgTrans [21]AvgConn
    // [22]MaxIO [23]MaxRead [24]MaxWrite [25]MaxTrans [26]MaxConn
    // [27]Errors [28]ReadErrors [29]WriteErrors
    // [30-36] CPU fields
    out << "ALL,All,," << workerCount << "," << mgCount << "," << workerCount << ","
        << fmt(agg.iops) << "," << fmt(agg.readIops) << "," << fmt(agg.writeIops) << ","
        << fmt(agg.mbpsBin) << "," << fmt(agg.mbpsBin) << ",0.000000,"
        << fmt(agg.mbpsDec) << "," << fmt(agg.readMbpsDec) << "," << fmt(agg.writeMbpsDec) << ","
        << fmt(agg.iops) << ",0.000000,"
        << fmt(agg.avgLatencyMs) << "," << fmt(agg.avgLatencyMs) << ",0.000000,0.000000,0.000000,"
        << fmt(agg.maxLatencyMs) << "," << fmt(agg.maxLatencyMs) << ",0.000000,0.000000,0.000000,"
        << agg.errors << ",0,0,"
        << fmt(agg.cpuUtil) << "," << fmt(agg.cpuUser) << "," << fmt(agg.cpuKernel)
        << ",0.000000,0.000000,0.000000,0.000000\n";

    // Write per-worker rows
    for (const auto &r : results) {
        if (r.isAggregate) continue;
        out << "DISK," << escapeCsvField(r.workerName) << "," << escapeCsvField(r.managerName) << ",1,1,1,"
            << fmt(r.iops) << "," << fmt(r.readIops) << "," << fmt(r.writeIops) << ","
            << fmt(r.mbpsBin) << "," << fmt(r.mbpsBin) << ",0.000000,"
            << fmt(r.mbpsDec) << "," << fmt(r.readMbpsDec) << "," << fmt(r.writeMbpsDec) << ","
            << fmt(r.iops) << ",0.000000,"
            << fmt(r.avgLatencyMs) << "," << fmt(r.avgLatencyMs) << ",0.000000,0.000000,0.000000,"
            << fmt(r.maxLatencyMs) << "," << fmt(r.maxLatencyMs) << ",0.000000,0.000000,0.000000,"
            << r.errors << ",0,0,"
            << fmt(r.cpuUtil) << "," << fmt(r.cpuUser) << "," << fmt(r.cpuKernel)
            << ",0.000000,0.000000,0.000000,0.000000\n";
    }

    out.close();
    return true;
}

std::string ResultsWriter::formatDouble(double value, int precision)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

std::string ResultsWriter::escapeCsvField(const std::string &field)
{
    // Quote only when needed (comma, quote, or newline present)
    bool needsQuotes = field.find(',')  != std::string::npos ||
                       field.find('"')  != std::string::npos ||
                       field.find('\n') != std::string::npos;

    if (!needsQuotes) {
        return field;
    }

    std::string escaped = "\"";
    for (char c : field) {
        if (c == '"') {
            escaped += "\"\"";  // double embedded quotes
        } else {
            escaped += c;
        }
    }
    escaped += "\"";
    return escaped;
}
