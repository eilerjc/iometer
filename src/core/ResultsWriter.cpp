#include "ResultsWriter.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cmath>

bool ResultsWriter::writeBatchResultsCsv(const std::string &filepath,
                                         const std::vector<WorkerResult> &results,
                                         const TestConfig &cfg)
{
    std::ofstream out(filepath);
    if (!out) return false;

    // Write header row with column names
    std::vector<std::string> headers = {
        "Target Type", "Target Name", "Manager", "Workers", "Managers", "Targets",
        "IOps", "Read IOps", "Write IOps", "MBps (Binary)", "Read MBps (Bin)", "Write MBps (Bin)",
        "MBps (Decimal)", "Read MBps (Dec)", "Write MBps (Dec)", "Trans/s", "Conn/s",
        "Avg Latency (ms)", "Avg Read Latency", "Avg Write Latency", "Avg Trans Time", "Avg Conn Time",
        "Max Latency (ms)", "Max Read Latency", "Max Write Latency", "Max Trans Time", "Max Conn Time",
        "CPU %", "CPU User %", "CPU Kernel %",
        "Errors", "Read Errors", "Write Errors"
    };

    // Write header
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) out << ",";
        out << "\"" << headers[i] << "\"";
    }
    out << "\n";

    // Write data rows
    for (const auto &result : results) {
        out << "DISK";              // COL_TARGET_TYPE
        out << "," << escapeCsvField(result.workerName);
        out << "," << escapeCsvField(result.managerName);
        out << ",1,1,1";             // Workers, Managers, Targets (placeholder)
        out << "," << formatDouble(result.iops);
        out << "," << formatDouble(result.readIops);
        out << "," << formatDouble(result.writeIops);
        out << "," << formatDouble(result.mbpsBin);
        out << ",0,0";               // Read/Write MBps Bin (placeholder)
        out << "," << formatDouble(result.mbpsDec);
        out << ",0,0";               // Read/Write MBps Dec (placeholder)
        out << ",0,0";               // Trans/s, Conn/s (placeholder)
        out << "," << formatDouble(result.avgLatencyMs);
        out << ",0,0,0,0";           // Various latencies (placeholder)
        out << "," << formatDouble(result.maxLatencyMs);
        out << ",0,0,0,0";           // Max latencies (placeholder)
        out << "," << formatDouble(result.cpuUtil);
        out << "," << formatDouble(result.cpuUser);
        out << "," << formatDouble(result.cpuKernel);
        out << "," << result.errors;
        out << ",0,0\n";             // Read/Write errors (placeholder)
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
    // Check if field needs quoting
    bool needsQuotes = field.find(',') != std::string::npos ||
                       field.find('"') != std::string::npos ||
                       field.find('\n') != std::string::npos;

    if (!needsQuotes) {
        return field;
    }

    std::string escaped = "\"";
    for (char c : field) {
        if (c == '"') {
            escaped += "\"\"";  // Escape quotes by doubling
        } else {
            escaped += c;
        }
    }
    escaped += "\"";
    return escaped;
}
