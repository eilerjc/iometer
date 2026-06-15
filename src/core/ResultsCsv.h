#pragma once

#include <ostream>
#include <iomanip>
#include <string>
#include <cstdint>

// Canonical Iometer results-CSV schema, shared by both GUIs. MFC is the correct
// format (the ~76-column layout users have parsed for 20 years); this is the
// single source of truth for it. The header is byte-identical to the string MFC
// historically emitted inline in ManagerList::SaveResults.
//
// (The per-row writer - one templated function that replaces the 5-6 duplicated
// column dumps across ManagerList/Manager/Worker SaveResults - follows in a later
// increment; the row variants differ only in which blocks are filled vs blank.)

namespace iocore {

// Emit the results section header: the "'Results" line followed by the full
// column header line (37 result columns + raw counters + CPU/network + the 21
// latency-bin columns). Ends each line with '\n'.
inline void writeResultsHeader(std::ostream &out)
{
    out << "'Results\n";
    out << "'Target Type,Target Name,Access Specification Name,# Managers,"
           "# Workers,# Disks,IOps,Read IOps,Write IOps,MiBps (Binary),Read MiBps (Binary),Write MiBps (Binary),"
           "MBps (Decimal),Read MBps (Decimal),Write MBps (Decimal),Transactions per Second,Connections per Second,"
           "Average Response Time,Average Read Response Time,"
           "Average Write Response Time,Average Transaction Time,"
           "Average Connection Time,Maximum Response Time,"
           "Maximum Read Response Time,Maximum Write Response Time,"
           "Maximum Transaction Time,Maximum Connection Time,"
           "Errors,Read Errors,Write Errors,Bytes Read,Bytes Written,Read I/Os,"
           "Write I/Os,Connections,Transactions per Connection,"
           "Total Raw Read Response Time,Total Raw Write Response Time,"
           "Total Raw Transaction Time,Total Raw Connection Time,"
           "Maximum Raw Read Response Time,Maximum Raw Write Response Time,"
           "Maximum Raw Transaction Time,Maximum Raw Connection Time,"
           "Total Raw Run Time,Starting Sector,Maximum Size,Queue Depth,"
           "% CPU Utilization,% User Time,% Privileged Time,% DPC Time,"
           "% Interrupt Time,Processor Speed,Interrupts per Second,"
           "CPU Effectiveness,Packets/Second,Packet Errors,Segments Retransmitted/Second"
           ",0 to 50 uS,50 to 100 uS,100 to 200 uS,200 to 500 uS,0.5 to 1 mS,1 to 2 mS,2 to 5 mS,5 to 10 mS,"
           "10 to 15 mS,15 to 20 mS,20 to 30 mS,30 to 50 mS,50 to 100 mS,100 to 200 mS,200 to 500 mS,0.5 to 1 S,"
           "1 to 2 s,2 to 4.7 s,4.7 to 5 s,5 to 10 s, >= 10 s"
           "\n";
}

// One canonical results-CSV data row. All four MFC row variants (ALL / MANAGER /
// WORKER / TARGET) share this 80-column layout; they differ only in which blocks
// are filled vs left blank, captured here by rawBlock / cpuNetBlock / procSpeed:
//   - ALL    : rawBlock=false, cpuNetBlock=true,  procSpeedPresent=false
//   - MANAGER: rawBlock=true,  cpuNetBlock=true,  procSpeedPresent=true
//   - WORKER : rawBlock=true,  cpuNetBlock=false, procSpeedPresent=true
// Doubles are written fixed/6 and integers plain, exactly like the MFC output.
// managers/workers/disks and sector/size/queue are strings so a variant can leave
// a count blank (MANAGER blanks #Managers; WORKER blanks #Managers and #Workers).
struct ResultRow {
    std::string targetType, targetName, accessSpec;   // [0-2]
    std::string managers, workers, disks;             // [3-5]

