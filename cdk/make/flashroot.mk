$(flashprefix):
	$(INSTALL) -d $@

$(flashprefix)/root: bootstrap $(wildcard root-local.sh) | $(flashprefix)
	rm -rf $@
	$(INSTALL) -d $@/bin
	$(INSTALL) -d $@/dev
	$(INSTALL) -d $@/lib/tuxbox
if !BOXTYPE_DREAMBOX
	$(INSTALL) -d $@/mnt
endif
	$(INSTALL) -d $@/proc
	$(INSTALL) -d $@/sbin
	$(INSTALL) -d $@/share/tuxbox
	$(INSTALL) -d $@/share/fonts
	$(INSTALL) -d $@/var/tuxbox/config
	$(INSTALL) -d $@/var/etc
	$(INSTALL) -d $@/tmp
	$(INSTALL) -d $@/etc/init.d
	$(INSTALL) -d $@/root
if KERNEL26
	$(INSTALL) -d $@/sys
endif
	ln -s /tmp $@/var/run
	ln -s /tmp $@/var/tmp
if ENABLE_IDE
if BOXTYPE_DREAMBOX
	$(INSTALL) -d $@/hdd
else
	$(INSTALL) -d $@/mnt/hdd1
	$(INSTALL) -d $@/mnt/hdd2
	$(INSTALL) -d $@/mnt/mmc
	ln -sf /mnt/hdd1 $@/hdd
endif
endif
	$(MAKE) $@/etc/update.urls

if BOXTYPE_DBOX2
	$(MAKE) flash-tuxinfo
	$(MAKE) flash-camd2
	$(MAKE) flash-ucodes
endif
	$(MAKE) flash-config
	$(MAKE) flash-busybox
	$(MAKE) flash-ftpd
	$(MAKE) flash-streampes
if ENABLE_FS_LUFS
	$(MAKE) flash-lufsd
endif
if ENABLE_ETHERWAKE
	$(MAKE) flash-ether-wake
endif
if BOXTYPE_DREAMBOX
# TODO: pip and mosaic only work with neutrino...
	$(MAKE) flash-pip
	$(MAKE) flash-mosaic
if ENABLE_IDEMMC
	$(MAKE) flash-sfdisk
endif
endif
if ENABLE_MOUNT_STANDALONE
	$(MAKE) flash-mount
endif
if ENABLE_AUTOMOUNT
	$(MAKE) flash-automount
endif
if BOXTYPE_DBOX2
if KERNEL26
# those right now only make sense on dbox2 since the other platforms use devfs
	$(MAKE) flash-makedevices
	$(MAKE) flash-hotplug
endif
endif
if ENABLE_E2FSPROGS
	$(MAKE) flash-e2fsprogs
endif
if ENABLE_XFSPROGS
	$(MAKE) flash-xfsprogs
endif
if ENABLE_REISERFS
	$(MAKE) flash-reiserfsprogs
endif
if ENABLE_DOSFSTOOLS
	$(MAKE) flash-dosfstools
endif
if ENABLE_NFSSERVER
	$(MAKE) flash-nfsserver
endif
if ENABLE_SAMBASERVER
	$(MAKE) flash-sambaserver
endif
if ENABLE_FS_SMBFS
	$(MAKE) flash-smbmount
endif
if ENABLE_GERMAN_KEYMAPS
	$(MAKE) flash-german-keymaps
endif
if ENABLE_AFORMAT
	$(MAKE) flash-aformat
endif
if ENABLE_BLOCKADS
	$(MAKE) flash-blockads
endif
if ENABLE_CDKVCINFO
	$(MAKE) flash-cdkVcInfo
endif
if ENABLE_CLOCK
	$(MAKE) flash-clock
endif
if ENABLE_DBOXSHOT
	$(MAKE) flash-dboxshot
endif
if ENABLE_DROPBEAR
	$(MAKE) flash-dropbear
endif
if ENABLE_DVBSNOOP
	$(MAKE) flash-dvbsnoop
endif
if ENABLE_ERASEALL
	$(MAKE) flash-eraseall
endif
if ENABLE_FBSHOT
	$(MAKE) flash-fbshot
endif
if ENABLE_GDBSERVER
	$(MAKE) flash-gdbserver
endif
if ENABLE_GETRC
	$(MAKE) flash-getrc
endif
if ENABLE_HDDTEMP
	$(MAKE) flash-hddtemp
endif
if ENABLE_HTOP
	$(MAKE) flash-htop
endif
if ENABLE_INADYN_MT
	$(MAKE) flash-inadyn-mt
endif
if ENABLE_INPUT
	$(MAKE) flash-input
endif
if ENABLE_IPKG
	$(MAKE) flash-ipkg
endif
if ENABLE_KB2RCD
	$(MAKE) flash-kb2rcd
endif
if ENABLE_LCSHOT
	$(MAKE) flash-lcshot
