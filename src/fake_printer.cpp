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

#include <iomanip>
#include <map>
#include <vector>

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

void FakePrinter::printSummary() {
    spdlog::info("\n=== Fake Print Summary ===");

    // General statistics
    spdlog::info("Total layers processed: {}", totalLayersPrinted);
    spdlog::info("Total errors encountered: {}", totalErrors);
    if (totalLayersPrinted == 0) {
        spdlog::warn("No layers were successfully printed.");
        return;
    }

    // Layer error breakdown
    std::map<std::string, int> errorCounts;
    std::map<std::string, int> materialUsage;
    std::map<int, int> printSpeeds;
    double totalPrintTime = 0.0;
    double minPrintSpeed = 9999, maxPrintSpeed = 0, avgPrintSpeed = 0;
    int minLayerTime = 9999, maxLayerTime = 0;

    for (const auto& layer : layers) {
        if (!layer.layerError.empty() && layer.layerError != "SUCCESS") {
            errorCounts[layer.layerError]++;
        }
        materialUsage[layer.materialType]++;
        printSpeeds[layer.printSpeed]++;

        // Print speed analysis
        if (layer.printSpeed > maxPrintSpeed) maxPrintSpeed = layer.printSpeed;
        if (layer.printSpeed < minPrintSpeed) minPrintSpeed = layer.printSpeed;
        avgPrintSpeed += layer.printSpeed;

        // Time analysis (parsing time from "5min_12sec" format)
        int layerTimeSec = 0;
        try {
            size_t minPos = layer.layerTime.find("min");
            size_t secPos = layer.layerTime.find("sec");
            if (minPos != std::string::npos) {
                layerTimeSec += std::stoi(layer.layerTime.substr(0, minPos)) * 60;
            }
            if (secPos != std::string::npos) {
                layerTimeSec += std::stoi(layer.layerTime.substr(minPos + 4, secPos));
            }
            totalPrintTime += layerTimeSec;
            if (layerTimeSec < minLayerTime) minLayerTime = layerTimeSec;
            if (layerTimeSec > maxLayerTime) maxLayerTime = layerTimeSec;
        } catch (...) {
            spdlog::warn("Error parsing time for layer {}", layer.layerNumber);
        }
    }

    avgPrintSpeed /= totalLayersPrinted;

    // Print material usage statistics
    spdlog::info("\nMaterial Usage:");
    for (const auto& material : materialUsage) {
        spdlog::info("  - {}: {} layers", material.first, material.second);
    }

    // Print speed statistics
    spdlog::info("\nPrint Speed Analysis:");
    spdlog::info("  - Min Speed: {} mm/s", minPrintSpeed);
    spdlog::info("  - Max Speed: {} mm/s", maxPrintSpeed);
    spdlog::info("  - Avg Speed: {:.2f} mm/s", avgPrintSpeed);

    // Print time statistics
    spdlog::info("\nTime Statistics:");
    spdlog::info("  - Total print time: {:.2f} minutes", totalPrintTime / 60.0);
    spdlog::info("  - Min layer time: {} sec", minLayerTime);
    spdlog::info("  - Max layer time: {} sec", maxLayerTime);

    // Print error breakdown
    if (!errorCounts.empty()) {
        spdlog::info("\nError Breakdown:");
        for (const auto& err : errorCounts) {
            spdlog::error("  - {}: {} occurrences", err.first, err.second);
        }
    }

    // Print ASCII Bar Chart for error frequency
    spdlog::info("\nError Distribution:");
    for (const auto& err : errorCounts) {
        std::cout << "  " << std::setw(15) << std::left << err.first << " | ";
        for (int i = 0; i < err.second; ++i) {
            std::cout << "#";
        }
        std::cout << " (" << err.second << ")\n";
    }

    // Print ASCII Bar Chart for print speed distribution
    spdlog::info("\nPrint Speed Distribution:");
    for (const auto& speed : printSpeeds) {
        std::cout << "  " << std::setw(3) << speed.first << " mm/s | ";
        for (int i = 0; i < speed.second; ++i) {
            std::cout << "#";
        }
        std::cout << " (" << speed.second << " layers)\n";
    }

    spdlog::info("\n=== End of Fake Print Summary ===");
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
            layers.push_back(layer);
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
