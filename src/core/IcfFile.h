#pragma once

#include <string>
#include <vector>
#include "IometerTypes.h"

// Encapsulates ICF 1.1.0 file format parsing and writing (platform-agnostic)
// Shared interface for the GUI front-ends
// Handles: TestConfig + AccessSpecs + worker assignments, version validation

class IcfFile {
public:
    // Represents a worker's batch configuration (from ICF file)
    struct BatchWorker {
        std::string name;
        std::string type = "DISK";   // worker type from 'Worker type (DISK/TCP/VI)
        std::vector<std::string> assignedSpecs;
        std::vector<std::string> targets;
    };

    // Load ICF configuration from file
    // Returns: true if file read successfully and parsed
    static bool load(const std::string &filepath,
                     TestConfig &cfg,
                     std::vector<AccessSpec> &specs,
                     std::vector<BatchWorker> &workers);

    // Save ICF configuration to file
    static bool save(const std::string &filepath,
                     const TestConfig &cfg,
                     const std::vector<AccessSpec> &specs,
                     const std::vector<BatchWorker> &workers = {});

    // Validate ICF version string
    static bool isValidVersion(const std::string &versionStr);

    // Format version
    static constexpr const char *VERSION = "1.1.0";

private:
    static const std::string VERSION_STRING;
};
