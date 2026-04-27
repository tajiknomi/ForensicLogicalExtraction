#include "extractor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

ForensicExtractor::ForensicExtractor(ADB& adbRef) : adb(adbRef) {}



void ForensicExtractor::saveToFile(const std::string& data, const std::string& filePath) {
    std::ofstream file(filePath, std::ios::out);

    if (!file.is_open()) {
        std::cerr << "[-] Failed to open file: " << filePath << std::endl;
        return;
    }

    file << data;
    file.close();
}

std::map<std::string, std::string> ForensicExtractor::parseRow(const std::string& line) {
    std::map<std::string, std::string> row;

    std::istringstream iss(line);
    std::string token;

    while (iss >> token) {
        size_t pos = token.find('=');
        if (pos != std::string::npos) {
            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);
            row[key] = value;
        }
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




void ForensicExtractor::listDevices() {
    std::string output = adb.exec("devices");
    std::cout << "\nConnected Devices:\n" << output << std::endl;
}

void ForensicExtractor::extractSMS(const std::string& outputFile) {

    std::cout << "[*] Extracting SMS...\n";
    std::string output = adb.exec("shell content query --uri content://sms");
    json data = parseADBOutputToJSON(output);
    saveJSONToFile(data, outputFile);
    std::cout << "[+] SMS saved to " << outputFile << std::endl;
}

void ForensicExtractor::extractCallLogs(const std::string& outputFile) {

    std::cout << "[*] Extracting Call Logs...\n";
    std::string output = adb.exec("shell content query --uri content://call_log/calls");
    json data = parseADBOutputToJSON(output);
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
    
    std::cout << "[*] Extracting MediaStore database...\n";
    std::string output = adb.exec(
        "shell content query --uri content://media/external/file"
    );
    json data = parseADBOutputToJSON(output);
    saveJSONToFile(data, outputFile);
    std::cout << "[+] MediaStore DB saved to " << outputFile << std::endl;
}