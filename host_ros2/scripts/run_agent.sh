#!/usr/bin/env bash
set -e

# ==============================================================================
# Helper Script to Run micro-ROS Agent on Ubuntu WSL2
# ==============================================================================

PORT=8888
MODE=${1:-udp} # default: udp (or serial)

echo "=========================================================="
echo "          micro-ROS Agent Launcher for KidBright          "
echo "=========================================================="

# Display WSL & Host IP information
WSL_IP=$(hostname -I | awk '{print $1}')
GATEWAY_IP=$(ip route show default | awk '{print $3}')

echo "Network Diagnostics:"
echo "  • WSL2 Internal IP : $WSL_IP"
echo "  • Host / Gateway IP: $GATEWAY_IP"
echo ""
echo "Note for Wi-Fi Mode:"
echo "  If your KidBright board is on the same local Wi-Fi router:"
echo "  1) In Windows 11 (Mirrored mode): Use your PC's Wi-Fi IP (e.g. 192.168.1.xxx)"
echo "  2) In WSL2 NAT mode: Ensure port 8888/UDP is forwarded or use host IP."
echo "=========================================================="

# Source ROS 2 environment
if [ -f "/opt/ros/lyrical/setup.bash" ]; then
    source /opt/ros/lyrical/setup.bash
elif [ -f "/opt/ros/kilted/setup.bash" ]; then
    source /opt/ros/kilted/setup.bash
elif [ -n "$ROS_DISTRO" ] && [ -f "/opt/ros/$ROS_DISTRO/setup.bash" ]; then
    source "/opt/ros/$ROS_DISTRO/setup.bash"
fi

# Check if a custom micro-ros agent workspace exists
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS_DIR="$SCRIPT_DIR/../../ros2_ws"

if [ -f "$WS_DIR/install/local_setup.bash" ]; then
    source "$WS_DIR/install/local_setup.bash"
fi

if [ "$MODE" = "udp" ]; then
    echo "Starting micro-ROS Agent on UDP4 Port $PORT..."
    echo "Waiting for KidBright boards to connect..."
    ros2 run micro_ros_agent micro_ros_agent udp4 --port $PORT -v6 || {
        echo ""
        echo "If micro_ros_agent is not installed yet, you can build it using:"
        echo "  bash $SCRIPT_DIR/setup_agent_ws.sh"
    }
elif [ "$MODE" = "serial" ]; then
    DEV=${2:-/dev/ttyUSB0}
    BAUD=${3:-115200}
    echo "Starting micro-ROS Agent on Serial Device $DEV at $BAUD baud..."
    ros2 run micro_ros_agent micro_ros_agent serial --dev "$DEV" -b "$BAUD" -v6
else
    echo "Usage:"
    echo "  bash run_agent.sh udp [port]"
    echo "  bash run_agent.sh serial [/dev/ttyUSB0] [baudrate]"
fi
