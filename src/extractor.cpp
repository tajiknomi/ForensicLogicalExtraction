#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <cwctype>
#include "extractor.h"
#include "parser.h"
#include "stringUtil.h"
#include "zipHelper.h"
#include "databaseOperations.h"


ForensicExtractor::ForensicExtractor(ADB& adbRef) : adb(adbRef) {}

// ===================================== PRIVATE =====================================

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

std::filesystem::path ForensicExtractor::createDirForConnectedDevice(const std::wstring& input) {
    std::wstring deviceSerial;
    std::wstringstream stream(input);
    std::getline(stream, deviceSerial);
    std::getline(stream, deviceSerial);
    std::wstringstream(deviceSerial) >> deviceSerial;
    std::error_code ec;
    std::filesystem::path pathToDir = std::filesystem::current_path() / deviceSerial;
    if (std::filesystem::exists(pathToDir)) {
    //    std::cout << "[*] " << pathToDir << " exists" << std::endl;
        return pathToDir;
    }
    std::filesystem::create_directory(pathToDir, ec);
    if (ec) {
        // An error occurred
        std::cout << "Error occurred while creating directory: " << ec.message() << std::endl;
    }
    else {
        // Directory creation succeeded
        std::cout << "[*] Directory created successfully: " << pathToDir << std::endl;
    }
    return pathToDir;
}

int ForensicExtractor::collectCalendarRawDataViaAdb(std::wstring& calendarRawData,
    std::wstring& eventsRawData,
    std::wstring& calendarWhenRawData,
    std::wstring& attendeesRawData,
    std::wstring& remindersRawData,
    std::wstring& extendedPropertiesRawData) {

    calendarRawData = adb.exec(
        L"shell content query --uri content://com.android.calendar/calendars"
    );
    eventsRawData = adb.exec(
        L"shell content query --uri content://com.android.calendar/events"
    );
    calendarWhenRawData = adb.exec(
        L"shell content query --uri content://com.android.calendar/instances/when/0/9999999999999"
    );
    attendeesRawData = adb.exec(
        L"shell content query --uri content://com.android.calendar/attendees"
    );
    remindersRawData = adb.exec(
        L"shell content query --uri content://com.android.calendar/reminders"
    );
    extendedPropertiesRawData = adb.exec(
        L"shell content query --uri content://com.android.calendar/extendedproperties"
    );
    return 0;
}

bool ForensicExtractor::agentExistOnTarget() {
    std::wstring output = adb.exec(L"shell pm path com.example.exporter");
    if (output.find(L"package:") != std::wstring::npos) {
        //std::wcout << L"Agent found on the target!" << std::endl;
        return true;
    }
    return false;
}

bool ForensicExtractor::installAgent() {
    std::wstring output = adb.exec(L"install -r -g app-debug.apk");
    if (output.find(L"Success") == std::wstring::npos) {
        return false;
    }
    return true;
}

bool ForensicExtractor::launchAgent() {
    std::wstring output = adb.exec(L"shell am start -n com.example.exporter/.MainActivity");
    if (output.find(L"Error") != std::wstring::npos || output.find(L"exception") != std::wstring::npos) {
        return false;
    }
    return true;
}

bool ForensicExtractor::isAgentRunning(unsigned int& interval_sec, unsigned int& maxAttempts) {
    unsigned int attempts = 0;
    bool runningSuccessFlag{ false };
    while (attempts++ != maxAttempts) {
        std::wstring output = adb.exec(L"shell pidof com.example.exporter");
        if (!output.empty() && std::iswdigit(output[0])) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(interval_sec));
    }
    return false;
}

bool ForensicExtractor::waitForAccessPermission(unsigned int& interval_sec, unsigned int& maxAttempts) {
    
    for (unsigned int i = 0; i < maxAttempts; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(interval_sec));
        std::wstring permCheck = adb.exec(L"shell \"appops get com.example.exporter android:read_call_log | findstr allow\"");
        if (!permCheck.empty()) {
            return true;
        }
    }
    return false;
}

