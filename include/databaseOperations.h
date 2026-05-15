#pragma once

#include "SQLiteCpp/SQLiteCpp.h"
#include "json.h"



class dbOperations {

private:
    static void storeJsonToSQLite(SQLite::Database& db,
        const std::string& tableName,
        const nlohmann::json& jsonArray);

public:

    static int storeTablesToDbFile(std::filesystem::path pathToOutputDbFile,
                           std::vector<std::string> sqlTableNames,
                           std::vector< nlohmann::json > sqlTables);
};

