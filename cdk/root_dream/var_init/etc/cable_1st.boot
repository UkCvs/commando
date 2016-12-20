cd /var/bin
rm -f epg* stats
mv -f miniops-c miniops
chmod 755 miniops
cd $etc
rm -fr headers info
rm -f .ab-fast .parent a*.csv bq* ch* cu* dic* mult* net* sat* sml* sup* the*
mv -f issue.net-c issue.net
cd /var/tuxbox/plugins
rm -f 00-shell* 02-auto* 03-get*
cd $cfg
rm -fr enigma
mv -f scan.conf-c scan.conf
rm -f menu_* scan.end scan.start shell*
cd $cfg/zapit
rm -f *.xml tv* *.pid
cd /
