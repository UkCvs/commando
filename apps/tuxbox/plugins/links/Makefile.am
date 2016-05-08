AM_CPPFLAGS =  \
	-I$(top_srcdir)/include

noinst_LTLIBRARIES = links.la

links_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

links_la_SOURCES = \
	start.c

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install links.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/links.cfg $(DESTDIR)$(PLUGINDIR)
	install -m 0755 $(srcdir)/links.sh $(DESTDIR)$(bindir)
