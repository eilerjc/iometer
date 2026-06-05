#pragma once

#include <QString>

// Forward declarations
struct TestConfig;

// Encapsulates ICF 1.1.0 file format parsing and writing
// Shared interface for MFC and Qt implementations
// Handles: TestConfig serialization, version validation, round-trip fidelity

class IcfFile {
public:
    // Load ICF configuration from file
    static bool load(const QString &filepath, TestConfig &cfg);

    // Save ICF configuration to file
    static bool save(const QString &filepath, const TestConfig &cfg);

    // Validate ICF version string
    static bool isValidVersion(const QString &versionStr);

    // Format version
    static constexpr const char *VERSION = "1.1.0";

private:
    static const QString VERSION_STRING;
};
