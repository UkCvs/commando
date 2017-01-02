AM_CPPFLAGS =  \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

sbin_SCRIPTS = cnitscan

noinst_LTLIBRARIES = nitscan.la

nitscan_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

nitscan_la_SOURCES = start_nitscan.c

install-exec-local:
	install -d $(DESTDIR)$(bindir)
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install nitscan.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/nitscan.cfg $(DESTDIR)$(PLUGINDIR)
