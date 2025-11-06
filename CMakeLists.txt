cmake_minimum_required(VERSION 3.20)
project(MD1001LB_Microwave_Controller CXX)

# Set C++ Standard to 17 (good for <thread> and <chrono>)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the built-in Threads package (needed for std::thread)
find_package(Threads REQUIRED)

# --- 1. Build the Library (DLL) ---
add_library(${PROJECT_NAME} SHARED
        arduino_link.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/asio/include
)
# This macro matches your .h and .cpp files
target_compile_definitions(${PROJECT_NAME} PRIVATE
        BUILDING_DLL
)

# --- THIS IS THE FIX ---
# Link the library to its dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
        Threads::Threads # <-- Fixes the link error
)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32)
endif()
# --- THIS IS THE OTHER FIX ---
# Links C++ libraries statically to prevent the runtime .dll error
target_link_options(${PROJECT_NAME} PRIVATE -static)


# --- 2. Build the Tester (Executable) ---
# (Assumes main_tester.cpp is in your root folder)
add_executable(main_tester
        main_tester.cpp
)
# Link the tester *against* the library
target_link_libraries(main_tester PRIVATE
        ${PROJECT_NAME}
)
# Also link the tester statically
target_link_options(main_tester PRIVATE -static)