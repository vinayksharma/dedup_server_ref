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
    status INTEGER DEFAULT 0,
    worker_id TEXT,
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