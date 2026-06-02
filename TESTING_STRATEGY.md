# Testing Strategy: Behavioral Equivalence (MFC vs Qt)

## Goal
Verify that both Iometer implementations (original MFC and new Qt) produce **identical outputs** for **identical inputs**, regardless of internal implementation differences.

## Approach

Rather than assume specifications are accurate, we:
1. **Unit test the Qt implementation** against the published ICF 1.1.0 and Dynamo protocol specs
2. **Empirically validate both** by running them on identical test data and comparing outputs
3. **Compare code coverage** to ensure tests exercise equivalent business logic

## Test Execution Model

### Layer 1: Unit Tests (Qt only, runs in CI)
**Location**: `src/qt/tests/tst_*.cpp`  
**Coverage**: 148 tests across 9 test classes

These verify Qt against the published specs:
- Protocol struct sizes, command codes (tst_protocol.cpp)
- ICF format parsing/writing (tst_icf.cpp)
- CSV results format and aggregation (tst_results.cpp)
- Access spec library (tst_accessspecs.cpp)

### Layer 2: Smoke Tests (both implementations, manual)
**Location**: `test/smoke_test.ps1`  
**Input**: `test/smoke_test.icf` (shared test config)  
**Executables**: 
- MFC: `src/Release/Iometer.exe`
- Qt: `src/qt/build/Release/IometerQt.exe`

These empirically validate both implementations:
- Load identical config
- Run identical test
- Compare CSV output column-by-column
- Verify numerical equivalence (IOps, latency, errors)

### Layer 3: Comparison Tests (to be added)
**Location**: `test/compare_guis.ps1` (framework) + new unit tests  
**Purpose**: Direct behavioral equivalence validation

## Current Test Coverage

| Behavior | Qt Unit Test | MFC Validation | Coverage |
|---|---|---|---|
| **ICF parsing** | tst_icf.cpp (8 tests) | smoke_test.ps1 load | ✓ Both directions |
| **Access specs** | tst_accessspecs.cpp (17 tests) | smoke_test config | ✓ Library equivalence |
| **CSV output format** | tst_results.cpp (14 tests) | smoke_test.ps1 parse | ✓ Column layout verified |
| **Results aggregation** | tst_results.cpp (5 tests) | smoke_test results | ✓ Numerical equivalence |
| **Protocol structures** | tst_protocol.cpp (33 tests) | Dynamo connection | ✓ Wire format |
| **ICF round-trip** | tst_icf.cpp::roundTrip | smoke_test verify | ✓ Load/save/reload |
| **UI widget state** | tst_meters, tst_pagesetup, tst_mainwindow (36 tests) | N/A (UI only) | Qt only |

**Total**: 148 tests

## Proposed Additional Tests for Exact Behavioral Equivalence

To make the comparison tests more explicit and comprehensive:

### 1. Direct CLI Comparison Tests
```powershell
# Test: Both load same ICF, produce identical parsed config
compare_guis.ps1 --test icf-parsing --config test/smoke_test.icf

# Test: Both run same config, produce identical CSV results
compare_guis.ps1 --test batch-results --config test/smoke_test.icf

# Test: Both write ICF, output is byte-for-byte identical
compare_guis.ps1 --test icf-roundtrip --config test/smoke_test.icf
```

### 2. C++ Comparison Unit Tests
Add to `src/qt/tests/tst_mfc_compare.cpp` (new file):

