# MFC Coverage Implementation Requirements

What would be needed to measure code coverage for the original MFC Iometer implementation.

## Current Situation

**MFC Build:**
- Compiler: MSVC 2011 (Visual Studio 2012)
- Location: `src/msvs11/`
- No coverage instrumentation
- No clang-cl support (Windows-only MFC)

**Qt Build:**
- Compiler: clang-cl (LLVM)
- Location: `src/qt/`
- Coverage-ready: `-fprofile-instr-generate -fcoverage-mapping`
- Tools: llvm-profdata, llvm-cov

**The Problem:**
- MFC uses MSVC compiler
- Qt uses clang-cl compiler
- Coverage tools incompatible between them
- Can't directly instrument both with same toolchain

---

## Option 1: MSVC Profile-Guided Optimization (PGO)

### Requirements

**Compiler Flags:**
```
Compiler:
  /PROFILE                 # Enable instrumentation
  /O2                      # Optimization (required for PGO)

Linker:
  /PROFILE                 # Link with profiling
  /PGD:iometer.pgd        # Profile database
```

**Tools:**
- Visual Studio 2012 (already required for MFC build)
- pgosweep.exe — collect profile data
- pgomgr.exe — manage/merge profile databases

**Build Changes:**
```
1. Add /PROFILE flags to MFC project
2. Create new configuration: Release_Coverage
3. Rebuild src/msvs11/ with coverage
4. Output: Release_Coverage/x64/Iometer_Coverage.exe
```

### Implementation Steps

```powershell
# 1. Open src/msvs11/Iometer.sln in Visual Studio 2012
# 2. Add new configuration "Release_Coverage"
# 3. Copy settings from Release, add /PROFILE flags
# 4. Build Release_Coverage configuration
# 5. Run test with instrumented binary

# Run instrumented MFC
.\Iometer_Coverage.exe /c test.icf /r results.csv

# Collect profile data
pgosweep.exe -shutdown:iometer.pgd

# View coverage in Visual Studio
# Project → Properties → Code Coverage
```

### Coverage Tools

**Visual Studio Built-in:**
- Test → Analyze Code Coverage
- Menu: Project → Analyze Code Coverage
- Shows coverage by file/function

**LLVM Conversion (Optional):**
```bash
# Convert MSVC PGO to LLVM format (advanced)
# Not straightforward; requires PGO to JSON conversion
```

### Pros
- ✓ Native Visual Studio integration
- ✓ Per-function coverage reports
- ✓ Relatively easy to set up
- ✓ Standard Microsoft tooling

### Cons
- ✗ Only works with MSVC compiler
- ✗ PGO and clang-cl coverage incompatible
- ✗ Can't use same HTML report for both
- ✗ Separate toolchain from Qt coverage

**Effort:** ~2-4 hours  
**Complexity:** Medium

---

## Option 2: Clang-CL for MFC (Advanced)

### Requirements

**Toolchain Change:**
- Install LLVM with clang-cl
- Switch MFC build to clang-cl instead of MSVC
- Requires: LLVM 16+ with Windows support

**Build Changes:**
```
1. Install: winget install LLVM.LLVM
2. Open src/msvs11/Iometer.sln
3. Retarget solution to LLVM toolset
4. Add clang-cl flags: -fprofile-instr-generate -fcoverage-mapping
5. Rebuild entire MFC with clang-cl
6. Output: src/msvs11/Release/Iometer.exe (clang-cl)
```

### Challenge: MFC + Clang-CL Compatibility

**Compatibility Issues:**
- MFC headers assume MSVC compiler
- Some MSVC-specific pragmas/intrinsics
- Windows API headers may need adjustment
- Runtime library linking differences

**Required Fixes:**
```cpp
// MFC headers: Replace MSVC-only code
#ifdef _MSC_VER
    // MSVC-specific code
#elif defined(__clang__)
    // clang-cl equivalent
#endif

// Link against correct runtime
// -fuse-ld=lld (LLVM linker)
// Target Windows runtime libraries
```

### Coverage Collection

Once MFC builds with clang-cl:

```powershell
# 1. Run with coverage instrumentation
$env:LLVM_PROFILE_FILE = "mfc_coverage.profraw"
.\Iometer.exe /c test.icf /r results.csv

# 2. Merge profile data
llvm-profdata merge -o mfc_coverage.profdata mfc_coverage.profraw

# 3. Generate HTML report
llvm-cov show -format=html \
    -instr-profile=mfc_coverage.profdata \
    src/msvs11/Release/Iometer.exe \
    src/IOManager.cpp src/GalileoApp.cpp \
    > mfc_coverage.html

# 4. Compare with Qt coverage report
# Both use same llvm-cov tools → consistent reporting
```

### Pros
- ✓ Same tooling as Qt (llvm-cov, llvm-profdata)
- ✓ Unified coverage reports (MFC + Qt comparable)
- ✓ HTML reports match Qt format
- ✓ Can compare line-by-line