bool ForensicExtractor::intiateAgentExtraction() {

    struct ExportTask {
        const wchar_t* action;
        const wchar_t* name;
    };

    ExportTask tasks[] = {
        {L"com.exporter.DUMP_CALLS", L"Call logs"},
        {L"com.exporter.DUMP_SMS", L"SMS"},
        {L"com.exporter.DUMP_MMS", L"MMS"},
        {L"com.exporter.DUMP_CONTACTS", L"Contacts"},
        {L"com.exporter.DUMP_CALENDAR", L"Calendar"}
    };

    for (const auto& task : tasks) {
        std::wcout << L"[*] Exporting " << task.name << L"..." << std::endl;
        std::wstring cmd = L"shell am start -n com.example.exporter/.ExportActivity --es action ";
        cmd += task.action;
        std::wstring output = adb.exec(cmd);

        if (output.find(L"Error") != std::wstring::npos || output.find(L"exception") != std::wstring::npos) {
            std::wcerr << L"[-] Failed to export " << task.name << L": " << output << std::endl;
            // Continue with other exports instead of failing completely
        }
        else {
            std::wcout << L"[+] Successfully exported " << task.name << std::endl;
        }
        // Small delay between exports
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return true;
}

bool ForensicExtractor::pullExportedDataFromTarget(std::filesystem::path& pathToOutputDir){
    std::wstring pullCmd = L"pull /sdcard/Android/data/com.example.exporter/files/exporter/ " + pathToOutputDir.generic_wstring();
    std::wstring output = adb.exec(pullCmd);

    if (output.find(L"error") != std::wstring::npos || output.find(L"failed") != std::wstring::npos) {     
        return false;
    }
    return true;
}

bool ForensicExtractor::hasPermissionError(const std::wstring& output) {
    std::vector<std::wstring> errors = {
        L"SecurityException", L"Permission denied",
        L"requires permission", L"Access denied",
        L"IllegalArgumentException"
    };
    for (const auto& err : errors) {
        if (output.find(err) != std::wstring::npos) return true;
    }
    return output.empty();
}

int ForensicExtractor::extractViaAgent(const std::wstring& artifactType) {
    std::wstring action;
    if (artifactType == L"CALLS") action = L"com.exporter.DUMP_CALLS";
    else if (artifactType == L"SMS") action = L"com.exporter.DUMP_SMS";
    else if (artifactType == L"CONTACTS") action = L"com.exporter.DUMP_CONTACTS";
    else if (artifactType == L"MMS") action = L"com.exporter.DUMP_MMS";
    else if (artifactType == L"CALENDAR") action = L"com.exporter.DUMP_CALENDAR";

    std::wcout << L"[*] Checking existing agent on the target..." << std::endl;
    if (agentExistOnTarget()) {
        std::wcout << L"[+] Agent found on the target!" << std::endl;
    }
    else {
        std::wcout << L"[*] Installing app..." << std::endl;
        if (installAgent()) {
            std::wcout << L"[+] Agent installed successfully" << std::endl;
        }
        else {
            std::wcerr << L"[-] Failed to install Agent" << std::endl;
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    std::wcout << L"[*] Launching Agent..." << std::endl;
    if (launchAgent() == false) {
        std::wcerr << L"[-] Failed to launch Agent" << std::endl;
        return -1;
    }
    unsigned int interval_sec = 1;
    unsigned int maxAttempts = 10;

    if (isAgentRunning(interval_sec, maxAttempts) == false) {
        std::wcerr << L"[-] Failed to Run Agent" << std::endl;
        return -1;
    }
    if (waitForAccessPermission(interval_sec, maxAttempts) == false) {
        std::cerr << L"[-] Permissions not granted by user. Cannot proceed." << std::endl;
        return -1;
    }

    // Launch specific artifact extraction
    std::wstring cmd = L"shell am start -n com.example.exporter/.ExportActivity --es action " + action;
    std::wstring output = adb.exec(cmd);

    if (output.find(L"Error") != std::wstring::npos || output.find(L"exception") != std::wstring::npos) {
        std::wcerr << L"[-] Failed to export " << action << L": " << output << std::endl;
        return -1;
    }
    else {
        std::wcout << L"[+] Successfully exported " << action << std::endl;
        isAgentDumpDataAvailable = true;
    }
    return 0;
}

bool ForensicExtractor::deviceCleanup() {
    std::wstring output = adb.exec(L"uninstall com.example.exporter");
    if (output.find(L"Success") != std::wstring::npos) {
        return true;
    }
    return false;
}



// ===================================== PUBLIC =====================================


nlohmann::json parseAgentRawToJSON(const std::wstring& agentRawWStr) {
    // 1. Convert wide string from ADB to standard UTF-8 string
    std::string input = StringUtils::convertWStringToUTF8(agentRawWStr);

    std::istringstream stream(input);
    std::string line;
    nlohmann::json result = nlohmann::json::array();

    // 2. Read line by line (each line is a full row artifact)
    while (std::getline(stream, line)) {
        // Skip empty lines or padding
        if (line.empty() || line.find(L'=') == std::string::npos) {
            continue;
        }

        nlohmann::json rowObj = nlohmann::json::object();
        std::istringstream lineStream(line);
        std::string token;

        // 3. Split the line by the vertical bar '|'
        while (std::getline(lineStream, token, '|')) {
            if (token.empty()) continue;

            // Find the key-value assignment boundary
            size_t eqPos = token.find('=');
            if (eqPos != std::string::npos) {
                std::string key = token.substr(0, eqPos);
                std::string value = token.substr(eqPos + 1);

                // Forensic Clean-up: handle null strings natively in JSON
                if (value == "null") {
                    rowObj[key] = nullptr;
                }
                else {
                    rowObj[key] = value;
                }
            }
        }

        // 4. Append the parsed row object into our final JSON array
        if (!rowObj.empty()) {
            result.push_back(rowObj);
        }
    }

    return result;
}

std::map<std::string, nlohmann::json> processExtractedDirectory(const std::filesystem::path& dirPath) {
    std::map<std::string, nlohmann::json> directoryJsonMatrix;

    // Verify directory validity before processing
    if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
        std::wcerr << L"Error: Target directory does not exist: " << dirPath.wstring() << std::endl;
        return directoryJsonMatrix;
    }

    // Iterate through every item in the folder
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            std::filesystem::path filePath = entry.path();

            // STRICT GATE: Only look at files that end in exactly ".raw"
            if (filePath.extension() != ".raw") {
                continue;
            }

            // Read the file contents natively as UTF-8 / wide text
            std::wifstream fileStream(filePath);
            if (!fileStream.is_open()) {
                std::wcerr << L"Failed to open file: " << filePath.wstring() << std::endl;
                continue;
            }

            // Read the entire file buffer into a single wstring
            std::wstring fileContent((std::istreambuf_iterator<wchar_t>(fileStream)),
                std::istreambuf_iterator<wchar_t>());
            fileStream.close();

            // Parse content to JSON using your class method
            nlohmann::json parsedJson = parseAgentRawToJSON(fileContent);

            // Store the JSON using the filename (e.g., "sms.raw") as the map key
            std::string filenameStr = filePath.filename().string();
            directoryJsonMatrix[filenameStr] = parsedJson;

            std::wcout << L"Successfully extracted JSON from: " << filePath.filename().wstring() << std::endl;
        }
    }

    return directoryJsonMatrix;
}

