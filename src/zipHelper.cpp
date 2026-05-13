#include "ZipHelper.h"
#include "zip.h"
#include "ioapi.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace ZipHelper {

    // Helper: read file into memory
    static std::vector<char> readFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            std::cerr << "[-] Failed to open file: " << path << std::endl;
            return {};
        }
        return std::vector<char>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    bool addFileToZip(const std::string& filePath, const std::string& zipPath, const std::string& relativePath) {
        std::vector<char> data = readFile(filePath);
        if (data.empty()) {
            return false;
        }

        // Choose mode: create new if zip doesn't exist, otherwise append
        int mode = std::filesystem::exists(zipPath) ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE;

        zipFile zf = zipOpen(zipPath.c_str(), mode);
        if (!zf) {
            std::cerr << "[-] Failed to create/open zip file: " << zipPath << std::endl;
            return false;
        }

        std::string pathInZip = relativePath.empty() ? fs::path(filePath).filename().string() : relativePath;

        zip_fileinfo zi = {};
        int err = zipOpenNewFileInZip(zf, pathInZip.c_str(), &zi,
            nullptr, 0, nullptr, 0, nullptr,
            Z_DEFLATED, Z_DEFAULT_COMPRESSION);
        if (err != ZIP_OK) {
            std::cerr << "[-] Failed to add file to zip: " << pathInZip << std::endl;
            zipClose(zf, nullptr);
            return false;
        }

        err = zipWriteInFileInZip(zf, data.data(), static_cast<unsigned int>(data.size()));
        if (err != ZIP_OK) {
            std::cerr << "[-] Failed to write file data: " << pathInZip << std::endl;
            zipCloseFileInZip(zf);
            zipClose(zf, nullptr);
            return false;
        }

        zipCloseFileInZip(zf);
        zipClose(zf, nullptr);
        return true;
    }

    bool zipFolderRecursively(const std::string& folderPath, const std::string& zipPath) {
        try {
            for (auto& entry : fs::recursive_directory_iterator(folderPath)) {
                //std::cout << entry << std::endl;
                if (!entry.is_regular_file()) {
                    continue;
                }

                std::string relativePath = fs::relative(entry.path(), folderPath).generic_string();
                if (!ZipHelper::addFileToZip(entry.path().string(), zipPath/*, relativePath*/)) {
                    std::cerr << "[-] Failed to add file: " << entry.path() << std::endl;
                }
            }
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "[-] Filesystem error: " << e.what() << std::endl;
            return false;
        }

        return true;
    }

}