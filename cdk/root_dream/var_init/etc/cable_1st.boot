# changelog for C16 cable
# Wed 04 Jan 2017: add default services and bouquets (Luton- bandit area)- AL ;)

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
# WIP...
# mv -f scan.end-c scan.end
# mv -f scan.start-c scan.start
rm -f menu_* shell*
cd $cfg/zapit
mv -f bouquets-c.xml bouquets.xml
mv -f services-c.xml services.xml
rm -f tv* *.pid
cd /
