AM_CPPFLAGS = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = sudoku.la

sudoku_la_SOURCES = misc.cpp menu.cpp sudoku.cpp board.cpp somain.cpp

sudoku_la_LIBFLAGS = $(top_builddir)/fx2/lib/libfx2.la

sudoku_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version -lfx2

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	install -d $(DESTDIR)$(CONFIGDIR)/sudoku
	$(LIBTOOL) --mode=install install sudoku.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/sudoku.cfg $(DESTDIR)$(PLUGINDIR)
	$(STRIP) -d $(DESTDIR)$(PLUGINDIR)/sudoku.so
