AUTOMAKE_OPTIONS = gnu

SUBDIRS = \
	liblcddisplay \
	liblircdclient

if ENABLE_TUXTXT
SUBDIRS += \
	libtuxtxt
endif

if !BOXTYPE_GENERIC
if !BOXTYPE_TRIPLE
SUBDIRS += \
	libavs
endif
endif
