#!/bin/sh

REVISION1=$(git log -1 | grep '^commit')
REVISION2=$(echo "$REVISION1" | cut -d ' ' -f2 | cut -c1-7)
GITLEVEL1=$(git rev-list --count HEAD)
GITLEVEL2=$(git log -1 | grep '^Date' | cut -c9-32)
VERSION_FILE=$1/root/.version
echo "version=1160`date +%Y%m%d%H%M`" >  $VERSION_FILE
echo "subversion=16.0 Public" >> $VERSION_FILE
echo "cvs=$GITLEVEL1-$REVISION2" >> $VERSION_FILE
echo "creator=public@ukcvs" >> $VERSION_FILE
echo "imagename=Commando" >> $VERSION_FILE
echo "homepage=http://www.ukcvs.net" >> $VERSION_FILE
echo "comment1=$REVISION1" >> $VERSION_FILE
echo "comment2=commit $GITLEVEL1 - $GITLEVEL2" >> $VERSION_FILE

