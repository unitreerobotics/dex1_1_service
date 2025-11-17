#!/bin/bash

# ==============================================================
# dex1_1_service Auto-Start Setup Script
# --------------------------------------------------------------
# This script sets up a systemd service to automatically start
# the dex1_1_service on system boot.
#
# Usage:
#   bash setup_autostart.sh
#
# After setup, manage the service with:
#   sudo systemctl status dex1_gripper.service     # Check status
#   sudo journalctl -u dex1_gripper.service -f     # View logs
#   sudo systemctl stop $SERVICE_NAME              # Stop service
#   sudo systemctl restart dex1_gripper.service    # Restart service
#   sudo systemctl disable dex1_gripper.service    # Disable auto-start on boot
#
# Author: https://github.com/silencht
# Company: Unitree Robotics
# Copyright © 2016–2025 YuShu Technology Co., Ltd. All Rights Reserved.
# ==============================================================

set -e
echo "=== Setting up dex1_gripper auto-start service ==="

SERVICE_NAME="dex1_gripper.service"
SERVICE_FILE="/etc/systemd/system/$SERVICE_NAME"
SCRIPT_BIN="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/bin"
echo "Script directory detected: $SCRIPT_BIN"

if [ ! -d "$SCRIPT_BIN" ]; then
    echo "Error: Script directory $SCRIPT_BIN does not exist."
    exit 1
fi

sudo tee $SERVICE_FILE > /dev/null << EOL
[Unit]
Description=Dex1-1 Service Start
After=multi-user.target

[Service]
Type=simple
User=unitree
Group=dialout
WorkingDirectory=$SCRIPT_BIN
ExecStart=$SCRIPT_BIN/dex1_1_gripper_server
Restart=always
RestartSec=5
Environment="LD_LIBRARY_PATH=$(dirname $SCRIPT_BIN)/lib:$LD_LIBRARY_PATH"
StandardOutput=journal+console
StandardError=journal+console

[Install]
WantedBy=multi-user.target
EOL

echo "reloading systemd daemon..."
sudo systemctl daemon-reload

echo "enabling service to start on boot..."
sudo systemctl enable $SERVICE_NAME

echo "starting service now..."
sudo systemctl restart $SERVICE_NAME

echo "checking service status..."
sleep 2
sudo systemctl status $SERVICE_NAME --no-pager

echo ""
echo "=== dex1_gripper auto-start service setup completed successfully! ==="
echo "You can manage the service using:"
echo "  sudo systemctl status $SERVICE_NAME      # Check status"
echo "  sudo journalctl -u $SERVICE_NAME -f      # View logs in real-time"
echo "  sudo systemctl restart $SERVICE_NAME     # Restart service"
echo "  sudo systemctl stop $SERVICE_NAME        # Stop service"
echo "  sudo systemctl disable $SERVICE_NAME     # Disable auto-start on boot"
echo ""
echo "You also could test the gripper server manually by running:"
echo "  sudo $SCRIPT_BIN/test_dex1_1_gripper_server -h"
echo "================================================================"