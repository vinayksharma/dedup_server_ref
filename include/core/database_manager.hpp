#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include "media_processor.hpp"

/**
 * @brief SQLite database manager for storing media processing results
 */
class DatabaseManager
{
public:
    /**
     * @brief Constructor - initializes database connection
     * @param db_path Path to SQLite database file
     */
    explicit DatabaseManager(const std::string &db_path);

    /**
     * @brief Destructor - closes database connection
     */
    ~DatabaseManager();

    /**
     * @brief Initialize database tables
     * @return true if successful, false otherwise
     */
    bool initializeTables();

    /**
     * @brief Store media processing result
     * @param file_path Original file path
     * @param mode Processing mode used
     * @param result Processing result
     * @return true if successful, false otherwise
     */
    bool storeProcessingResult(const std::string &file_path,
                               DedupMode mode,
                               const ProcessingResult &result);

    /**
     * @brief Get processing results for a file
     * @param file_path File path to query
     * @return Vector of processing results
     */
    std::vector<ProcessingResult> getProcessingResults(const std::string &file_path);

    /**
     * @brief Get all processing results
     * @return Vector of all processing results
     */
    std::vector<std::pair<std::string, ProcessingResult>> getAllProcessingResults();

    /**
     * @brief Clear all processing results
     * @return true if successful, false otherwise
     */
    bool clearAllResults();

private:
    sqlite3 *db_;
    std::string db_path_;

    /**
     * @brief Execute SQL statement
     * @param sql SQL statement to execute
     * @return true if successful, false otherwise
     */
    bool executeStatement(const std::string &sql);

    /**
     * @brief Convert ProcessingResult to JSON string for storage
     * @param result Processing result to convert
     * @return JSON string representation
     */
    std::string resultToJson(const ProcessingResult &result);

    /**
     * @brief Convert JSON string back to ProcessingResult
     * @param json_str JSON string to convert
     * @return ProcessingResult object
     */
    ProcessingResult jsonToResult(const std::string &json_str);
};