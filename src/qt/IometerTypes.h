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
#include "../core/IometerTypes.h"   // single source of truth for domain types

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
inline QString formatSizeCompact(int bytes) {
    if (bytes <= 0)              return "0 B";
    if (bytes >= 1048576)        return QString("%1 MiB").arg(bytes / 1048576);
    if (bytes >= 1024)           return QString("%1 KiB").arg(bytes / 1024);
    return                              QString("%1 B").arg(bytes);
}

// Display label for an access spec (was AccessSpec::displayLabel()).
// Single-line specs show "64 KiB; 100% Read; 0% random"; others show the name.
inline QString accessSpecDisplayLabel(const AccessSpec &s) {
    if (s.lines.size() != 1) return QString::fromStdString(s.name);
    const auto &l = s.lines[0];
    return QString("%1; %2% Read; %3% random")
        .arg(formatSizeCompact(l.sizeBytes))
        .arg(l.readPercent)
        .arg(100 - l.seqPercent);
}
