#ifndef CSV_READER_H
#define CSV_READER_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// CSV reader that handles quoted fields and embedded newlines.
class CSVReader
{
public:
    CSVReader(const std::string &filename)
        : file(filename)
    {
    }

    // Reads the next CSV record into the provided vector.
    // Returns true if a record was read successfully.
    bool readNextRow(std::vector<std::string> &row)
    {
        row.clear();
        std::string record;
        if (!std::getline(file, record))
        {
            return false;
        }
        // If the record has an unbalanced quote, keep reading.
        while (!isRecordComplete(record))
        {
            std::string nextLine;
            if (!std::getline(file, nextLine))
            {
                break;
            }
            record += "\n" + nextLine;
        }
        parseRecord(record, row);
        return true;
    }

private:
    std::ifstream file;

    // Check if the record has balanced quotes.
    bool isRecordComplete(const std::string &record)
    {
        size_t count = 0;
        for (char c : record)
        {
            if (c == '"')
                count++;
        }
        return count % 2 == 0;
    }

    // Parses a CSV record into fields using a simple state machine.
    void parseRecord(const std::string &record, std::vector<std::string> &fields)
    {
        std::string field;
        bool inQuotes = false;
        for (size_t i = 0; i < record.size(); ++i)
        {
            char c = record[i];
            if (inQuotes)
            {
                if (c == '"')
                {
                    if (i + 1 < record.size() && record[i + 1] == '"')
                    {
                        // Escaped quote
                        field.push_back('"');
                        i++; // Skip the escaped quote.
                    }
                    else
                    {
                        inQuotes = false;
                    }
                }
                else
                {
                    field.push_back(c);
                }
            }
            else
            {
                if (c == '"')
                {
                    inQuotes = true;
                }
                else if (c == ',')
                {
                    fields.push_back(field);
                    field.clear();
                }
                else
                {
                    field.push_back(c);
                }
            }
        }
        fields.push_back(field);
    }
};

#endif // CSV_READER_H
