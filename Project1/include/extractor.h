#pragma once

#include "adb.h"
#include <string>
#include <vector>
#include <filesystem>

class ForensicExtractor {

private:
    std::vector<std::tuple<std::string, std::string>> getFilePathsFromJson(const std::string& jsonFilePath);
    std::filesystem::path createDirForConnectedDevice(const std::wstring& input);

private:
    ADB& adb;

public:
    ForensicExtractor(ADB& adb);
    int numOfConnectedAdbDevices(void);
    void showConnectedDevices(void);
    int extractDeviceInfo(const std::wstring& outputFileName);
    int extractUserInstalledAppsList(const std::wstring& outputFileName);
    int extractSMS(const std::wstring& outputFileName);
    int extractCallLogs(const std::wstring& outputFileName);
    int pullMedia(const std::wstring& dirName);
    int extractMediaStoreDb(const std::wstring& outputFileName);
    
};