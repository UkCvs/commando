AC_INIT(tuxbox-apps-misc-tools,0.0.1,,tuxbox-misc-tools)
AM_INIT_AUTOMAKE(tuxbox-misc-tools,0.0.1)

TUXBOX_APPS
TUXBOX_BOXTYPE

AC_PROG_CC
AM_PROG_LIBTOOL

AC_ARG_WITH(etherwake,
	AS_HELP_STRING(--with-etherwake,replace Busybox applet ether-wake with original version),
	[enable_etherwake="$withval"],[enable_etherwake=no])
AM_CONDITIONAL(ENABLE_ETHERWAKE,test "$enable_etherwake" = "yes")

AC_OUTPUT([
Makefile
etherwake/Makefile
fbshot/Makefile
mtd/Makefile
rtc/Makefile
devinit/Makefile
dboxshot/Makefile
makedevices/Makefile
])
