#pragma once

#include "adb.h"
#include <string>
#include <vector>

class ForensicExtractor {

public:
    ForensicExtractor(ADB& adb);

    int listDevices();
    void extractSMS(const std::string& outputFile);
    void extractCallLogs(const std::string& outputFile);
    void pullMedia(const std::string& localPath);
    void extractMediaStoreDb(const std::string& outputFile);
    void extractDeviceInfo(const std::string& outputFile);
    void extractUserInstalledAppsList(const std::string& outputFile);

private:
    std::vector<std::tuple<std::string, std::string>> getFilePathsFromJson(const std::string& jsonFilePath);
private:
    ADB& adb;
};