#!/bin/sh
mknod /dev/swioreg0 c  $(awk "\$2==\"swioreg\" {print \$1}" /proc/devices) 0
chmod 0666 /dev/swioreg0
