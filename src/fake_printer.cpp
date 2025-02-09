#include "fake_printer.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>

namespace fs = std::filesystem;

FakePrinter::FakePrinter(const std::string &printName,
                         const std::string &destFolder,
                         Mode mode)
    : printName(printName), destFolder(destFolder), mode(mode)
{
}

bool FakePrinter::prepareOutputDirectory() {
    fs::path outputPath = fs::path(destFolder) / printName;
    try {
        if (!fs::exists(outputPath))
            fs::create_directories(outputPath);
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Error creating output directory: " << e.what() << "\n";
        return false;
    }
    return true;
}

bool FakePrinter::validateLayer(const Layer &layer, std::string &errorMsg) {
    if (layer.layerError != "SUCCESS") {
        errorMsg = "Layer error reported: " + layer.layerError;
        return false;
    }
    if (layer.layerNumber <= 0) {
        errorMsg = "Layer number must be greater than 0.";
        return false;
    }
    // Additional validations can be added here.
    return true;
}

bool FakePrinter::processLayer(const Layer &layer) {
    fs::path basePath = fs::path(destFolder) / printName;
    fs::path layerDataPath = basePath / "layers";
    fs::path imagePath = basePath / "images";

    try {
        if (!fs::exists(layerDataPath))
            fs::create_directories(layerDataPath);
        if (!fs::exists(imagePath))
            fs::create_directories(imagePath);
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Error creating output directories: " << e.what() << "\n";
        return false;
    }

    // Write layer data as JSON.
    char jsonFileName[100];
    std::snprintf(jsonFileName, sizeof(jsonFileName), "layer_%05d.json", layer.layerNumber);
    fs::path jsonFilePath = layerDataPath / jsonFileName;
    std::ofstream ofs(jsonFilePath);
    if (!ofs) {
        std::cerr << "Failed to write layer file: " << jsonFilePath << "\n";
        return false;
    }
    ofs << layer.toString() << "\n";
    ofs.close();

    // Use DownloadService to download the image.
    DownloadService downloader;
    fs::path imageFilePath = imagePath / layer.fileName;
    if (!downloader.downloadFile(layer.imageUrl, imageFilePath.string())) {
        std::cerr << "Failed to download image for layer " << layer.layerNumber << "\n";
        return false;
    }
    return true;
}

void FakePrinter::printSummary() {
    std::cout << "\n=== Fake Print Summary ===\n";
    std::cout << "Total layers printed: " << totalLayersPrinted << "\n";
    std::cout << "Total errors encountered: " << totalErrors << "\n";
}

void FakePrinter::run() {
    if (!prepareOutputDirectory()) {
        std::cerr << "Error preparing output directory. Exiting.\n";
        return;
    }

    // Check if the CSV file exists locally; if not, download it.
    const std::string csvFileName = "fake_print_data.csv";
    if (!fs::exists(csvFileName)) {
        std::cout << "CSV data file not found locally. Attempting to download...\n";
        // You can use the DownloadService here too, or a simple system call.
        std::string cmd = "curl -L -o " + csvFileName + " https://bit.ly/3AE4mbA";
        if (std::system(cmd.c_str()) != 0) {
            std::cerr << "Failed to download CSV data file. Exiting.\n";
            return;
        }
    }

    CSVReader reader(csvFileName);
    std::vector<std::string> row;
    int rowNumber = 0;
    while (reader.readNextRow(row)) {
        rowNumber++;
        // Skip header row.
        if (rowNumber == 1) continue;
        if (row.size() < 18) {
            std::cerr << "Row " << rowNumber << " does not have enough columns. Skipping.\n";
            totalErrors++;
            continue;
        }

        Layer layer;
        layer.layerError = row[0];
        try { layer.layerNumber = std::stoi(row[1]); } catch (...) { totalErrors++; continue; }
        try { layer.layerHeight = std::stod(row[2]); } catch (...) { totalErrors++; continue; }
        layer.materialType = row[3];
        try { layer.extrusionTemperature = std::stoi(row[4]); } catch (...) { totalErrors++; continue; }
        try { layer.printSpeed = std::stoi(row[5]); } catch (...) { totalErrors++; continue; }
        layer.layerAdhesionQuality = row[6];
        try { layer.infillDensity = std::stoi(row[7]); } catch (...) { totalErrors++; continue; }
        layer.infillPattern = row[8];
        try { layer.shellThickness = std::stoi(row[9]); } catch (...) { totalErrors++; continue; }
        try { layer.overhangAngle = std::stoi(row[10]); } catch (...) { totalErrors++; continue; }
        try { layer.coolingFanSpeed = std::stoi(row[11]); } catch (...) { totalErrors++; continue; }
        layer.retractionSettings = row[12];
        try { layer.zOffsetAdjustment = std::stod(row[13]); } catch (...) { totalErrors++; continue; }
        try { layer.printBedTemperature = std::stoi(row[14]); } catch (...) { totalErrors++; continue; }
        layer.layerTime = row[15];
        layer.fileName = row[16];
        layer.imageUrl = row[17];

        std::string errorMsg;
        if (!validateLayer(layer, errorMsg)) {
            totalErrors++;
            if (mode == SUPERVISED) {
                std::cout << "Error in layer " << layer.layerNumber << ": " << errorMsg << "\n";
                std::cout << "Type 'i' to ignore or 'e' to end the FakePrint: ";
                std::string userInput;
                std::getline(std::cin, userInput);
                if (userInput == "e" || userInput == "E") {
                    std::cout << "Ending FakePrint.\n";
                    break;
                } else {
                    std::cout << "Ignoring error and continuing.\n";
                }
            } else {
                std::cerr << "Error in layer " << layer.layerNumber << ": " << errorMsg << ". Continuing automatically.\n";
            }
        }

        if (mode == SUPERVISED) {
            std::cout << "Press [return] to print layer " << layer.layerNumber << "...";
            std::string dummy;
            std::getline(std::cin, dummy);
        }

        if (processLayer(layer)) {
            totalLayersPrinted++;
            std::cout << "Layer " << layer.layerNumber << " printed successfully.\n";
        } else {
            std::cerr << "Failed to process layer " << layer.layerNumber << ".\n";
            totalErrors++;
        }
    }
    printSummary();
}
