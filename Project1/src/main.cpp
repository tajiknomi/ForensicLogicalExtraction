#include <iostream>
#include "adb.h"
#include "extractor.h"
#include "prompts.h"


int main() {

    std::wstring pathToAdb = ADB::FindAdbPath();
    if (pathToAdb.empty()) {
        std::cerr << "Couldn't found adb.exe on your system, try adding adb.exe to you PATH environment variable" << std::endl;
        return -1;
    }
    ADB adb(pathToAdb);
    ForensicExtractor extractor(adb);
    showAndroidAdbSetup();
    int choice;

    while (true) {
        showMenu();
        std::cin >> choice;
        switch (choice) {
        case 1:
            extractor.showConnectedDevices(); break;

        case 2:
            extractor.extractDeviceInfo(L"device_info.json");  break;

        case 3:
            extractor.extractUserInstalledAppsList(L"UserInstalledAppsList.json"); break;

        case 4:
            extractor.extractMediaStoreDb(L"mediastore.json");    break;
 
        case 5:
            extractor.extractCallLogs(L"calllogs.txt"); break;     

        case 6:
            extractor.extractSMS(L"sms.txt"); break;
            
        case 7:
            extractor.pullMedia(L"media"); break;          

        case 0:
            std::cout << "Exiting...\n"; return 0;
            
        default:
            std::cout << "Invalid option.\n";
        }
    }

    return 0;
}