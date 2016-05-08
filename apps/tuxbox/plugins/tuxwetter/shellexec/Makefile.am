install-exec-local:
	install -m 0755 $(srcdir)/twops $(DESTDIR)$(bindir)
	install -d $(DESTDIR)$(CONFIGDIR)
	install -d $(DESTDIR)$(CONFIGDIR)/tuxwetter
	install -m 0644 $(srcdir)/tuxwetter.mcfg $(DESTDIR)$(CONFIGDIR)/tuxwetter
	install -m 0644 $(srcdir)/plconfig_tuxwetter.mnu $(DESTDIR)$(CONFIGDIR)/flexinc
	echo "INCLUDE=/var/tuxbox/config/flexinc/plconfig_tuxwetter.mnu" >> $(DESTDIR)$(CONFIGDIR)/shellexec.conf
	echo "COMMENT=!" >> $(DESTDIR)$(CONFIGDIR)/shellexec.conf
