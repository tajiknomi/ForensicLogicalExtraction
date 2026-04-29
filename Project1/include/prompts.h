void showMenu(void) {
    std::cout << "\n==== Android Forensic Tool (Windows) ====\n";
    std::cout << "1. List Devices\n";
    std::cout << "2. Extract Device Info\n";
    std::cout << "3. Extract Installed Apps list\n";
    std::cout << "4. Extract MediaStore DB (JSON)\n";
    std::cout << "5. Extract Call Logs\n";
    std::cout << "6. Extract SMS\n";
    std::cout << "7. Pull Media\n";
    std::cout << "0. Exit\n";
    std::cout << "Select option: ";
}

void showAndroidAdbSetup(void) {
    std::cout << "\nEnable USB Debugging:\n"
        << "Settings > About Phone > Tap 'Build Number' 7 times\n"
        << "Then go to Developer Options and enable USB Debugging.\n" << std::endl;
}