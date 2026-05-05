#include <iostream>
#include <Windows.h>
#include "adb.h"
#include "extractor.h"
#include "prompts.h"


std::string FindAdbPath(void)
{
    char buffer[MAX_PATH];

    // Search in current directory + PATH automatically
    unsigned long result = SearchPathA(
        nullptr,        // search in standard locations (current dir + PATH)
        "adb.exe",      // file to find
        nullptr,        // no extension
        MAX_PATH,
        buffer,
        nullptr
    );

    if (result > 0 && result < MAX_PATH)
        return std::string(buffer);

    return std::string("");
}

int main() {

    std::string pathToAdb = FindAdbPath();
    if (pathToAdb.empty()) {
        std::cerr << "Couldn't found adb.exe on your system, try adding adb.exe to you PATH environment variable" << std::endl;
        return -1;
    }
    ADB adb(pathToAdb); // or full path: "C:\\platform-tools\\adb.exe"
    ForensicExtractor extractor(adb);
    showAndroidAdbSetup();
    int choice;

    while (true) {
        showMenu();
        std::cin >> choice;

        switch (choice) {
        case 1:
            extractor.listDevices();
            break;

        case 2:
            extractor.extractDeviceInfo("device_info.json");  
            break;
        
        case 3:
            extractor.extractUserInstalledAppsList("UserInstalledAppsList.json");
            break;
        case 4:
            extractor.extractMediaStoreDb("mediastore.json");    
            break;

        case 5:
            extractor.extractCallLogs("calllogs.txt");
            break;

        case 6:
            extractor.extractSMS("sms.txt");
            break;

        case 7:
            extractor.pullMedia("media");
            break;

        case 0:
            std::cout << "Exiting...\n";
            return 0;

        default:
            std::cout << "Invalid option.\n";
        }
    }

    return 0;
}