int ForensicExtractor::extractAll() {

    if (this->numOfConnectedAdbDevices() == 0) {
        std::cerr << "No device is connected !" << std::endl;
        return -1;
    }
    std::filesystem::path pathToDir;
    if ((pathToDir = this->createDirForConnectedDevice(adb.exec(L"devices"))).empty()) {
        return -1;
    }
    
//    extractContacts(L"contacts.json");
    extractSMS(L"sms.db");
    extractCallLogs(L"calllogs.db");
    extractDeviceInfo(L"deviceInfo.json");
    extractMediaStoreDb(L"mediastore.db");
    extractCalendarEntities(L"calendarEntities.db"); 
    extractUserInstalledAppsList(L"appList.db");

    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (isAgentDumpDataAvailable && (pullExportedDataFromTarget(pathToDir) == false)) {
        std::wcerr << L"Failed to pull files" << std::endl;
    }
    else {
        std::wcout << L"[+] Extraction complete! Files saved to: " << pathToDir << std::endl;
    }

    // Process the files from agent
    std::map<std::string, nlohmann::json> agentFiles_json = processExtractedDirectory(pathToDir/"exporter");

    for (const auto& [filename, json_data] : agentFiles_json) {

        std::string pureTableName = std::filesystem::path(filename).stem().string();
        std::vector<nlohmann::json> sqlTables = { json_data };
        std::vector<std::string> sqlTableNames = { pureTableName }; // Will be exactly "sms", "callog", etc.
        std::filesystem::path dbPath = pathToDir / (pureTableName + ".db");
        if (dbOperations::storeTablesToDbFile(dbPath, sqlTableNames, sqlTables) != 0) {
            std::cerr << "[-] Database operation on " << dbPath.string() << " is NOT Successful" << std::endl;
        }
    }

    if(isAgentDumpDataAvailable) {
        std::wcout << L"Cleaning up target device..." << std::endl;
        if (deviceCleanup() == true) {
            std::wcout << L"[+] Agent uninstalled successfully. Device left clean!" << std::endl;
        }
        else {
            std::wcerr << L"[-] Warning: Failed to uninstall Agent." << std::endl;
        }
    }
    return 0;
}

