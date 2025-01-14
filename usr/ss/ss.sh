#!/bin/sh

if [ $# -lt 1 ]; then
  echo "Usage : `basename $0` <filename> [<as filename>]" >&2
  echo "Usage : `basename $0` <filename> [<portNum>]" >&2

  exit 1
fi

case `uname` in
  Linux )
     dev="/dev/ttyS3"
     if [ $# = 2 ]; then
     	dev="/dev/ttyS${2}"
     fi
     echo "use port:${dev}"
     stty -F ${dev} cs8 9600 -ixon -isig -icanon -onlcr ;;
#  FreeBSD )
#     dev="/dev/cuaU0" ;;
  Darwin )
     dev=`ls /dev/cu.usbserial-* 2>/dev/null` ;;
  * )
     echo "`uname` is nsupported." >&2 ;;
esac

if [ `echo ${dev} | wc -w` -lt 1 ]; then
  echo "No devices were detected." >&2
  exit 1
fi

if [ `echo ${dev} | wc -w` -gt 1 ]; then
  echo "Two or more devices were detected." >&2
  echo "${dev}" | awk '{printf("\t%s\n", $0);}' >&2
  echo "Detach needless devices." >&2
  exit 1
fi

ss.bin $1 > ${dev} < ${dev}
