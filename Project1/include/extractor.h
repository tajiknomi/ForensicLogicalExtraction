#pragma once

#include "adb.h"
#include <string>
#include <map>
#include "json.h"
using json = nlohmann::json;

enum class DataType {
    SMS,
    CALL,
    MEDIA,
    CONTACTS,
    WHATSAPP,
    USER_INSTALLED_APPS
};

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
    std::map<std::string, std::string> parseRow(const std::string& line);
//    void saveToFile(const std::string& data, const std::string& filePath);
    json parseADBOutputToJSON(const std::string& output, const std::string& rowPrefix = "Row:");
    void saveJSONToFile(const json& data, const std::string& outputFile);
    json parseArtifact(const std::string& output, DataType type);
    json parseSMS(const std::string& output);
    json parseCallLogs(const std::string& output);
    json parseMedia(const std::string& output);
    json parseInstalledApps(const std::string& output);
    std::vector<std::map<std::string, std::string>> extractRows(const std::string& output);
    std::string trim(const std::string& str);


private:
    ADB& adb;
    //void saveToFile(const std::string& data, const std::string& filePath);
};