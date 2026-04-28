#pragma once

#include "adb.h"
#include <string>

class ForensicExtractor {

public:
    ForensicExtractor(ADB& adb);

    void listDevices();
    void extractSMS(const std::string& outputFile);
    void extractCallLogs(const std::string& outputFile);
    void pullMedia(const std::string& localPath);
    void extractMediaStoreDb(const std::string& outputFile);
    void extractDeviceInfo(const std::string& outputFile);
    void extractUserInstalledAppsList(const std::string& outputFile);

private:
    ADB& adb;
};