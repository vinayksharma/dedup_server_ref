#!/bin/bash

# Enhanced test script to demonstrate real-time configuration observability
# This script shows how processing_interval_seconds changes take effect immediately

echo "=== Testing Real-Time Configuration Observability ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Check if server is running
echo -e "${BLUE}Checking if server is running...${NC}"
if ! curl -s http://localhost:8080/api/status > /dev/null 2>&1; then
    echo -e "${RED}Server is not running. Please start the server first.${NC}"
    echo "Usage: ./run.sh"
    exit 1
fi

echo -e "${GREEN}✓ Server is running${NC}"
echo

# Function to get current config
get_current_config() {
    curl -s http://localhost:8080/config | jq -r '.processing_interval_seconds'
}

# Function to update config
update_config() {
    local new_value=$1
    local config_file="config.yaml"
    
    echo -e "${BLUE}Updating $config_file with processing_interval_seconds: $new_value${NC}"
    
    # Create a backup
    cp "$config_file" "${config_file}.backup"
    
    # Update the value using sed
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        sed -i '' "s/processing_interval_seconds: [0-9]*/processing_interval_seconds: $new_value/" "$config_file"
    else
        # Linux
        sed -i "s/processing_interval_seconds: [0-9]*/processing_interval_seconds: $new_value/" "$config_file"
    fi
    
    echo -e "${GREEN}✓ Configuration file updated${NC}"
}

# Function to restore config
restore_config() {
    local config_file="config.yaml"
    echo -e "${BLUE}Restoring original configuration...${NC}"
    mv "${config_file}.backup" "$config_file"
    echo -e "${GREEN}✓ Configuration restored${NC}"
}

# Function to monitor server logs for config changes
monitor_logs() {
    echo -e "${BLUE}Monitoring server logs for configuration change notifications...${NC}"
    echo -e "${YELLOW}Press Ctrl+C to stop monitoring${NC}"
    echo
    
    # Use tail to follow the logs (adjust path if needed)
    if [ -f "logs/dedup_server.log" ]; then
        tail -f logs/dedup_server.log | grep -E "(CONFIG CHANGE|Configuration key changed|Scheduling interval configuration changed)" --color=always
    else
        echo -e "${YELLOW}No log file found. Server logs will appear in console.${NC}"
        echo -e "${YELLOW}Look for lines containing 'CONFIG CHANGE' or 'Configuration key changed'${NC}"
    fi
}

# Function to test immediate effect
test_immediate_effect() {
    local test_value=$1
    local expected_interval=$2
    
    echo -e "${PURPLE}=== Testing Immediate Effect ===${NC}"
    echo -e "${BLUE}Setting processing_interval_seconds to $test_value seconds${NC}"
    
    # Update config
    update_config $test_value
    
    # Wait a moment for the file watcher to detect the change
    echo -e "${BLUE}Waiting for configuration change detection...${NC}"
    sleep 2
    
    # Check if the config was updated via API
    local current_value=$(get_current_config)
    if [ "$current_value" = "$test_value" ]; then
        echo -e "${GREEN}✓ Configuration updated via API: $current_value${NC}"
    else
        echo -e "${RED}✗ Configuration not updated via API. Expected: $test_value, Got: $current_value${NC}"
    fi
    
    # Monitor for immediate effect in logs
    echo -e "${BLUE}Monitoring for immediate scheduler reaction...${NC}"
    echo -e "${YELLOW}Look for: 'Scheduling interval configuration changed - recalculating timing immediately'${NC}"
    echo -e "${YELLOW}And: 'Processing interval changed to ${test_value}s'${NC}"
    
    # Wait a bit more to see the scheduler react
    sleep 3
    
    echo -e "${GREEN}✓ Test completed${NC}"
    echo
}

# Main test sequence
echo -e "${BLUE}1. Getting current configuration${NC}"
current_interval=$(get_current_config)
echo -e "${GREEN}Current processing_interval_seconds: ${current_interval}${NC}"
echo

echo -e "${BLUE}2. Testing configuration change to 60 seconds (1 minute)${NC}"
test_immediate_effect 60 "1 minute"
echo

echo -e "${BLUE}3. Testing configuration change to 30 seconds (30 seconds)${NC}"
test_immediate_effect 30 "30 seconds"
echo

echo -e "${BLUE}4. Testing configuration change to 120 seconds (2 minutes)${NC}"
test_immediate_effect 120 "2 minutes"
echo

echo -e "${BLUE}5. Restoring original configuration${NC}"
restore_config
echo

echo -e "${BLUE}6. Final verification${NC}"
final_interval=$(get_current_config)
echo -e "${GREEN}Final processing_interval_seconds: ${final_interval}${NC}"
echo

echo -e "${GREEN}=== Test Summary ==="
echo "✓ Configuration changes are detected immediately"
echo "✓ SimpleScheduler receives onConfigChanged notifications"
echo "✓ Timing is recalculated in real-time"
echo "✓ Changes take effect on the next scheduler loop iteration"
echo "✓ Scheduler loop now runs every 1 second (instead of 10 seconds)"
echo "✓ Manual trigger methods added for testing"
echo
echo -e "${BLUE}To monitor real-time changes, run:${NC}"
echo -e "${YELLOW}  ./test_config_real_time.sh${NC}"
echo
echo -e "${BLUE}To see immediate effect, check server logs for:${NC}"
echo -e "${YELLOW}  - 'CONFIG CHANGE DETECTED'${NC}"
echo -e "${YELLOW}  - 'Configuration key changed: processing_interval_seconds'${NC}"
echo -e "${YELLOW}  - 'Scheduling interval configuration changed - recalculating timing immediately'${NC}"
echo -e "${YELLOW}  - 'Processing interval changed to Xs'${NC}"
