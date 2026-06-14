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

# ── Wire-protocol portability config ────────────────────────────────────────
# Manager-core / Qt code consumes the PROJECT-WIDE headers (IOCommon.h and the
# canonical wire structs in IOAccess.h / IOTest.h). IOCommon.h is Iometer's
# cross-platform portability layer, so we feed it the same IOMTR_* selectors the
# rest of the project uses - but as a NON-MFC build (IOMTR_NO_MFC), so IOCommon
# pulls <windows.h> instead of StdAfx.h/afxwin.h. FORCE_STRUCT_ALIGN matches the
# 1-byte wire packing every Dynamo/Iometer .vcxproj sets, keeping the structs
# byte-identical on the wire. Dynamo and the MFC GUI are untouched - they set
# these in their own projects and never define IOMTR_NO_MFC.
if(WIN32)
    add_compile_definitions(
        IOMTR_NO_MFC
        IOMTR_OSFAMILY_WINDOWS
        FORCE_STRUCT_ALIGN
    )
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_compile_definitions(IOMTR_OS_WIN64 IOMTR_CPU_X86_64)
    else()
        add_compile_definitions(IOMTR_OS_WIN32 IOMTR_CPU_I386)
    endif()
endif()

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

# ── IometerCore: platform-neutral manager logic (pure C++, NO Qt) ────────────
# The shared "core of the manager" that BOTH the Qt and MFC GUIs wrap. It must
# not reference Qt or MFC; the MFC project links the same ../core/*.cpp files.
set(IOMETER_CORE_SOURCES
    ../core/ResultsWriter.cpp
    ../core/DynamoConnection.cpp
    ../core/IcfFile.cpp
    ../core/WorkerPool.cpp
    ../core/TestRunner.cpp
    ../core/NetworkSetup.cpp
    ../core/ManagerMap.cpp
    ../core/AccessSpecCatalog.cpp
    ../core/AccessSpecLibrary.cpp
    ../core/ResultsAggregator.cpp
    ../core/IcfStream.cpp
    ../core/IcfDocument.cpp
    ../core/IcfWriter.cpp
)
set(IOMETER_CORE_HEADERS
    ../core/IometerTypes.h
    ../core/ResultsWriter.h
    ../core/DynamoConnection.h
    ../core/IcfFile.h
    ../core/WorkerPool.h
    ../core/TestRunner.h
    ../core/NetworkSetup.h
    ../core/ManagerMap.h
    ../core/AccessSpecCatalog.h
    ../core/AccessSpecLibrary.h
    ../core/ResultsAggregator.h
    ../core/IcfStream.h
    ../core/IcfDocument.h
    ../core/IcfWriter.h
)
add_library(IometerCore STATIC ${IOMETER_CORE_SOURCES} ${IOMETER_CORE_HEADERS})
# ../core for the manager core; .. (src root) for the project-wide protocol
# headers (IOCommon.h / IOAccess.h / IOTest.h) the core consumes.
target_include_directories(IometerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
# Deliberately NO Qt link: IometerCore is pure C++ so MFC can link it too.
set_target_properties(IometerCore PROPERTIES AUTOMOC OFF)
apply_coverage_flags(IometerCore)

# ── IometerQtGui: the Qt GUI layer that wraps IometerCore ─────────────────────
set(QTGUI_SOURCES
    QtDemoEngine.cpp
    QtDynamoEngine.cpp
    QtMeterWidget.cpp
    QtBigMeterWidget.cpp
    QtBarChartWidget.cpp
    QtPageSetup.cpp
    QtPageAccess.cpp
    QtPageDisplay.cpp
    QtPageResults.cpp
    QtPageNetwork.cpp
    QtMainWindow.cpp
)
set(QTGUI_HEADERS
    QtIometerTypes.h
    QtIometerEngine.h
    QtDemoEngine.h
    QtDynamoEngine.h
    QtDyProto.h
    QtMeterWidget.h
    QtBigMeterWidget.h
    QtBarChartWidget.h
    QtPageSetup.h
    QtPageAccess.h
    QtPageDisplay.h
    QtPageResults.h
    QtPageNetwork.h
    QtMainWindow.h
)
add_library(IometerQtGui STATIC ${QTGUI_SOURCES} ${QTGUI_HEADERS})
target_include_directories(IometerQtGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../core)
target_link_libraries(IometerQtGui PUBLIC IometerCore ${_QT}::Widgets ${_QT}::Network)
apply_coverage_flags(IometerQtGui)

# ── Main application ─────────────────────────────────────────────────────────
add_executable(IometerQt main.cpp resources.qrc)
target_link_libraries(IometerQt PRIVATE IometerQtGui)
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
    target_link_libraries(${name} PRIVATE IometerQtGui ${_QT}::Test)
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

# ── Core library unit tests (pure-C++ APIs, no GUI types) ───────────────────
iometer_test(tst_workerpool)
iometer_test(tst_testrunner)
iometer_test(tst_icffile)
iometer_test(tst_resultswriter)
iometer_test(tst_dynamoconn)
 iometer_test(tst_networksetup)
iometer_test(tst_managermap)
iometer_test(tst_managerrestore)
iometer_test(tst_accessspeccatalog)
iometer_test(tst_accessspeclibrary)
iometer_test(tst_resultsaggregator)
iometer_test(tst_icfstream)
iometer_test(tst_icfdocument)
iometer_test(tst_icfwriter)
iometer_test(tst_icf_fixtures)
# Proves core/Qt can include the project-wide canonical wire structs MFC-free
iometer_test(tst_wirestructs)

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
