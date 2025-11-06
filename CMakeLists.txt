cmake_minimum_required(VERSION 3.20)
project(MD1001LB_Microwave_Controller)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include headers in current directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Create a shared library (DLL on Windows, .so on Linux)
add_library(${PROJECT_NAME} SHARED
        arduino_link.cpp

)

# 1. Tell this target where to find its own headers (the arduino_link.h)
#    and the Asio headers.
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/asio/include  # Path to Asio
)
# Define export macro for Windows
target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDING_DLL)

# 3. Link the required socket libraries for Asio
#    On Windows, this is required.
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 wsock32)
endif()

