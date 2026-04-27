#include "adb.h"
#include <cstdio>
#include <array>
#include <stdexcept>

ADB::ADB(const std::string& path) : adbPath(path) {}

std::string ADB::exec(const std::string& command) {
    std::string fullCommand = adbPath + " " + command;

    std::array<char, 256> buffer;
    std::string result;

    FILE* pipe = _popen(fullCommand.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("Failed to execute ADB command");
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    _pclose(pipe);

    return result;
}