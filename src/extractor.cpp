#include <iostream>
#include <fstream>
#include <sstream>
#include "extractor.h"
#include "parser.h"
#include "stringUtil.h"
#include "zipHelper.h"

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
        std::cout << "[*] " << pathToDir << " exists" << std::endl;
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

// ===================================== PUBLIC =====================================

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
    std::wstring output = adb.exec(L"shell getprop");
    nlohmann::json deviceInfo = PARSER::parseAdbDeviceInfo(StringUtils::convertWStringToUTF8(output));
    PARSER::saveJSONToFile(deviceInfo, pathToOutputFile.string());
    ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/deviceInfo.zip");
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
    std::cout << "[*] Extracting user Installed App list...\n";
    std::wstring output = adb.exec(L"shell pm list packages -3");
    nlohmann::json data = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(output), DataType::USER_INSTALLED_APPS);
    PARSER::saveJSONToFile(data, pathToOutputFile.string());
    ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/installedApps.zip");
    std::cout << "[+] User-Installed App list saved to " << pathToOutputFile << std::endl;
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
    std::wstring output = adb.exec(L"shell content query --uri content://sms");
    nlohmann::json data = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(output), DataType::SMS);
    PARSER::saveJSONToFile(data, pathToOutputFile.string());
    ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string()+"/sms.zip");
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
    std::wstring output = adb.exec(L"shell content query --uri content://call_log/calls");
    nlohmann::json data = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(output), DataType::CALL);
    PARSER::saveJSONToFile(data, pathToOutputFile.string());
    ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/callLogs.zip");
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
        std::cout << "[*] Extracting MediaStore...\n";
        this->extractMediaStoreDb(L"mediastore.json");
    }
    const std::filesystem::path pathToOutputDir = pathToDir / dirName;

    // Check for mediastore.json
    if (!std::filesystem::exists(mediastorePath)) {
        // Extract mediaStore index file and save it in json format
        std::cout << "[*] Extracting MediaStore...\n";
        this->extractMediaStoreDb(L"mediastore.json");
    }

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
    const std::filesystem::path pathToOutputFile{ pathToDir.wstring() + L"\\" + outputFileName };
    std::cout << "[*] Extracting MediaStore...\n";
    std::wstring output = adb.exec(
        L"shell content query --uri content://media/external/file"
    );
    nlohmann::json data = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(output), DataType::MEDIA);
    PARSER::saveJSONToFile(data, pathToOutputFile.string());
    ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/MediaStore.zip");
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
    
//    PARSER::saveJSONToFile(calendars_json, "calander.json");
//    PARSER::saveJSONToFile(events_json, "events.json");
//    PARSER::saveJSONToFile(calendarWhen_json, "when.json");
//    PARSER::saveJSONToFile(attendees_json, "attendees.json");
//    PARSER::saveJSONToFile(reminders_json, "reminders.json");
//    PARSER::saveJSONToFile(extendedproperties_json, "extendedProperties.json");
//    PARSER::saveJSONToFile(mergedArtifact_json, "mergedArtifact.json");
    std::error_code ec;
    std::filesystem::path pathToCalendarEntitiesDir = pathToDir / "calendarEntities";
    std::filesystem::create_directory(pathToCalendarEntitiesDir, ec);
    if (ec) {
        std::cout << "error while creating " << pathToCalendarEntitiesDir << std::endl;
        return -1;
    }
    const std::filesystem::path pathToMergedReport{ pathToCalendarEntitiesDir.generic_wstring() + L"\\" + L"calendarEntities_2.json" };
    PARSER::saveJSONToFile(mergedArtifact_json, pathToMergedReport.generic_string());
    const std::filesystem::path pathToCalendarReport{ pathToCalendarEntitiesDir.wstring() + L"\\" + outputFileName };
    PARSER::saveJSONToFile(calendarReport, pathToCalendarReport.generic_string());
    std::cout << " ---> " << pathToCalendarEntitiesDir.generic_string() << std::endl;
    ZipHelper::zipFolderRecursively(pathToCalendarEntitiesDir.generic_string(), pathToDir.generic_string() + "\\" + "calendarEntities.zip");
    std::cout << "[+] Calendar Entities saved to " << pathToCalendarReport << std::endl;
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
    std::cout << "[*] Extracting Contacts...\n";
    std::wstring output = adb.exec(L"shell content query --uri content://contacts/phones/");
    nlohmann::json data = PARSER::parseArtifact(StringUtils::convertWStringToUTF8(output), DataType::CONTACTS);
    PARSER::saveJSONToFile(data, pathToOutputFile.string());
    ZipHelper::addFileToZip(pathToOutputFile.string(), pathToDir.generic_string() + "/contacts.zip");
    std::wcout << "[+] Device CONTACTS saved to " << outputFileName << std::endl;
    return 0;
}