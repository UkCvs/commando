# changelog for C16 cable (conditional call from start_neutrino on first boot)
# Wed 04 Jan 2017: add default services and bouquets (Luton- bandit area)- AL ;)
# Thu 12 Jan 2017: let's have some helpfiles for cable, too? -AL
# Sun 22 Jan 2017: added scan.end and scan.start scripts- AL
# Wed 22 Mar 2017: added shellexec menu support for cable- AL
# Sun 26 Mar 2017: corrected shellexec menu support for cable (key Blue)- AL

cd /var/bin
rm -f epg*
mv -f miniops-c miniops
mv -f stats-c stats
cd $etc
rm -f .ab-fast .parent a*.csv bq* ch* cu* dic* mult* sat* sml* sup* the*
mv -f issue.net-c issue.net
cd $etc/info
rm -f Auto* Get* Multi* Set*
cd /var/tuxbox/plugins
rm -f 02-auto* 03-get*
sed -i 's/Toolbox/Utilities/
/desc=/cdesc=this is NOT for plugin Timers!!
' 00-shellexec.cfg
cd $cfg
rm -fr enigma
# Disable broken cable scan end script, PaphosAL (RIP) is no longer with us to fix/maintain ;(
# mv -f scan.end-c scan.end
mv -f scan.start-c scan.start
mv -f shellexec-c.conf shellexec.conf
rm -f menu_a* menu_b* menu_g* menu_P* menu_S* menu_u*
cd $cfg/zapit
mv -f bouquets-c.xml bouquets.xml
mv -f services-c.xml services.xml
rm -f tv* *.pid
cd /
