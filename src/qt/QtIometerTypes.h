// IometerTypes.h -- GUI-facing shim.
//
// The domain model (WorkerInfo, AccessSpec, ManagerInfo, WorkerResult,
// TestConfig, TargetInfo, ResultType, TargetKind, ...) lives in the
// platform-agnostic core and is shared verbatim with the MFC/Dynamo side.
// This header pulls those types in and adds Qt-only *display* helpers used by
// the GUI widgets. No Qt domain types are defined here.
#pragma once

#include <QString>
#include <QStringList>
#include "../core/IometerTypes.h"       // single source of truth for domain types
#include "../core/AccessSpecCatalog.h"  // shared access-spec naming/defaults

// ---- GUI display helpers (Qt-only; operate on the core domain types) --------

// Human-readable result-type labels, in the order used by the display menus.
inline QStringList resultTypeNames() {
    return {
        "IOps",
        "MBps (Decimal)",
        "MBps (Binary)",
        "Read IOps",
        "Write IOps",
        "Read MBps (Dec)",
        "Write MBps (Dec)",
        "Avg Latency (ms)",
        "Max Latency (ms)",
        "CPU Util (%)",
        "CPU User (%)",
        "CPU Kernel (%)",
        "Errors",
    };
}

// Format one size value as "X MiB" / "Y KiB" / "Z B" (largest unit that fits).
// Thin Qt wrapper over the shared core implementation.
inline QString formatSizeCompact(int bytes) {
    return QString::fromStdString(iocore::formatSize(bytes));
}

// Display label for an access spec (single-line specs show
// "64 KiB; 100% Read; 0% random"; others show the name). Shared core impl.
inline QString accessSpecDisplayLabel(const AccessSpec &s) {
    return QString::fromStdString(iocore::accessSpecLabel(s));
}
