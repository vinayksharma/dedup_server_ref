-- Cache Map Operations SQL Queries
-- This file contains all SQL queries for cache_map table operations

-- =============================================================================
-- TRANCODING OPERATIONS
-- =============================================================================

-- Get source file path for transcoding
-- Used in: TranscodingManager::getNextTranscodingJob()
SELECT source_file_path FROM cache_map WHERE status = 0;

-- Reset worker assignment for stuck jobs
-- Used in: TranscodingManager::resetStuckJobs()
UPDATE cache_map SET status = 0, worker_id = NULL WHERE status = 1;

-- Count active worker jobs
-- Used in: TranscodingManager::getActiveWorkerCount()
SELECT COUNT(*) FROM cache_map WHERE status = 1;

-- =============================================================================
-- CACHE MAP QUERIES
-- =============================================================================

-- Get transcoded file path
-- Used in: getTranscodedFilePath()
SELECT transcoded_file_path
FROM cache_map
WHERE
    source_file_path = ?;

-- Get next available transcoding job
-- Used in: getNextTranscodingJob()
SELECT source_file_path
FROM cache_map
WHERE
    status = 0
    AND transcoded_file_path IS NULL
ORDER BY created_at ASC
LIMIT 1;

-- =============================================================================
-- CACHE MAP STATUS UPDATES
-- =============================================================================

-- Mark job as assigned to worker
-- Used in: assignJobToWorker()
UPDATE cache_map
SET
    status = 1,
    worker_id = ?,
    updated_at = CURRENT_TIMESTAMP
WHERE
    source_file_path = ?;

-- Mark job as completed
-- Used in: markJobAsCompleted()
UPDATE cache_map
SET
    status = 2,
    transcoded_file_path = ?,
    updated_at = CURRENT_TIMESTAMP
WHERE
    source_file_path = ?;

-- Mark job as failed
-- Used in: markJobAsFailed()
UPDATE cache_map
SET
    status = 3,
    updated_at = CURRENT_TIMESTAMP
WHERE
    source_file_path = ?;

-- =============================================================================
-- CACHE MAP CHECKS
-- =============================================================================

-- Check if file is in transcoding queue
-- Used in: isFileInTranscodingQueue()
SELECT 1
FROM cache_map
WHERE
    source_file_path = ?
    AND transcoded_file_path IS NULL;

-- =============================================================================
-- CACHE MAP CLEANUP
-- =============================================================================

-- Delete specific cache entry
-- Used in: removeFromCache()
DELETE FROM cache_map WHERE source_file_path = ?;

-- Clear all cache entries
-- Used in: clearAllCache()
DELETE FROM cache_map;

-- =============================================================================
-- STATISTICS QUERIES
-- =============================================================================

-- Count files in transcoding queue
-- Used in: getTranscodingStatistics()
SELECT COUNT(*) FROM cache_map WHERE status = 0;

-- Count successfully transcoded files
-- Used in: getTranscodingStatistics()
SELECT COUNT(*) FROM cache_map WHERE status = 2;