# Dynamo Test Mode Implementation

## Status: implemented as a separate `dynamotest` binary (compile-time)

Test mode is a **compile-time** decision, not a runtime flag. A dedicated
project, `src/msvs11/Dynamotest.vcxproj`, builds the same Dynamo sources with
`IOMTR_TEST_MODE` defined and produces `dynamotest.exe`. The production
`Dynamo.exe` contains **none** of the test code and keeps its
`RequireAdministrator` manifest (so real device discovery works as users
expect). `dynamotest.exe` uses an `AsInvoker` manifest and runs **non-elevated**.

### What test mode does

It keeps as much real Dynamo code in use as possible — protocol exchange,
manager/grunt scheduling, results aggregation and reporting all run unchanged.
Only the device boundary is synthetic:

- `Manager::Report_Disks` (IOManagerWin.cpp) reports a single synthetic logical
  target and skips real enumeration (the physical-drive probe opens
  `\\.\PhysicalDriveN`, which needs elevation).
- `TargetDisk::Initialize` assigns synthetic geometry (no device query).
- `TargetDisk::Open` / `Prepare` / `Close` use a sentinel handle (no `CreateFile`).
- `TargetDisk::Read` / `Write` complete synchronously and synthetically
  (`ReturnSuccess`), so `Grunt::Do_IOs` records each I/O through the normal
  `Record_IO` path — real result accounting, no completion queue.

### Targetable performance (`--rdelay` / `--wdelay`)

By default synthetic I/Os complete instantly (effectively unbounded IOPS).
`dynamotest` accepts per-I/O delays in microseconds so a run can target a chosen
latency/IOPS:

```
dynamotest --rdelay 100 --wdelay 200 -i 127.0.0.1 -m <managername>
```

`--rdelay 100` makes each read take 100 us (busy-wait on the high-resolution
counter), yielding ~10,000 IOPS per outstanding read and an average latency of
~0.100 ms. These options are compiled only into `dynamotest` (guarded by
`IOMTR_TEST_MODE` in `Pulsar.cpp`); production Dynamo does not have them.

### Validated

`dynamotest.exe` connects to `IometerQt` (batch mode) over TCP with **no
elevation**, completes the full login -> target report -> worker setup -> test ->
results cycle, and writes a results CSV. With `--rdelay 100 --wdelay 200`:
IOps ~9,937, MBps ~651, AvgLatency 0.1004 ms, Errors 0.

### Notes

- The old `TestDataGenerator.{h,cpp}` foundation was removed: it referenced
  struct fields that do not exist in the real Dynamo types (it never compiled),
  and the synchronous-completion approach above makes it unnecessary — synthetic
  result counts come from the real `Record_IO` path.
- Cosmetic: the CPU performance counter prints "percentage is less than zero"
  warnings under synthetic timing; harmless.