int ForensicExtractor::numOfConnectedAdbDevices() {
    
    std::string output = StringUtils::convertWStringToUTF8(adb.exec(L"devices"));
    int deviceCount{ -2 };
    for (char c : output) {
        if (c == '\n') {
            deviceCount++;
        }
    }
    return deviceCount;
}

void ForensicExtractor::showConnectedDevices() {

    int deviceCount{-2};
    std::string output = StringUtils::convertWStringToUTF8(adb.exec(L"devices"));
    for (char c : output) {
        if (c == '\n') {
            deviceCount++;
        }
    }
    if (deviceCount == 0) {
        std::cerr << "No device is connected !" << std::endl;
    }
    else {
        std::cout << "\nConnected Devices: " << deviceCount << std::endl << output << std::endl;
    }
}

int ForensicExtractor::extractDeviceInfo(const std::wstring& outputFileName) {

    if (this->numOfConnectedAdbDevices() == 0) {
        std::cerr << "No device is connected !" << std::endl;
        return -1;
    }
    std::filesystem::path pathToDir;
    if ((pathToDir = this->createDirForConnectedDevice(adb.exec(L"devices"))).empty()) {
        return -1;
    }
    const std::filesystem::path pathToOutputFile{ pathToDir.wstring() + L"\\" + outputFileName };
    std::cout << "[*] Extracting device information...\n";
    std::wstring deviceInfoRAW = adb.exec(L"shell getprop");
    nlohmann::json deviceInfo_json = PARSER::parseAdbDeviceInfo(StringUtils::convertWStringToUTF8(deviceInfoRAW));

    //std::vector< nlohmann::json > sqlTables = { deviceInfo_json };
    //std::vector<std::string> sqlTableNames = { "deviceInfo" };

    //if (dbOperations::storeTablesToDbFile(pathToDir / "DeviceInfo.db", sqlTableNames, sqlTables) != 0) {
    //    std::cerr << "[-] Database operation on " << pathToDir / "DeviceInfo.db is NOT Successful" << std::endl;
    //}

    PARSER::saveJSONToFile(deviceInfo_json, pathToOutputFile.string());
//    ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/deviceInfo.zip");
    std::cout << "[+] Device info saved to " << pathToOutputFile << std::endl;
    return 0;
}

