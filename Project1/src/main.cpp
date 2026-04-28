#include <iostream>
#include "adb.h"
#include "extractor.h"
#include "prompts.h"


int main() {
    ADB adb("adb"); // or full path: "C:\\platform-tools\\adb.exe"
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