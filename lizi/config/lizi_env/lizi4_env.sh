#!/bin/sh
export ROS_HOSTNAME="192.168.0.104"
export ROS_IP="192.168.0.104"
. ~/catkin_ws/devel/setup.sh
exec "$@"
