-- Flags and User Inputs Operations SQL Queries
-- This file contains all SQL queries for flags and user_inputs table operations

-- =============================================================================
-- FLAGS OPERATIONS
-- =============================================================================

-- Get flag value
-- Used in: getFlag()
SELECT value FROM flags WHERE name = ?;

-- Set or update flag value
-- Used in: setFlag()
INSERT INTO
    flags (name, value, updated_at)
VALUES (?, ?, CURRENT_TIMESTAMP)
ON CONFLICT (name) DO
UPDATE
SET
    value = excluded.value,
    updated_at = CURRENT_TIMESTAMP;

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

-- Clear all user inputs
-- Used in: clearAllUserInputs()
DELETE FROM user_inputs;

-- =============================================================================
-- TRIGGER OPERATIONS (Embedded in create_triggers.sql)
-- =============================================================================

-- Insert or update flag when scanned_files changes (INSERT)
-- Used in: scanned_files change triggers
INSERT INTO
    flags (name, value, updated_at)
VALUES (
        'transcode_preprocess_scanned_files_changed',
        1,
        CURRENT_TIMESTAMP
    )
ON CONFLICT (name) DO
UPDATE
SET
    value = 1,
    updated_at = CURRENT_TIMESTAMP;

-- Insert or update flag when scanned_files changes (UPDATE)
-- Used in: scanned_files change triggers
INSERT INTO
    flags (name, value, updated_at)
VALUES (
        'transcode_preprocess_scanned_files_changed',
        1,
        CURRENT_TIMESTAMP
    )
ON CONFLICT (name) DO
UPDATE
SET
    value = 1,
    updated_at = CURRENT_TIMESTAMP;

-- Insert or update flag when scanned_files changes (DELETE)
-- Used in: scanned_files change triggers
INSERT INTO
    flags (name, value, updated_at)
VALUES (
        'transcode_preprocess_scanned_files_changed',
        1,
        CURRENT_TIMESTAMP
    )
ON CONFLICT (name) DO
UPDATE
SET
    value = 1,
    updated_at = CURRENT_TIMESTAMP;