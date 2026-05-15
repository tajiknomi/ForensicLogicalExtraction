#include "databaseOperations.h"
#include <iostream>
#include <sstream>


// ===================================== PRIVATE =====================================
void dbOperations::storeJsonToSQLite(SQLite::Database& db,
    const std::string& tableName,
    const nlohmann::json& jsonArray)
{
    if (jsonArray.empty()) {
        std::cout << "[*] No data to insert into table: " << tableName << std::endl;
        return;
    }

    try {
        // 1. Determine columns from the first row
        std::vector<std::string> columns;
        for (auto it = jsonArray[0].begin(); it != jsonArray[0].end(); ++it) {
            columns.push_back(it.key());
        }

        // 2. Create table if not exists (all columns as TEXT)
        std::stringstream createTableSS;
        createTableSS << "CREATE TABLE IF NOT EXISTS " << tableName << " (";
        for (size_t i = 0; i < columns.size(); ++i) {
            createTableSS << columns[i] << " TEXT";
            if (i + 1 < columns.size()) createTableSS << ", ";
        }
        createTableSS << ");";
        db.exec(createTableSS.str());

        // 3. Prepare INSERT statement with placeholders
        std::stringstream insertSS;
        insertSS << "INSERT INTO " << tableName << " (";
        for (size_t i = 0; i < columns.size(); ++i) {
            insertSS << columns[i];
            if (i + 1 < columns.size()) insertSS << ", ";
        }
        insertSS << ") VALUES (";
        for (size_t i = 0; i < columns.size(); ++i) {
            insertSS << "?";
            if (i + 1 < columns.size()) insertSS << ", ";
        }
        insertSS << ");";

        SQLite::Transaction transaction(db);

        for (const auto& row : jsonArray) {
            SQLite::Statement query(db, insertSS.str());
            int idx = 1;
            for (const auto& col : columns) {
                if (row.contains(col)) {
                    query.bind(idx, row.at(col).is_null() ? "" : row.at(col).get<std::string>());
                }
                else {
                    query.bind(idx, "");
                }
                ++idx;
            }
            query.exec();
        }

        transaction.commit();
        std::cout << "[+] Inserted " << jsonArray.size() << " rows into table: " << tableName << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "SQLite Error in table '" << tableName << "': " << e.what() << std::endl;
    }
}




// ===================================== PUBLIC =====================================

int dbOperations::storeTablesToDbFile(const std::filesystem::path& pathToOutputDbFile,
    const std::vector<std::string>& sqlTableNames,
    const std::vector<nlohmann::json>& sqlTables) {

    if (sqlTableNames.size() != sqlTableNames.size()) {
        std::cerr << "Number of tables doesn't match the Number of provided table names";
        return -1;
    }
    SQLite::Database db(pathToOutputDbFile, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    unsigned int index{ 0 };

    for (auto table : sqlTables) {
        storeJsonToSQLite(db, sqlTableNames[index], table);
        index++;
    }
    return 0;
}

