#include "ResultsWriter.h"
#include "../IometerTypes.h"  // WorkerResult, TestConfig

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSet>
#include <algorithm>

bool ResultsWriter::writeBatchResultsCsv(const QString &filepath,
                                         const QVector<WorkerResult> &results,
                                         const TestConfig &cfg)
{
    QFile f(filepath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&f);

    // -- File header (matches original ManagerList output) -----------------
    out << "'Iometer Output File\n";
    out << "Version " << VERSION << " \n";
    out << "'Test Description\n\t" << cfg.description << "\n";
    out << "'Time Stamp\n\t"
        << QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss") << "\n";
    out << "'Run Time\n'\thours      minutes    seconds\n";
    out << "\t" << cfg.runHours
        << "          " << cfg.runMinutes
        << "          " << cfg.runSeconds << "\n";
    out << "'Ramp Up Time (s)\n\t" << cfg.rampSeconds << "\n";

    // -- Column headers (exact original order, 37 columns) ----------------
    // Key fields verified by smoke tests: [0]="ALL", [6]=IOps, [12]=MBps(Dec), [27]=Errors
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

    // Build aggregate from results
    WorkerResult agg;
    agg.managerName = "ALL";
    agg.workerName  = "All";
    agg.isAggregate = true;
    int workerCount = 0, mgCount = 0;
    QSet<QString> managers;

    for (const auto &r : results) {
        if (r.isAggregate) continue;
        agg.iops         += r.iops;
        agg.readIops     += r.readIops;
        agg.writeIops    += r.writeIops;
        agg.mbpsDec      += r.mbpsDec;
        agg.readMbpsDec  += r.readMbpsDec;
        agg.writeMbpsDec += r.writeMbpsDec;
        agg.mbpsBin      += r.mbpsBin;
        agg.avgLatencyMs += r.avgLatencyMs;
        agg.maxLatencyMs  = qMax(agg.maxLatencyMs, r.maxLatencyMs);
        agg.cpuUtil      += r.cpuUtil;
        agg.errors       += r.errors;
        managers.insert(r.managerName);
        ++workerCount;
    }
    if (workerCount > 0) agg.avgLatencyMs /= workerCount;
    mgCount = managers.size();

    // Format numbers with 6 decimal places (matches original)
    auto fmt = [](double v, int dec = 6) { return QString::number(v, 'f', dec); };

    // Write the ALL aggregate row
    // Column positions:
    // [0]="ALL" [1]="All" [6]=IOps [12]=MBps(Dec) [27]=Errors
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
        out << "DISK," << r.workerName << "," << r.managerName << ",1,1,1,"
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

    f.close();
    return true;
}
