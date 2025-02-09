cmake_minimum_required(VERSION 3.12)
project(FakePrinter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libcurl (make sure libcurl development files are installed)
find_package(CURL REQUIRED)

# Include our header files
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add the executable and list our source files
add_executable(FakePrinter
    src/main.cpp
    src/fake_printer.cpp
    src/download_service.cpp
    # Note: csv_reader.h is header-only, so no separate .cpp is needed.
)

# Link libraries: libcurl and (if necessary) filesystem support.
target_link_libraries(FakePrinter PRIVATE CURL::libcurl stdc++fs)
