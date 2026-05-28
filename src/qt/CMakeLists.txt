cmake_minimum_required(VERSION 3.16)
project(IometerQtMeter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ── Find Qt (prefer Qt6, fall back to Qt5) ──────────────────────────────────
find_package(Qt6 QUIET COMPONENTS Widgets)
if(Qt6_FOUND)
    message(STATUS "Found Qt ${Qt6_VERSION}")
    set(_QT Qt6)
else()
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)
    message(STATUS "Found Qt ${Qt5_VERSION}")
    set(_QT Qt5)
endif()

# ── Target ───────────────────────────────────────────────────────────────────
add_executable(IometerQtMeterDemo
    main.cpp
    MeterWidget.cpp
    BigMeterWidget.cpp
)

target_link_libraries(IometerQtMeterDemo PRIVATE ${_QT}::Widgets)

# On Windows, build as a GUI app (no console window)
if(WIN32)
    set_target_properties(IometerQtMeterDemo PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS IometerQtMeterDemo RUNTIME DESTINATION bin)
