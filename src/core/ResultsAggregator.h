#pragma once

#include <vector>
#include "IometerTypes.h"

// The "ALL" aggregate row computed from per-worker results - ONE canonical
// implementation shared by the CSV writer and both GUIs' live displays.
// (Previously computed independently in ResultsWriter, QtDynamoEngine and
// QtDemoEngine, with drifting semantics for the CPU/latency fields.)
//
// Semantics:
//   - input rows flagged isAggregate are skipped (never aggregate an aggregate)
//   - iops/readIops/writeIops, mbps (dec+bin, read/write), errors: SUM
//   - avgLatencyMs: MEAN over workers
//   - maxLatencyMs: MAX over workers
//   - cpuUtil/cpuUser/cpuKernel: MEAN over workers (CPU is reported per manager
//     and mirrored onto its workers; summing would overcount - the original
//     Iometer averages CPU across managers)
//   - result: managerName "ALL", workerName "All", isAggregate = true

namespace iocore {

struct AggregateResult {
    WorkerResult all;            // the aggregate row
    int          workerCount  = 0;   // non-aggregate rows aggregated
    int          managerCount = 0;   // distinct manager names among them
};

AggregateResult aggregateResults(const std::vector<WorkerResult> &results);

} // namespace iocore
