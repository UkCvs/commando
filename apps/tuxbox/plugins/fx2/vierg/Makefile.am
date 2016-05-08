AM_CPPFLAGS = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = vierg.la

vierg_la_SOURCES = board.c somain.c

vierg_la_LIBFLAGS = $(top_builddir)/fx2/lib/libfx2.la

vierg_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version -lfx2

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install vierg.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/vierg.cfg $(DESTDIR)$(PLUGINDIR)

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/vierg.so
	-rm $(DESTDIR)$(PLUGINDIR)/vierg.cfg
	-rmdir $(DESTDIR)$(PLUGINDIR)
