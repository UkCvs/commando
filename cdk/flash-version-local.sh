#!/bin/sh

VERSION_FILE=$1/root/.version
echo "version=1200`date +%Y%m%d%H%M`" >  $VERSION_FILE
echo "creator=public@ukcvs" >> $VERSION_FILE
echo "imagename=Based-on-Commando" >> $VERSION_FILE
echo "homepage=http://www.ukcvs.net" >> $VERSION_FILE

