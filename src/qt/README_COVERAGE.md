# Code Coverage for IometerQt Tests

Collect and view code coverage metrics using **clang-cl** (LLVM) and **llvm-cov**. Coverage builds are **separate from your default build** — they use a different compiler and don't interfere with Release/Debug builds.

## Prerequisites

```powershell
winget install LLVM.LLVM
```

Verify: `clang-cl --version` and `llvm-cov --version` should both work.

## Setup & Usage

### Step 1: Configure a coverage build

```powershell
cd src/qt
mkdir build_coverage && cd build_coverage

# Configure WITH clang-cl and coverage instrumentation
cmake .. -DIOMETER_COVERAGE=ON `
    -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" `
    -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" `
    -G Ninja
```

> Why Ninja instead of Visual Studio?  
> The Visual Studio generator expects MSVC; Ninja gives us more control over the toolchain.

### Step 2: Build

```powershell
cmake --build . --config Release
```

The first build takes longer due to coverage instrumentation flags. Subsequent builds are faster.

### Step 3: Collect coverage

After tests are built, run the helper script:

```powershell
cd ..  # back to src/qt
.\collect_coverage.ps1 -BuildDir build_coverage
```

This will:
1. Run all 6 tests with `LLVM_PROFILE_FILE` set (generates `*.profraw` files)
2. Merge profdata: `llvm-profdata merge -sparse *.profraw -o merged.profdata`
3. Generate HTML report: `llvm-cov show -format=html`
4. Offer to open `coverage_report/index.html` in your browser

### Step 4: View the report

Open `build_coverage/coverage_report/index.html`. Click any file to see:
- **Green**: line executed
- **Red**: line not executed
- **White**: not testable (comments, declarations only)

## Output Files

- `build_coverage/coverage_report/` — HTML report (open `index.html`)
- `build_coverage/coverage/merged.profdata` — raw coverage data
- `build_coverage/coverage/*.profraw` — per-test profiling data (intermediate)

## What Gets Covered?

The coverage instrumentation applies to:
- ✓ **DemoEngine.cpp** — simulated I/O engine
- ✓ **DynamoEngine.cpp** — Dynamo protocol, ICF parsing/writing, CSV output
- ✓ **IometerEngine library code** — built-in access specs, utilities
- ✗ UI widgets — MeterWidget, PageSetup, etc. (out of scope for unit tests)
- ✗ Main app — IometerQt.exe (GUI testing requires integration tests)

## Troubleshooting

**"cmake: not found"**  
CMake is installed with VS Studio but not in PATH. Either:
- Use: `"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"`
- Or add it to your PATH in System > Environment Variables

**"clang-cl not recognized"**  
```powershell
winget install LLVM.LLVM
# Restart PowerShell for PATH changes to take effect
```

**"Ninja not found"**  
```powershell
winget install Ninja-build.Ninja
```

**Tests fail to run (DLL not found)**  
```powershell
Copy-Item "C:\Qt\6.8.3\msvc2022_64\bin\Qt6*.dll" "build_coverage\Release\"
```

**No coverage data / empty report**  
- Verify tests ran: check `build_coverage\coverage\*.profraw` exists
- Verify profdata merged: `ls build_coverage\coverage\merged.profdata`
- Verify binary paths: the script lists which binaries it's reporting on

## CI/CD

For GitHub Actions:
```yaml
- name: Install LLVM
  run: winget install LLVM.LLVM --accept-source-agreements

- name: Configure coverage build
  run: |
    cd src/qt
    mkdir build_coverage && cd build_coverage
    cmake .. -DIOMETER_COVERAGE=ON -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" -G Ninja
    
- name: Build & collect coverage
  run: |
    cd src/qt/build_coverage
    cmake --build . --config Release
    cd ..
    .\collect_coverage.ps1 -BuildDir build_coverage

- name: Upload coverage
  uses: actions/upload-artifact@v3
  with:
    name: coverage-report
    path: src/qt/build_coverage/coverage_report
```

## Notes

- **Separate Compiler**: Coverage builds use clang-cl (LLVM); regular builds use MSVC. They don't interfere.
- **Build Time**: First coverage build is slower due to instrumentation. Link is the main bottleneck.
- **Report Size**: HTML report is typically 2–5 MB depending on code size.
- **Line vs Branch Coverage**: llvm-cov provides both; the HTML report shows both metrics.

## One-command report (recommended)

`src/qt/make_coverage_report.ps1` runs both engines and assembles the committed,
browsable report under `/coverage` (open `coverage/index.html`):

- **full/** — OpenCppCoverage line coverage over every MSVC binary incl. the MFC GUI
- **branch/** — llvm-cov line+branch for the clang-cl-buildable core+Qt

The steps below describe the llvm-cov pass alone (what `collect_coverage.ps1` does).
