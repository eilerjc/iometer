#pragma once

#include <QString>
#include <QList>

// Forward declarations
struct TestConfig;
struct AccessSpec;

// Encapsulates ICF 1.1.0 file format parsing and writing
// Shared interface for MFC and Qt implementations
// Handles: TestConfig + AccessSpecs + worker assignments, version validation, round-trip fidelity

class IcfFile {
public:
    // Represents a worker's batch configuration (from ICF file)
    struct BatchWorker {
        QString name;
        QStringList assignedSpecs;
        QStringList targets;
    };

    // Load ICF configuration from file
    // Returns: true if file read successfully and parsed
    // Outputs: cfg (TestConfig), specs (AccessSpecs), workers (batch worker configs)
    static bool load(const QString &filepath,
                     TestConfig &cfg,
                     QList<AccessSpec> &specs,
                     QList<BatchWorker> &workers);

    // Save ICF configuration to file
    // Parameters:
    //   filepath - output file path
    //   cfg - TestConfig to write
    //   specs - AccessSpecs to write
    //   workers - batch worker configs (or empty if loading from live Dynamo)
    static bool save(const QString &filepath,
                     const TestConfig &cfg,
                     const QList<AccessSpec> &specs,
                     const QList<BatchWorker> &workers = QList<BatchWorker>());

    // Validate ICF version string
    static bool isValidVersion(const QString &versionStr);

    // Format version
    static constexpr const char *VERSION = "1.1.0";

private:
    static const QString VERSION_STRING;
};
