# dirs / files placed in this list will remain in, 
# /dreambox/cdkflash/root-neutrino-squashfs folder,
# but will not be added to root-neutrino-squashfs.img
#####################################################

# please leave this var_init ignore alone - LraiZer ;) 
# we build this into dedicated var jffs2 in the image
flashprefix/var_init

# ignore 1.2 MB of cosmetics
flashprefix/share/tuxbox/neutrino/icons/shutdown.raw
flashprefix/share/tuxbox/neutrino/icons/shutdown.pal
flashprefix/share/tuxbox/neutrino/icons/scan.raw
flashprefix/share/tuxbox/neutrino/icons/scan.pal
flashprefix/share/tuxbox/neutrino/icons/radiomode.raw
flashprefix/share/tuxbox/neutrino/icons/radiomode.pal

# ignore for download packaging:
flashprefix/bin/dvbsnoop
flashprefix/bin/input
flashprefix/bin/msgbox
flashprefix/bin/shellexec
flashprefix/bin/dccamd
flashprefix/bin/inadyn-mt
flashprefix/bin/smbmount
flashprefix/bin/smbmnt
flashprefix/sbin/openvpn
flashprefix/lib/libssl.so.0.9.7
flashprefix/lib/libcrypto.so.0.9.7
flashprefix/lib/tuxbox/plugins/mosaic.so
flashprefix/lib/tuxbox/plugins/mosaic.cfg
flashprefix/lib/tuxbox/plugins/dvbsub.so
flashprefix/lib/tuxbox/plugins/dvbsub.cfg
flashprefix/lib/tuxbox/plugins/shellexec.so
flashprefix/lib/tuxbox/plugins/shellexec.cfg
flashprefix/lib/tuxbox/plugins/pip.so
flashprefix/lib/tuxbox/plugins/pip.cfg

# ignore folders
flashprefix/share/tuxbox/enigma

# ignore locales
flashprefix/share/tuxbox/neutrino/locale/bayrisch.locale
flashprefix/share/tuxbox/neutrino/locale/bosanski.locale
flashprefix/share/tuxbox/neutrino/locale/ch-baslerdeutsch.locale
flashprefix/share/tuxbox/neutrino/locale/ch-berndeutsch.locale
#flashprefix/share/tuxbox/neutrino/locale/deutsch.locale
flashprefix/share/tuxbox/neutrino/locale/ellinika.locale
flashprefix/share/tuxbox/neutrino/locale/francais.locale
flashprefix/share/tuxbox/neutrino/locale/italiano.locale
flashprefix/share/tuxbox/neutrino/locale/nederlands.locale
flashprefix/share/tuxbox/neutrino/locale/polski.locale
flashprefix/share/tuxbox/neutrino/locale/portugues.locale
flashprefix/share/tuxbox/neutrino/locale/russkij.locale
flashprefix/share/tuxbox/neutrino/locale/suomi.locale
flashprefix/share/tuxbox/neutrino/locale/svenska.locale


