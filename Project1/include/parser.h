#pragma once

#include <map>
#include "json.h"


enum class DataType {
    SMS,
    CALL,
    MEDIA,
    CONTACTS,
    WHATSAPP,
    USER_INSTALLED_APPS
};


class PARSER {

private:
    //    void saveToFile(const std::string& data, const std::string& filePath);
    static std::map<std::string, std::string> parseRow(const std::string& line);
    static nlohmann::json parseADBOutputToJSON(const std::string& output, const std::string& rowPrefix = "Row:");
    static nlohmann::json parseSMS(const std::string& output);
    static nlohmann::json parseCallLogs(const std::string& output);
    static nlohmann::json parseMedia(const std::string& output);
    static nlohmann::json parseInstalledApps(const std::string& output);
    static std::vector<std::map<std::string, std::string>> extractRows(const std::string& output);
    static std::string trim(const std::string& str);

public:
    static nlohmann::json parseAdbDeviceInfo(const std::string& deviceInfoFromAdb);
    static void saveJSONToFile(const nlohmann::json& data, const std::string& outputFile);
    static nlohmann::json parseArtifact(const std::string& output, DataType type);
};