#!/bin/bash

# Sentinel Edge-AI Wildfire Detection System
# Deployment Script for Raspberry Pi
#
# Usage: sudo ./scripts/deploy.sh [node_id] [node_name]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default values
NODE_ID=${1:-1}
NODE_NAME=${2:-"sentinel-node-${NODE_ID}"}

echo "========================================="
echo "Sentinel Edge-AI Deployment Script"
echo "========================================="
echo ""
echo "Node ID: $NODE_ID"
echo "Node Name: $NODE_NAME"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "ERROR: This script must be run as root (use sudo)"
    exit 1
fi

# Get the actual user
ACTUAL_USER=${SUDO_USER:-$USER}
USER_HOME=$(eval echo ~$ACTUAL_USER)

echo "[1/9] Checking prerequisites..."

# Check if build exists
if [ ! -f "$PROJECT_ROOT/build/sentinel" ]; then
    echo "ERROR: Sentinel binary not found. Please build first:"
    echo "  mkdir build && cd build"
    echo "  cmake .."
    echo "  make -j4"
    exit 1
fi

echo "✓ Binary found"

# Check if config exists
if [ ! -f "$PROJECT_ROOT/configs/node_config.json" ]; then
    echo "ERROR: Configuration file not found: configs/node_config.json"
    exit 1
fi

echo "✓ Configuration found"

# Stop existing service if running
echo "[2/9] Stopping existing service..."
if systemctl is-active --quiet sentinel; then
    systemctl stop sentinel
    echo "✓ Service stopped"
else
    echo "✓ No service running"
fi

# Install binary
echo "[3/9] Installing binary..."
install -m 755 "$PROJECT_ROOT/build/sentinel" /usr/local/bin/sentinel
echo "✓ Binary installed to /usr/local/bin/sentinel"

# Create directories
echo "[4/9] Creating application directories..."
mkdir -p /etc/sentinel
mkdir -p /var/log/sentinel
mkdir -p /var/lib/sentinel/data
mkdir -p /var/lib/sentinel/models

# Set permissions
chown -R $ACTUAL_USER:$ACTUAL_USER /var/log/sentinel
chown -R $ACTUAL_USER:$ACTUAL_USER /var/lib/sentinel
chown -R $ACTUAL_USER:$ACTUAL_USER /etc/sentinel

echo "✓ Directories created"

# Install configuration
echo "[5/9] Installing configuration..."
cp "$PROJECT_ROOT/configs/node_config.json" /etc/sentinel/config.json

# Update node ID in config
sed -i "s/\"id\": [0-9]*/\"id\": $NODE_ID/" /etc/sentinel/config.json
sed -i "s/\"name\": \"[^\"]*\"/\"name\": \"$NODE_NAME\"/" /etc/sentinel/config.json

chown $ACTUAL_USER:$ACTUAL_USER /etc/sentinel/config.json
echo "✓ Configuration installed"

# Install model
echo "[6/9] Installing AI model..."
if [ -f "$PROJECT_ROOT/models/smoke_detection.tflite" ]; then
    cp "$PROJECT_ROOT/models/smoke_detection.tflite" /var/lib/sentinel/models/
    chown $ACTUAL_USER:$ACTUAL_USER /var/lib/sentinel/models/smoke_detection.tflite
    echo "✓ Model installed"
else
    echo "⚠ Warning: Model file not found, creating placeholder"
    echo "Placeholder model" > /var/lib/sentinel/models/smoke_detection.tflite
fi

# Update model path in config
sed -i 's|"model_path": "[^"]*"|"model_path": "/var/lib/sentinel/models/smoke_detection.tflite"|' /etc/sentinel/config.json

# Create systemd service
echo "[7/9] Creating systemd service..."

cat > /etc/systemd/system/sentinel.service << EOF
[Unit]
Description=Sentinel Edge-AI Wildfire Detection System
After=network.target

[Service]
Type=simple
User=$ACTUAL_USER
Group=$ACTUAL_USER
WorkingDirectory=/var/lib/sentinel
ExecStart=/usr/local/bin/sentinel --config /etc/sentinel/config.json
Restart=always
RestartSec=10
StandardOutput=append:/var/log/sentinel/sentinel.log
StandardError=append:/var/log/sentinel/sentinel.log

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/log/sentinel /var/lib/sentinel

# Resource limits
LimitNOFILE=65536
CPUQuota=80%
MemoryLimit=512M

[Install]
WantedBy=multi-user.target
EOF

echo "✓ Systemd service created"

# Reload systemd
echo "[8/9] Reloading systemd..."
systemctl daemon-reload
echo "✓ Systemd reloaded"

# Enable and start service
echo "[9/9] Starting Sentinel service..."
systemctl enable sentinel
systemctl start sentinel

# Wait a moment for service to start
sleep 2

# Check status
if systemctl is-active --quiet sentinel; then
    echo "✓ Service started successfully"
else
    echo "✗ Service failed to start"
    echo ""
    echo "Check logs with: journalctl -u sentinel -f"
    exit 1
fi

echo ""
echo "========================================="
echo "Deployment Complete!"
echo "========================================="
echo ""
echo "Service Status:"
systemctl status sentinel --no-pager -l

echo ""
echo "Useful Commands:"
echo "  View logs:        journalctl -u sentinel -f"
echo "  Stop service:     sudo systemctl stop sentinel"
echo "  Start service:    sudo systemctl start sentinel"
echo "  Restart service:  sudo systemctl restart sentinel"
echo "  Service status:   sudo systemctl status sentinel"
echo "  Edit config:      sudo nano /etc/sentinel/config.json"
echo ""
echo "Configuration file: /etc/sentinel/config.json"
echo "Log file:          /var/log/sentinel/sentinel.log"
echo "Data directory:    /var/lib/sentinel/data"
echo ""

# Create helper scripts
echo "Creating helper scripts..."

# Log viewer script
cat > /usr/local/bin/sentinel-logs << 'EOF'
#!/bin/bash
journalctl -u sentinel -f --no-pager
EOF
chmod +x /usr/local/bin/sentinel-logs

# Status check script
cat > /usr/local/bin/sentinel-status << 'EOF'
#!/bin/bash
echo "========================================="
echo "Sentinel System Status"
echo "========================================="
echo ""
systemctl status sentinel --no-pager
echo ""
echo "Recent logs:"
journalctl -u sentinel -n 20 --no-pager
EOF
chmod +x /usr/local/bin/sentinel-status

# Restart script
cat > /usr/local/bin/sentinel-restart << 'EOF'
#!/bin/bash
echo "Restarting Sentinel service..."
sudo systemctl restart sentinel
sleep 2
sudo systemctl status sentinel --no-pager
EOF
chmod +x /usr/local/bin/sentinel-restart

echo "✓ Helper scripts created:"
echo "  sentinel-logs     - View live logs"
echo "  sentinel-status   - Check system status"
echo "  sentinel-restart  - Restart the service"
echo ""

# Display initial logs
echo "Initial startup logs:"
echo "========================================="
journalctl -u sentinel -n 50 --no-pager
echo ""

echo "Deployment successful! Sentinel is now running as a system service."
echo ""