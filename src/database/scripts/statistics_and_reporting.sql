-- Statistics and Reporting SQL Queries
-- This file contains all SQL queries for statistics, reporting, and complex operations

-- =============================================================================
-- BASIC STATISTICS
-- =============================================================================

-- Count total scanned files
-- Used in: getProcessingStatistics()
SELECT COUNT(*) FROM scanned_files;

-- Count successfully processed files
-- Used in: getProcessingStatistics()
SELECT COUNT(DISTINCT file_path)
FROM media_processing_results
WHERE
    success = 1;

-- Count files with duplicates
-- Used in: getProcessingStatistics()
SELECT COUNT(*)
FROM scanned_files
WHERE (
        links_fast IS NOT NULL
        AND links_fast != ''
    )
    OR (
        links_balanced IS NOT NULL
        AND links_balanced != ''
    )
    OR (
        links_quality IS NOT NULL
        AND links_quality != ''
    );

-- Count files with processing errors
-- Used in: getProcessingStatistics()
SELECT COUNT(DISTINCT file_path)
FROM media_processing_results
WHERE
    success = 0;

-- =============================================================================
-- TRANCODING STATISTICS
-- =============================================================================

-- Count files in transcoding queue
-- Used in: getTranscodingStatistics()
SELECT COUNT(*) FROM cache_map WHERE status = 0;

-- Count successfully transcoded files
-- Used in: getTranscodingStatistics()
SELECT COUNT(*) FROM cache_map WHERE status = 2;

-- =============================================================================
-- DATABASE SCHEMA OPERATIONS
-- =============================================================================

-- Check if table exists
-- Used in: tableExists()
SELECT name FROM sqlite_master WHERE type = 'table' AND name = ?;

-- Get all table names
-- Used in: getAllTableNames()
SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY name;

-- Get all rows from a table
-- Used in: getAllTableRows()
SELECT * FROM ? ORDER BY rowid;

-- Check if table exists (alternative)
-- Used in: tableExists()
SELECT name FROM sqlite_master WHERE type = 'table' AND name = ?;

-- =============================================================================
-- DYNAMIC FIELD OPERATIONS
-- =============================================================================

-- Update dynamic field in scanned_files
-- Used in: updateDynamicField()
UPDATE scanned_files SET ? = ? WHERE file_path = ?;

-- Get dynamic field value from scanned_files
-- Used in: getDynamicFieldValue()
SELECT ? FROM scanned_files WHERE file_path = ?;