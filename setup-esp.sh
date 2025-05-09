#!/bin/bash

# Colors for better readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== ESP-IDF Environment Setup ===${NC}"

# Common locations for ESP-IDF installation
COMMON_PATHS=(
    "$HOME/esp/esp-idf"
    "/opt/esp/esp-idf"
    "/usr/local/esp/esp-idf"
    "$HOME/esp-idf"
    "$HOME/Documents/esp-idf"
)

# Try to find ESP-IDF in common locations
IDF_PATH=""
for path in "${COMMON_PATHS[@]}"; do
    if [ -d "$path" ] && [ -f "$path/export.sh" ]; then
        IDF_PATH="$path"
        break
    fi
done

# If ESP-IDF not found in common locations, ask user for the path
if [ -z "$IDF_PATH" ]; then
    echo -e "${YELLOW}ESP-IDF not found in common locations.${NC}"
    
    # Try to find using find command
    echo "Searching for ESP-IDF installation..."
    FOUND_PATHS=$(find $HOME -name "export.sh" -path "*/esp-idf/*" -not -path "*/examples/*" -not -path "*/docs/*" 2>/dev/null)
    
    if [ ! -z "$FOUND_PATHS" ]; then
        echo -e "${GREEN}Found potential ESP-IDF installations:${NC}"
        IFS=$'\n' read -rd '' -a FOUND_PATH_ARRAY <<< "$FOUND_PATHS"
        
        for i in "${!FOUND_PATH_ARRAY[@]}"; do
            path=$(dirname "${FOUND_PATH_ARRAY[$i]}")
            echo -e "$((i+1)). ${path}"
        done
        
        echo -e "${YELLOW}Please select a number or enter a custom path:${NC}"
        read -r selection
        
        if [[ "$selection" =~ ^[0-9]+$ ]] && [ "$selection" -ge 1 ] && [ "$selection" -le "${#FOUND_PATH_ARRAY[@]}" ]; then
            path=$(dirname "${FOUND_PATH_ARRAY[$((selection-1))]}") 
            IDF_PATH="$path"
        else
            IDF_PATH="$selection"
        fi
    else
        echo -e "${YELLOW}Please enter the path to your ESP-IDF installation:${NC}"
        read -r IDF_PATH
    fi
fi

# Verify the path is valid
if [ ! -f "$IDF_PATH/export.sh" ]; then
    echo -e "${RED}Error: $IDF_PATH does not appear to be a valid ESP-IDF installation.${NC}"
    echo "The path should point to the root of the ESP-IDF repository containing export.sh"
    exit 1
fi

# Source the export.sh script
echo -e "${GREEN}Setting up ESP-IDF environment from ${IDF_PATH}...${NC}"
. "$IDF_PATH/export.sh"

# Check if setup was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ ESP-IDF environment successfully set up!${NC}"
    echo ""
    echo "You can now use ESP-IDF commands like:"
    echo "  - idf.py menuconfig"
    echo "  - idf.py build"
    echo "  - idf.py flash"
    echo "  - idf.py monitor"
    echo ""
    echo "To configure the SSH functionality for PC shutdown/restart:"
    echo "  1. Run 'idf.py menuconfig'"
    echo "  2. Navigate to 'ESP Wakeup Keypress' section"
    echo "  3. Configure SSH hostname (192.168.1.64), username (Edward), and password"
else
    echo -e "${RED}❌ Failed to set up ESP-IDF environment.${NC}"
fi

# Add a reminder to use source with this script
if ! echo "$0" | grep -q "source"; then
    echo -e "${YELLOW}⚠️  Remember: This script must be run with 'source' command:${NC}"
    echo -e "    ${YELLOW}source $(basename $0)${NC}"
fi