int ForensicExtractor::extractUserInstalledAppsList(const std::wstring& outputFileName) {

    if (this->numOfConnectedAdbDevices() == 0) {
        std::cerr << "No device is connected !" << std::endl;
        return -1;
    }
    std::filesystem::path pathToDir;
    if ((pathToDir = this->createDirForConnectedDevice(adb.exec(L"devices"))).empty()) {
        return -1;
    }
    const std::filesystem::path pathToOutputFile{ pathToDir.wstring() + L"\\" + outputFileName };
    std::cout << "[*] Extract Application list...\n";
    std::wstring installedAppsRAW = adb.exec(L"shell pm list packages -f -u -i");

    if (hasPermissionError(installedAppsRAW)) {
        std::wcout << L"[-] ADB failed (permission error). Striping the -i flag..." << std::endl;
        installedAppsRAW = adb.exec(L"shell pm list packages -f -u");
    }

    nlohmann::json installedApps_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(installedAppsRAW), DataType::USER_INSTALLED_APPS);

    std::vector< nlohmann::json > sqlTables = { installedApps_json };
    std::vector<std::string> sqlTableNames = { "apps" };

    if (dbOperations::storeTablesToDbFile(pathToDir / outputFileName, sqlTableNames, sqlTables) != 0) {
        std::cerr << "[-] Database operation on " << pathToDir / outputFileName <<" is NOT Successful" << std::endl;
    }
    
//    PARSER::saveJSONToFile(data, pathToOutputFile.string());
//    ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/installedApps.zip");
    std::cout << "[+] App list saved to " << pathToOutputFile << std::endl;
    return 0;
}

int ForensicExtractor::extractSMS(const std::wstring& outputFileName) {

    if (this->numOfConnectedAdbDevices() == 0) {
        std::cerr << "No device is connected !" << std::endl;
        return -1;
    }
    std::filesystem::path pathToDir;
    if ((pathToDir = this->createDirForConnectedDevice(adb.exec(L"devices"))).empty()) {
        return -1;
    }
    const std::filesystem::path pathToOutputFile{ pathToDir.wstring() + L"\\" + outputFileName };
    std::cout << "[*] Extracting SMS...\n";
    std::wstring smsRAW = adb.exec(L"shell content query --uri content://sms");

    if (hasPermissionError(smsRAW)) {
        std::wcout << L"[-] ADB failed (permission error). Falling back to agent..." << std::endl;
        extractViaAgent(L"SMS");
        return 0;
    }
    
    nlohmann::json sms_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(smsRAW), DataType::ADB_RAW);
    std::vector< nlohmann::json > sqlTables = { sms_json };
    std::vector<std::string> sqlTableNames = { "sms" };

    if (dbOperations::storeTablesToDbFile(pathToDir / outputFileName, sqlTableNames, sqlTables) != 0) {
        std::cerr << "[-] Database operation on " << pathToDir / outputFileName << " is NOT Successful" << std::endl;
    }
//    PARSER::saveJSONToFile(sms_json, pathToOutputFile.string());
//    ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string()+"/sms.zip");
    std::wcout << "[+] Device SMS saved to " << outputFileName << std::endl;

    return 0;
}

int ForensicExtractor::extractCallLogs(const std::wstring& outputFileName) {
    
    if (this->numOfConnectedAdbDevices() == 0) {
        std::cerr << "No device is connected !" << std::endl;
        return -1;
    }
    std::filesystem::path pathToDir;
    if ((pathToDir = this->createDirForConnectedDevice(adb.exec(L"devices"))).empty()) {
        return -1;
    }
    const std::filesystem::path pathToOutputFile{ pathToDir.wstring() + L"\\" + outputFileName };
    std::cout << "[*] Extracting Call Logs...\n";
    std::wstring callLogsRAW = adb.exec(L"shell content query --uri content://call_log/calls");

    if (hasPermissionError(callLogsRAW)) {
        std::wcout << L"[-] ADB failed (permission error). Falling back to agent..." << std::endl;
        extractViaAgent(L"CALLS");
        return 0;
    }


    nlohmann::json callLogs_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(callLogsRAW), DataType::ADB_RAW);
    std::vector< nlohmann::json > sqlTables = { callLogs_json };
    std::vector<std::string> sqlTableNames = { "callLogs" };

    if (dbOperations::storeTablesToDbFile(pathToDir / outputFileName, sqlTableNames, sqlTables) != 0) {
        std::cerr << "[-] Database operation on " << pathToDir / outputFileName << " is NOT Successful" << std::endl;
    }
    //nlohmann::json data = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(output), DataType::CALL);
    //PARSER::saveJSONToFile(data, pathToOutputFile.string());
    //ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/callLogs.zip");
    std::cout << "[+] Call logs saved to " << pathToOutputFile << std::endl;
    return 0;
}

