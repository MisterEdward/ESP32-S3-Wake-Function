#!/bin/bash

# Find the ESP-IDF installation
if [ -d "$HOME/esp/esp-idf" ]; then
    IDF_PATH="$HOME/esp/esp-idf"
elif [ -d "/opt/esp/esp-idf" ]; then
    IDF_PATH="/opt/esp/esp-idf"
else
    echo "ESP-IDF not found. Please specify your ESP-IDF installation path:"
    read -r IDF_PATH
    
    if [ ! -d "$IDF_PATH" ]; then
        echo "Invalid ESP-IDF path. Please install ESP-IDF and try again."
        exit 1
    fi
fi

# Source the export script
echo "Setting up ESP-IDF environment from: $IDF_PATH"

# Source the ESP-IDF export script
. "$IDF_PATH/export.sh"

echo ""
echo "ESP-IDF environment set up successfully!"
echo "You can now use idf.py commands, such as:"
echo "  - idf.py menuconfig"
echo "  - idf.py build"
echo "  - idf.py flash"
echo "  - idf.py monitor"
echo ""
echo "Run 'source setup-env.sh' whenever you open a new terminal session"
