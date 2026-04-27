#pragma once

#include <string>

class ADB {
public:
    ADB(const std::string& adbPath = "adb");

    // Execute adb command and return output
    std::string exec(const std::string& command);

private:
    std::string adbPath;
};