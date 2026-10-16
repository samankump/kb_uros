#!/usr/bin/env bash
set -e

# ==============================================================================
# Build micro-ROS Agent Workspace for Ubuntu 26.04 (ROS 2 Kilted / Lyrical)
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS_DIR="$SCRIPT_DIR/../../ros2_ws"

echo "=========================================================="
echo "    Setting up micro-ROS Agent Workspace in: $WS_DIR      "
echo "=========================================================="

mkdir -p "$WS_DIR/src"
cd "$WS_DIR"

# Source ROS 2 base
if [ -f "/opt/ros/lyrical/setup.bash" ]; then
    source /opt/ros/lyrical/setup.bash
    DISTRO="lyrical"
elif [ -f "/opt/ros/kilted/setup.bash" ]; then
    source /opt/ros/kilted/setup.bash
    DISTRO="kilted"
else
    DISTRO="${ROS_DISTRO:-rolling}"
    source "/opt/ros/$DISTRO/setup.bash"
fi

echo "Using ROS 2 Distro: $DISTRO"

if [ ! -d "src/micro_ros_setup" ]; then
    echo "Cloning micro_ros_setup..."
    git clone -b "$DISTRO" https://github.com/micro-ROS/micro_ros_setup.git src/micro_ros_setup || \
    git clone -b kilted https://github.com/micro-ROS/micro_ros_setup.git src/micro_ros_setup || \
    git clone -b rolling https://github.com/micro-ROS/micro_ros_setup.git src/micro_ros_setup
fi

echo "Building micro_ros_setup base..."
colcon build

source install/local_setup.bash

echo "Creating micro-ROS Agent workspace..."
ros2 run micro_ros_setup create_agent_ws.sh

echo "Building micro-ROS Agent..."
ros2 run micro_ros_setup build_agent.sh

echo "=========================================================="
echo "Build finished! Source with:"
echo "  source $WS_DIR/install/local_setup.bash"
echo "Then run:"
echo "  ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888"
echo "=========================================================="