int ForensicExtractor::pullMedia(const std::wstring& dirName) {

    if (this->numOfConnectedAdbDevices() == 0) {
        std::cerr << "No device is connected !" << std::endl;
        return -1;
    }
    std::filesystem::path pathToDir;
    if ((pathToDir = this->createDirForConnectedDevice(adb.exec(L"devices"))).empty()) {
        return -1;
    }

    const std::filesystem::path mediastorePath = pathToDir / "mediastore.json";
    if (!std::filesystem::exists(mediastorePath)) {
        // Extract mediaStore index file and save it in json format
       // std::cout << "[*] Extracting MediaStore...\n";
        this->extractMediaStoreDb(L"mediastore.json");
    }
    const std::filesystem::path pathToOutputDir = pathToDir / dirName;

    std::cout << "[*] Reading MediaStore JSON file...\n";
    // Read and parse the mediastore.json file to get the file paths and MIME types
    std::vector<std::tuple<std::string, std::string>> filePaths = getFilePathsFromJson(StringUtils::convertWStringToUTF8(mediastorePath));
    if (filePaths.empty()) {
        std::cout << "[!] No files found in MediaStore JSON.\n";
        return -1;
    }

    // Pull the indexed files from MediaStore, organizing by MIME type
    for (const auto& [filePath, mimeType] : filePaths) {
        // Create directory for MIME type if it doesn't exist
        std::string mimeDir = pathToOutputDir.u8string() + "\\" + mimeType;
        if (!std::filesystem::exists(mimeDir)) {
            std::cout << L"[*] Creating directory: " << mimeDir << L"\n";
            std::filesystem::create_directories(mimeDir);  // Create the directory for the MIME type
        }

        // Construct the full destination file path
        std::string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);  // Extract filename
        std::string destFilePath = mimeDir + "\\" + fileName;
        std::cout << L"[*] Pulling file: " << filePath << " to " << destFilePath << "\n";
        std::string pullCommand = "pull \"" + filePath + "\" \"" + destFilePath + "\"";
        std::wstring result = adb.exec(StringUtils::convertUTF8ToWString(pullCommand));
        std::wcout << result << std::endl;
    }
    std::cout << "[+] All MediaStore indexed files pulled.\n";


    // Pull directories that are typically missed by MediaStore
    std::vector<std::wstring> directoriesToPull = {
        L"/storage/emulated/0/Android/media/",
        L"/storage/emulated/0/Android/data/",
        L"/storage/emulated/0/Android/obb/",
        L"/storage/emulated/0/WhatsApp/Media/",
        L"/storage/emulated/0/Telegram/",
        L"/storage/emulated/0/Instagram/",
        L"/storage/emulated/0/.thumbnails/",
        L"/storage/emulated/0/.Statuses/",
        L"/storage/emulated/0/.nomedia/",
        L"/storage/emulated/0/.android_secure/",
        L"/storage/emulated/0/Google Drive/",
        L"/storage/emulated/0/Dropbox/",
        L"/storage/emulated/0/OneDrive/",
        L"/storage/emulated/0/TWRP/",
        L"/storage/emulated/0/clockworkmod/"
    };

    // Pull these directories and organize them by MIME type (if possible)
    for (const auto& dir : directoriesToPull) {
        std::wcout << "[*] Pulling directory: " << dir << "\n";
        std::wstring dirPullCommand = L"pull \"" + dir + L"\" \"" + pathToOutputDir.wstring() + L"\"";
        std::wstring result = adb.exec(dirPullCommand);
        std::wcout << result << std::endl;
    }

    std::cout << "[+] All directories pulled successfully.\n";
    return 0;
}

