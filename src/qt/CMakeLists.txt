cmake_minimum_required(VERSION 3.16)

# Coverage build option (separate from default build)
# Must be set BEFORE project() so compiler can be selected early
option(IOMETER_COVERAGE "Build with code coverage instrumentation (clang-cl)" OFF)

# ── Coverage: use clang-cl compiler if enabled ──────────────────────────────
if(IOMETER_COVERAGE)
    set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang-cl.exe")
    set(CMAKE_C_COMPILER   "C:/Program Files/LLVM/bin/clang-cl.exe")
    message(STATUS "Coverage mode: using clang-cl compiler")
endif()

project(IometerQt VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Find Qt (prefer Qt6, fall back to Qt5) ──────────────────────────────────
find_package(Qt6 QUIET COMPONENTS Widgets Network Test)
if(Qt6_FOUND)
    message(STATUS "Using Qt ${Qt6_VERSION}")
    set(_QT Qt6)
else()
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Network Test)
    message(STATUS "Using Qt ${Qt5_VERSION}")
    set(_QT Qt5)
endif()

# ── Helper function: apply coverage instrumentation flags ──────────────────────
function(apply_coverage_flags target)
    if(IOMETER_COVERAGE)
        # clang-cl instrumentation for llvm-cov
        target_compile_options(${target} PRIVATE
            -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(${target} PRIVATE
            -fprofile-instr-generate)
    endif()
endfunction()

# ── Engine + widget sources (shared between app and tests via static lib) ────
set(CORE_SOURCES
    DemoEngine.cpp
    DynamoEngine.cpp
    MeterWidget.cpp
    BigMeterWidget.cpp
    BarChartWidget.cpp
    PageSetup.cpp
    PageAccess.cpp
    PageDisplay.cpp
    PageResults.cpp
    PageNetwork.cpp
    MainWindow.cpp
    ../core/ResultsWriter.cpp
    ../core/DynamoConnection.cpp
    ../core/IcfFile.cpp
    ../core/WorkerPool.cpp
    ../core/TestRunner.cpp
)
set(CORE_HEADERS
    IometerTypes.h
    IometerEngine.h
    DemoEngine.h
    DynamoEngine.h
    DyProto.h
    MeterWidget.h
    BigMeterWidget.h
    BarChartWidget.h
    PageSetup.h
    PageAccess.h
    PageDisplay.h
    PageResults.h
    PageNetwork.h
    MainWindow.h
    ../core/ResultsWriter.h
    ../core/DynamoConnection.h
    ../core/IcfFile.h
    ../core/WorkerPool.h
    ../core/TestRunner.h
)

# ── Static library shared by app + all test targets ──────────────────────────
add_library(IometerQtCore STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(IometerQtCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../core)
target_link_libraries(IometerQtCore PUBLIC ${_QT}::Widgets ${_QT}::Network)

# Apply coverage instrumentation to the library (so test coverage includes it)
apply_coverage_flags(IometerQtCore)

# ── Main application ─────────────────────────────────────────────────────────
add_executable(IometerQt main.cpp resources.qrc)
target_link_libraries(IometerQt PRIVATE IometerQtCore)
if(WIN32)
    set_target_properties(IometerQt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ── Unit tests — one executable per test class (standard Qt Test pattern) ────
enable_testing()

# Paths to fixture files baked in at compile time
set(FIXTURE_DIR  "${CMAKE_SOURCE_DIR}/../../test/fixtures")
set(SMOKE_ICF    "${CMAKE_SOURCE_DIR}/../../test/smoke_test.icf")

# Helper macro: creates one test executable and registers it with CTest
macro(iometer_test name)
    add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name} PRIVATE IometerQtCore ${_QT}::Test)
    target_compile_definitions(${name} PRIVATE
        IOMETER_TESTING
        IOMETER_FIXTURE_DIR="${FIXTURE_DIR}"
        IOMETER_SMOKE_ICF="${SMOKE_ICF}"
    )
    if(WIN32)
        set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE FALSE)
    endif()

    # Apply coverage instrumentation if enabled
    apply_coverage_flags(${name})

    add_test(NAME ${name} COMMAND ${name} -v2)
endmacro()

iometer_test(tst_types)
iometer_test(tst_accessspecs)
iometer_test(tst_icf)
iometer_test(tst_results)
iometer_test(tst_demo)
iometer_test(tst_protocol)

# ── UI Tests (Qt widget tests) ──────────────────────────────────────────────
iometer_test(tst_meters)
iometer_test(tst_pagesetup)
iometer_test(tst_mainwindow)

# ── Coverage: set up test environment for profdata collection ──────────────────
if(IOMETER_COVERAGE)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/coverage")
    # Set environment variable for all tests so profraw goes to the right place
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CTestCoverageEnv.cmake")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CTestCoverageEnv.cmake"
        "set(ENV{LLVM_PROFILE_FILE} \"${CMAKE_BINARY_DIR}/coverage/%p.profraw\")\n")
    message(STATUS "Coverage profraw output: ${CMAKE_BINARY_DIR}/coverage")
endif()

# ── Qt Runtime Deployment (copy DLLs and plugins post-build) ──────────────────
if(WIN32)
    # Find Qt bin and plugins directories
    get_target_property(QT_QMAKE_EXECUTABLE ${_QT}::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    get_filename_component(QT_ROOT_DIR "${QT_BIN_DIR}" DIRECTORY)
    set(QT_PLUGINS_DIR "${QT_ROOT_DIR}/plugins")

    # Copy essential Qt DLLs post-build
    add_custom_command(TARGET IometerQt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt runtime libraries..."
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/DeployQt.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Copying Qt DLLs and plugins for Release deployment"
    )
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS IometerQt RUNTIME DESTINATION bin)
