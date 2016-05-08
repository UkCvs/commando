all:
	@echo "You probably do not want to build all possible targets."
	@echo "Sensible targets are, e.g. yadd-enigma or flash-neutrino-jffs2-2x."
	@echo "If you REALLY want to build everything, then \"make everything\""

everything: yadd-all flash-all-all-all serversupport extra 

if KERNEL26
bare-os: yadd-u-boot kernel-cdk driver busybox \
		tuxbox_hotplug tuxinfo misc_tools
else
bare-os: yadd-u-boot kernel-cdk driver busybox tuxinfo misc_tools
endif
	@TUXBOX_YADD_CUSTOMIZE@

if ENABLE_IDEMMC
FILESYSTEM_DEBS = utillinux
else
if ENABLE_MOUNT_STANDALONE
FILESYSTEM_DEBS = utillinux
else
FILESYSTEM_DEBS =
endif
endif
if ENABLE_E2FSPROGS
FILESYSTEM_DEBS += e2fsprogs
endif
if ENABLE_XFSPROGS
FILESYSTEM_DEBS += xfsprogs
endif
if ENABLE_REISERFSPROGS
FILESYSTEM_DEBS += reiserfsprogs
endif
if ENABLE_DOSFSTOOLS
FILESYSTEM_DEBS += dosfstools
endif
if ENABLE_FS_SMBFS
FILESYSTEM_DEBS += sambaserver
endif

ADDITIONAL_DEBS =
if ENABLE_CDKVCINFO
ADDITIONAL_DEBS += cdkVcInfo
endif
if ENABLE_GDBSERVER
ADDITIONAL_DEBS += gdb
endif
if ENABLE_HDDTEMP
ADDITIONAL_DEBS += hddtemp
endif
if ENABLE_HTOP
ADDITIONAL_DEBS += htop
endif
if ENABLE_INADYN_MT
ADDITIONAL_DEBS += inadyn-mt
endif
if ENABLE_LINKS
ADDITIONAL_DEBS += links
endif
if ENABLE_LINKS_G
ADDITIONAL_DEBS += links_g
endif
if ENABLE_LIRC
ADDITIONAL_DEBS += lirc
endif
if ENABLE_NETIO
ADDITIONAL_DEBS += netio
endif
if ENABLE_OPENVPN
ADDITIONAL_DEBS += openvpn
endif
if ENABLE_PROCPS
ADDITIONAL_DEBS += procps
endif
if ENABLE_STRACE
ADDITIONAL_DEBS += strace
endif
if ENABLE_WGET
ADDITIONAL_DEBS += wget
endif

yadd-none: bare-os config tuxbox_tools lcd ftpd yadd-ucodes yadd-bootlogos @AUTOMOUNT@ @NFSSERVER@ @SAMBASERVER@ @LUFS@ @CONSOLE_TOOLS@ $(ADDITIONAL_DEBS) $(FX2_DEPS) $(FILESYSTEM_DEBS) version defaultlocale
	@TUXBOX_YADD_CUSTOMIZE@

yadd-none-etc: yadd-none yadd-etc
	@TUXBOX_YADD_CUSTOMIZE@

yadd-os: bare-os ftpd yadd-ucodes yadd-bootlogos yadd-etc @AUTOMOUNT@ $(FILESYSTEM_DEBS)
	@TUXBOX_YADD_CUSTOMIZE@

yadd-micro-neutrino: bare-os config yadd-ucodes camd2 switch neutrino
	@TUXBOX_YADD_CUSTOMIZE@

yadd-neutrino: neutrino-plugins yadd-none-etc @ESOUND@ neutrino
	@TUXBOX_YADD_CUSTOMIZE@

yadd-enigma: yadd-none-etc enigma-plugins enigma
	@TUXBOX_YADD_CUSTOMIZE@

yadd-lcars: yadd-none-etc lcars
	@TUXBOX_YADD_CUSTOMIZE@

yadd-radiobox: yadd-none-etc radiobox
	@TUXBOX_YADD_CUSTOMIZE@

yadd-all: yadd-none-etc plugins neutrino enigma lcars @ESOUND@
	@TUXBOX_YADD_CUSTOMIZE@

yadd-bootlogos:
	$(INSTALL) -d $(targetprefix)/var/tuxbox/boot
	if [ -e $(logosdir)/logo-lcd  ] ; then \
		 cp $(logosdir)/logo-lcd $(targetprefix)/var/tuxbox/boot ; \
	fi
	if [ -e $(logosdir)/logo-fb ] ; then \
		 cp $(logosdir)/logo-fb $(targetprefix)/var/tuxbox/boot ; \
	fi

extra: libs libs_optional contrib_apps fun dvb_apps root_optional udev devel devel_optional
