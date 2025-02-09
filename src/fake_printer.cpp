#include "fake_printer.h"
#include "csv_reader.h"
#include "download_service.h"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <atomic>
#include <csignal>

namespace fs = std::filesystem;

// Declare external shutdown flag.
extern std::atomic<bool> g_shutdownRequested;

// Function to read user input asynchronously
void userInputListener(std::atomic<bool> &inputReceived, std::string &inputBuffer)
{
    std::getline(std::cin, inputBuffer);
    inputReceived = true;
}

FakePrinter::FakePrinter(const std::string &printName,
                         const std::string &destFolder,
                         Mode mode)
    : printName(printName), destFolder(destFolder), mode(mode)
{
}

bool FakePrinter::prepareOutputDirectory()
{
    fs::path outputPath = fs::path(destFolder) / printName;
    try
    {
        if (!fs::exists(outputPath))
            fs::create_directories(outputPath);
    }
    catch (const fs::filesystem_error &e)
    {
        spdlog::error("Error creating output directory: {}", e.what());
        return false;
    }
    return true;
}

bool FakePrinter::validateLayer(const Layer &layer, std::string &errorMsg)
{
    if (layer.layerError != "SUCCESS")
    {
        errorMsg = "Layer error reported: " + layer.layerError;
        return false;
    }
    if (layer.layerNumber <= 0)
    {
        errorMsg = "Layer number must be greater than 0.";
        return false;
    }
    // Additional validations can be added here.
    return true;
}

bool FakePrinter::processLayer(const Layer &layer)
{
    fs::path basePath = fs::path(destFolder) / printName;
    fs::path layerDataPath = basePath / "layers";
    fs::path imagePath = basePath / "images";

    try
    {
        if (!fs::exists(layerDataPath))
            fs::create_directories(layerDataPath);
        if (!fs::exists(imagePath))
            fs::create_directories(imagePath);
    }
    catch (const fs::filesystem_error &e)
    {
        spdlog::error("Error creating output directories: {}", e.what());
        return false;
    }

    // Write layer data as a JSON file.
    char jsonFileName[100];
    std::snprintf(jsonFileName, sizeof(jsonFileName), "layer_%05d.json", layer.layerNumber);
    fs::path jsonFilePath = layerDataPath / jsonFileName;
    std::ofstream ofs(jsonFilePath);
    if (!ofs)
    {
        spdlog::error("Failed to write layer file: {}", jsonFilePath.string());
        return false;
    }
    ofs << layer.toString() << "\n";
    ofs.close();

    // Use the DownloadService to download the image.
    DownloadService downloader;
    fs::path imageFilePath = imagePath / layer.fileName;
    if (!downloader.downloadFile(layer.imageUrl, imageFilePath.string()))
    {
        spdlog::error("Failed to download image for layer {}", layer.layerNumber);
        return false;
    }
    return true;
}

void FakePrinter::printSummary()
{
    spdlog::info("=== Fake Print Summary ===");
    spdlog::info("Total layers printed: {}", totalLayersPrinted);
    spdlog::info("Total errors encountered: {}", totalErrors);
}

