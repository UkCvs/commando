AM_CPPFLAGS = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = yahtzee.la

yahtzee_la_SOURCES = yahtzee.c somain.c

yahtzee_la_LIBFLAGS = $(top_builddir)/fx2/lib/libfx2.la

yahtzee_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version -lfx2

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install yahtzee.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/yahtzee.cfg $(DESTDIR)$(PLUGINDIR)

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/yahtzee.so
	-rm $(DESTDIR)$(PLUGINDIR)/yahtzee.cfg
	-rmdir $(DESTDIR)$(PLUGINDIR)
