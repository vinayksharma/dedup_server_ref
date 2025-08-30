-- Scanned Files Operations SQL Queries
-- This file contains all SQL queries for scanned_files table operations

-- =============================================================================
-- FILE METADATA OPERATIONS
-- =============================================================================

-- Get file metadata and processing status
-- Used in: getFileMetadata()
SELECT
    file_metadata,
    processed_fast,
    processed_balanced,
    processed_quality
FROM scanned_files
WHERE
    file_path = ?;

-- Update file metadata and reset processing flags
-- Used in: updateFileMetadata()
UPDATE scanned_files
SET
    file_metadata = ?,
    processed_fast = 0,
    processed_balanced = 0,
    processed_quality = 0,
    created_at = CURRENT_TIMESTAMP
WHERE
    file_path = ?;

-- Insert new scanned file
-- Used in: storeScannedFile()
INSERT INTO
    scanned_files (
        file_path,
        file_name,
        relative_path,
        share_name,
        is_network_file,
        file_metadata
    )
VALUES (?, ?, ?, ?, ?, ?);

-- Get all scanned files ordered by creation date
-- Used in: getAllScannedFiles()
SELECT file_path, file_name
FROM scanned_files
ORDER BY created_at DESC;

-- =============================================================================
-- FILE EXISTENCE CHECKS
-- =============================================================================

-- Check if file exists by relative path or full path
-- Used in: fileExists()
SELECT COUNT(*)
FROM scanned_files
WHERE
    relative_path = ?
    OR file_path = ?;

-- Check if file exists by full path only
-- Used in: fileExists()
SELECT COUNT(*) FROM scanned_files WHERE file_path = ?;

-- Check if file exists by full path only (alternative)
-- Used in: fileExists()
SELECT COUNT(*) FROM scanned_files WHERE file_path = ?;

-- =============================================================================
-- PROCESSING STATUS OPERATIONS
-- =============================================================================

-- Get files needing processing for FAST mode
-- Used in: getFilesNeedingProcessing()
SELECT file_path, file_name
FROM scanned_files
WHERE
    processed_fast = 0
    AND (?)
ORDER BY created_at DESC
LIMIT ?;

-- Get files needing processing for BALANCED mode
-- Used in: getFilesNeedingProcessing()
SELECT file_path, file_name
FROM scanned_files
WHERE
    processed_balanced = 0
    AND (?)
ORDER BY created_at DESC
LIMIT ?;

-- Get files needing processing for QUALITY mode
-- Used in: getFilesNeedingProcessing()
SELECT file_path, file_name
FROM scanned_files
WHERE
    processed_quality = 0
    AND (?)
ORDER BY created_at DESC
LIMIT ?;

-- =============================================================================
-- HASH OPERATIONS
-- =============================================================================

-- Update file hash
-- Used in: updateFileHash()
UPDATE scanned_files SET hash = ? WHERE file_path = ?;

-- Update file hash (alternative)
-- Used in: updateFileHash()
UPDATE scanned_files SET hash = ? WHERE file_path = ?;

-- =============================================================================
-- USER INPUTS OPERATIONS
-- =============================================================================

-- Get latest input value by type
-- Used in: getUserInput()
SELECT input_value
FROM user_inputs
WHERE
    input_type = ?
ORDER BY created_at DESC;

-- Get all user inputs
-- Used in: getAllUserInputs()
SELECT input_type, input_value
FROM user_inputs
ORDER BY created_at DESC;

-- =============================================================================
-- PROCESSING STATUS UPDATES
-- =============================================================================

-- Mark file as processed successfully (FAST)
-- Used in: markFileAsProcessed()
UPDATE scanned_files
SET
    processed_fast = 1
WHERE
    file_path = ?
    AND (
        processed_fast = -1
        OR processed_fast = 0
    );

-- Mark file as processed successfully (BALANCED)
-- Used in: markFileAsProcessed()
UPDATE scanned_files
SET
    processed_balanced = 1
WHERE
    file_path = ?
    AND (
        processed_balanced = -1
        OR processed_balanced = 0
    );

-- Mark file as processed successfully (QUALITY)
-- Used in: markFileAsProcessed()
UPDATE scanned_files
SET
    processed_quality = 1
WHERE
    file_path = ?
    AND (
        processed_quality = -1
        OR processed_quality = 0
    );

-- Mark file as processing (FAST)
-- Used in: markFileAsProcessing()
UPDATE scanned_files
SET
    processed_fast = 0