int ForensicExtractor::extractMediaStoreDb(const std::wstring& outputFileName) {

    if (this->numOfConnectedAdbDevices() == 0) {
        std::cerr << "No device is connected !" << std::endl;
        return -1;
    }
    std::filesystem::path pathToDir;
    if ((pathToDir = this->createDirForConnectedDevice(adb.exec(L"devices"))).empty()) {
        return -1;
    }
    std::filesystem::path pathToOutputFile{ pathToDir.wstring() + L"\\" + outputFileName };
    std::cout << "[*] Extracting MediaStore...\n";
    std::wstring mediaStoreDbRaw = adb.exec(
        L"shell content query --uri content://media/external/file"
    );

    if (hasPermissionError(mediaStoreDbRaw)) {
        std::wcout << L"[-] ADB failed (permission error). Falling back to agent..." << std::endl;
        extractViaAgent(L"MediaStore");
        return 0;
    }

    nlohmann::json mediaStoreDb_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(mediaStoreDbRaw), DataType::ADB_RAW);
    std::vector< nlohmann::json > sqlTables = { mediaStoreDb_json };
    std::vector<std::string> sqlTableNames = { "MediaStore" };

    if (dbOperations::storeTablesToDbFile(pathToDir / "mediaStore.db", sqlTableNames, sqlTables) != 0) {
        std::cerr << "database operation on " << pathToDir / "MediaStore.db is NOT Successful" << std::endl;
    }
    nlohmann::json mediaStoreDbParsed_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(mediaStoreDbRaw), DataType::MEDIA);
    PARSER::saveJSONToFile(mediaStoreDbParsed_json, pathToOutputFile.replace_extension(".json").string());
  //  ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/MediaStore.zip");
    std::cout << "[+] MediaStore DB saved to " << pathToOutputFile << std::endl;
    return 0;
}

int ForensicExtractor::extractCalendarEntities(const std::wstring& outputFileName) {

    if (this->numOfConnectedAdbDevices() == 0) {
        std::cerr << "No device is connected !" << std::endl;
        return -1;
    }
    std::filesystem::path pathToDir;
    if ((pathToDir = this->createDirForConnectedDevice(adb.exec(L"devices"))).empty()) {
        return -1;
    }

    std::cout << "[*] Extracting Calendar Entities...\n";
    std::wstring calendarRawData, eventsRawData, calendarWhenRawData, attendeesRawData, remindersRawData, extendedPropertiesRawData;
    int errorCheck = collectCalendarRawDataViaAdb(calendarRawData, 
                                            eventsRawData, 
                                            calendarWhenRawData, 
                                            attendeesRawData, 
                                            remindersRawData, 
                                            extendedPropertiesRawData);
 


    nlohmann::json calendars_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(calendarRawData), DataType::CALENDAR);
    nlohmann::json events_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(eventsRawData), DataType::EVENTS);
    nlohmann::json calendarWhen_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(calendarWhenRawData), DataType::WHEN);
    nlohmann::json attendees_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(attendeesRawData), DataType::ATTENDEES);
    nlohmann::json reminders_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(remindersRawData), DataType::REMINDERS);
    nlohmann::json extendedproperties_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(extendedPropertiesRawData), DataType::EXTENDED_PROPERTIES);

    nlohmann::json mergedArtifact_json = PARSER::mergeCalendarArtifacts(calendars_json, events_json, calendarWhen_json, attendees_json, reminders_json, extendedproperties_json);
    nlohmann::json calendarReport = PARSER::flattenForensicCalendar(mergedArtifact_json);
    
    std::vector<nlohmann::json> sqlTables = { calendars_json,
                                          events_json,
                               //           calendarWhen_json,
                                          attendees_json,
                                          reminders_json,
                                          extendedproperties_json,
                                          mergedArtifact_json,
                                          calendarReport };
    std::vector<std::string> sqlTableNames = {  "CALENDAR",
                                                "EVENTS",
                                    //            "WHEN",
                                                "ATTENDEES",
                                                "REMINDERS",
                                                "EXTENDED_PROPERTIES",
                                                "mergedArtifact",
                                                "calendarReport"};

    std::filesystem::path dbPath = pathToDir / "calendar.db";

    if (dbOperations::storeTablesToDbFile(dbPath, sqlTableNames, sqlTables) != 0) {
        std::cerr << "[-] Database operation on " << dbPath.string() << " is NOT Successful" << std::endl;
    }

    //std::error_code ec;
    //std::filesystem::path pathToCalendarEntitiesDir = pathToDir / "calendarEntities";
    //std::filesystem::create_directory(pathToCalendarEntitiesDir, ec);
    //if (ec) {
    //    std::cout << "error while creating " << pathToCalendarEntitiesDir << std::endl;
    //    return -1;
    //}
    //const std::filesystem::path pathToMergedReport{ pathToCalendarEntitiesDir.generic_wstring() + L"\\" + L"calendarEntities_2.json" };
    //PARSER::saveJSONToFile(mergedArtifact_json, pathToMergedReport.generic_string());
    //const std::filesystem::path pathToCalendarReport{ pathToCalendarEntitiesDir.wstring() + L"\\" + outputFileName };
