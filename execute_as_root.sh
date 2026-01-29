#!/bin/bash

# Allow root to access the display
if command -v xhost &> /dev/null; then
    xhost +SI:localuser:root
elif command -v xauth &> /dev/null; then
    # Fallback using xauth: extract cookie and merge into root's authority
    xauth extract - $DISPLAY | sudo xauth -f /root/.Xauthority merge -
else
    echo "Warning: Neither xhost nor xauth found. GUI elevation may fail."
fi

# Run the app with sudo, explicitly preserving environment
sudo -E env DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY ./build/kerneldrive

if command -v xhost &> /dev/null; then
    xhost -SI:localuser:root
fi
