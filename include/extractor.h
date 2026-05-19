#pragma once

#include "adb.h"
#include <string>
#include <vector>
#include <filesystem>

class ForensicExtractor {

private:
    std::vector<std::tuple<std::string, std::string>> getFilePathsFromJson(const std::string& jsonFilePath);
    std::filesystem::path createDirForConnectedDevice(const std::wstring& input);
    bool agentExistOnTarget(void);
    bool installAgent(void);
    bool launchAgent(void);
    bool isAgentRunning(unsigned int& interval_sec, unsigned int& maxAttempts);
    bool waitForAccessPermission(unsigned int& timeout_sec, unsigned int& maxAttempts);
    bool intiateAgentExtraction(void);
    bool pullExportedDataFromTarget(std::filesystem::path& pathToOutputDir);
    bool deviceCleanup(void);


private:
    ADB& adb;

public:
    ForensicExtractor(ADB& adb);
    int extractAll(void);
    int numOfConnectedAdbDevices(void);
    void showConnectedDevices(void);
    int extractDeviceInfo(const std::wstring& outputFileName);
    int extractUserInstalledAppsList(const std::wstring& outputFileName);
    int extractSMS(const std::wstring& outputFileName);
    int extractCallLogs(const std::wstring& outputFileName);
    int pullMedia(const std::wstring& dirName);
    int extractMediaStoreDb(const std::wstring& outputFileName);
    int extractCalendarEntities(const std::wstring& outputFileName);
    int extractContacts(const std::wstring& outputFileName);
    int collectCalendarRawDataViaAdb(std::wstring& calendarRawData, 
                                     std::wstring& eventsRawData, 
                                     std::wstring& calendarWhenRawData, 
                                     std::wstring& attendeesRawData, 
                                     std::wstring& remindersRawData,
                                     std::wstring& extendedPropertiesRawData);
    
};