#include "core/database_manager.hpp"
#include "logging/logger.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

DatabaseManager::DatabaseManager(const std::string &db_path)
    : db_(nullptr), db_path_(db_path)
{
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK)
    {
        Logger::error("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        sqlite3_close(db_);
        db_ = nullptr;
    }
    else
    {
        Logger::info("Database opened successfully: " + db_path);
    }
}

DatabaseManager::~DatabaseManager()
{
    if (db_)
    {
        sqlite3_close(db_);
        Logger::info("Database connection closed");
    }
}

bool DatabaseManager::initializeTables()
{
    if (!db_)
    {
        Logger::error("Database not initialized");
        return false;
    }

    const std::string create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS media_processing_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL,
            processing_mode TEXT NOT NULL,
            success BOOLEAN NOT NULL,
            error_message TEXT,
            artifact_format TEXT,
            artifact_hash TEXT,
            artifact_confidence REAL,
            artifact_metadata TEXT,
            artifact_data BLOB,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";

    return executeStatement(create_table_sql);
}

bool DatabaseManager::storeProcessingResult(const std::string &file_path,
                                            DedupMode mode,
                                            const ProcessingResult &result)
{
    if (!db_)
    {
        Logger::error("Database not initialized");
        return false;
    }

    const std::string insert_sql = R"(
        INSERT INTO media_processing_results 
        (file_path, processing_mode, success, error_message, 
         artifact_format, artifact_hash, artifact_confidence, 
         artifact_metadata, artifact_data)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db_, insert_sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        Logger::error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    // Bind parameters
    sqlite3_bind_text(stmt, 1, file_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, DedupModes::getModeName(mode).c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, result.success ? 1 : 0);

    if (result.error_message.empty())
    {
        sqlite3_bind_null(stmt, 4);
    }
    else
    {
        sqlite3_bind_text(stmt, 4, result.error_message.c_str(), -1, SQLITE_STATIC);
    }

    if (result.artifact.format.empty())
    {
        sqlite3_bind_null(stmt, 5);
    }
    else
    {
        sqlite3_bind_text(stmt, 5, result.artifact.format.c_str(), -1, SQLITE_STATIC);
    }

    if (result.artifact.hash.empty())
    {
        sqlite3_bind_null(stmt, 6);
    }
    else
    {
        sqlite3_bind_text(stmt, 6, result.artifact.hash.c_str(), -1, SQLITE_STATIC);
    }

    sqlite3_bind_double(stmt, 7, result.artifact.confidence);

    if (result.artifact.metadata.empty())
    {
        sqlite3_bind_null(stmt, 8);
    }
    else
    {
        sqlite3_bind_text(stmt, 8, result.artifact.metadata.c_str(), -1, SQLITE_STATIC);
    }

    if (result.artifact.data.empty())
    {
        sqlite3_bind_null(stmt, 9);
    }
    else
    {
        sqlite3_bind_blob(stmt, 9, result.artifact.data.data(),
                          static_cast<int>(result.artifact.data.size()), SQLITE_STATIC);
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        Logger::error("Failed to insert result: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    Logger::info("Stored processing result for: " + file_path);
    return true;
}

std::vector<ProcessingResult> DatabaseManager::getProcessingResults(const std::string &file_path)
{
    std::vector<ProcessingResult> results;

    if (!db_)
    {
        Logger::error("Database not initialized");
        return results;
    }

    const std::string select_sql = R"(
        SELECT processing_mode, success, error_message, 
               artifact_format, artifact_hash, artifact_confidence, 
               artifact_metadata, artifact_data
        FROM media_processing_results 
        WHERE file_path = ?
        ORDER BY created_at DESC
    )";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db_, select_sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        Logger::error("Failed to prepare select statement: " + std::string(sqlite3_errmsg(db_)));
        return results;
    }

    sqlite3_bind_text(stmt, 1, file_path.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ProcessingResult result;

        result.success = sqlite3_column_int(stmt, 1) != 0;

        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
        {
            result.error_message = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        }

        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
        {
            result.artifact.format = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        }

        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
        {
            result.artifact.hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        }

        result.artifact.confidence = sqlite3_column_double(stmt, 5);

        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL)
        {
            result.artifact.metadata = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        }

        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL)
        {
            const void *blob_data = sqlite3_column_blob(stmt, 7);
            int blob_size = sqlite3_column_bytes(stmt, 7);
            result.artifact.data.assign(
                static_cast<const uint8_t *>(blob_data),
                static_cast<const uint8_t *>(blob_data) + blob_size);
        }

        results.push_back(result);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<std::pair<std::string, ProcessingResult>> DatabaseManager::getAllProcessingResults()
{
    std::vector<std::pair<std::string, ProcessingResult>> results;

    if (!db_)
    {
        Logger::error("Database not initialized");
        return results;
    }

    const std::string select_sql = R"(
        SELECT file_path, processing_mode, success, error_message, 
               artifact_format, artifact_hash, artifact_confidence, 
               artifact_metadata, artifact_data
        FROM media_processing_results 
        ORDER BY created_at DESC
    )";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db_, select_sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        Logger::error("Failed to prepare select statement: " + std::string(sqlite3_errmsg(db_)));
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string file_path = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        ProcessingResult result;

        result.success = sqlite3_column_int(stmt, 2) != 0;

        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
        {
            result.error_message = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        }

        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
        {
            result.artifact.format = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        }

        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL)
        {
            result.artifact.hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        }

        result.artifact.confidence = sqlite3_column_double(stmt, 6);

        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL)
        {
            result.artifact.metadata = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
        }

        if (sqlite3_column_type(stmt, 8) != SQLITE_NULL)
        {
            const void *blob_data = sqlite3_column_blob(stmt, 8);
            int blob_size = sqlite3_column_bytes(stmt, 8);
            result.artifact.data.assign(
                static_cast<const uint8_t *>(blob_data),
                static_cast<const uint8_t *>(blob_data) + blob_size);
        }

        results.emplace_back(file_path, result);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool DatabaseManager::clearAllResults()
{
    if (!db_)
    {
        Logger::error("Database not initialized");
        return false;
    }

    const std::string delete_sql = "DELETE FROM media_processing_results";
    return executeStatement(delete_sql);
}

