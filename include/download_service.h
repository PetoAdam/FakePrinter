#ifndef DOWNLOAD_SERVICE_H
#define DOWNLOAD_SERVICE_H

#include <string>

class DownloadService {
public:
    // Downloads the file at 'url' and saves it to 'destinationPath'.
    // Returns true on success, false on failure.
    bool downloadFile(const std::string &url, const std::string &destinationPath);
};

#endif // DOWNLOAD_SERVICE_H
