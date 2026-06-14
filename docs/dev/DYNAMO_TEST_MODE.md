# Dynamo Test Mode Design

Add a non-elevated test mode to Dynamo that returns synthetic but protocol-valid responses for verification purposes.

## Problem Solved

Current smoke/integration tests require:
- ✗ Elevation (UAC prompt)
- ✗ Real disk access
- ✗ Actual I/O performance data
- ✗ Can't run in headless CI/CD

With test mode:
- ✓ No elevation needed
- ✓ No disk access required
- ✓ Deterministic, reproducible data
- ✓ Runs anywhere (CI/CD, headless, etc.)

## Implementation Options

### Option A: Command-Line Flag (Recommended)

```bash
# Normal mode (real I/O)
Dynamo.exe -i 127.0.0.1 -m hostname

# Test mode (synthetic data)
Dynamo.exe -i 127.0.0.1 -m hostname -test
Dynamo.exe -i 127.0.0.1 -m hostname --mock
```

**Behavior in test mode:**
- Skip raw disk access (no elevation)
- Return hardcoded Manager_Info
- Simulate 2 disk targets (physical + logical)
- Return synthetic but realistic WorkerResults
- Maintain identical protocol format

### Option B: Separate Binary

```bash
# New executable
Dynamo-Test.exe -i 127.0.0.1 -m hostname
# Always runs in test mode
```

**Pros**: No flag confusion
**Cons**: Duplicate binary maintenance

## Proposed Architecture

### Dynamo Flow with Test Mode

```
Dynamo startup
  ├─ Parse args (-test flag?)
  │
  ├─ [Test Mode OFF]
  │  └─ Normal: Enumerate disks → raw I/O → return real data
  │
  └─ [Test Mode ON]
     └─ Synthetic: Return hardcoded protocol responses
```

### Test Data to Generate

When `-test` flag set, Dynamo returns:

1. **Manager_Info** (on LOGIN)
   ```cpp
   struct TestManagerInfo {
       char name[] = "TestManager";
       uint16_t processor_count = 4;
       double timer_resolution = 1.0;
       // ... other fields with sensible defaults
   };
   ```

2. **Target List** (REPORT_TARGETS)
   ```cpp
   // Two synthetic targets:
   // 1. Physical disk "C:" (500 GB)
   // 2. Logical volume "D:" (1 TB)
   ```

3. **Worker Results** (per test cycle)
   ```cpp
   // Synthetic but realistic:
   // - IOps: 1000-5000 (varies slightly per cycle)
   // - MBps: 100-500 (realistic range)
   // - Latency: 1-5ms (typical)
   // - CPU: 50-80% (moderate load)
   // - Errors: 0 (no failures in test)
   ```

## Benefits

### For Testing
- ✓ Run integration tests without `sudo`/elevation
- ✓ Deterministic results (same test always produces same output)
- ✓ Faster execution (no real I/O wait)
- ✓ Run on any system (headless, CI/CD, Windows/Linux)

### For Validation
- ✓ Both Original (MFC) and Qt can be tested automatically
- ✓ Comparison tests with known inputs → predictable outputs
- ✓ Protocol implementation validation (same message format)
- ✓ Integration test in CI/CD pipeline

### For Development
- ✓ Easy to debug (data is known)
- ✓ Reproducible failures (same test data every time)
- ✓ No environmental dependencies
- ✓ Supports TDD workflows

## Test Scenarios with Test Mode

### Scenario 1: Protocol Validation (Headless CI)
```bash
# Run both engines against test Dynamo (no elevation)
Dynamo.exe -i 127.0.0.1 -m test-mgr -test &
Iometer.exe /c test.icf /r results_mfc.csv
IometerQt.exe /c test.icf /r results_qt.csv

# Compare CSV outputs
compare_guis.ps1  # Both use identical test data
```

### Scenario 2: Comparison Test (Automated)
```bash
# Known inputs → known outputs
# Both engines produce identical results
./test/compare_guis.ps1 -DynamoMode test

# Can run in parallel now (deterministic, no disk contention)
```

### Scenario 3: CI/CD Pipeline
```yaml
# GitHub Actions / GitLab CI
test:
  script:
    - Dynamo.exe -test &  # Start test Dynamo
    - ./test/smoke_test.ps1 -Engine Original -DynamoTestMode
    - ./test/smoke_test.ps1 -Engine Qt -Demo
    - ./test/compare_guis.ps1
```

## Implementation Checklist

### Phase 1: Add Test Mode to Dynamo

- [ ] Add `-test` command-line flag parsing
- [ ] Create `TestDataGenerator` class:
  - [ ] Synthetic Manager_Info
  - [ ] Synthetic target enumeration
  - [ ] Synthetic worker results
- [ ] Update protocol handlers to use test data when `-test` enabled
- [ ] Ensure protocol format is identical (tests both paths)

**Effort**: ~4-6 hours

### Phase 2: Update Test Scripts

- [ ] Add `-DynamoTestMode` flag to `smoke_test.ps1`
- [ ] Add `-test` option to `compare_guis.ps1`
- [ ] Update CI/CD pipeline (if applicable)

**Effort**: ~2-3 hours

### Phase 3: Validation

- [ ] Verify both Original and Qt work with test Dynamo
- [ ] Confirm no elevation required
- [ ] Verify comparison tests pass with test data
- [ ] Document test mode in BUILDING.md

**Effort**: ~1-2 hours

---

## Test Data Constants

What synthetic data should test mode return?

**Option 1: Minimal Valid Data**
```
IOps: 1000
MBps: 100
Latency: 2ms
Errors: 0
Workers: 1
Targets: 2 (C: and D:)
```

**Option 2: Realistic Variation**
```
IOps: varies by cycle (1000-5000)
MBps: varies by cycle (100-500)
Latency: varies by cycle (1-5ms)
Errors: 0
Workers: 2-4
Targets: 2-4
```

**Recommendation**: Option 2 (more realistic, catches edge cases)

---

## Risk Assessment

**Risk Level: LOW**

- Test mode is additive (doesn't change normal path)
- Can be toggled with simple flag
- Protocol format unchanged (validation still works)
- Doesn't affect production builds
- Easy to verify (output is predictable)

## Success Criteria

✅ Dynamo test mode works without elevation
✅ Both Original (MFC) and Qt engines accept test Dynamo
✅ Protocol validation still works (same message formats)
✅ CSV outputs are deterministic (same test data → same results)
✅ Comparison tests can run automated without human intervention
✅ All tests pass in headless CI/CD environment

---

**Recommendation**: Implement Option A (command-line flag) in Phase 2, right after Phase 1 refactoring is complete. This unlocks automated comparison testing without elevation.

**Timeline**: ~7-10 hours total (Dynamo + test scripts + validation)