    double iops = 0, readIops = 0, writeIops = 0;                       // [6-8]
    double mbpsBin = 0, readMbpsBin = 0, writeMbpsBin = 0;              // [9-11]
    double mbpsDec = 0, readMbpsDec = 0, writeMbpsDec = 0;              // [12-14]
    double transPerSec = 0, connPerSec = 0;                            // [15-16]
    double aveLatency = 0, aveRead = 0, aveWrite = 0, aveTrans = 0, aveConn = 0;   // [17-21]
    double maxLatency = 0, maxRead = 0, maxWrite = 0, maxTrans = 0, maxConn = 0;   // [22-26]

    unsigned long totalErrors = 0, readErrors = 0, writeErrors = 0;    // [27-29]
    uint64_t bytesRead = 0, bytesWritten = 0,
             readCount = 0, writeCount = 0, connectionCount = 0;       // [30-34]
    long long transPerConn = 0;                                        // [35]

    bool rawBlock = false;                                             // [36-44]
    uint64_t rawReadLatSum = 0, rawWriteLatSum = 0,
             rawTransLatSum = 0, rawConnLatSum = 0;
    uint64_t rawMaxRead = 0, rawMaxWrite = 0, rawMaxTrans = 0, rawMaxConn = 0;
    long long rawCounterTime = 0;   // Total Raw Run Time [44]

    std::string startingSector, maxSize, queueDepth;                  // [45-47]

    bool cpuNetBlock = false;       // [48-52] CPU + [54] interrupts + [55] eff + [56-58] net
    double cpuUtil[5] = {0, 0, 0, 0, 0};
    bool procSpeedPresent = false;  // [53]
    double procSpeed = 0;
    double interrupts = 0, effectiveness = 0;                         // [54-55]
    double niPackets = 0, niErrors = 0, tcpRetrans = 0;               // [56-58]

