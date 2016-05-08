AM_CPPFLAGS =  \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

logoset_LDADD = \
	@FREETYPE_LIBS@

bin_PROGRAMS = 	logomask logoset

logomask_SOURCES = \
	logomask.c \
	gfx.c

logoset_SOURCES = \
	logoset.c \
	io.c \
	text.c\
	gfx.c

noinst_LTLIBRARIES = logomask.la logoset.la

logomask_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version
logoset_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

logomask_la_SOURCES = \
	start_logomask.c

logoset_la_SOURCES = \
	start_logoset.c

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	$(LIBTOOL) --mode=install install logomask.la $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install logoset.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/logomask.cfg $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/logoset.cfg $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/logomask.conf $(DESTDIR)$(CONFIGDIR)
