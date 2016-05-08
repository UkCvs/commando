defaultlocale: $(targetprefix)/var/tuxbox/config/defaultlocale

$(targetprefix)/var/tuxbox/config/defaultlocale:
	$(INSTALL) -d $(dir $@)
	echo $(DEFAULTLOCALE) > $@

flash-defaultlocale: $(flashprefix)/root/var/tuxbox/config/defaultlocale

$(flashprefix)/root/var/tuxbox/config/defaultlocale: | $(flashprefix)/root
	$(INSTALL) -d $(dir $@)
	echo $(DEFAULTLOCALE) > $@
	@FLASHROOTDIR_MODIFIED@
