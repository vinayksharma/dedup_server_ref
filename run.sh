#!/bin/bash

# Simple Dedup Server Startup Script
# This script just starts the server and lets it run

set -e

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}Starting Dedup Server...${NC}"

# Check if build directory exists
if [ ! -d "build" ]; then
    echo -e "${YELLOW}Error: build directory not found. Please run 'make' first.${NC}"
    exit 1
fi

# Check if server binary exists
if [ ! -f "build/dedup_server" ]; then
    echo -e "${YELLOW}Error: dedup_server binary not found. Please run 'make' first.${NC}"
    exit 1
fi

# Proactively clean up stale PID file if present
if [ -f "dedup_server.pid" ]; then
    EXISTING_PID=$(cat dedup_server.pid 2>/dev/null || echo "")
    if [ -n "$EXISTING_PID" ]; then
        if ! kill -0 "$EXISTING_PID" 2>/dev/null; then
            echo -e "${YELLOW}Found stale PID file (PID $EXISTING_PID not running). Removing...${NC}"
            rm -f dedup_server.pid || true
        fi
    fi
fi

# Check for command line arguments
FORCE_SHUTDOWN=""
DETACH_MODE=0

for arg in "$@"; do
    if [ "$arg" = "--shutdown" ] || [ "$arg" = "-s" ]; then
        FORCE_SHUTDOWN="--shutdown"
        echo -e "${YELLOW}Force shutdown mode enabled${NC}"
    fi
    if [ "$arg" = "--detach" ] || [ "$arg" = "-d" ]; then
        DETACH_MODE=1
        echo -e "${YELLOW}Detach mode: will not block; returning after startup${NC}"
    fi
done

# Start the server
if [ $DETACH_MODE -eq 1 ]; then
    echo -e "${YELLOW}Starting dedup server in background...${NC}"
    nohup ./build/dedup_server $FORCE_SHUTDOWN >/dev/null 2>&1 &
    SERVER_PID=$!
    echo -e "${GREEN}✓ Server started in background (PID: ${SERVER_PID})${NC}"
    echo -e "${BLUE}Server will be available on the configured port${NC}"
    exit 0
else
    echo -e "${YELLOW}Starting dedup server...${NC}"
    echo -e "${GREEN}✓ Server started successfully${NC}"
    echo -e "${BLUE}Press Ctrl+C to stop the server${NC}"
    
    # Run the server in foreground
    ./build/dedup_server $FORCE_SHUTDOWN
fi 