WHERE
    file_path = ?
    AND processed_fast = -1;

-- Mark file as processing (BALANCED)
-- Used in: markFileAsProcessing()
UPDATE scanned_files
SET
    processed_balanced = 0
WHERE
    file_path = ?
    AND processed_balanced = -1;

-- Mark file as processing (QUALITY)
-- Used in: markFileAsProcessing()
UPDATE scanned_files
SET
    processed_quality = 0
WHERE
    file_path = ?
    AND processed_quality = -1;

-- Mark file as failed (FAST)
-- Used in: markFileAsFailed()
UPDATE scanned_files SET processed_fast = 2 WHERE file_path = ?;

-- Mark file as failed (BALANCED)
-- Used in: markFileAsFailed()
UPDATE scanned_files
SET
    processed_balanced = 2
WHERE
    file_path = ?;

-- Mark file as failed (QUALITY)
-- Used in: markFileAsFailed()
UPDATE scanned_files SET processed_quality = 2 WHERE file_path = ?;

-- Mark file as skipped (FAST)
-- Used in: markFileAsSkipped()
UPDATE scanned_files SET processed_fast = 3 WHERE file_path = ?;

-- Mark file as skipped (BALANCED)
-- Used in: markFileAsSkipped()
UPDATE scanned_files
SET
    processed_balanced = 3
WHERE
    file_path = ?;

-- Mark file as skipped (QUALITY)
-- Used in: markFileAsSkipped()
UPDATE scanned_files SET processed_quality = 3 WHERE file_path = ?;

-- Mark file as cancelled (FAST)
-- Used in: markFileAsCancelled()
UPDATE scanned_files SET processed_fast = 4 WHERE file_path = ?;

-- Mark file as cancelled (BALANCED)
-- Used in: markFileAsCancelled()
UPDATE scanned_files
SET
    processed_balanced = 4
WHERE
    file_path = ?;

-- Mark file as cancelled (QUALITY)
-- Used in: markFileAsCancelled()
UPDATE scanned_files SET processed_quality = 4 WHERE file_path = ?;

-- =============================================================================
-- PROCESSING STATUS QUERIES
-- =============================================================================

-- Get files by processing status (FAST)
-- Used in: getFilesByProcessingStatus()
SELECT file_path FROM scanned_files WHERE processed_fast = ?;

-- Get files by processing status (BALANCED)
-- Used in: getFilesByProcessingStatus()
SELECT file_path FROM scanned_files WHERE processed_balanced = ?;

-- Get files by processing status (QUALITY)
-- Used in: getFilesByProcessingStatus()
SELECT file_path FROM scanned_files WHERE processed_quality = ?;

-- Get processing status for file (FAST)
-- Used in: getProcessingStatus()
SELECT processed_fast FROM scanned_files WHERE file_path = ?;

-- Get processing status for file (BALANCED)
-- Used in: getProcessingStatus()
SELECT processed_balanced FROM scanned_files WHERE file_path = ?;

-- Get processing status for file (QUALITY)
-- Used in: getProcessingStatus()
SELECT processed_quality FROM scanned_files WHERE file_path = ?;

-- =============================================================================
-- PROCESSING STATUS RESETS
-- =============================================================================

-- Reset processing status to pending (FAST)
-- Used in: resetProcessingStatus()
UPDATE scanned_files
SET
    processed_fast = -1
WHERE
    file_path = ?
    AND processed_fast = 0;

-- Reset processing status to pending (BALANCED)
-- Used in: resetProcessingStatus()
UPDATE scanned_files
SET
    processed_balanced = -1
WHERE
    file_path = ?
    AND processed_balanced = 0;

-- Reset processing status to pending (QUALITY)
-- Used in: resetProcessingStatus()
UPDATE scanned_files
SET
    processed_quality = -1
WHERE
    file_path = ?
    AND processed_quality = 0;

-- =============================================================================
-- ADVANCED PROCESSING QUERIES
-- =============================================================================

-- Get files needing processing with exclusions (FAST)
-- Used in: getFilesNeedingProcessingWithExclusions()
SELECT file_path, file_name
FROM scanned_files
WHERE
    processed_fast = 0
    AND processed_fast != -1
    AND (?)
ORDER BY created_at DESC
LIMIT ?;

