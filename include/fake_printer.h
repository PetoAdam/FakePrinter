#ifndef FAKE_PRINTER_H
#define FAKE_PRINTER_H

#include "layer.h"
#include "csv_reader.h"
#include "download_service.h"
#include <string>
#include <vector>

class FakePrinter
{
public:
    enum Mode
    {
        SUPERVISED,
        AUTOMATIC
    };

    FakePrinter(const std::string &printName,
                const std::string &destFolder,
                Mode mode);

    // Runs the complete print job.
    void run();

private:
    std::string printName;
    std::string destFolder;
    Mode mode;

    // Statistics.
    int totalLayersPrinted = 0;
    int totalErrors = 0;

    // Validates a layer; returns true if valid (errorMsg contains details on failure).
    bool validateLayer(const Layer &layer, std::string &errorMsg);

    // Processes a layer: writes out layer data and downloads its image.
    bool processLayer(const Layer &layer);

    // Prepares the output directories.
    bool prepareOutputDirectory();

    // Prints a summary of the print job.
    void printSummary();
};

#endif // FAKE_PRINTER_H
