AUTOMAKE_OPTIONS = gnu

SUBDIRS = \
	autobouquets getepg nitscan include tuxmail tuxcom tuxcal tuxclock fx2 vncviewer enigma shellexec tuxwetter sysinfo clock logomask blockads links

if ENABLE_TUXTXT
SUBDIRS += tuxtxt
endif

if BOXTYPE_DREAMBOX
SUBDIRS += pip mosaic
else
SUBDIRS += pluginx
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	tuxbox-plugins.pc