    long long latencyBin[21] = {0};                                   // [59-79]
};

// Write one results-CSV data row (ends with '\n'). Byte-exact to the MFC
// SaveResults column dump.
inline void writeResultRow(std::ostream &out, const ResultRow &r)
{
    out << std::fixed << std::setprecision(6);   // doubles 6dp; integers unaffected
    out << r.targetType << "," << r.targetName << "," << r.accessSpec
        << "," << r.managers << "," << r.workers << "," << r.disks
        << "," << r.iops << "," << r.readIops << "," << r.writeIops
        << "," << r.mbpsBin << "," << r.readMbpsBin << "," << r.writeMbpsBin
        << "," << r.mbpsDec << "," << r.readMbpsDec << "," << r.writeMbpsDec
        << "," << r.transPerSec << "," << r.connPerSec
        << "," << r.aveLatency << "," << r.aveRead << "," << r.aveWrite
        << "," << r.aveTrans << "," << r.aveConn
        << "," << r.maxLatency << "," << r.maxRead << "," << r.maxWrite
        << "," << r.maxTrans << "," << r.maxConn
        << "," << r.totalErrors << "," << r.readErrors << "," << r.writeErrors
        << "," << r.bytesRead << "," << r.bytesWritten
        << "," << r.readCount << "," << r.writeCount << "," << r.connectionCount
        << "," << r.transPerConn;

    if (r.rawBlock)
        out << "," << r.rawReadLatSum << "," << r.rawWriteLatSum
            << "," << r.rawTransLatSum << "," << r.rawConnLatSum
            << "," << r.rawMaxRead << "," << r.rawMaxWrite
            << "," << r.rawMaxTrans << "," << r.rawMaxConn
            << "," << r.rawCounterTime;
    else
        out << ",,,,,,,,,";          // [36-44] blank (ALL row)

    out << "," << r.startingSector << "," << r.maxSize << "," << r.queueDepth;

    if (r.cpuNetBlock)
        out << "," << r.cpuUtil[0] << "," << r.cpuUtil[1] << "," << r.cpuUtil[2]
            << "," << r.cpuUtil[3] << "," << r.cpuUtil[4];
    else
        out << ",,,,,";              // [48-52] blank (WORKER row)

    out << ",";                      // [53] processor speed
    if (r.procSpeedPresent)
        out << r.procSpeed;

    if (r.cpuNetBlock)
        out << "," << r.interrupts << "," << r.effectiveness;
    else
        out << ",,";                 // [54-55] blank

    if (r.cpuNetBlock)
        out << "," << r.niPackets << "," << r.niErrors << "," << r.tcpRetrans;
    else
        out << ",,,";                // [56-58] blank

    for (int i = 0; i < 21; ++i)
        out << "," << r.latencyBin[i];
    out << "\n";
}

// Copy the measured / raw / CPU / network / latency-bin fields from a canonical
// MFC Results struct into a ResultRow (the [6-44], [48-58], [59-79] columns).
// Templated so core stays free of the MFC Results type (the decodeRawResult
// pattern); only MFC instantiates it. The caller sets the metadata columns
// ([0-5]), transPerConn, sector/size/queue, and the rawBlock / cpuNetBlock /
// procSpeed variant fields. Array indices match IOCommon.h
// (CPU_UTILIZATION_RESULTS=5, CPU_IRQ=5, NI_PACKETS=0, NI_ERRORS=1,
// TCP_SEGMENTS_RESENT=0, LATENCY_BIN_SIZE=21).
template <class ResultsT>
void fillResultRow(ResultRow &row, const ResultsT &res)
{
    row.iops = res.IOps; row.readIops = res.read_IOps; row.writeIops = res.write_IOps;
    row.mbpsBin = res.MBps_Bin; row.readMbpsBin = res.read_MBps_Bin; row.writeMbpsBin = res.write_MBps_Bin;
    row.mbpsDec = res.MBps_Dec; row.readMbpsDec = res.read_MBps_Dec; row.writeMbpsDec = res.write_MBps_Dec;
    row.transPerSec = res.transactions_per_second; row.connPerSec = res.connections_per_second;
    row.aveLatency = res.ave_latency; row.aveRead = res.ave_read_latency; row.aveWrite = res.ave_write_latency;
    row.aveTrans = res.ave_transaction_latency; row.aveConn = res.ave_connection_latency;
    row.maxLatency = res.max_latency; row.maxRead = res.max_read_latency; row.maxWrite = res.max_write_latency;
    row.maxTrans = res.max_transaction_latency; row.maxConn = res.max_connection_latency;

    row.totalErrors = res.total_errors; row.readErrors = res.raw.read_errors; row.writeErrors = res.raw.write_errors;
    row.bytesRead = res.raw.bytes_read; row.bytesWritten = res.raw.bytes_written;
    row.readCount = res.raw.read_count; row.writeCount = res.raw.write_count;
    row.connectionCount = res.raw.connection_count;

    row.rawReadLatSum = res.raw.read_latency_sum; row.rawWriteLatSum = res.raw.write_latency_sum;
    row.rawTransLatSum = res.raw.transaction_latency_sum; row.rawConnLatSum = res.raw.connection_latency_sum;
    row.rawMaxRead = res.raw.max_raw_read_latency; row.rawMaxWrite = res.raw.max_raw_write_latency;
    row.rawMaxTrans = res.raw.max_raw_transaction_latency; row.rawMaxConn = res.raw.max_raw_connection_latency;
    row.rawCounterTime = res.raw.counter_time;

    for (int i = 0; i < 5; ++i) row.cpuUtil[i] = res.CPU_utilization[i];
    row.interrupts = res.CPU_utilization[5];          // CPU_IRQ
    row.effectiveness = res.CPU_effectiveness;
    row.niPackets = res.ni_statistics[0]; row.niErrors = res.ni_statistics[1];
    row.tcpRetrans = res.tcp_statistics[0];
    for (int i = 0; i < 21; ++i) row.latencyBin[i] = res.raw.latency_bin[i];
}

} // namespace iocore
