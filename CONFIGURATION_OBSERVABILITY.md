# Configuration Observability Implementation

## Overview

This document describes the implementation of real-time configuration observability for the `processing_interval_seconds` setting in the dedup-server application.

## What Was Added

### 1. Configuration File Update

- **File**: `config.yaml`
- **Addition**: `processing_interval_seconds: 1800` (30 minutes)
- **Location**: Added after `scan_interval_seconds: 300`

### 2. ServerConfigManager Fix

- **File**: `src/server_config_manager.cpp`
- **Fix**: Corrected `getProcessingIntervalSeconds()` method to read from the correct config key
- **Previous Bug**: Method was reading from `duplicate_linker_check_interval` instead of `processing_interval_seconds`
- **New Implementation**:
  - Reads from `processing_interval_seconds` config key
  - Includes validation (must be > 0 and ≤ 86400 seconds)
  - Provides proper error handling and logging
  - Falls back to default value (1800 seconds) if invalid

### 3. SimpleScheduler Observer Pattern (Enhanced)

- **File**: `include/core/simple_scheduler.hpp`
- **Change**: Made `SimpleScheduler` inherit from `ConfigObserver`
- **Addition**: `onConfigChanged(const ConfigEvent &event)` method declaration
- **Addition**: Manual control methods `triggerProcessingNow()` and `triggerScanNow()`

- **File**: `src/simple_scheduler.cpp`
- **Change**: Constructor now registers as a configuration observer
- **Enhanced `onConfigChanged()` implementation**:
  - Immediately recalculates timing when intervals change
  - Provides detailed logging of timing changes
  - Shows when operations will execute next
- **Improved responsiveness**: Scheduler loop now runs every 1 second (instead of 10 seconds)

## How It Works

### Configuration Change Flow

1. **API Call**: `PUT /config` with new `processing_interval_seconds` value
2. **Validation**: `ServerConfigManager::validateConfig()` validates the new configuration
3. **Update**: `ServerConfigManager::updateConfig()` applies the change and triggers notifications
4. **Notification**: All registered `ConfigObserver` instances receive `onConfigChanged()` calls
5. **Reaction**: `SimpleScheduler::onConfigChanged()` logs the change and prepares for the new interval
6. **Application**: New interval takes effect on the next scheduler loop iteration (within 1 second)

### Real-time Observability Features

- **Immediate API Response**: Configuration changes are reflected immediately in `GET /config`
- **Observer Notifications**: All components registered as observers receive real-time updates
- **Logging**: Configuration changes are logged with context and key information
- **Validation**: Invalid values are rejected with appropriate error messages
- **Fallback**: System gracefully handles invalid configurations with default values

## API Endpoints

### GET /config

- **Purpose**: Retrieve current configuration
- **Response**: JSON with current config values including `processing_interval_seconds`
- **Authentication**: Required

### PUT /config

- **Purpose**: Update configuration values
- **Request Body**: JSON with key-value pairs to update
- **Example**: `{"processing_interval_seconds": 900}`
- **Response**: Success/error message
- **Authentication**: Required
- **Real-time**: Changes are immediately observable and trigger notifications

## Testing

### Test Scripts

#### Basic Test Script

- **File**: `test_config_observability.sh`
- **Purpose**: Demonstrates basic real-time configuration observability
- **Features**:
  - Retrieves current configuration
  - Changes `processing_interval_seconds` to 900 seconds (15 minutes)
  - Verifies the change was applied
  - Restores original value
  - Shows real-time monitoring capabilities

#### Enhanced Real-Time Test Script

- **File**: `test_config_real_time.sh`
- **Purpose**: Demonstrates immediate configuration change effects
- **Features**:
  - Tests multiple configuration values (60s, 30s, 120s)
  - Shows immediate scheduler reaction to changes
  - Monitors real-time log output for configuration notifications
  - Demonstrates timing recalculation in real-time
  - Provides comprehensive testing of the observer pattern

### Manual Testing

```bash
# Start the server
./run.sh

# In another terminal, run the basic test
./test_config_observability.sh

# Or run the enhanced real-time test
./test_config_real_time.sh

# Monitor server logs for configuration change notifications
tail -f dedup_server.log | grep -E "(SimpleScheduler|Configuration|processing_interval|CONFIG CHANGE)"
```

## Configuration Values

### Default Values

- **Default**: 1800 seconds (30 minutes)
- **Range**: 1 to 86400 seconds (1 second to 24 hours)
- **Validation**: Must be positive integer within range

### Example Values

- **Frequent Processing**: 60 seconds (1 minute)
- **Standard Processing**: 1800 seconds (30 minutes)
- **Infrequent Processing**: 7200 seconds (2 hours)
- **Daily Processing**: 86400 seconds (24 hours)

## Benefits

### 1. Real-time Control

- Change processing frequency without server restart
- Immediate response to configuration updates
- No downtime for configuration changes

### 2. Observability

- Monitor configuration changes in real-time
- Log all configuration modifications
- Track when and what changed

### 3. Flexibility

- Adjust processing frequency based on workload
- Respond to system performance needs
- Support different operational scenarios

### 4. Reliability

- Validation prevents invalid configurations
- Fallback values ensure system stability
- Observer pattern ensures all components stay synchronized

## Future Enhancements

### Potential Improvements

1. **Configuration History**: Track all configuration changes over time
2. **Rollback Capability**: Revert to previous configuration states
3. **Scheduled Changes**: Automatically change configuration at specific times
4. **Configuration Templates**: Predefined configuration sets for different scenarios
5. **Web UI**: Graphical interface for configuration management

### Monitoring Integration

1. **Metrics**: Expose configuration values as Prometheus metrics
2. **Alerts**: Notify when configuration changes occur
3. **Audit Log**: Comprehensive logging of all configuration operations
4. **Health Checks**: Verify configuration validity in health endpoints

## Conclusion

The implementation provides robust, real-time configuration observability for the `processing_interval_seconds` setting. The observer pattern ensures that all system components are immediately aware of configuration changes, while the API endpoints provide easy access for monitoring and modification. The system is designed to be reliable, observable, and flexible for operational needs.
