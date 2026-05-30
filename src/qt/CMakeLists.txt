cmake_minimum_required(VERSION 3.16)
project(IometerQt VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ── Find Qt (prefer Qt6, fall back to Qt5) ──────────────────────────────────
find_package(Qt6 QUIET COMPONENTS Widgets Network)
if(Qt6_FOUND)
    message(STATUS "Using Qt ${Qt6_VERSION}")
    set(_QT Qt6)
else()
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Network)
    message(STATUS "Using Qt ${Qt5_VERSION}")
    set(_QT Qt5)
endif()

# ── All sources ───────────────────────────────────────────────────────────────
set(SOURCES
    main.cpp

    # Engine layer
    DemoEngine.cpp
    DynamoEngine.cpp

    # Widgets
    MeterWidget.cpp
    BigMeterWidget.cpp
    BarChartWidget.cpp

    # Tab pages
    PageSetup.cpp
    PageAccess.cpp
    PageDisplay.cpp
    PageResults.cpp
    PageNetwork.cpp

    # Main window
    MainWindow.cpp
)

set(HEADERS
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

# ── Qt resources (toolbar bitmap etc.) ───────────────────────────────────────
set(CMAKE_AUTORCC ON)
set(RESOURCES resources.qrc)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(IometerQt ${SOURCES} ${HEADERS} ${RESOURCES})
target_link_libraries(IometerQt PRIVATE ${_QT}::Widgets ${_QT}::Network)

# On Windows: GUI subsystem (no console window)
if(WIN32)
    set_target_properties(IometerQt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS IometerQt RUNTIME DESTINATION bin)
