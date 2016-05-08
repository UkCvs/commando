AM_CPPFLAGS = -I$(srcdir)/../lib -I$(top_srcdir)/include -I$(DRIVER)/include

noinst_LTLIBRARIES = outdoor.la

outdoor_la_SOURCES = outdoor.c

outdoor_la_LIBFLAGS = $(top_builddir)/fx2/lib/libfx2.la

outdoor_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version -lfx2

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install outdoor.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/outdoor.cfg $(DESTDIR)$(PLUGINDIR)

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/outdoor.so
	-rm $(DESTDIR)$(PLUGINDIR)/outdoor.cfg
	-rmdir $(DESTDIR)$(PLUGINDIR)
