#include "extractor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

ForensicExtractor::ForensicExtractor(ADB& adbRef) : adb(adbRef) {}



//void ForensicExtractor::saveToFile(const std::string& data, const std::string& filePath) {
//    std::ofstream file(filePath, std::ios::out);
//
//    if (!file.is_open()) {
//        std::cerr << "[-] Failed to open file: " << filePath << std::endl;
//        return;
//    }
//
//    file << data;
//    file.close();
//}

std::string ForensicExtractor::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r,");

    if (start == std::string::npos) return "";
    return str.substr(start, end - start + 1);
}

std::map<std::string, std::string> ForensicExtractor::parseRow(const std::string& line) {
    std::map<std::string, std::string> row;

    // Find start after "Row: X "
    size_t startPos = line.find("Row:");
    if (startPos == std::string::npos) return row;

    // Move past "Row: <num> "
    startPos = line.find(' ', startPos + 4);
    if (startPos == std::string::npos) return row;

    std::string content = line.substr(startPos + 1);

    std::vector<size_t> keyPositions;

    // Find all positions where a key starts (pattern: word=)
    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '=') {
            // walk backwards to find key start
            size_t j = i;
            while (j > 0 && std::isalnum(content[j - 1]) || content[j - 1] == '_') {
                --j;
            }
            keyPositions.push_back(j);
        }
    }

    // Extract key-value pairs
    for (size_t i = 0; i < keyPositions.size(); ++i) {
        size_t keyStart = keyPositions[i];
        size_t eqPos = content.find('=', keyStart);

        if (eqPos == std::string::npos) continue;

        std::string key = content.substr(keyStart, eqPos - keyStart);

        size_t valueStart = eqPos + 1;
        size_t valueEnd;

        if (i + 1 < keyPositions.size()) {
            valueEnd = keyPositions[i + 1];
        }
        else {
            valueEnd = content.size();
        }

        std::string value = content.substr(valueStart, valueEnd - valueStart);

        row[trim(key)] = trim(value);
    }

    return row;
}

json ForensicExtractor::parseADBOutputToJSON(const std::string& output, const std::string& rowPrefix) {
    std::istringstream stream(output);
    std::string line;

    json jsonArray = json::array();

    while (std::getline(stream, line)) {
        if (line.find(rowPrefix) != std::string::npos) {
            auto parsed = parseRow(line);

            if (!parsed.empty()) {
                json obj;

                for (const auto& kv : parsed) {
                    obj[kv.first] = kv.second;
                }

                jsonArray.push_back(obj);
            }
        }
    }

    return jsonArray;
}

void ForensicExtractor::saveJSONToFile(const json& data, const std::string& outputFile) {
    std::ofstream file(outputFile);
    if (!file.is_open()) {
        std::cerr << "[-] Failed to open file: " << outputFile << std::endl;
        return;
    }
    file << data.dump(4); // pretty print
    file.close();
}

json ForensicExtractor::parseArtifact(const std::string& output, DataType type) {
    switch (type) {
    case DataType::SMS:
        return parseSMS(output);

    case DataType::CALL:
        return parseCallLogs(output);

    case DataType::USER_INSTALLED_APPS:
        return parseInstalledApps(output);

    case DataType::MEDIA:
        return parseMedia(output);

    default:
        return json::array();
    }
}

std::vector<std::map<std::string, std::string>> ForensicExtractor::extractRows(const std::string& output) {
    std::istringstream stream(output);
    std::string line;
    std::vector<std::map<std::string, std::string>> rows;

    while (std::getline(stream, line)) {
        if (line.find("Row:") != std::string::npos) {
            auto parsed = parseRow(line);
            if (!parsed.empty()) {
                rows.push_back(parsed);
            }
        }
    }

    return rows;
}



json ForensicExtractor::parseSMS(const std::string& output) {
    auto rows = extractRows(output);
    json result = json::array();

    for (const auto& row : rows) {
        json obj;

        obj["type"] = "sms";
        obj["address"] = row.count("address") ? row.at("address") : "";
        obj["message"] = row.count("body") ? row.at("body") : "";
        obj["timestamp"] = row.count("date") ? row.at("date") : "";

        // SMS type mapping
        if (row.count("type")) {
            std::string t = row.at("type");
            if (t == "1") obj["direction"] = "inbox";
            else if (t == "2") obj["direction"] = "sent";
            else obj["direction"] = "unknown";
        }

        result.push_back(obj);
    }

    return result;
}

