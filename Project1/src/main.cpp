#include <iostream>
#include "adb.h"
#include "extractor.h"

void showMenu(void) {
    std::cout << "\n==== Android Forensic Tool (Windows) ====\n";
    std::cout << "1. List Devices\n";
    std::cout << "2. Extract Device Info\n";
    std::cout << "3. Extract Installed Apps list\n";
    std::cout << "4. Extract MediaStore DB (JSON)\n";
    std::cout << "5. Extract Call Logs\n";
    std::cout << "6. Extract SMS\n";
    std::cout << "7. Pull Media (DCIM)\n";
    std::cout << "0. Exit\n";
    std::cout << "Select option: ";
}

void showAndroidAdbSetup(void) {
    std::cout << "\nEnable USB Debugging:\n"
        << "Settings > About Phone > Tap 'Build Number' 7 times\n"
        << "Then go to Developer Options and enable USB Debugging.\n" << std::endl;
}

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