#ifndef CSV_READER_H
#define CSV_READER_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class CSVReader {
public:
    CSVReader(const std::string &filename)
        : file(filename)
    {
    }

    // Reads the next CSV row into the provided vector. Returns true if successful.
    bool readNextRow(std::vector<std::string> &row) {
        row.clear();
        std::string line;
        if (!std::getline(file, line)) {
            return false;
        }
        parseLine(line, row);
        return true;
    }

private:
    std::ifstream file;

    void parseLine(const std::string &line, std::vector<std::string> &result) {
        std::stringstream ss(line);
        std::string token;
        // Basic splitting on commas. (Assumes no commas within quoted strings.)
        while (std::getline(ss, token, ',')) {
            result.push_back(token);
        }
    }
};

#endif // CSV_READER_H
