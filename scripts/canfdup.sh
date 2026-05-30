#!/bin/bash
#
# canfdup.sh
#
# Brings up a Linux SocketCAN FD (CANbus FD) interface.
#
# USAGE: canfdup.sh [iface] [bitrate] [data_bitrate]
#

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

IFACE=can0
BITRATE=500000
DATABITRATE=2000000

[ -n "$1" ] && IFACE=$1
[ -n "$2" ] && BITRATE=$2
[ -n "$3" ] && DATABITRATE=$3

ip link set ${IFACE} up type can bitrate ${BITRATE} dbitrate ${DATABITRATE} fd on && \
  printf "%s brought up at bitrate %s\n" "${IFACE}" "${BITRATE}"

