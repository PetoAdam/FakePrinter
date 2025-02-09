#include "fake_printer.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <iostream>
#include <string>
#include <atomic>
#include <csignal>
#include <memory>
#include <vector>

// Global shutdown flag.
std::atomic<bool> g_shutdownRequested{false};

void signal_handler(int signal)
{
    if (signal == SIGINT)
    {
        spdlog::info("SIGINT received. Requesting graceful shutdown.");
        g_shutdownRequested = true;
    }
}

void printUsage(const char *progName)
{
    std::cout << "Usage: " << progName
              << " --name <print_name> --dest <destination_folder> --mode <supervised|automatic>\n";
}

int main(int argc, char *argv[])
{
    // Set up signal handling.
    std::signal(SIGINT, signal_handler);

    // Logging initialization
    try
    {
        // Console Sink: Show only important logs (INFO and above)
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info); // Only important messages
        console_sink->set_pattern("%v");

        // File Sink: Capture detailed logs (DEBUG and above)
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("FakePrinter.log", true);
        file_sink->set_level(spdlog::level::debug); // Capture everything
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

        // Combine sinks with different levels
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

        logger->set_level(spdlog::level::debug); // Global level (allows filtering per sink)
        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::warn); // Flush on warnings or worse
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        return 1;
    }

    if (argc != 7)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string printName, destFolder, modeStr;
    for (int i = 1; i < argc; i += 2)
    {
        std::string argKey = argv[i];
        std::string argVal = argv[i + 1];
        if (argKey == "--name")
        {
            printName = argVal;
        }
        else if (argKey == "--dest")
        {
            destFolder = argVal;
        }
        else if (argKey == "--mode")
        {
            modeStr = argVal;
        }
        else
        {
            printUsage(argv[0]);
            return 1;
        }
    }

    FakePrinter::Mode mode;
    if (modeStr == "supervised")
        mode = FakePrinter::SUPERVISED;
    else if (modeStr == "automatic")
        mode = FakePrinter::AUTOMATIC;
    else
    {
        spdlog::error("Invalid mode: {}", modeStr);
        printUsage(argv[0]);
        return 1;
    }

    FakePrinter printer(printName, destFolder, mode);
    printer.run();

    return 0;
}
