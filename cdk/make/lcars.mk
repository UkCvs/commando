# tuxbox/lcars

$(appsdir)/tuxbox/lcars/config.status: bootstrap libcurl libfreetype $(targetprefix)/include/tuxbox/plugin.h
	cd $(appsdir)/tuxbox/lcars && $(CONFIGURE)

lcars: $(appsdir)/tuxbox/lcars/config.status
	$(MAKE) -C $(appsdir)/tuxbox/lcars all
	$(MAKE) -C $(appsdir)/tuxbox/lcars install

# Untested! 
flash-lcars: $(flashprefix)/root-lcars

$(flashprefix)/root-lcars: $(appsdir)/tuxbox/lcars/config.status
	$(MAKE) -C $(appsdir)/tuxbox/lcars all prefix=$@
	$(MAKE) -C $(appsdir)/tuxbox/lcars install prefix=$@
	touch $@
	@TUXBOX_CUSTOMIZE@
