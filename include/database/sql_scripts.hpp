#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace DatabaseScripts
{
    // Function to get the correct script path based on current working directory
    inline std::string getScriptPath(const std::string &filename)
    {
        std::filesystem::path cwd = std::filesystem::current_path();

        // Try different possible paths
        std::vector<std::string> possible_paths = {
            "../src/database/scripts/" + filename,          // From build directory
            "src/database/scripts/" + filename,             // From project root
            "../../../src/database/scripts/" + filename,    // From deep test directory
            "../../../../src/database/scripts/" + filename, // From very deep test directory
        };

        for (const auto &path : possible_paths)
        {
            if (std::filesystem::exists(path))
            {
                return path;
            }
        }

        // If none found, return the most likely path and let it fail with a clear error
        return "../src/database/scripts/" + filename;
    }

    // SQL script file paths
    const std::string CREATE_TABLES_SCRIPT = getScriptPath("create_tables.sql");
    const std::string CREATE_TRIGGERS_SCRIPT = getScriptPath("create_triggers.sql");
    const std::string CREATE_INDEXES_SCRIPT = getScriptPath("create_indexes.sql");
    const std::string INIT_DATABASE_SCRIPT = getScriptPath("init_database.sql");

    // Individual table creation scripts
    const std::string MEDIA_PROCESSING_RESULTS_TABLE_SCRIPT = getScriptPath("create_tables.sql");
    const std::string SCANNED_FILES_TABLE_SCRIPT = getScriptPath("create_tables.sql");
    const std::string USER_INPUTS_TABLE_SCRIPT = getScriptPath("create_tables.sql");
    const std::string CACHE_MAP_TABLE_SCRIPT = getScriptPath("create_tables.sql");
    const std::string FLAGS_TABLE_SCRIPT = getScriptPath("create_tables.sql");
}
