#include "adb.h"
#include <cstdio>
#include <array>
#include <Windows.h>
#include <stdexcept>
#include "stringUtil.h"

ADB::ADB(const std::wstring& path) : adbPath(path) {}


std::wstring ADB::FindAdbPath(void)
{
    wchar_t buffer[MAX_PATH];

    // Search in current directory + PATH automatically
    unsigned long result = SearchPathW(
        nullptr,        // search in standard locations (current dir + PATH)
        L"adb.exe",      // file to find
        nullptr,        // no extension
        MAX_PATH,
        buffer,
        nullptr
    );
    if (result > 0 && result < MAX_PATH) {
        return std::wstring(buffer);
    }
    return std::wstring(L"");
}

std::wstring ADB::exec(const std::wstring& command) {
 
    std::string narrowAdbPath = StringUtils::convertWStringToUTF8(adbPath);
    std::string narrowCommand = StringUtils::convertWStringToUTF8(command);
    std::string fullCommand = narrowAdbPath + " " + narrowCommand;

    std::array<char, 256> buffer;
    std::string result;

    FILE* pipe = _popen(fullCommand.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute ADB command");
    }
    // Read output as UTF-8
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }
    _pclose(pipe);
    return StringUtils::convertUTF8ToWString(result);
}