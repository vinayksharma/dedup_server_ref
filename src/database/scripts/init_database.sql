-- Database initialization script for dedup-server
-- This script creates all tables, triggers, and indexes

-- Table creation scripts for dedup-server database

-- Media processing results table
CREATE TABLE IF NOT EXISTS media_processing_results (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path TEXT NOT NULL,
    processing_mode TEXT NOT NULL,
    success BOOLEAN NOT NULL,
    artifact_format TEXT,
    artifact_hash TEXT,
    artifact_confidence REAL,
    artifact_metadata TEXT,
    artifact_data BLOB,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (file_path, processing_mode),
    FOREIGN KEY (file_path) REFERENCES scanned_files (file_path) ON DELETE CASCADE
);

-- Scanned files table
CREATE TABLE IF NOT EXISTS scanned_files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path TEXT NOT NULL UNIQUE,
    relative_path TEXT, -- For network mounts: share:relative/path
    share_name TEXT, -- The share name (B, G, etc.)
    file_name TEXT NOT NULL,
    file_metadata TEXT, -- File metadata for change detection (creation date, modification date, size)
    processed_fast BOOLEAN DEFAULT 0, -- Processing flag for FAST mode
    processed_balanced BOOLEAN DEFAULT 0, -- Processing flag for BALANCED mode
    processed_quality BOOLEAN DEFAULT 0, -- Processing flag for QUALITY mode
    links_fast TEXT, -- Comma-separated list of duplicate file IDs found in FAST mode
    links_balanced TEXT, -- Comma-separated list of duplicate file IDs found in BALANCED mode
    links_quality TEXT, -- Comma-separated list of duplicate file IDs found in QUALITY mode
    is_network_file BOOLEAN DEFAULT 0, -- True if on network mount
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- User inputs table
CREATE TABLE IF NOT EXISTS user_inputs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    input_type TEXT NOT NULL,
    input_value TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (input_type, input_value)
);

-- Cache map table
CREATE TABLE IF NOT EXISTS cache_map (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_file_path TEXT NOT NULL UNIQUE,
    transcoded_file_path TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (source_file_path) REFERENCES scanned_files (file_path) ON DELETE CASCADE
);

-- Flags table
CREATE TABLE IF NOT EXISTS flags (
    name TEXT PRIMARY KEY,
    value TEXT NOT NULL DEFAULT '0',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Trigger creation scripts for dedup-server database

-- Create a trigger to set transcode_preprocess_scanned_files_changed to 1 on INSERT
CREATE TRIGGER IF NOT EXISTS trg_scanned_files_changed_insert
AFTER INSERT ON scanned_files
BEGIN
    INSERT INTO flags(name, value, updated_at) VALUES ('transcode_preprocess_scanned_files_changed', 1, CURRENT_TIMESTAMP)
    ON CONFLICT(name) DO UPDATE SET value = 1, updated_at = CURRENT_TIMESTAMP;
END;

-- Create a trigger to set transcode_preprocess_scanned_files_changed to 1 on UPDATE
CREATE TRIGGER IF NOT EXISTS trg_scanned_files_changed_update
AFTER UPDATE ON scanned_files
BEGIN
    INSERT INTO flags(name, value, updated_at) VALUES ('transcode_preprocess_scanned_files_changed', 1, CURRENT_TIMESTAMP)
    ON CONFLICT(name) DO UPDATE SET value = 1, updated_at = CURRENT_TIMESTAMP;
END;

-- Create a trigger to set transcode_preprocess_scanned_files_changed to 1 on DELETE
CREATE TRIGGER IF NOT EXISTS trg_scanned_files_changed_delete
AFTER DELETE ON scanned_files
BEGIN
    INSERT INTO flags(name, value, updated_at) VALUES ('transcode_preprocess_scanned_files_changed', 1, CURRENT_TIMESTAMP)
    ON CONFLICT(name) DO UPDATE SET value = 1, updated_at = CURRENT_TIMESTAMP;
END;

-- Index creation scripts for dedup-server database

-- Create index on cache_map status for faster job selection
CREATE INDEX IF NOT EXISTS idx_cache_map_status ON cache_map (status, created_at);

-- Create index on scanned_files file_path for faster lookups
CREATE INDEX IF NOT EXISTS idx_scanned_files_file_path ON scanned_files (file_path);

-- Create index on scanned_files processed flags for faster processing queries
CREATE INDEX IF NOT EXISTS idx_scanned_files_processed_fast ON scanned_files (processed_fast);

CREATE INDEX IF NOT EXISTS idx_scanned_files_processed_balanced ON scanned_files (processed_balanced);

CREATE INDEX IF NOT EXISTS idx_scanned_files_processed_quality ON scanned_files (processed_quality);

-- Create index on scanned_files created_at for ordering
CREATE INDEX IF NOT EXISTS idx_scanned_files_created_at ON scanned_files (created_at);

-- Create index on media_processing_results file_path for faster lookups
CREATE INDEX IF NOT EXISTS idx_media_processing_results_file_path ON media_processing_results (file_path);

-- Create index on flags name for faster lookups
CREATE INDEX IF NOT EXISTS idx_flags_name ON flags (name);

-- Schema is now complete in table definitions - no ALTER TABLE needed

-- Insert initial flags
INSERT OR IGNORE INTO
    flags (name, value, updated_at)
VALUES (
        'transcode_preprocess_scanned_files_changed',
        '0',
        CURRENT_TIMESTAMP
    ),
    (
        'database_initialized',
        '1',
        CURRENT_TIMESTAMP
    );