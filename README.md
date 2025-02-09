# FakePrinter

FakePrinter is a command-line tool that simulates the printing of 3D model layers from a CSV dataset. It supports both supervised and automatic modes, robust CSV parsing, file logging using spdlog, image downloading via libcurl (with RAII wrappers), and graceful shutdown handling.

## Features

- **Advanced CSV Parsing:**  
  Supports quoted fields containing commas, newlines, and escaped quotes.

- **Image Downloading:**  
  Downloads images via HTTP using libcurl. A custom RAII wrapper ensures proper resource management.

- **Robust Logging:**  
  Uses [spdlog](https://github.com/gabime/spdlog) to log messages (to both the console and a file).

- **Graceful Shutdown:**  
  Captures SIGINT (Ctrl+C) to allow for a controlled shutdown of a print job.

- **Modular and Extensible:**  
  Clean separation of concerns (CSV parsing, download service, print job processing) enables easy future extensions.

- **CMake Build System:**  
  Uses CMake for cross-platform building and dependency management.

## Installation

### Prerequisites

- **C++17 Compiler** (e.g., g++ 7 or higher)
- **CMake** (version 3.12 or higher)

Install required libraries via APT:

```bash
sudo apt-get update
sudo apt-get install libcurl4-openssl-dev libspdlog-dev
```

### Clone the Repository

```bash
git clone https://github.com/PetoAdam/FakePrinter.git
```

## Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

The FakePrinter executable will be built in the build/ directory

## Usage

Run the executable with the following arguments:

```bash
./FakePrinter --name <print_name> --dest <destination_folder> --mode <supervised|automatic>
```

For example:
```bash
./FakePrinter --name "MyTestPrint" --dest "/home/yourusername/fakeprints" --mode supervised
```

 - Supervised mode:
    - Waits for user input before processing each layer and prompts on errors.
 - Automatic mode:
    - Processes all layers continuously, logging errors without prompting.

## Project Structure

```graphql
FakePrinter/
├── FakePrinter            # The executable
├── CMakeLists.txt         # CMake build configuration.
├── README.md              # Project documentation.
├── include/
│   ├── csv_reader.h       # Advanced CSV parsing.
│   ├── download_service.h # Download service interface.
│   ├── fake_printer.h     # Main controller interface.
│   └── layer.h            # Domain model for print layers.
└── src/
    ├── main.cpp           # Entry point: command-line parsing, logging, and signal handling.
    ├── fake_printer.cpp   # Implements the FakePrinter controller.
    └── download_service.cpp  # Implements the download service with RAII for libcurl.
```

## Acknowledgements

- [spdlog](https://github.com/gabime/spdlog) for robust logging capabilities.
- [libcurl](https://github.com/curl/curl) for reliable file downloading