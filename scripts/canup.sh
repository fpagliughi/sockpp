#!/bin/bash
#
# canup.sh
#
# Brings up a Linux SocketCAN (CANbus) intterface.
#
# USAGE: canup.sh [iface] [bitrate]
#

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

IFACE=can0
BITRATE=500000

[ -n "$1" ] && IFACE=$1
[ -n "$2" ] && BITRATE=$2

ip link set ${IFACE} up type can bitrate ${BITRATE} && \
  printf "%s brought up at bitrate %s\n" "${IFACE}" "${BITRATE}"

