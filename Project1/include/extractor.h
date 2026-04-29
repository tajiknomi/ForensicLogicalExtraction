#pragma once

#include "adb.h"
#include <string>
#include <vector>
#include <filesystem>

class ForensicExtractor {

public:
    ForensicExtractor(ADB& adb);

    int listDevices();
    int extractSMS(const std::string& outputFileName);
    int extractCallLogs(const std::string& outputFileName);
    int pullMedia(const std::string& dirName);
    int extractMediaStoreDb(const std::string& outputFileName);
    int extractDeviceInfo(const std::string& outputFileName);
    int extractUserInstalledAppsList(const std::string& outputFileName);

private:
    std::vector<std::tuple<std::string, std::string>> getFilePathsFromJson(const std::string& jsonFilePath);
    std::filesystem::path createDirForConnectedDevice(const std::string& input);

private:
    ADB& adb;
};