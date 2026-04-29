#include <iostream>
#include <fstream>
#include <sstream>
#include "extractor.h"
#include "parser.h"
#include <filesystem>


ForensicExtractor::ForensicExtractor(ADB& adbRef) : adb(adbRef) {}

int ForensicExtractor::listDevices() {

    std::string output = adb.exec("devices");
    int deviceCount = -2;
    for (char c : output) {
        if (c == '\n') {
            deviceCount++;
        }
    }
    if(deviceCount == 0)    {
        std::cerr << "No device is connected yet" << std::endl;   
    }
    else {
        std::cout << "\nConnected Devices: "<< deviceCount << std::endl << output << std::endl;
    }
    return deviceCount;
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


std::vector<std::tuple<std::string, std::string>> ForensicExtractor::getFilePathsFromJson(const std::string& jsonFilePath) {
    std::vector<std::tuple<std::string, std::string>> filePaths;

    // Open the JSON file
    std::ifstream file(jsonFilePath);
    if (!file.is_open()) {
        std::cerr << "[!] Failed to open " << jsonFilePath << std::endl;
        return filePaths;
    }

    // Parse the JSON content
    nlohmann::json jsonData;
    file >> jsonData;

    // Extract file paths and MIME types from the parsed JSON data
    for (const auto& entry : jsonData) {
        std::string mime_type = entry["mime_type"];
        std::string filePath = entry["path"];

        // We only care about valid files (non-directory entries)
        if (mime_type != "NULL" && !filePath.empty() && filePath != "/storage/emulated") {
            filePaths.push_back({ filePath, mime_type });
        }
    }

    return filePaths;
}

void ForensicExtractor::pullMedia(const std::string& localPath) {
    
    // Check for connected device
    if (this->listDevices() == 0) {
        return;
    }

    // Check for mediastore.json
    std::string mediastorePath = std::filesystem::current_path().string() + "\\mediastore.json";
    if (!std::filesystem::exists(mediastorePath)) {
        // Extract mediaStore index file and save it in json format
        std::cout << "[*] Extracting MediaStore...\n";
        this->extractMediaStoreDb("mediastore.json");
    }
    
    std::cout << "[*] Reading MediaStore JSON file...\n";
    // Read and parse the mediastore.json file to get the file paths and MIME types
    std::vector<std::tuple<std::string, std::string>> filePaths = getFilePathsFromJson("mediastore.json");
    if (filePaths.empty()) {
        std::cout << "[!] No files found in MediaStore JSON.\n";
        return;
    }

    // Pull the indexed files from MediaStore, organizing by MIME type
    for (const auto& [filePath, mimeType] : filePaths) {
        // Create directory for MIME type if it doesn't exist
        std::string mimeDir = localPath + "/" + mimeType;
        if (!std::filesystem::exists(mimeDir)) {
            std::cout << "[*] Creating directory: " << mimeDir << "\n";
            std::filesystem::create_directories(mimeDir);  // Create the directory for the MIME type
        }

        // Construct the full destination file path
        std::string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);  // Extract filename
        std::string destFilePath = mimeDir + "/" + fileName;

        std::cout << "[*] Pulling file: " << filePath << " to " << destFilePath << "\n";
        std::string pullCommand = "pull \"" + filePath + "\" \"" + destFilePath + "\"";

        std::string result = adb.exec(pullCommand);
        std::cout << result << std::endl;
    }
    std::cout << "[+] All MediaStore indexed files pulled.\n";


    // Pull directories that are typically missed by MediaStore
    std::vector<std::string> directoriesToPull = {
        "/storage/emulated/0/Android/media/",
        "/storage/emulated/0/Android/data/",
        "/storage/emulated/0/Android/obb/",
        "/storage/emulated/0/WhatsApp/Media/",
        "/storage/emulated/0/Telegram/",
        "/storage/emulated/0/Instagram/",
        "/storage/emulated/0/.thumbnails/",
        "/storage/emulated/0/.Statuses/",
        "/storage/emulated/0/.nomedia/",
        "/storage/emulated/0/.android_secure/",
        "/storage/emulated/0/Google Drive/",
        "/storage/emulated/0/Dropbox/",
        "/storage/emulated/0/OneDrive/",
        "/storage/emulated/0/TWRP/",
        "/storage/emulated/0/clockworkmod/"
    };

    // Pull these directories and organize them by MIME type (if possible)
    for (const auto& dir : directoriesToPull) {
        std::cout << "[*] Pulling directory: " << dir << "\n";
        std::string dirPullCommand = "pull \"" + dir + "\" \"" + localPath + "\"";
        std::string result = adb.exec(dirPullCommand);
        std::cout << result << std::endl;
    }

    std::cout << "[+] All directories pulled successfully.\n";
}

void ForensicExtractor::extractMediaStoreDb(const std::string& outputFile) {
    
    std::string output = adb.exec(
        "shell content query --uri content://media/external/file"
    );
    nlohmann::json data = PARSER::parseArtifact(output, DataType::MEDIA);
    PARSER::saveJSONToFile(data, outputFile);
    std::cout << "[+] MediaStore DB saved to " << outputFile << std::endl;
}