-- Get files needing processing with exclusions (BALANCED)
-- Used in: getFilesNeedingProcessingWithExclusions()
SELECT file_path, file_name
FROM scanned_files
WHERE
    processed_balanced = 0
    AND processed_balanced != -1
    AND (?)
ORDER BY created_at DESC
LIMIT ?;

-- Get files needing processing with exclusions (QUALITY)
-- Used in: getFilesNeedingProcessingWithExclusions()
SELECT file_path, file_name
FROM scanned_files
WHERE
    processed_quality = 0
    AND processed_quality != -1
    AND (?)
ORDER BY created_at DESC
LIMIT ?;

-- =============================================================================
-- PROCESSING STATUS FORCE RESETS
-- =============================================================================

-- Force reset processing status to pending (FAST)
-- Used in: forceResetProcessingStatus()
UPDATE scanned_files SET processed_fast = -1 WHERE file_path = ?;

-- Force reset processing status to pending (BALANCED)
-- Used in: forceResetProcessingStatus()
UPDATE scanned_files
SET
    processed_balanced = -1
WHERE
    file_path = ?;

-- Force reset processing status to pending (QUALITY)
-- Used in: forceResetProcessingStatus()
UPDATE scanned_files
SET
    processed_quality = -1
WHERE
    file_path = ?;

-- =============================================================================
-- BATCH PROCESSING STATUS UPDATES
-- =============================================================================

-- Batch reset processing status to pending (FAST)
-- Used in: batchResetProcessingStatus()
UPDATE scanned_files
SET
    processed_fast = -1
WHERE
    file_path = ?
    AND processed_fast = 0;

-- Batch reset processing status to pending (BALANCED)
-- Used in: batchResetProcessingStatus()
UPDATE scanned_files
SET
    processed_balanced = -1
WHERE
    file_path = ?
    AND processed_balanced = 0;

-- Batch reset processing status to pending (QUALITY)
-- Used in: batchResetProcessingStatus()
UPDATE scanned_files
SET
    processed_quality = -1
WHERE
    file_path = ?
    AND processed_quality = 0;

-- =============================================================================
-- COMPLEX PROCESSING QUERIES
-- =============================================================================

-- Get files needing any processing with file type filter
-- Used in: getFilesNeedingAnyProcessing()
SELECT file_path, file_name
FROM scanned_files
WHERE (?)
    AND (
        processed_fast = 0
        OR processed_balanced = 0
        OR processed_quality = 0
    )
ORDER BY created_at DESC
LIMIT ?;

-- =============================================================================
-- FILE LINKS OPERATIONS
-- =============================================================================

-- Update file links
-- Used in: setFileLinks()
UPDATE scanned_files SET links = ? WHERE file_path = ?;

-- Get file links
-- Used in: getFileLinks()
SELECT links FROM scanned_files WHERE file_path = ?;

-- =============================================================================
-- FILE ID OPERATIONS
-- =============================================================================

-- Get file ID by path
-- Used in: getFileId()
SELECT id FROM scanned_files WHERE file_path = ?;

-- =============================================================================
-- LINKED FILES QUERIES
-- =============================================================================

-- Find files with specific links
-- Used in: findFilesWithLinks()
SELECT file_path
FROM scanned_files
WHERE (links_fast LIKE ?)
    OR (links_balanced LIKE ?)
    OR (links_quality LIKE ?);

-- =============================================================================
-- PROCESSING FLAG CHECKS
-- =============================================================================

-- Check processing flag for file
-- Used in: checkProcessingFlag()
SELECT ?, file_name FROM scanned_files WHERE file_path = ?;

-- =============================================================================
-- METADATA UPDATES
-- =============================================================================

-- Update file metadata
-- Used in: updateFileMetadata()
UPDATE scanned_files SET file_metadata = ? WHERE file_path = ?;

-- Update file metadata (alternative)
-- Used in: updateFileMetadata()
UPDATE scanned_files SET file_metadata = ? WHERE file_path = ?;

-- =============================================================================
-- DYNAMIC FIELD UPDATES
-- =============================================================================

-- Update dynamic field
-- Used in: updateDynamicField()
UPDATE scanned_files SET ? = ? WHERE file_path = ?;

-- Get dynamic field value
-- Used in: getDynamicFieldValue()
SELECT ? FROM scanned_files WHERE file_path = ?;

-- =============================================================================
-- CLEANUP OPERATIONS
-- =============================================================================

-- Clear all scanned files
-- Used in: clearAllScannedFiles()
DELETE FROM scanned_files;