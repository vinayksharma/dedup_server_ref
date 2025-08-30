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