### Cons
- ✗ Requires extensive MFC code changes
- ✗ Unknown compatibility issues
- ✗ High risk of breaking MFC
- ✗ Maintenance burden (different toolchain)
- ✗ 20+ hours of work

**Effort:** ~20-40 hours  
**Complexity:** High  
**Risk:** Very High

---

## Option 3: Hybrid Approach (Recommended if Coverage Needed)

### Strategy

1. **Qt Coverage** (present, automated)
   - Use existing clang-cl instrumentation
   - Run: `./collect_gui_connection_coverage.ps1`
   - Output: llvm-cov HTML reports

2. **MFC Coverage** (MSVC PGO)
   - Add /PROFILE flags to MFC project
   - Run: `.\Iometer_Coverage.exe /c test.icf`
   - Output: Visual Studio coverage reports

3. **Comparison Documentation**
   - Side-by-side coverage analysis
   - Manual mapping of equivalent functions
   - Document which MFC functions execute
   - Compare to Qt execution traces

### Benefits
- ✓ Coverage for both implementations
- ✓ Minimal MFC changes (~30 minutes)
- ✓ Uses native tools for each (MSVC for MFC, llvm for Qt)
- ✓ Separate reports but documented equivalence
- ✓ Maintainable long-term

### Limitations
- ✗ Different reporting tools (VS vs llvm-cov)
- ✗ Manual comparison required
- ✗ Not automated in CI/CD

**Effort:** ~4-6 hours  
**Complexity:** Medium  
**Risk:** Low

---

## Option 4: Just Use Behavioral Equivalence (Current)

### What We Already Have

1. **Qt Unit Tests** — 102 core tests, 100% coverage
2. **Smoke Tests** — Both engines with real Dynamo
3. **Comparison Tests** — Same input, verify same output
4. **Function Mapping** — Document MFC ↔ Qt equivalence

### Why This Works

- ✓ Verifies behavioral equivalence (what matters)
- ✓ Catches any protocol/format mismatches
- ✓ Works in CI/CD (no special tooling)
- ✓ Low maintenance burden
- ✓ Honest about limitations

### Limitations

- ✗ Can't see which MFC functions execute
- ✗ Can't compare code paths directly
- ✗ No line-level MFC coverage metrics

**Effort:** $0 (already done)  
**Complexity:** None  
**Risk:** None

---

## Recommendation Matrix

| Goal | Approach | Effort | Risk | Value |
|------|----------|--------|------|-------|
| **See MFC line coverage** | MSVC PGO (Option 1) | 4h | Low | Medium |
| **Unified Qt+MFC coverage** | Clang-CL rewrite (Option 2) | 40h | High | High |
| **Best practical balance** | Hybrid (Option 3) | 6h | Low | High |
| **Verify equivalence** | Status quo (Option 4) | 0h | None | High |

---

## Implementation: MSVC PGO (Quick Win)

If you want MFC coverage now:

### Step 1: Prepare
```powershell
# Open Visual Studio 2012
# File → Open → src/msvs11/Iometer.sln
```

### Step 2: Create Coverage Configuration
```
1. Right-click solution → Configuration Manager
2. Copy "Release|x64" to new "Coverage|x64"
3. Right-click project → Properties
4. For "Coverage" configuration:
   - C/C++ → Code Generation:
     Add /PROFILE flag
   - Linker → Advanced:
     Profile Guided Database: iometer.pgd
   - Linker → General:
     Add /PROFILE
```

### Step 3: Build
```powershell
# In Visual Studio: Build → Batch Build
# Select "Coverage|x64" → Build

# Output: src/msvs11/Coverage/x64/Iometer.exe
```

### Step 4: Run Test
```powershell
# Run with profile collection
cd src/msvs11/Coverage/x64
.\Iometer.exe /c ../../test/smoke_test.icf /r results.csv

# This generates iometer.pgd (profile database)
```

### Step 5: View Coverage
```
In Visual Studio:
1. Open the project
2. Test → Analyze Code Coverage
3. Open iometer.pgd file
4. View coverage by file/function
```

### Timing
- Configure: ~15 min
- Build: ~5 min
- Run test: ~1 min
- View: ~5 min
- Total: ~30 min

---

## Conclusion

**For measuring MFC coverage:**

1. **Easiest:** Option 1 (MSVC PGO) — 4-6 hours, integrated with Visual Studio
2. **Best long-term:** Option 3 (Hybrid) — Both engines with native tools
3. **Most effort:** Option 2 (Clang-CL) — 20+ hours, high risk
4. **Current approach:** Option 4 (Behavioral) — Works well, no tooling needed

**My recommendation:** Start with **Option 4** (we're done). If you need MFC line-level coverage, implement **Option 1** (MSVC PGO) to complement it.

---

**Last Updated:** 2026-06-05
