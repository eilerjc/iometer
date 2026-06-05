# DeployQt.cmake - Deploy Qt DLLs and plugins to build output directory
# This script is called as a post-build step to ensure Qt runtime files are available

# Get Qt installation path from environment or qmake
if(DEFINED ENV{QT_ROOT})
    set(QT_ROOT "$ENV{QT_ROOT}")
else()
    # Try to find qmake in PATH
    find_program(QMAKE_EXECUTABLE qmake)
    if(QMAKE_EXECUTABLE)
        get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
        get_filename_component(QT_ROOT "${QT_BIN_DIR}" DIRECTORY)
    else()
        # Fallback to common installation path
        set(QT_ROOT "C:/Qt/6.8.3/msvc2022_64")
    endif()
endif()

set(QT_BIN_DIR "${QT_ROOT}/bin")
set(QT_PLUGINS_DIR "${QT_ROOT}/plugins")

# Detect build configuration from the binary path
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Release/IometerQt.exe")
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Release")
    set(CONFIG "Release")
elseif(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Debug/IometerQt.exe")
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Debug")
    set(CONFIG "Debug")
else()
    message(WARNING "Could not find IometerQt.exe in build directory")
    return()
endif()

message(STATUS "Deploying Qt for ${CONFIG} to ${OUTPUT_DIR}")

# Copy Qt DLLs
if(EXISTS "${QT_BIN_DIR}")
    message(STATUS "Copying Qt DLLs from ${QT_BIN_DIR}")
    file(GLOB QT_DLLS "${QT_BIN_DIR}/Qt6*.dll")
    foreach(DLL ${QT_DLLS})
        file(COPY "${DLL}" DESTINATION "${OUTPUT_DIR}")
    endforeach()
else()
    message(WARNING "Qt bin directory not found: ${QT_BIN_DIR}")
endif()

# Copy plugins directory
if(EXISTS "${QT_PLUGINS_DIR}")
    message(STATUS "Copying Qt plugins from ${QT_PLUGINS_DIR}")
    file(COPY "${QT_PLUGINS_DIR}/" DESTINATION "${OUTPUT_DIR}/plugins")
else()
    message(WARNING "Qt plugins directory not found: ${QT_PLUGINS_DIR}")
endif()

message(STATUS "Qt deployment complete")
