cmake_minimum_required(VERSION 3.12)
project(FakePrinter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message("Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message("Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

# Find required packages.
find_package(CURL REQUIRED)
find_package(spdlog REQUIRED)

# Include our header files.
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add the executable.
add_executable(FakePrinter
    src/main.cpp
    src/fake_printer.cpp
    src/download_service.cpp
    # csv_reader.h is header-only.
)

# Link external libraries.
target_link_libraries(FakePrinter PRIVATE CURL::libcurl spdlog::spdlog spdlog::spdlog_header_only stdc++fs)
