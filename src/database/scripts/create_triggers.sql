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