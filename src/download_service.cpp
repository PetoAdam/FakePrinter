#include "download_service.h"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <locale>

// Helper function to trim whitespace from both ends of a string.
static inline std::string trim(const std::string &s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

// Callback to write downloaded data to a std::ofstream.
size_t writeData(void *ptr, size_t size, size_t nmemb, void *stream) {
    std::ofstream *ofs = static_cast<std::ofstream*>(stream);
    size_t count = size * nmemb;
    ofs->write(static_cast<char*>(ptr), count);
    return count;
}

bool DownloadService::downloadFile(const std::string &url, const std::string &destinationPath) {
    // Trim the URL to remove any unwanted whitespace/newlines.
    std::string trimmedUrl = trim(url);
    std::cout << "Downloading from URL: " << trimmedUrl << std::endl;

    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize libcurl.\n";
        return false;
    }

    std::ofstream ofs(destinationPath, std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open file: " << destinationPath << "\n";
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, trimmedUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Download error: " << curl_easy_strerror(res) << "\n";
        ofs.close();
        curl_easy_cleanup(curl);
        return false;
    }

    ofs.close();
    curl_easy_cleanup(curl);
    return true;
}

