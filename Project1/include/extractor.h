#pragma once

#include "adb.h"
#include <string>
#include <map>
#include "json.h"
using json = nlohmann::json;

class ForensicExtractor {

public:
    ForensicExtractor(ADB& adb);

    void listDevices();
    void extractSMS(const std::string& outputFile);
    void extractCallLogs(const std::string& outputFile);
    void pullMedia(const std::string& localPath);
    void extractMediaStoreDb(const std::string& outputFile);
    
private:
    std::map<std::string, std::string> parseRow(const std::string& line);
    void saveToFile(const std::string& data, const std::string& filePath);
    json parseADBOutputToJSON(const std::string& output, const std::string& rowPrefix = "Row:");
    void saveJSONToFile(const json& data, const std::string& outputFile);

private:
    ADB& adb;
    //void saveToFile(const std::string& data, const std::string& filePath);
};