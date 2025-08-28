# Configuration Endpoints Documentation

## Overview

The dedup-server provides comprehensive configuration management through a set of REST API endpoints. All configuration changes are automatically persisted to `config.json` and trigger reactive updates through the observer pattern.

## Authentication

All configuration endpoints require authentication. Include the JWT token in the Authorization header:

```
Authorization: Bearer <your-jwt-token>
```

## Base Configuration Endpoints

### GET /config

Retrieves the complete configuration.

**Response:**

```json
{
  "status": "success",
  "config": {
    // Complete configuration object
  }
}
```

### PUT /config

Updates the complete configuration.

**Request Body:**

```json
{
  // Complete configuration object
}
```

**Response:**

```json
{
  "message": "Configuration updated successfully"
}
```

### POST /config/reload

Reloads configuration from a file.

**Request Body:**

```json
{
  "file_path": "config/config.json"
}
```

**Response:**

```json
{
  "message": "Configuration reloaded successfully"
}
```

### POST /config/save

Saves configuration to a file.

**Request Body:**

```json
{
  "file_path": "config/config.json"
}
```

**Response:**

```json
{
  "message": "Configuration saved successfully"
}
```

## Server Configuration Endpoints

### GET /config/server

Retrieves server-specific configuration.

**Response:**

```json
{
  "status": "success",
  "config": {
    "server_host": "localhost",
    "server_port": 8080,
    "auth_secret": "your-secret-key-here"
  }
}
```

### PUT /config/server

Updates server configuration.

**Request Body:**

```json
{
  "server_host": "0.0.0.0",
  "server_port": 9090
}
```

**Response:**

```json
{
  "message": "Server configuration updated successfully"
}
```

## Threading Configuration Endpoints

### GET /config/threading

Retrieves threading configuration.

**Response:**

```json
{
  "status": "success",
  "config": {
    "max_processing_threads": 8,
    "max_scan_threads": 4,
    "database_threads": 2
  }
}
```

### PUT /config/threading

Updates threading configuration.

**Request Body:**

```json
{
  "max_processing_threads": 16,
  "max_scan_threads": 8
}
```

**Response:**

```json
{
  "message": "Threading configuration updated successfully"
}
```

## Database Configuration Endpoints

### GET /config/database

Retrieves database configuration.

**Response:**

```json
{
  "status": "success",
  "config": {
    "retry": {
      "max_attempts": 3,
      "backoff_base_ms": 100,
      "max_backoff_ms": 1000
    },
    "timeout": {
      "busy_timeout_ms": 30000,
      "operation_timeout_ms": 60000
    }
  }
}
```

### PUT /config/database

Updates database configuration.

**Request Body:**

```json
{
  "retry": {
    "max_attempts": 5,
    "backoff_base_ms": 200
  },
  "timeout": {
    "busy_timeout_ms": 60000
  }
}
```

**Response:**

```json
{
  "message": "Database configuration updated successfully"
}
```

## File Types Configuration Endpoints

### GET /config/filetypes

Retrieves file type configuration.

**Response:**

```json
{
  "status": "success",
  "config": {
    "supported_file_types": {
      "images": {
        "png": true,
        "jpg": true,
        "gif": false
      },
      "video": {
        "mp4": true,
        "avi": true
      }
    },
    "transcoding_file_types": {
      "cr2": true,
      "nef": true,
      "raw": false
    }
  }
}
```

### PUT /config/filetypes

Updates file type configuration.

**Request Body:**

```json
{
  "supported_file_types": {
    "images": {
      "png": false,
      "jpg": true
    }
  },
  "transcoding_file_types": {
    "cr2": true,
    "nef": false
  }
}
```

**Response:**

```json
{
  "message": "File types configuration updated successfully"
}
```

## Video Processing Configuration Endpoints

### GET /config/video

Retrieves video processing configuration.

**Response:**

```json
{
  "status": "success",
  "config": {
    "dedup_mode": "QUALITY",
    "video_processing": {
      "QUALITY": {
        "frames_per_skip": 3,
        "skip_count": 12,
        "skip_duration_seconds": 1
      },
      "BALANCED": {
        "frames_per_skip": 2,
        "skip_count": 8,
        "skip_duration_seconds": 1
      },
      "FAST": {
        "frames_per_skip": 2,
        "skip_count": 5,
        "skip_duration_seconds": 2
      }
    }
  }
}
```

### PUT /config/video

Updates video processing configuration.

**Request Body:**

```json
{
  "dedup_mode": "FAST",
  "video_processing": {
    "FAST": {
      "frames_per_skip": 4,
      "skip_count": 10
    }
  }
}
```