```cpp
// Test both implementations with same input data:
void test_icf_parsing_equivalence() {
    // Load with Qt
    DynamoEngine qt_engine;
    qt_engine.loadConfig("test/smoke_test.icf");
    auto qt_config = qt_engine.testConfig();
    auto qt_specs = qt_engine.accessSpecs();
    
    // Load with MFC (via embedded executable or shared library)
    // Compare: run time, access specs, worker config
    QCOMPARE(mfc_config.runSeconds, qt_config.runSeconds);
    QCOMPARE(mfc_specs.size(), qt_specs.size());
    // ... field-by-field comparison
}

void test_csv_aggregation_equivalence() {
    // Feed same raw Dynamo results to both
    QVector<WorkerResult> results = /* identical test data */;
    
    // Qt aggregation
    auto qt_csv = DynamoEngine::writeBatchResultsCsv(...);
    
    // MFC aggregation
    auto mfc_csv = call_mfc_aggregation(...);
    
    // Compare: column-by-column, value-by-value
    QCOMPARE(parse_csv_field(qt_csv, 0), parse_csv_field(mfc_csv, 0)); // "ALL"
    QCOMPARE(parse_csv_field(qt_csv, 6), parse_csv_field(mfc_csv, 6)); // IOps
    QCOMPARE(parse_csv_field(qt_csv, 12), parse_csv_field(mfc_csv, 12)); // MBps(Dec)
    QCOMPARE(parse_csv_field(qt_csv, 27), parse_csv_field(mfc_csv, 27)); // Errors
}
```

### 3. Property-Based Tests
For numeric fields that might have floating-point precision differences:

```cpp
void test_latency_aggregation_within_tolerance() {
    // Both average latencies should match to within floating-point precision
    QCOMPARE_WITH_TOLERANCE(mfc_avg_latency, qt_avg_latency, /*epsilon*/ 1e-6);
}
```

## How to Run Tests

### Unit Tests (Qt only)
```powershell
cd src/qt/build/Release
foreach ($test in @("tst_types", "tst_accessspecs", "tst_icf", "tst_results", "tst_demo", "tst_protocol", "tst_meters", "tst_pagesetup", "tst_mainwindow")) {
    & ".\$test.exe" -silent
    if ($LASTEXITCODE -ne 0) { Write-Host "FAIL: $test" }
}
```

### Smoke Tests (both implementations)
```powershell
powershell -ExecutionPolicy Bypass -File test/smoke_test.ps1
```

### Comparison Tests (both implementations, to be added)
```powershell
powershell -ExecutionPolicy Bypass -File test/compare_guis.ps1
```

## Success Criteria

All tests pass when:

1. **Unit tests**: 148/148 Qt unit tests pass
2. **Smoke tests**: 
   - MFC and Qt both load `test/smoke_test.icf`
   - Both run in their respective modes (Original, Qt -Demo, Qt -Batch)
   - CSV outputs have identical column counts
   - Field [0] = "ALL", [6] = IOps, [12] = MBps(Dec), [27] = Errors
   - Numerical values match to within acceptable precision
3. **Comparison tests** (when added):
   - ICF round-trip produces byte-for-byte identical output
   - Aggregation produces numerically equivalent results
   - All fields in CSV match to within floating-point tolerance

## Why This Approach

**Instead of**: Testing MFC code directly (requires MFC libraries, Windows-only, complex linking)

**We do**: Test both implementations with identical inputs and compare outputs

**Benefits**:
- No need to refactor MFC code for testability
- Validates actual behavior, not assumed behavior
- Catches implementation deviations from spec
- Works regardless of spec accuracy
- Runs in CI (Qt tests) + manual validation (smoke tests)
- Scalable: easy to add more test configs

## Maintenance

If either implementation changes:
1. Update the corresponding unit tests (Qt) or add comparison tests
2. Run smoke_test.ps1 to verify empirical equivalence
3. If outputs differ, document why (e.g., deliberate bug fix, spec clarification)
4. Update comparison tests to validate the change applies to both implementations

## Code Coverage

| Implementation | Unit Tests | Smoke Tests | Coverage Method |
|---|---|---|---|
| **Qt** | Direct (148 tests) | ICF/CSV parsing | Instrumented |
| **MFC** | Via comparison | ICF/CSV parsing + Dynamo | Behavioral |

Both implementations exercise:
- ICF parsing logic (test/smoke_test.icf)
- Access spec handling (built-in library)
- Dynamo protocol (real Dynamo connection)
- CSV aggregation (results output)
- Ramp/test lifecycle (start/stop commands)

Equivalence is demonstrated by identical outputs on identical inputs.
