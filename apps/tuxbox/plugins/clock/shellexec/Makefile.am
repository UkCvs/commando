install-exec-local:
	install -m 0755 $(srcdir)/cops $(DESTDIR)$(bindir)
	install -m 0755 $(srcdir)/ssops $(DESTDIR)$(bindir)
	install -m 0644 $(srcdir)/plconfig_clock.mnu $(DESTDIR)$(CONFIGDIR)/flexinc
	install -m 0644 $(srcdir)/plrun_clock.mnu $(DESTDIR)$(CONFIGDIR)/flexinc
	install -m 0644 $(srcdir)/plconfig_ssaver.mnu $(DESTDIR)$(CONFIGDIR)/flexinc
	install -m 0644 $(srcdir)/plrun_ssaver.mnu $(DESTDIR)$(CONFIGDIR)/flexinc
	echo "INCLUDE=/var/tuxbox/config/flexinc/plconfig_clock.mnu" >> $(DESTDIR)$(CONFIGDIR)/shellexec.conf
	echo "INCLUDE=/var/tuxbox/config/flexinc/plconfig_ssaver.mnu" >> $(DESTDIR)$(CONFIGDIR)/shellexec.conf
	echo "COMMENT=!" >> $(DESTDIR)$(CONFIGDIR)/shellexec.conf