bool DatabaseManager::executeStatement(const std::string &sql)
{
    if (!db_)
    {
        Logger::error("Database not initialized");
        return false;
    }

    char *err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        Logger::error("SQL execution failed: " + std::string(err_msg));
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

std::string DatabaseManager::resultToJson(const ProcessingResult &result)
{
    json j;
    j["success"] = result.success;
    j["error_message"] = result.error_message;
    j["artifact"]["format"] = result.artifact.format;
    j["artifact"]["hash"] = result.artifact.hash;
    j["artifact"]["confidence"] = result.artifact.confidence;
    j["artifact"]["metadata"] = result.artifact.metadata;

    // Convert binary data to hex string
    std::stringstream ss;
    for (uint8_t byte : result.artifact.data)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    j["artifact"]["data"] = ss.str();

    return j.dump();
}

ProcessingResult DatabaseManager::jsonToResult(const std::string &json_str)
{
    ProcessingResult result;

    try
    {
        json j = json::parse(json_str);
        result.success = j["success"];
        result.error_message = j["error_message"];
        result.artifact.format = j["artifact"]["format"];
        result.artifact.hash = j["artifact"]["hash"];
        result.artifact.confidence = j["artifact"]["confidence"];
        result.artifact.metadata = j["artifact"]["metadata"];

        // Convert hex string back to binary data
        std::string hex_data = j["artifact"]["data"];
        for (size_t i = 0; i < hex_data.length(); i += 2)
        {
            std::string byte_str = hex_data.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
            result.artifact.data.push_back(byte);
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Failed to parse JSON result: " + std::string(e.what()));
    }

    return result;
}