void FakePrinter::run()
{
    if (!prepareOutputDirectory())
    {
        spdlog::error("Error preparing output directory. Exiting.");
        return;
    }

    // Ensure the CSV file exists locally; if not, download it.
    const std::string csvFileName = "fake_print_data.csv";
    if (!fs::exists(csvFileName))
    {
        spdlog::info("CSV data file not found locally. Attempting to download...");
        std::string cmd = "curl -L -o " + csvFileName + " https://bit.ly/3AE4mbA";
        if (std::system(cmd.c_str()) != 0)
        {
            spdlog::error("Failed to download CSV data file. Exiting.");
            return;
        }
    }

    CSVReader reader(csvFileName);
    std::vector<std::string> row;
    int rowNumber = 0;
    while (reader.readNextRow(row))
    {
        if (g_shutdownRequested)
        {
            spdlog::info("Shutdown requested. Exiting print job.");
            break;
        }
        rowNumber++;
        // Skip header row.
        if (rowNumber == 1)
            continue;
        if (row.size() < 18)
        {
            spdlog::error("Row {} does not have enough columns. Skipping.", rowNumber);
            totalErrors++;
            continue;
        }

        Layer layer;
        layer.layerError = row[0];
        try
        {
            layer.layerNumber = std::stoi(row[1]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        try
        {
            layer.layerHeight = std::stod(row[2]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        layer.materialType = row[3];
        try
        {
            layer.extrusionTemperature = std::stoi(row[4]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        try
        {
            layer.printSpeed = std::stoi(row[5]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        layer.layerAdhesionQuality = row[6];
        try
        {
            layer.infillDensity = std::stoi(row[7]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        layer.infillPattern = row[8];
        try
        {
            layer.shellThickness = std::stoi(row[9]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        try
        {
            layer.overhangAngle = std::stoi(row[10]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        try
        {
            layer.coolingFanSpeed = std::stoi(row[11]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        layer.retractionSettings = row[12];
        try
        {
            layer.zOffsetAdjustment = std::stod(row[13]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        try
        {
            layer.printBedTemperature = std::stoi(row[14]);
        }
        catch (...)
        {
            totalErrors++;
            continue;
        }
        layer.layerTime = row[15];
        layer.fileName = row[16];
        layer.imageUrl = row[17];

        std::string errorMsg;
        if (!validateLayer(layer, errorMsg))
        {
            totalErrors++;
            if (mode == SUPERVISED)
            {
                spdlog::error("Error in layer {}: {}", layer.layerNumber, errorMsg);
                spdlog::info("Type 'i' to ignore or 'e' to end the FakePrint: ");
                std::atomic<bool> inputReceived(false);
                std::string userInput;
                std::thread inputThread(userInputListener, std::ref(inputReceived), std::ref(userInput));

                // Polling loop to check for shutdown requests
                while (!inputReceived)
                {
                    if (g_shutdownRequested)
                    {
                        spdlog::warn("Shutdown requested. Exiting supervised mode.");
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Avoid busy waiting
                }

                // Clean up input thread if still running
                if (inputThread.joinable())
                {
                    inputThread.detach(); // Prevent blocking
                }

                if (g_shutdownRequested)
                {
                    spdlog::warn("Shutdown requested. Exiting FakePrint.");
                    break;
                }
                if (userInput == "e" || userInput == "E")
                {
                    spdlog::info("Ending FakePrint.");
                    break;
                }
                else
                {
                    spdlog::info("Ignoring error and continuing.");
                }
            }
            else
            {
                spdlog::error("Error in layer {}: {}. Continuing automatically.", layer.layerNumber, errorMsg);
            }
        }

        if (mode == SUPERVISED)
        {
            spdlog::info("Press <return> to print layer {}...", layer.layerNumber);
            std::atomic<bool> inputReceived(false);
            std::string userInput;
            std::thread inputThread(userInputListener, std::ref(inputReceived), std::ref(userInput));

            // Polling loop to check for shutdown requests
            while (!inputReceived)
            {
                if (g_shutdownRequested)
                {
                    spdlog::warn("Shutdown requested. Exiting supervised mode.");
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Avoid busy waiting
            }

            // Clean up input thread if still running
            if (inputThread.joinable())
            {
                inputThread.detach(); // Prevent blocking
            }

            if (g_shutdownRequested)
            {
                spdlog::warn("Shutdown requested. Exiting FakePrint.");
                break;
            }
        }

        if (processLayer(layer))
        {
            totalLayersPrinted++;
            spdlog::info("Layer {} printed successfully.", layer.layerNumber);
        }
        else
        {
            spdlog::error("Failed to process layer {}.", layer.layerNumber);
            totalErrors++;
        }
    }
    printSummary();
}