//    PARSER::saveJSONToFile(calendarWhen_json, pathToDir.generic_string()+"when.json");
//    ZipHelper::zipFolderRecursively(pathToCalendarEntitiesDir.generic_string(), pathToDir.generic_string() + "\\" + "calendarEntities.zip");
    std::cout << "[+] Calendar Entities saved to " << pathToDir / "calendarEntities" << std::endl;
    //std::cout << calendars_json << std::endl;
    return 0;
}

int ForensicExtractor::extractContacts(const std::wstring& outputFileName) {

    if (this->numOfConnectedAdbDevices() == 0) {
        std::cerr << "No device is connected !" << std::endl;
        return -1;
    }
    std::filesystem::path pathToDir;
    if ((pathToDir = this->createDirForConnectedDevice(adb.exec(L"devices"))).empty()) {
        return -1;
    }
    const std::filesystem::path pathToOutputFile{ pathToDir.wstring() + L"\\" + outputFileName };
    const std::filesystem::path pathToRawADBOutput{ pathToDir.wstring() + L"\\contacts_RAW.json" };
    
    std::cout << "[*] Extracting Contacts...\n";
  //  std::wstring output = adb.exec(L"shell content query --uri content://contacts/phones/");
    std::wstring dataRaw = adb.exec(L"shell content query --uri content://com.android.contacts/data/");

    if (hasPermissionError(dataRaw)) {
        std::wcout << L"[-] ADB failed (permission error). Falling back to agent..." << std::endl;
        extractViaAgent(L"CONTACTS");
        return 0;
    }

    std::wstring contactsSim_table = adb.exec(L"shell content query --uri content://icc/adn");
    std::wstring contacts_table = adb.exec(L"shell content query --uri content://com.android.contacts/contacts");
    
    nlohmann::json dataRaw_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(dataRaw), DataType::ADB_RAW);
    nlohmann::json contactsSim_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(contactsSim_table), DataType::ADB_RAW);
    nlohmann::json contacts_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(contacts_table), DataType::ADB_RAW);
    
    std::vector< nlohmann::json > sqlTables = { dataRaw_json , contactsSim_json, contacts_json };
    std::vector<std::string> sqlTableNames = { "Contacts", "ContactsSim", "Data" };
    
    if (dbOperations::storeTablesToDbFile(pathToDir / "contacts.db", sqlTableNames, sqlTables) != 0) {
        std::cerr << "database operation on " << pathToDir / "contacts.db is NOT Successful" << std::endl;
    }
    
    //std::wstring phonesRaw = adb.exec(L"shell content query --uri content://contacts/phones/");
    //nlohmann::json contactsStructured_json = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(phonesRaw), DataType::CONTACTS);
    //PARSER::saveJSONToFile(contactsStructured_json, pathToOutputFile.string());
    //ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/contacts.zip");
    std::wcout << "[+] Device CONTACTS saved to " << outputFileName << std::endl;
    return 0;
}