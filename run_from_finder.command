#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Change to the project directory
cd "$SCRIPT_DIR"

# Check if cmake-build-debug exists
if [ ! -d "cmake-build-debug" ]; then
    echo "Error: cmake-build-debug directory not found!"
    echo "Please build the project first by running:"
    echo "mkdir cmake-build-debug && cd cmake-build-debug && cmake .. && make"
    read -p "Press Enter to exit..."
    exit 1
fi

# Check if the executable exists
if [ ! -f "cmake-build-debug/BlackEngineProject" ]; then
    echo "Error: BlackEngineProject executable not found!"
    echo "Please build the project first by running:"
    echo "cd cmake-build-debug && make"
    read -p "Press Enter to exit..."
    exit 1
fi

# Run the program
echo "Starting BlackEngineProject..."
cd cmake-build-debug
./BlackEngineProject

# Keep the terminal window open if there's an error
if [ $? -ne 0 ]; then
    echo "Program exited with error code $?"
    read -p "Press Enter to exit..."
fi
