#!/bin/sh

CWD=`pwd`
cd ./bin
sed -e s%@PREFIX@%${CWD}%g < pmacs.in > pmacs
chmod 755 pmacs
exit 0
