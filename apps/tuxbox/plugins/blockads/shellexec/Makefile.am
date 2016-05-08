install-exec-local:
	install -m 0755 $(srcdir)/wzops $(DESTDIR)$(bindir)
	install -m 0644 $(srcdir)/plconfig_blockads.mnu $(DESTDIR)$(CONFIGDIR)/flexinc
	echo "INCLUDE=/var/tuxbox/config/flexinc/plconfig_blockads.mnu" >> $(DESTDIR)$(CONFIGDIR)/shellexec.conf
	echo "COMMENT=!" >> $(DESTDIR)$(CONFIGDIR)/shellexec.conf
