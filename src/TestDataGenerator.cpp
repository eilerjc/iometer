#include "TestDataGenerator.h"
#include <cstring>
#include <cmath>

// Generate synthetic Manager_Info for LOGIN response
void TestDataGenerator::getManagerInfo(Manager_Info &info)
{
    memset(&info, 0, sizeof(Manager_Info));

    // Set manager name
    strncpy_s(info.name, sizeof(info.name), TEST_MANAGER_NAME, strlen(TEST_MANAGER_NAME));

    // Set processor count
    info.processor_count = TEST_PROCESSOR_COUNT;

    // Set timer resolution
    info.timer_resolution = TEST_TIMER_RESOLUTION;

    // Set network addresses (names[1] is primary, others are alternatives)
    strncpy_s(info.names[1], sizeof(info.names[1]), "127.0.0.1", strlen("127.0.0.1"));
}

// Generate synthetic disk target list
int TestDataGenerator::getTargets(DyTargetSpec *targetArray, int maxTargets)
{
    if (maxTargets < 2) return 0;

    // Target 1: Physical disk C:
    DyTargetSpec &target1 = targetArray[0];
    memset(&target1, 0, sizeof(DyTargetSpec));
    strncpy_s(target1.name, sizeof(target1.name), TEST_TARGET_C, strlen(TEST_TARGET_C));
    target1.type = DY_PHYSICAL_DISK;
    target1.size = TEST_DISK_SIZE;
    target1.ready = 1;

    // Target 2: Logical volume D:
    DyTargetSpec &target2 = targetArray[1];
    memset(&target2, 0, sizeof(DyTargetSpec));
    strncpy_s(target2.name, sizeof(target2.name), TEST_TARGET_D, strlen(TEST_TARGET_D));
    target2.type = DY_LOGICAL_DISK;
    target2.size = TEST_DISK_SIZE * 2;  // D: is 1 TB
    target2.disk_info.ready = 1;

    return 2;  // Two targets
}

// Generate synthetic worker results with deterministic variation
void TestDataGenerator::getWorkerResults(DyWorkerResults &results)
{
    memset(&results, 0, sizeof(DyWorkerResults));

    // Generate deterministic but realistic variation based on current time
    // Use modulo arithmetic to get repeatable but varying values
    static int cycle = 0;
    cycle = (cycle + 1) % 10;  // Cycle 0-9

    // Add ±20% variation based on cycle
    double variation = 0.9 + (cycle * 0.02);  // 0.9 to 1.1

    // IOps: 2000-3000
    results.raw_results[0].io_ops = static_cast<int64_t>(BASE_IOPS * variation);

    // MBps (Decimal): 200-300
    results.raw_results[0].bytes_read = static_cast<int64_t>(BASE_MBPS * variation * 1024 * 1024);

    // Latency: 2-3ms
    results.raw_results[0].latency_ms = BASE_LATENCY * variation;

    // CPU utilization: 50-70%
    results.cpu_utilization = BASE_CPU * variation;

    // No errors in synthetic test
    results.errors = 0;

    // Response time stats
    results.raw_results[0].response_time_ms = BASE_LATENCY * variation;
    results.raw_results[0].max_response_time_ms = BASE_LATENCY * variation * 1.5;
}
