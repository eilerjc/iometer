#pragma once

#include <ostream>

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

} // namespace iocore
