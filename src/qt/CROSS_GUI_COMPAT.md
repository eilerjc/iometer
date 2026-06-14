# Cross-GUI Compatibility: MFC vs Qt

This document explains the business logic alignment between the original MFC Iometer (Galileo) and the new Qt Iometer, ensuring both versions are compatible and interoperable.

## Architecture Comparison

### MFC (Original)
- **Location**: `src/` (GalileoApp, GalileoDoc, GalileoView, etc.)
- **GUI Framework**: MFC (Microsoft Foundation Classes, Windows-only)
- **Config Format**: ICF files (Iometer Configuration Format v1.1.0)
- **Protocol**: Dynamo protocol (binary, #pragma pack(1) structs)
- **Key Classes**:
  - `CGalileoApp` — application shell
  - `GalileoDoc` — document model
  - `GalileoView` — main UI and toolbar
  - `IOManager`, `IOTest` — business logic
  - `ICF_ifstream` — ICF file parser

### Qt (New)
- **Location**: `src/qt/`
- **GUI Framework**: Qt6 (cross-platform: Windows, Mac, Linux)
- **Config Format**: Same ICF v1.1.0
- **Protocol**: Same Dynamo protocol
- **Key Classes**:
  - `MainWindow` — application shell
  - `DynamoEngine`, `DemoEngine` — business logic
  - `IometerEngine` — abstract engine interface
  - ICF parsing in `DynamoEngine::loadConfig()`

## Compatibility Matrix

### 1. ICF File Format (Read/Write)
**Requirement**: Both versions must read and write the same ICF format

| Aspect | MFC | Qt | Status |
|---|---|---|---|
| **Version string** | "Version 1.1.0" at start/end | "Version 1.1.0" at start/end | ✓ Verified by tst_icf.cpp |
| **Test Setup section** | Parses run time, ramp time, description | Same parsing | ✓ Tested |
| **Access Specifications** | Multi-line specs with per-line metadata | Same format | ✓ Tested |
| **Manager List** | Per-manager workers and targets | Same hierarchy | ✓ Batch mode tests |
| **Comment lines** | Lines starting with `'` are comments | Preserved | ✓ Tested |
| **Round-trip fidelity** | Load → Save → Reload | Bit-for-bit identical | ✓ tst_icf::roundTrip |

**Test Coverage**: 
- `tst_icf.cpp` verifies round-trip (smoke_test.icf loads identically after save/load)
- `tst_results.cpp` verifies CSV format matches original spec
- Both use standard formats from ICF 1.1.0 specification

### 2. Dynamo Protocol (Wire Compatibility)
**Requirement**: Both implementations must speak the same binary protocol with Dynamo

| Aspect | Specification | Qt Implementation | Verification |
|---|---|---|---|
| **Port** | 1066 | `DY_PORT = 1066` | ✓ tst_protocol::port_is1066 |
| **Version** | 0x01000100 (v1.1.0) | Same constant | ✓ tst_protocol::version_major1_minor1 |
| **Message sizes** | Strict #pragma pack(1) | Identical structs | ✓ tst_protocol (7 struct size asserts) |
| **DyMsg** | 8 bytes | 8 bytes | ✓ sizeof assertion |
| **DyDataMessage** | 888,840 bytes | 888,840 bytes | ✓ sizeof assertion |
| **Command codes** | DY_LOGIN=0x10000001, etc. | Identical constants | ✓ tst_protocol (9 command tests) |
| **Target types** | DY_PHYSICAL_DISK, DY_LOGICAL_DISK, etc. | Same enums | ✓ tst_protocol::targetType_* |

**Test Coverage**: 
- `tst_protocol.cpp` documents and verifies all protocol structure sizes
- Smoke tests exercise both MFC and Qt via real Dynamo communication
- Any protocol mismatch would cause Dynamo connection failure (self-evident)

### 3. Results Aggregation (CSV Output)
**Requirement**: Both versions produce identical CSV results format

| Column | MFC | Qt | Verification |
|---|---|---|---|
| **Field [0]** | "ALL" aggregate marker | Same | ✓ tst_results::allRow_field0_isALL |
| **Field [6]** | IOps (total I/O ops/sec) | Same | ✓ tst_results::allRow_field6_isIops |
| **Field [12]** | MBps (Decimal) | Same | ✓ tst_results::allRow_field12_isMbpsDec |
| **Field [27]** | Errors (count) | Same | ✓ tst_results::allRow_field27_isErrors |
| **Header row** | Column names matching MFC | Same headers | ✓ tst_results::csv_header_row |
| **Aggregation** | Sum IOps, sum errors, average latency | Same logic | ✓ tst_results::aggregate_sumsIops |

**Test Coverage**: 
- `tst_results.cpp` verifies CSV column layout and aggregation logic
- `smoke_test.ps1` compares MFC and Qt CSV output line-by-line
- Smoke test uses same ICF for both versions and compares results

### 4. Access Spec Library (Built-in Specs)
**Requirement**: Both versions have same set of access specs for consistency

| Aspect | MFC | Qt | Status |
|---|---|---|---|
| **Count** | 32 specs | 32 specs | ✓ tst_accessspecs::builtinCount |
| **First spec** | "Idle" | "Idle" | ✓ tst_accessspecs::firstSpecIsIdle |
| **Second spec** | "Default" | "Default" | ✓ tst_accessspecs::secondSpecIsDefault |
| **Last spec** | "All in one" (multi-line) | "All in one" (29 lines) | ✓ tst_accessspecs::allInOne_lineCount |
| **"All in one" distribution** | ofSize sums to 100% | Same | ✓ tst_accessspecs::allInOne_ofSizeSumsTo100 |
| **No duplicates** | N/A | Verified | ✓ tst_accessspecs::noDuplicateNames |

**Test Coverage**: 
- `tst_accessspecs.cpp` verifies all 32 specs exist with correct parameters
- `IometerEngine::builtinAccessSpecs()` is shared by both Qt and smoke tests
- MFC has its own spec library (not directly tested, but empirically compared via smoke tests)

## Test Strategy for Cross-GUI Validation

### Unit Tests (Qt-only, verified in CI)
Since testing MFC directly would require:
1. MFC compilation (Visual Studio MFC libraries)
2. Complex linking and dependencies
3. Windows-only execution

Instead, we:
- **Verify Qt against published spec**: `tst_*.cpp` verify Qt against ICF 1.1.0 and Dynamo protocol specs
- **Empirically validate via smoke tests**: `smoke_test.ps1` exercises both MFC and Qt on the same configs
- **Format round-trip tests**: Both read/write the same ICF and CSV formats

### Smoke Tests (Real-world validation)
File: `test/smoke_test.ps1`

Validates:
1. Original Iometer (MFC) in -Engine Original mode
2. Qt Iometer in -Engine Qt -Demo and -Engine Qt -Batch modes
3. Both use `test/smoke_test.icf` (one config file, multiple engines)
4. Both produce CSV results with same column layout
5. Results are compared programmatically

Run:
```powershell
powershell -ExecutionPolicy Bypass -File test/smoke_test.ps1
```

## Known Compatibility Guarantees

1. **ICF Format**: Both read/write version 1.1.0; round-trips are lossless
2. **Dynamo Protocol**: Identical struct sizes and command codes (verified by tests)
3. **Results CSV**: Column layout and aggregation are identical
4. **Access Specs**: Qt has verified 32 built-in specs; MFC also has 32
5. **Configuration**: Batch mode can load configs created by either version

## Design Rationale

### Why No Direct MFC Unit Tests?
- MFC is legacy code (2003 era C++)
- Requires Windows-only MFC libraries and compilation
- No value in unit-testing closed, stable code
- Smoke tests provide empirical validation
- Qt version is the forward direction

### Why Comparison Tests Matter
- ICF format compatibility ensures config portability
- Protocol compatibility ensures both can talk to Dynamo
- CSV format compatibility ensures result interoperability
- Access spec parity ensures consistent test definitions

## Future Maintenance

If you modify:
- **ICF parser in Qt**: Run `test/smoke_test.ps1` to ensure MFC can still read configs
- **Dynamo protocol in Qt** (unlikely): Verify struct sizes in `tst_protocol.cpp` and re-run smoke tests
- **Access specs in Qt**: Ensure count stays 32, "All in one" has 29 lines, etc.
- **Results CSV output in Qt**: Verify columns [0], [6], [12], [27] match spec

Cross-GUI compatibility is maintained by following the **ICF 1.1.0 format specification** and **Dynamo protocol specification**, not by coupling implementations.
