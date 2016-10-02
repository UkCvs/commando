AM_CPPFLAGS =  \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

sbin_SCRIPTS = autoscan

bin_PROGRAMS = autobouquets

autobouquets_SOURCES = autobouquets.cpp

noinst_LTLIBRARIES = autobouquets.la

autobouquets_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

autobouquets_la_SOURCES = start_autobouquets.c

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	$(LIBTOOL) --mode=install install autobouquets.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/autobouquets.cfg $(DESTDIR)$(PLUGINDIR)

