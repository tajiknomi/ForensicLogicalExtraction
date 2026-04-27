#include <iostream>
#include "adb.h"
#include "extractor.h"

void showMenu() {
    std::cout << "\n==== Android Forensic Tool (Windows) ====\n";
    std::cout << "1. List Devices\n";
    std::cout << "2. Extract SMS\n";
    std::cout << "3. Extract Call Logs\n";
    std::cout << "4. Pull Media (DCIM)\n";
    std::cout << "5. Extract MediaStore DB (JSON)\n";
    std::cout << "6. Extract Device Info\n";
    std::cout << "0. Exit\n";
    std::cout << "Select option: ";
}

int main() {
    ADB adb("adb"); // or full path: "C:\\platform-tools\\adb.exe"
    ForensicExtractor extractor(adb);

    int choice;

    while (true) {
        showMenu();
        std::cin >> choice;

        switch (choice) {
        case 1:
            extractor.listDevices();
            break;

        case 2:
            extractor.extractSMS("sms.txt");
            break;

        case 3:
            extractor.extractCallLogs("calllogs.txt");
            break;

        case 4:
            extractor.pullMedia("media");
            break;

        case 5:
            extractor.extractMediaStoreDb("mediastore.json");
            break;

        case 6:
            extractor.extractDeviceInfo("device_info.json");
            break;

        case 0:
            std::cout << "Exiting...\n";
            return 0;

        default:
            std::cout << "Invalid option.\n";
        }
    }
}