**Response:**

```json
{
  "message": "Video configuration updated successfully"
}
```

## Scanning Configuration Endpoints

### GET /config/scanning

Retrieves scanning configuration.

**Response:**

```json
{
  "status": "success",
  "config": {
    "scan_interval_seconds": 300,
    "max_scan_threads": 4
  }
}
```

### PUT /config/scanning

Updates scanning configuration.

**Request Body:**

```json
{
  "scan_interval_seconds": 600,
  "max_scan_threads": 6
}
```

**Response:**

```json
{
  "message": "Scanning configuration updated successfully"
}
```

## Logging Configuration Endpoints

### GET /config/logging

Retrieves logging configuration.

**Response:**

```json
{
  "status": "success",
  "config": {
    "log_level": "DEBUG"
  }
}
```

### PUT /config/logging

Updates logging configuration.

**Request Body:**

```json
{
  "log_level": "INFO"
}
```

**Response:**

```json
{
  "message": "Logging configuration updated successfully"
}
```

## Processing Configuration Endpoints

### GET /config/processing

Retrieves processing configuration.

**Response:**

```json
{
  "status": "success",
  "config": {
    "processing_batch_size": 200,
    "pre_process_quality_stack": true
  }
}
```

### PUT /config/processing

Updates processing configuration.

**Request Body:**

```json
{
  "processing_batch_size": 500,
  "pre_process_quality_stack": false
}
```

**Response:**

```json
{
  "message": "Processing configuration updated successfully"
}
```

## Observer Pattern

All configuration changes automatically trigger observers that react to specific configuration updates:

### LoggerObserver

- Reacts to `log_level` changes
- Automatically applies new log level at runtime

### ServerConfigObserver

- Reacts to `server_port` and `server_host` changes
- Logs changes (requires server restart for full effect)

### ScanConfigObserver

- Reacts to `scan_interval_seconds` and `max_scan_threads` changes
- Logs changes (requires scheduler restart for full effect)

### ThreadingConfigObserver

- Reacts to threading-related configuration changes
- Automatically updates ThreadPoolManager for processing threads
- Logs changes for other thread settings

### DatabaseConfigObserver

- Reacts to database retry and timeout configuration changes
- Logs changes (requires database restart for full effect)

### FileTypeConfigObserver

- Reacts to file type category and transcoding file type changes
- Logs changes (requires MediaProcessor/TranscodingManager restart for full effect)

### VideoProcessingConfigObserver

- Reacts to video processing quality and mode changes
- Logs changes (requires TranscodingManager restart for full effect)

### CacheConfigObserver

- Reacts to `decoder_cache_size_mb` changes
- Logs changes and provides cache management guidance
- Automatically adjusts cache size limits

### ProcessingConfigObserver

- Reacts to `processing_batch_size` and `pre_process_quality_stack` changes
- Logs changes and provides processing pipeline guidance
- Automatically adjusts batch processing configuration

### DedupModeConfigObserver

- Reacts to `dedup_mode` changes
- Logs changes and provides deduplication algorithm guidance
- Automatically adjusts deduplication parameters

## Configuration Persistence

All configuration changes are automatically persisted to `config.json` in the project's config directory. The configuration is also watched for file changes, allowing runtime updates from external file modifications.

## Error Handling

All endpoints return appropriate HTTP status codes:

- `200`: Success
- `400`: Bad request (invalid configuration)
- `401`: Unauthorized (missing or invalid token)
- `500`: Internal server error

Error responses include a descriptive error message:

```json
{
  "error": "Invalid configuration: log_level must be one of [TRACE, DEBUG, INFO, WARN, ERROR]"
}
```

## Examples

### Changing Log Level

```bash
curl -X PUT http://localhost:8080/config/logging \
  -H "Authorization: Bearer <your-token>" \
  -H "Content-Type: application/json" \
  -d '{"log_level": "INFO"}'
```

### Updating Threading Configuration

```bash
curl -X PUT http://localhost:8080/config/threading \
  -H "Authorization: Bearer <your-token>" \
  -H "Content-Type: application/json" \
  -d '{"max_processing_threads": 16, "max_scan_threads": 8}'
```

### Disabling PNG File Support

```bash
curl -X PUT http://localhost:8080/config/filetypes \
  -H "Authorization: Bearer <your-token>" \
  -H "Content-Type: application/json" \
  -d '{"supported_file_types": {"images": {"png": false}}}'
```

## Notes

- Some configuration changes require component restart for full effect
- All changes are logged for audit purposes
- Configuration is validated before application
- File watching allows runtime updates from external file modifications
- Observer pattern ensures reactive configuration updates
