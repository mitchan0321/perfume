#!/bin/sh

CWD=`pwd`
CONFDIR=${HOME}/.pmacs
DOTPMACS="lib/perfume/lib/dot-pmacs"

cd ./bin

sed -e s%@PREFIX@%${CWD}%g < pmacs.in > pmacs
chmod 755 pmacs
sed -e s%@PREFIX@%${CWD}%g < pmacs-client.in > pmacs-client
chmod 755 pmacs-client

cd ..

if [ ! -d ${CONFDIR} ]; then mkdir ${CONFDIR}; fi
if [ ! -f ${CONFDIR}/pmacs.conf ]; then install -m 644 ${DOTPMACS}/pmacs.conf ${CONFDIR}; fi
if [ ! -f ${CONFDIR}/default.key ]; then install -m 644 ${DOTPMACS}/default.key ${CONFDIR}; fi
if [ ! -f ${CONFDIR}/theme.conf ]; then install -m 644 ${DOTPMACS}/theme.conf ${CONFDIR}; fi
if [ ! -f ${CONFDIR}/startup.prfm ]; then install -m 644 ${DOTPMACS}/startup.prfm ${CONFDIR}; fi

exit 0