endif
if ENABLE_LINKS
	$(MAKE) flash-links
endif
if ENABLE_LINKS_G
	$(MAKE) flash-links_g
endif
if ENABLE_LIRC
	$(MAKE) flash-lircd
endif
if ENABLE_LOGOMASK
	$(MAKE) flash-logomask
endif
if ENABLE_MSGBOX
	$(MAKE) flash-msgbox
endif
if ENABLE_NETIO
	$(MAKE) flash-netio
endif
if ENABLE_OPENNTPD
	$(MAKE) flash-openntpd
endif
if ENABLE_OPENVPN
	$(MAKE) flash-openvpn
endif
if ENABLE_PROCPS
	$(MAKE) flash-procps
endif
if ENABLE_RTC
	$(MAKE) flash-rtc
endif
if ENABLE_SATFIND
	$(MAKE) flash-satfind
endif
if ENABLE_SHELLEXEC
	$(MAKE) flash-shellexec
endif
if ENABLE_SQLITE
	$(MAKE) flash-sqlite
endif
if ENABLE_STRACE
	$(MAKE) flash-strace
endif
if ENABLE_SYSINFO
	$(MAKE) flash-sysinfo
endif
if ENABLE_TUXCAL
	$(MAKE) flash-tuxcal
endif
if ENABLE_TUXCOM
	$(MAKE) flash-tuxcom
endif
if ENABLE_TUXMAIL
	$(MAKE) flash-tuxmail
endif
if ENABLE_TUXTXT
	$(MAKE) flash-tuxtxt
endif
if ENABLE_TUXWETTER
	$(MAKE) flash-tuxwetter
endif
if ENABLE_VNCVIEWER
	$(MAKE) flash-vncviewer
endif
if ENABLE_WGET
	$(MAKE) flash-wget
endif

if ENABLE_FX2_BOUQUET
	$(MAKE) flash-bouquet-fx2
endif
if ENABLE_FX2_C64EMU
	$(MAKE) flash-c64emu-fx2
endif
if ENABLE_FX2_LCDCIRC
	$(MAKE) flash-lcdcirc-fx2
endif
if ENABLE_FX2_LEMM
	$(MAKE) flash-lemm-fx2
endif
if ENABLE_FX2_MASTER
	$(MAKE) flash-master-fx2
endif
if ENABLE_FX2_MINES
	$(MAKE) flash-mines-fx2
endif
if ENABLE_FX2_OUTDOOR
	$(MAKE) flash-outdoor-fx2
endif
if ENABLE_FX2_PAC
	$(MAKE) flash-pac-fx2
endif
if ENABLE_FX2_SATFIND
	$(MAKE) flash-satfind-fx2
endif
if ENABLE_FX2_SNAKE
	$(MAKE) flash-snake-fx2
endif
if ENABLE_FX2_SOKOBAN
	$(MAKE) flash-sokoban-fx2
endif
if ENABLE_FX2_SOL
	$(MAKE) flash-sol-fx2
endif
if ENABLE_FX2_SOLITAIR
	$(MAKE) flash-solitair-fx2
endif
if ENABLE_FX2_SUDOKU
	$(MAKE) flash-sudoku-fx2
endif
if ENABLE_FX2_TANK
	$(MAKE) flash-tank-fx2
endif
if ENABLE_FX2_TETRIS
	$(MAKE) flash-tetris-fx2
endif
if ENABLE_FX2_VIERG
	$(MAKE) flash-vierg-fx2
endif
if ENABLE_FX2_YAHTZEE
	$(MAKE) flash-yahtzee-fx2
endif
if ENABLE_AUDIOPLAY
	$(MAKE) flash-tool-audioplay
endif
if ENABLE_AVIAEXT
	$(MAKE) flash-tool-aviaext
endif
if ENABLE_AVIAFBTOOL
	$(MAKE) flash-tool-aviafbtool
endif
if ENABLE_AVSWITCH
	$(MAKE) flash-tool-avswitch
endif
if ENABLE_FBCLEAR
	$(MAKE) flash-tool-fbclear
endif
if ENABLE_LCDDUMP
	$(MAKE) flash-tool-lcddump
endif
if ENABLE_PLUGINX
	$(MAKE) flash-tool-pluginx
endif
if ENABLE_RCINFO
	$(MAKE) flash-tool-rcinfo
endif
if ENABLE_RCSIM
	$(MAKE) flash-tool-rcsim
endif
if ENABLE_SAA
	$(MAKE) flash-tool-saa
endif
if ENABLE_SHOWPTSDIFF
	$(MAKE) flash-tool-showptsdiff
endif
if ENABLE_SWITCH
	$(MAKE) flash-tool-switch
endif
	$(MAKE) flash-defaultlocale
	$(MAKE) flash-version
	@FLASHROOTDIR_MODIFIED@
	@TUXBOX_CUSTOMIZE@
