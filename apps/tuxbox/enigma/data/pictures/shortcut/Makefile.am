EXTRA_DIST = $(srcdir)/red.png $(srcdir)/green.png $(srcdir)/yellow.png $(srcdir)/blue.png \
	$(srcdir)/button_0.png $(srcdir)/button_1.png $(srcdir)/button_2.png $(srcdir)/button_3.png $(srcdir)/button_4.png \
	$(srcdir)/button_5.png $(srcdir)/button_6.png $(srcdir)/button_7.png $(srcdir)/button_8.png $(srcdir)/button_9.png
	
install-data-local:
	install -d $(DESTDIR)$(TUXBOXDATADIR)/enigma/pictures/shortcut
	install -m 0644 $(EXTRA_DIST) $(DESTDIR)$(TUXBOXDATADIR)/enigma/pictures/shortcut
