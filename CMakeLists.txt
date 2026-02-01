cmake_minimum_required(VERSION 3.10)
project(Ga1Agent VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific settings
if(WIN32)
    # Force 32-bit build
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(WARNING "Building for 32-bit (x86) platform is recommended for OPC DA compatibility")
    endif()
    
    # Set Windows version (Windows 7 minimum)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWINVER=0x0601)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

# Source files
set(SOURCES
    src/main.cpp
    src/server/opc_server.cpp
    src/device/gabbiani_device.cpp
    src/service/opc_service.cpp
)

# Header files
set(HEADERS
    include/opcda_types.h
    include/opc_server.h
    include/gabbiani_device.h
    include/opc_service.h
)

# Create executable
add_executable(Ga1Agent ${SOURCES} ${HEADERS})

# Link libraries
if(WIN32)
    target_link_libraries(Ga1Agent
        ole32
        oleaut32
        advapi32
    )
endif()

# Set output directory
set_target_properties(Ga1Agent PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS Ga1Agent DESTINATION bin)

# Configuration files
install(FILES config/tags.ini DESTINATION config OPTIONAL)

# Documentation
install(FILES README.md DESTINATION .)
