AUTOMAKE_OPTIONS = gnu

SUBDIRS = \
	fbshot mtd devinit dboxshot
if BOXTYPE_DBOX2
SUBDIRS += rtc makedevices
endif
if ENABLE_ETHERWAKE
SUBDIRS += etherwake
endif
