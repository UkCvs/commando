AM_CPPFLAGS = -I$(DRIVER)/include

noinst_LTLIBRARIES = libfx2.la

libfx2_la_SOURCES = draw.c pig.c rcinput.c math.c

libfx2_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	$(LIBTOOL) --mode=install install libfx2.la $(DESTDIR)$(libdir)

uninstall-local:
	-rm $(DESTDIR)$(libdir)/libfx2.so
