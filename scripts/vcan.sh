#!/bin/bash
#
# Sets up a virtual CAN bus interface, "vcan0", on Linux. 
#
# If SOCKPP_WITH_CAN=ON, the virtual CAN interface is required to be
# loaded into the kernel to pass the unit and integration tests.
#

# Must have root privileges to run this script

if (( $EUID != 0 )); then
  echo "This script must be run as root"
  exit 1
fi

# Default to use "vcan0" if none is specified on the cmd line

IFACE=vcan0
[ -n "$1" ] && IFACE=$1

# Load the 'vcan' kernel module

VCAN_LOADED=$(lsmod | grep ^vcan)
if [ -z "${VCAN_LOADED}" ]; then
    if ! modprobe vcan ; then
        printf "Unable to load the 'vcan' kernel module.\n"
        exit 1
    fi
fi

# Add and set up the CAN interface
# Request an MTU size of 72 to allow for FD frames

ip link add type vcan && \
    ip link set "${IFACE}" mtu 72
    ip link set up "${IFACE}"

