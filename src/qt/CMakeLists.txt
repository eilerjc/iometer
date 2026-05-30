cmake_minimum_required(VERSION 3.16)
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
)

# ── Static library shared by app + all test targets ──────────────────────────
add_library(IometerQtCore STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(IometerQtCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(IometerQtCore PUBLIC ${_QT}::Widgets ${_QT}::Network)

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
    add_test(NAME ${name} COMMAND ${name} -v2)
endmacro()

iometer_test(tst_types)
iometer_test(tst_accessspecs)
iometer_test(tst_icf)
iometer_test(tst_results)
iometer_test(tst_demo)
iometer_test(tst_protocol)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS IometerQt RUNTIME DESTINATION bin)
