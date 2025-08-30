-- Media Processing Operations SQL Queries
-- This file contains all SQL queries for media_processing_results table operations

-- =============================================================================
-- PROCESSING RESULTS OPERATIONS
-- =============================================================================

-- Clear all processing results
-- Used in: clearAllResults()
DELETE FROM media_processing_results;

-- Get file ID by path
-- Used in: getFileId()
SELECT id FROM scanned_files WHERE file_path = ?;

-- Get maximum ID from processing results
-- Used in: getNextProcessingResultId()
SELECT IFNULL(MAX(id), 0) FROM media_processing_results;

-- =============================================================================
-- PROCESSING RESULTS QUERIES
-- =============================================================================

-- Get processing results by file path and processing mode
-- Used in: getProcessingResults()
SELECT id, file_path, artifact_hash
FROM media_processing_results
WHERE
    file_path = ?
    AND processing_mode = ?;

-- Get all processing results for a file
-- Used in: getAllProcessingResults()
SELECT file_path, artifact_hash
FROM media_processing_results
WHERE
    file_path = ?;

-- Get file paths that have been processed
-- Used in: getProcessedFilePaths()
SELECT sf.file_path
FROM
    media_processing_results mpr
    JOIN scanned_files sf ON sf.file_path = mpr.file_path
WHERE
    mpr.success = 1;

-- =============================================================================
-- PROCESSING STATUS CHECKS
-- =============================================================================

-- Check if file has been processed in FAST mode
-- Used in: getFilesNeedingProcessing()
NOT EXISTS (
    SELECT 1
    FROM media_processing_results mpr
    WHERE
        mpr.file_path = sf.file_path
        AND mpr.processing_mode = 'FAST'
)

-- Check if file has been processed in BALANCED mode
-- Used in: getFilesNeedingProcessing()
OR NOT EXISTS (
    SELECT 1
    FROM media_processing_results mpr
    WHERE
        mpr.file_path = sf.file_path
        AND mpr.processing_mode = 'BALANCED'
)

-- Check if file has been processed in QUALITY mode
-- Used in: getFilesNeedingProcessing()
OR NOT EXISTS (
    SELECT 1
    FROM media_processing_results mpr
    WHERE
        mpr.file_path = sf.file_path
        AND mpr.processing_mode = 'QUALITY'
)

-- =============================================================================
-- STATISTICS QUERIES
-- =============================================================================

-- Count successfully processed files
-- Used in: getProcessingStatistics()
SELECT COUNT(DISTINCT file_path)
FROM media_processing_results
WHERE
    success = 1;

-- Count files with processing errors
-- Used in: getProcessingStatistics()
SELECT COUNT(DISTINCT file_path)
FROM media_processing_results
WHERE
    success = 0;