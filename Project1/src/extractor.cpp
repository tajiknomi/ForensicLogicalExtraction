#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "extractor.h"
#include "parser.h"


ForensicExtractor::ForensicExtractor(ADB& adbRef) : adb(adbRef) {}

void ForensicExtractor::listDevices() {
    std::string output = adb.exec("devices");
    std::cout << "\nConnected Devices:\n" << output << std::endl;
}

void ForensicExtractor::extractDeviceInfo(const std::string& outputFile) {
    
    std::cout << "[*] Extracting device information...\n";
    std::string output = adb.exec("shell getprop");
    std::istringstream stream(output);
    std::string line;
    nlohmann::json deviceInfo;

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

    PARSER::saveJSONToFile(deviceInfo, outputFile);
    std::cout << "[+] Device info saved to " << outputFile << std::endl;
}

void ForensicExtractor::extractUserInstalledAppsList(const std::string& outputFile) {

    std::string output = adb.exec("shell pm list packages -3");
    nlohmann::json data = PARSER::parseArtifact(output, DataType::USER_INSTALLED_APPS);
    PARSER::saveJSONToFile(data, outputFile);
    std::cout << "[+] User-Installed App list saved to " << outputFile << std::endl;
}

void ForensicExtractor::extractSMS(const std::string& outputFile) {
    
    std::string output = adb.exec("shell content query --uri content://sms");
    nlohmann::json data = PARSER::parseArtifact(output, DataType::SMS);
    PARSER::saveJSONToFile(data, outputFile);
    std::cout << "[+] Device SMS saved to " << outputFile << std::endl;
}

void ForensicExtractor::extractCallLogs(const std::string& outputFile) {

    std::string output = adb.exec("shell content query --uri content://call_log/calls");
    nlohmann::json data = PARSER::parseArtifact(output, DataType::CALL);
    PARSER::saveJSONToFile(data, outputFile);
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
    nlohmann::json data = PARSER::parseArtifact(output, DataType::MEDIA);
    PARSER::saveJSONToFile(data, outputFile);
    std::cout << "[+] MediaStore DB saved to " << outputFile << std::endl;
}