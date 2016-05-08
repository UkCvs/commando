#!/bin/sh
killall kb2rcd
touch /tmp/keyboard.lck
kb2rcd --config=/var/tuxbox/config/kb2rcd_links.conf > /dev/null
HOME=/var/etc links_g -g http://www.mobi-list.de
killall kb2rcd
rm -f /tmp/keyboard.lck
