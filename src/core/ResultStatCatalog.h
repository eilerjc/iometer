#pragma once

#include "IometerTypes.h"   // RESULT_* enum + NUM_RESULT_TYPES
#include <cstring>

// Canonical display names for the result statistics, indexed by the RESULT_*
// enum. These are the EXACT strings the MFC GUI uses (Galileo.rc menu
// IDR_POPUP_DISPLAY_LIST) and that are written into ICF files as the
// 'Bar chart N statistic value - i.e. the cross-GUI / ICF-compat contract.
// Single source of truth so the MFC and Qt statistic lists cannot drift.
//
// The RESULT_* enum order (IometerTypes.h) deliberately matches the MFC menu
// order, so this is a flat table.

namespace iocore {

inline const char *const *resultStatNames()
{
    static const char *const kNames[NUM_RESULT_TYPES] = {
        "Total I/Os per Second",            // RESULT_IOPS
        "Read I/Os per Second",             // RESULT_READ_IOPS
        "Write I/Os per Second",            // RESULT_WRITE_IOPS
        "Transactions per Second",          // RESULT_TRANS_PS
        "Connections per Second",           // RESULT_CONN_PS
        "Total MBs per Second (Decimal)",   // RESULT_MBPS_DEC
        "Read MBs per Second (Decimal)",    // RESULT_READ_MBPS_DEC
        "Write MBs per Second (Decimal)",   // RESULT_WRITE_MBPS_DEC
        "Total MBs per Second (Binary)",    // RESULT_MBPS_BIN
        "Read MBs per Second (Binary)",     // RESULT_READ_MBPS_BIN
        "Write MBs per Second (Binary)",    // RESULT_WRITE_MBPS_BIN
        "Average I/O Response Time (ms)",   // RESULT_AVG_LATENCY_MS
        "Avg. Read Response Time (ms)",     // RESULT_AVG_READ_LATENCY_MS
        "Avg. Write Response Time (ms)",    // RESULT_AVG_WRITE_LATENCY_MS
        "Avg. Transaction Time (ms)",       // RESULT_AVG_TRANS_LATENCY_MS
        "Avg. Connection Time (ms)",        // RESULT_AVG_CONN_LATENCY_MS
        "Maximum I/O Response Time (ms)",   // RESULT_MAX_LATENCY_MS
        "Max. Read Response Time (ms)",     // RESULT_MAX_READ_LATENCY_MS
        "Max. Write Response Time (ms)",    // RESULT_MAX_WRITE_LATENCY_MS
        "Max. Transaction Time (ms)",       // RESULT_MAX_TRANS_LATENCY_MS
        "Max. Connection Time (ms)",        // RESULT_MAX_CONN_LATENCY_MS
        "% CPU Utilization (total)",        // RESULT_CPU_UTIL
        "% User Time",                      // RESULT_CPU_USER
        "% Privileged Time",                // RESULT_CPU_KERNEL
        "% DPC Time",                       // RESULT_CPU_DPC
        "% Interrupt Time",                 // RESULT_CPU_INT_TIME
        "Interrupts per Second",            // RESULT_CPU_INTERRUPTS_PS
        "CPU Effectiveness",                // RESULT_CPU_EFFECTIVENESS
        "Network Packets per Second",       // RESULT_NET_PACKETS_PS
        "Packet Errors",                    // RESULT_NET_PACKET_ERRORS
        "TCP Segments Retrans. per Sec.",   // RESULT_NET_RETRANS_PS
        "Total Error Count",                // RESULT_ERRORS
        "Read Error Count",                 // RESULT_READ_ERRORS
        "Write Error Count",                // RESULT_WRITE_ERRORS
    };
    return kNames;
}

// Canonical display name for a RESULT_* statistic ("" if out of range).
inline const char *resultStatName(int resultType)
{
    return (resultType >= 0 && resultType < NUM_RESULT_TYPES)
               ? resultStatNames()[resultType] : "";
}

// RESULT_* id for a display name (exact match), or -1 if unknown.
inline int resultStatByName(const char *name)
{
    if (!name) return -1;
    const char *const *n = resultStatNames();
    for (int i = 0; i < NUM_RESULT_TYPES; ++i)
        if (std::strcmp(n[i], name) == 0) return i;
    return -1;
}

} // namespace iocore
