#pragma once

#include <cstdint>

// Decode raw worker/target performance counters into displayable rates and
// latencies. This is the single shared implementation of the formula that both
// GUIs used to carry independently (MFC Worker::UpdateResults and Qt
// QtDySession::decodeWorkerResult). MFC behavior is canonical; this is a faithful
// port of the per-target decode in Worker.cpp, including the (double)(int64_t)
// casts.
//
// Like iocore::fillWireAccess, decodeRawResult is TEMPLATED on the raw counter
// struct so it serves both the canonical MFC Raw_Result (IOCommon.h) and Qt's
// DyRawResult (QtDyProto.h) - the field names are identical. The same function
// works on a single target's counters or a summed worker/manager set, because
// summing then decoding equals decoding then summing for these rates.

namespace iocore {

// Binary / decimal megabyte (match IOCommon.h MEGABYTE_BIN / MEGABYTE_DEC).
constexpr double kMegabyteBin = 1048576.0;
constexpr double kMegabyteDec = 1000000.0;

// Calculated result members, mirroring the MFC Results struct's derived fields.
struct ResultRates {
    double iops = 0, readIops = 0, writeIops = 0;
    double mbpsBin = 0, readMbpsBin = 0, writeMbpsBin = 0;
    double mbpsDec = 0, readMbpsDec = 0, writeMbpsDec = 0;
    double transactionsPerSecond = 0, connectionsPerSecond = 0;
    double aveLatency = 0, aveReadLatency = 0, aveWriteLatency = 0;
    double aveTransactionLatency = 0, aveConnectionLatency = 0;
    double maxLatency = 0, maxReadLatency = 0, maxWriteLatency = 0;
    double maxTransactionLatency = 0, maxConnectionLatency = 0;
    unsigned int totalErrors = 0;
};

// Decode one raw counter set. `runTimeSec` is the rate denominator in seconds
// (MFC passes run_time, Qt passes the snapshot elapsed); `timerResolution` is
// processor ticks/second, used to turn latency tick sums into milliseconds.
template <class RawT>
ResultRates decodeRawResult(const RawT &raw, double runTimeSec, double timerResolution)
{
    ResultRates r;
    // Replicate MFC's (double)(_int64)<unsigned counter> conversion exactly.
    auto d = [](uint64_t v) { return static_cast<double>(static_cast<int64_t>(v)); };

    r.totalErrors = raw.read_errors + raw.write_errors;

    // Maximum latencies (counter ticks -> ms). MFC computes these unconditionally.
    if (timerResolution > 0.0) {
        r.maxReadLatency        = d(raw.max_raw_read_latency)        * 1000.0 / timerResolution;
        r.maxWriteLatency       = d(raw.max_raw_write_latency)       * 1000.0 / timerResolution;
        r.maxTransactionLatency = d(raw.max_raw_transaction_latency) * 1000.0 / timerResolution;
        r.maxConnectionLatency  = d(raw.max_raw_connection_latency)  * 1000.0 / timerResolution;
    }
    r.maxLatency = (r.maxReadLatency > r.maxWriteLatency) ? r.maxReadLatency : r.maxWriteLatency;

    // Throughput rates (per second / per MB).
    if (runTimeSec != 0.0) {
        r.readMbpsBin  = d(raw.bytes_read)    / kMegabyteBin / runTimeSec;
        r.writeMbpsBin = d(raw.bytes_written) / kMegabyteBin / runTimeSec;
        r.mbpsBin      = r.readMbpsBin + r.writeMbpsBin;
        r.readMbpsDec  = d(raw.bytes_read)    / kMegabyteDec / runTimeSec;
        r.writeMbpsDec = d(raw.bytes_written) / kMegabyteDec / runTimeSec;
        r.mbpsDec      = r.readMbpsDec + r.writeMbpsDec;
        r.readIops     = d(raw.read_count)  / runTimeSec;
        r.writeIops    = d(raw.write_count) / runTimeSec;
        r.iops         = r.readIops + r.writeIops;
        r.transactionsPerSecond = d(raw.transaction_count) / runTimeSec;
        r.connectionsPerSecond  = d(raw.connection_count)  / runTimeSec;
    }

    // Average latencies (tick sums -> ms over completed count).
    if (timerResolution > 0.0 && (raw.read_count || raw.write_count)) {
        r.aveLatency = d(raw.read_latency_sum + raw.write_latency_sum) * 1000.0 / timerResolution
                       / d(raw.read_count + raw.write_count);
        if (raw.read_count)
            r.aveReadLatency = d(raw.read_latency_sum) * 1000.0 / timerResolution / d(raw.read_count);
        if (raw.write_count)
            r.aveWriteLatency = d(raw.write_latency_sum) * 1000.0 / timerResolution / d(raw.write_count);
        if (raw.transaction_count)
            r.aveTransactionLatency = d(raw.transaction_latency_sum) * 1000.0 / timerResolution
                                      / d(raw.transaction_count);
    }
    if (timerResolution > 0.0 && raw.connection_count)
        r.aveConnectionLatency = d(raw.connection_latency_sum) * 1000.0 / timerResolution
                                 / d(raw.connection_count);

    return r;
}

} // namespace iocore
