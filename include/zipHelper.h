#pragma once
#include <string>

namespace ZipHelper {

    /**
     * @brief Zips a single file into a zip archive (creates the zip if it doesn't exist).
     * @param filePath Path to the file to zip.
     * @param zipPath Path to the zip file to create/append.
     * @param relativePath Optional relative path inside the zip (for folders).
     * @return true on success, false on failure.
     */
    bool addFileToZip(const std::string& filePath, const std::string& zipPath, const std::string& relativePath = "");

    /**
     * @brief Zips all files in a folder recursively into a single zip archive.
     * @param folderPath Path to the folder to zip.
     * @param zipPath Path to the zip file to create.
     * @return true on success, false on failure.
     */
    bool zipFolderRecursively(const std::string& folderPath, const std::string& zipPath);

}