json ForensicExtractor::parseCallLogs(const std::string& output) {
    auto rows = extractRows(output);
    json result = json::array();

    for (const auto& row : rows) {
        json obj;

        obj["type"] = "call";
        obj["number"] = row.count("number") ? row.at("number") : "";
        obj["duration"] = row.count("duration") ? row.at("duration") : "";
        obj["timestamp"] = row.count("date") ? row.at("date") : "";

        // Call type mapping
        if (row.count("type")) {
            std::string t = row.at("type");
            if (t == "1") obj["call_type"] = "incoming";
            else if (t == "2") obj["call_type"] = "outgoing";
            else if (t == "3") obj["call_type"] = "missed";
            else obj["call_type"] = "unknown";
        }

        result.push_back(obj);
    }

    return result;
}

json ForensicExtractor::parseMedia(const std::string& output) {
    auto rows = extractRows(output);
    json result = json::array();

    for (const auto& row : rows) {
        json obj;

        obj["type"] = "media";
        obj["path"] = row.count("_data") ? row.at("_data") : "";
        obj["mime_type"] = row.count("mime_type") ? row.at("mime_type") : "";
        obj["size"] = row.count("size") ? row.at("size") : "";
        obj["timestamp"] = row.count("date_added") ? row.at("date_added") : "";

        result.push_back(obj);
    }

    return result;
}

json ForensicExtractor::parseInstalledApps(const std::string& output) {

    json result = json::array();
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line))
    {
        // skip empty lines
        if (line.empty())
            continue;

        // expected format: package:com.example.app
        const std::string prefix = "package:";
        if (line.find(prefix) == 0)
        {
            std::string packageName = line.substr(prefix.length());

            json obj;
            obj["type"] = "package";
            obj["package_name"] = packageName;

            result.push_back(obj);
        }
    }

    return result;
}





void ForensicExtractor::listDevices() {
    std::string output = adb.exec("devices");
    std::cout << "\nConnected Devices:\n" << output << std::endl;
}

void ForensicExtractor::extractDeviceInfo(const std::string& outputFile) {
    
    std::cout << "[*] Extracting device information...\n";
    std::string output = adb.exec("shell getprop");
    std::istringstream stream(output);
    std::string line;
    json deviceInfo;

    while (std::getline(stream, line)) {
        // Expected format: [key]: [value]
        size_t keyStart = line.find('[');
        size_t keyEnd = line.find(']');
        size_t valueStart = line.find('[', keyEnd);
        size_t valueEnd = line.find(']', valueStart);

        if (keyStart == std::string::npos || keyEnd == std::string::npos ||
            valueStart == std::string::npos || valueEnd == std::string::npos) {
            continue;
        }

        std::string key = line.substr(keyStart + 1, keyEnd - keyStart - 1);
        std::string value = line.substr(valueStart + 1, valueEnd - valueStart - 1);

        deviceInfo[key] = value;
    }

    saveJSONToFile(deviceInfo, outputFile);
    std::cout << "[+] Device info saved to " << outputFile << std::endl;
}

void ForensicExtractor::extractUserInstalledAppsList(const std::string& outputFile) {

    std::string output = adb.exec("shell pm list packages -3");
    json data = parseArtifact(output, DataType::USER_INSTALLED_APPS);
    saveJSONToFile(data, outputFile);
}

void ForensicExtractor::extractSMS(const std::string& outputFile) {
    
    std::string output = adb.exec("shell content query --uri content://sms");
    json data = parseArtifact(output, DataType::SMS);
    saveJSONToFile(data, outputFile);
}

void ForensicExtractor::extractCallLogs(const std::string& outputFile) {

    std::string output = adb.exec("shell content query --uri content://call_log/calls");
    json data = parseArtifact(output, DataType::CALL);
    saveJSONToFile(data, outputFile);
    std::cout << "[+] Call logs saved to " << outputFile << std::endl;
}

void ForensicExtractor::pullMedia(const std::string& localPath) {

    std::cout << "[*] Pulling media (DCIM)...\n";
    std::string command = "pull /sdcard/DCIM \"" + localPath + "\"";
    std::string result = adb.exec(command);
    std::cout << result << std::endl;
    std::cout << "[+] Media saved to " << localPath << std::endl;
}

void ForensicExtractor::extractMediaStoreDb(const std::string& outputFile) {
    
    std::string output = adb.exec(
        "shell content query --uri content://media/external/file"
    );
    json data = parseArtifact(output, DataType::MEDIA);
    saveJSONToFile(data, outputFile);
    std::cout << "[+] MediaStore DB saved to " << outputFile << std::endl;
}