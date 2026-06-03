@echo off
REM build.cmd - Windows build helper script
REM Usage: build.cmd <target>
REM
REM Targets:
REM   test              - Run unit tests
REM   coverage-report   - Generate interactive HTML coverage report
REM   coverage          - Generate code coverage with LLVM (requires tools)
REM   clean             - Clean build artifacts
REM   help              - Show this help message

setlocal enabledelayedexpansion

if "%1"=="" (
    call :show_help
    exit /b 0
)

if "%1"=="help" (
    call :show_help
    exit /b 0
)

if "%1"=="test" (
    call :run_tests
    exit /b !errorlevel!
)

if "%1"=="coverage-report" (
    call :gen_coverage_report
    exit /b !errorlevel!
)

if "%1"=="coverage" (
    call :gen_coverage
    exit /b !errorlevel!
)

if "%1"=="clean" (
    call :clean_build
    exit /b !errorlevel!
)

echo Unknown target: %1
call :show_help
exit /b 1

:show_help
echo.
echo Iometer Build Helper
echo ====================
echo.
echo Usage: build.cmd COMMAND
echo.
echo Commands:
echo   test              - Run core unit tests
echo   coverage-report   - Generate interactive HTML coverage report
echo   coverage          - Full coverage report with line mapping (requires cmake, ninja, LLVM)
echo   clean             - Clean build artifacts
echo   help              - Show this message
echo.
exit /b 0

:run_tests
echo Running unit tests...
cd /d "%~dp0src\qt\build\Release"
setlocal enabledelayedexpansion

set passed=0
set failed=0

for %%t in (tst_types tst_accessspecs tst_icf tst_results tst_demo tst_protocol) do (
    echo.
    echo Running %%t...
    %%t.exe -v2 >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [OK] %%t
        set /a passed=!passed!+1
    ) else (
        echo   [FAIL] %%t (exit code !errorlevel!)
        set /a failed=!failed!+1
    )
)

echo.
echo ============================================
echo Tests Passed: !passed!
echo Tests Failed: !failed!
echo ============================================
echo.
endlocal
exit /b 0

:gen_coverage_report
echo Generating interactive coverage report...
cd /d "%~dp0src\qt"

powershell -NoProfile -ExecutionPolicy Bypass -Command "& { & '.\generate_coverage_report.ps1' -OutputDir '../../coverage_report' }"

if !errorlevel! equ 0 (
    echo.
    echo Coverage report generated: coverage_report\index.html
    echo.
    powershell -Command "& { Start-Process '../../coverage_report/index.html' }"
) else (
    echo Coverage report generation failed
)

endlocal
exit /b !errorlevel!

:gen_coverage
echo Building with code coverage instrumentation...
echo This requires: cmake, ninja, and LLVM (clang-cl + llvm-cov)
echo.
cd /d "%~dp0src\qt"

powershell -NoProfile -ExecutionPolicy Bypass -Command "& { & '../../collect_coverage.ps1' }"

endlocal
exit /b !errorlevel!

:clean_build
echo Cleaning build artifacts...
cd /d "%~dp0"

if exist "src\qt\build" (
    echo Removing src\qt\build...
    rmdir /s /q "src\qt\build" >nul 2>&1
)

if exist "src\qt\build_coverage" (
    echo Removing src\qt\build_coverage...
    rmdir /s /q "src\qt\build_coverage" >nul 2>&1
)

if exist "coverage_report" (
    echo Removing coverage_report...
    rmdir /s /q "coverage_report" >nul 2>&1
)

echo Clean complete.
endlocal
exit /b 0
