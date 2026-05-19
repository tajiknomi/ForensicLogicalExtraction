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

    std::wstring fullCommand = adbPath + L" " + command;

    std::array<char, 256> buffer;
    std::string result;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hRead = NULL;
    HANDLE hWrite = NULL;

    if (!CreatePipe(&hRead, &hWrite, &saAttr, 0)) {
        throw std::runtime_error("Failed to create pipe");
    }

    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.hStdInput = NULL;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    // IMPORTANT: CreateProcessW requires writable buffer
    std::wstring cmdLine = fullCommand;

    BOOL success = CreateProcessW(
        NULL,
        cmdLine.data(),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    CloseHandle(hWrite);

    if (!success) {
        CloseHandle(hRead);
        throw std::runtime_error("Failed to execute ADB command");
    }

    DWORD bytesRead = 0;
    while (ReadFile(hRead, buffer.data(), (DWORD)buffer.size(), &bytesRead, NULL) && bytesRead > 0) {
        result.append(buffer.data(), bytesRead);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return StringUtils::convertUTF8ToWString(result);
}

//std::wstring ADB::exec(const std::wstring& command) {
// 
//    std::string narrowAdbPath = StringUtils::convertWStringToUTF8(adbPath);
//    std::string narrowCommand = StringUtils::convertWStringToUTF8(command);
//    std::string fullCommand = narrowAdbPath + " " + narrowCommand;
//
//    std::array<char, 256> buffer;
//    std::string result;
//
//    FILE* pipe = _popen(fullCommand.c_str(), "r");
//    if (!pipe) {
//        throw std::runtime_error("Failed to execute ADB command");
//    }
//    // Read output as UTF-8
//    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
//        result += buffer.data();
//    }
//    _pclose(pipe);
//    return StringUtils::convertUTF8ToWString(result);
//}