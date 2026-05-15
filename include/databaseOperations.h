#pragma once

#include "SQLiteCpp/SQLiteCpp.h"
#include "json.h"



class dbOperations {

private:
    static void storeJsonToSQLite(SQLite::Database& db,
        const std::string& tableName,
        const nlohmann::json& jsonArray);

public:

    static int storeTablesToDbFile(const std::filesystem::path& pathToOutputDbFile,
                           const std::vector<std::string>& sqlTableNames,
                           const std::vector<nlohmann::json>& sqlTables);
};

