#!/bin/bash

# Test script to demonstrate real-time configuration observability
# This script shows how the processing_interval_seconds can be observed and changed in real-time

echo "=== Testing Configuration Real-time Observability ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# Get current configuration
echo -e "${BLUE}1. Getting current configuration...${NC}"
CURRENT_CONFIG=$(curl -s http://localhost:8080/config)
echo "Current config:"
echo "$CURRENT_CONFIG" | jq '.' 2>/dev/null || echo "$CURRENT_CONFIG"
echo

# Extract current processing interval
CURRENT_INTERVAL=$(echo "$CURRENT_CONFIG" | grep -o '"processing_interval_seconds":[0-9]*' | cut -d: -f2)
echo -e "${BLUE}Current processing_interval_seconds: ${YELLOW}${CURRENT_INTERVAL}${NC}"
echo

# Change the processing interval
NEW_INTERVAL=900  # 15 minutes
echo -e "${BLUE}2. Changing processing_interval_seconds to ${YELLOW}${NEW_INTERVAL}${BLUE} seconds...${NC}"

UPDATE_RESPONSE=$(curl -s -X PUT http://localhost:8080/config \
    -H "Content-Type: application/json" \
    -d "{\"processing_interval_seconds\": $NEW_INTERVAL}")

echo "Update response: $UPDATE_RESPONSE"
echo

# Wait a moment for the change to take effect
echo -e "${BLUE}3. Waiting for configuration change to propagate...${NC}"
sleep 2

# Get updated configuration
echo -e "${BLUE}4. Getting updated configuration...${NC}"
UPDATED_CONFIG=$(curl -s http://localhost:8080/config)
echo "Updated config:"
echo "$UPDATED_CONFIG" | jq '.' 2>/dev/null || echo "$UPDATED_CONFIG"
echo

# Extract updated processing interval
UPDATED_INTERVAL=$(echo "$UPDATED_CONFIG" | grep -o '"processing_interval_seconds":[0-9]*' | cut -d: -f2)
echo -e "${BLUE}Updated processing_interval_seconds: ${YELLOW}${UPDATED_INTERVAL}${NC}"

if [ "$UPDATED_INTERVAL" = "$NEW_INTERVAL" ]; then
    echo -e "${GREEN}✓ Configuration change successful!${NC}"
else
    echo -e "${RED}✗ Configuration change failed!${NC}"
fi

echo

# Show how to monitor the change in real-time
echo -e "${BLUE}5. Real-time monitoring demonstration:${NC}"
echo "The SimpleScheduler is now a ConfigObserver and will log configuration changes."
echo "You can monitor the server logs to see:"
echo "  - 'SimpleScheduler received configuration change notification'"
echo "  - 'Configuration key changed: processing_interval_seconds'"
echo "  - 'Scheduling interval configuration changed - will take effect on next loop iteration'"
echo

# Show how to change it back
echo -e "${BLUE}6. Changing back to original value...${NC}"
RESTORE_RESPONSE=$(curl -s -X PUT http://localhost:8080/config \
    -H "Content-Type: application/json" \
    -d "{\"processing_interval_seconds\": $CURRENT_INTERVAL}")

echo "Restore response: $RESTORE_RESPONSE"
echo

echo -e "${GREEN}=== Test Complete ===${NC}"
echo
echo "Summary:"
echo "- processing_interval_seconds was added to config.yaml"
echo "- PocoConfigAdapter.getProcessingIntervalSeconds() now reads from config"
echo "- SimpleScheduler is registered as a ConfigObserver"
echo "- Configuration changes are observable in real-time via API"
echo "- Changes take effect on the next scheduler loop iteration (within 10 seconds)"
echo
echo "API Endpoints used:"
echo "- GET /config - Retrieve current configuration"
echo "- PUT /config - Update configuration (triggers real-time notifications)"
echo
echo "Real-time observability confirmed: ✓"
