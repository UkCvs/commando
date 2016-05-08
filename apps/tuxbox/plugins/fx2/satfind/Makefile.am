AM_CPPFLAGS = -I$(srcdir)/../lib -I$(top_srcdir)/include -I$(DRIVER)/include

noinst_LTLIBRARIES = satfind.la

satfind_la_SOURCES = satfind.c

satfind_la_LIBFLAGS = $(top_builddir)/fx2/lib/libfx2.la

satfind_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version -lfx2

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install satfind.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/satfind.cfg $(DESTDIR)$(PLUGINDIR)

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/satfind.so
	-rm $(DESTDIR)$(PLUGINDIR)/satfind.cfg
	-rmdir $(DESTDIR)$(PLUGINDIR)
