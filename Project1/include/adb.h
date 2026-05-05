#pragma once

#include <string>

class ADB {

private:
    std::wstring adbPath;

public:
    ADB(const std::wstring& adbPath = L"adb");
    std::wstring exec(const std::wstring& command);
    static std::wstring FindAdbPath(void);
};