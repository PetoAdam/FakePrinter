#include "download_service.h"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include "spdlog/spdlog.h"

// RAII wrapper for CURL*.
class CurlHandle
{
public:
    CurlHandle() : curl(curl_easy_init()) {}
    ~CurlHandle()
    {
        if (curl)
        {
            curl_easy_cleanup(curl);
        }
    }
    CURL *get() { return curl; }

private:
    CURL *curl;
};

static size_t writeData(void *ptr, size_t size, size_t nmemb, void *stream)
{
    std::ofstream *ofs = static_cast<std::ofstream *>(stream);
    size_t count = size * nmemb;
    ofs->write(static_cast<char *>(ptr), count);
    return count;
}

// Helper function to trim whitespace from a string.
#include <algorithm>
#include <cctype>
#include <locale>
static inline std::string trim(const std::string &s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start))
        start++;
    auto end = s.end();
    do
    {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

bool DownloadService::downloadFile(const std::string &url, const std::string &destinationPath)
{
    std::string trimmedUrl = trim(url);
    spdlog::info("Downloading from URL: {}", trimmedUrl);

    CurlHandle curlHandle;
    CURL *curl = curlHandle.get();
    if (!curl)
    {
        spdlog::error("Failed to initialize libcurl.");
        return false;
    }

    std::ofstream ofs(destinationPath, std::ios::binary);
    if (!ofs.is_open())
    {
        spdlog::error("Failed to open file: {}", destinationPath);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, trimmedUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // Set a timeout (in seconds) to avoid hanging.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        spdlog::error("Download error: {}", curl_easy_strerror(res));
        ofs.close();
        return false;
    }

    ofs.close();
    return true;
}
