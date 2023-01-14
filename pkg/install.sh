#!/bin/sh

CWD=`pwd`
cd ./bin
sed -e s%@PREFIX@%${CWD}%g < pmacs.in > pmacs
chmod 755 pmacs
sed -e s%@PREFIX@%${CWD}%g < pmacs-client.in > pmacs-client
chmod 755 pmacs-client

exit 0
