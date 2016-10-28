AM_CPPFLAGS =  \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

sbin_SCRIPTS = getepg-now

noinst_LTLIBRARIES = getepg.la

getepg_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

getepg_la_SOURCES = start_getepg.c

install-exec-local:
	install -d $(DESTDIR)$(bindir)
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install getepg.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/getepg.cfg $(DESTDIR)$(PLUGINDIR)
	install -m 0755 $(srcdir)/weekepglog $(DESTDIR)$(bindir)

