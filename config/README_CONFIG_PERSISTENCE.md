# Configuration Persistence

## Overview

The configuration system now automatically persists all programmatic changes to `config.json`. This ensures that any configuration modifications made through the API or code are immediately saved and survive server restarts.

## How It Works

1. **Specific Setter Methods**: Each configuration option has a dedicated setter method
2. **Automatic Persistence**: Every setter automatically calls `persistChanges()` to save to `config.json`
3. **Event Publishing**: Configuration changes trigger events for observers to react
4. **Immediate Effect**: Changes are applied in-memory and persisted to disk simultaneously

## Available Setter Methods

### Basic Configuration

- `setDedupMode(const std::string& mode)` - Set deduplication mode
- `setLogLevel(const std::string& level)` - Set logging level
- `setServerPort(int port)` - Set server port
- `setServerHost(const std::string& host)` - Set server host
- `setAuthSecret(const std::string& secret)` - Set authentication secret

### Threading Configuration

- `setMaxProcessingThreads(int threads)` - Set maximum processing threads
- `setMaxScanThreads(int threads)` - Set maximum scan threads
- `setHttpServerThreads(int threads)` - Set HTTP server threads
- `setDatabaseThreads(int threads)` - Set database threads
- `setMaxDecoderThreads(int threads)` - Set maximum decoder threads

### Processing Configuration

- `setProcessingBatchSize(int size)` - Set processing batch size
- `setScanIntervalSeconds(int seconds)` - Set scan interval
- `setProcessingIntervalSeconds(int seconds)` - Set processing interval
- `setPreProcessQualityStack(const std::string& stack)` - Set quality stack

### File Type Configuration

- `setFileTypeEnabled(const std::string& category, const std::string& extension, bool enabled)` - Enable/disable file type
- `setTranscodingFileType(const std::string& extension, bool enabled)` - Set transcoding file type
- `updateFileTypeConfig(const std::string& category, const std::string& extension, bool enabled)` - Update file type config

### Video Processing Configuration

- `setVideoSkipDurationSeconds(int seconds)` - Set video skip duration
- `setVideoFramesPerSkip(int frames)` - Set video frames per skip
- `setVideoSkipCount(int count)` - Set video skip count

### Database Configuration

- `setDatabaseMaxRetries(int retries)` - Set database max retries
- `setDatabaseBackoffBaseMs(int ms)` - Set database backoff base
- `setDatabaseMaxBackoffMs(int ms)` - Set database max backoff
- `setDatabaseBusyTimeoutMs(int ms)` - Set database busy timeout
- `setDatabaseOperationTimeoutMs(int ms)` - Set database operation timeout

### Cache Configuration

- `setDecoderCacheSizeMB(int mb)` - Set decoder cache size

## Event Publishing

Each setter method automatically publishes a `ConfigUpdateEvent` with:

- **changed_keys**: Vector of configuration keys that were modified
- **source**: Method name that triggered the change
- **update_id**: Unique identifier for the change

## Example Usage

```cpp
auto& config = PocoConfigAdapter::getInstance();

// Individual settings - each automatically persists
config.setLogLevel("DEBUG");
config.setServerPort(8081);
config.setMaxProcessingThreads(8);

// File type configuration
config.setFileTypeEnabled("images", "jpg", true);
config.setFileTypeEnabled("images", "png", false);

// Video processing
config.setVideoSkipDurationSeconds(5);
config.setVideoFramesPerSkip(10);
```

## Benefits

1. **Type Safety**: Compile-time checking for parameter types
2. **Clear API**: Explicit methods for each configuration option
3. **Automatic Persistence**: No need to manually save changes
4. **Event-Driven**: Observers can react to specific changes
5. **Validation**: Each setter can have specific validation logic
6. **Maintainability**: Clear, documented interface for developers

## Migration Notes

- **Removed**: Bulk update methods (`updateConfigAndPersist`)
- **Added**: Specific setter methods for each configuration option
- **Improved**: Event publishing with method source identification
- **Simplified**: Cleaner, more explicit API design

## Error Handling

All setter methods include error handling and logging. If persistence fails, the change is still applied in-memory but logged as an error.

## Testing

The configuration persistence is thoroughly tested with unit tests that verify:

- Individual setter methods persist correctly
- File type configuration updates work properly
- Configuration reloading reflects persisted changes
- Event publishing occurs for each change
