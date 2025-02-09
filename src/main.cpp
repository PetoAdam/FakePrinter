#include "fake_printer.h"
#include <iostream>
#include <string>

void printUsage(const char *progName) {
    std::cout << "Usage: " << progName 
              << " --name <print_name> --dest <destination_folder> --mode <supervised|automatic>\n";
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printUsage(argv[0]);
        return 1;
    }

    std::string printName, destFolder, modeStr;
    for (int i = 1; i < argc; i += 2) {
        std::string argKey = argv[i];
        std::string argVal = argv[i + 1];
        if (argKey == "--name") {
            printName = argVal;
        } else if (argKey == "--dest") {
            destFolder = argVal;
        } else if (argKey == "--mode") {
            modeStr = argVal;
        } else {
            printUsage(argv[0]);
            return 1;
        }
    }

    FakePrinter::Mode mode;
    if (modeStr == "supervised")
        mode = FakePrinter::SUPERVISED;
    else if (modeStr == "automatic")
        mode = FakePrinter::AUTOMATIC;
    else {
        std::cerr << "Invalid mode: " << modeStr << "\n";
        printUsage(argv[0]);
        return 1;
    }

    FakePrinter printer(printName, destFolder, mode);
    printer.run();

    return 0;
}
