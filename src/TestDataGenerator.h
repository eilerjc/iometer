#ifndef TEST_DATA_GENERATOR_H
#define TEST_DATA_GENERATOR_H

#include "IOCommon.h"

// Generates synthetic but protocol-valid test data for Dynamo test mode
// Returns hardcoded but realistic responses without real disk access
// Allows automated testing without elevation or raw disk I/O

class TestDataGenerator
{
public:
    // Generate synthetic Manager_Info for LOGIN response
    static void getManagerInfo(Manager_Info &info);

    // Generate synthetic disk target list
    static int getTargets(DyTargetSpec *targetArray, int maxTargets);

    // Generate synthetic worker results (with slight variation per cycle)
    // Variations make tests more realistic without randomness
    static void getWorkerResults(DyWorkerResults &results);

private:
    // Test data constants
    static constexpr const char *TEST_MANAGER_NAME = "TestManager";
    static constexpr int TEST_PROCESSOR_COUNT = 4;
    static constexpr double TEST_TIMER_RESOLUTION = 1.0;

    // Target constants
    static constexpr const char *TEST_TARGET_C = "C:";
    static constexpr const char *TEST_TARGET_D = "D:";
    static constexpr int64_t TEST_DISK_SIZE = 500000000000;  // 500 GB

    // Result constants (deterministic variation)
    static constexpr double BASE_IOPS = 2500.0;
    static constexpr double BASE_MBPS = 250.0;
    static constexpr double BASE_LATENCY = 2.5;
    static constexpr double BASE_CPU = 60.0;
};

#endif  // TEST_DATA_GENERATOR_H
