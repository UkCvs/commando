# tuxbox/radiobox

$(appsdir)/tuxbox/radiobox/config.status: bootstrap libboost libcurl libfreetype libmad libid3tag libvorbisidec libtuxbox misc_libs tuxbox_libs misc_tools @LUFS@
	cd $(appsdir)/tuxbox/radiobox && $(CONFIGURE)

radiobox: $(appsdir)/tuxbox/radiobox/config.status
	$(MAKE) -C $(appsdir)/tuxbox/radiobox all
	$(MAKE) -C $(appsdir)/tuxbox/radiobox install

flash-radiobox: $(flashprefix)/root-radiobox

$(flashprefix)/root-radiobox: $(appsdir)/tuxbox/radiobox/config.status
	$(MAKE) -C $(appsdir)/tuxbox/radiobox all prefix=$@
	$(MAKE) -C $(appsdir)/tuxbox/radiobox install prefix=$@
	touch $@
	@TUXBOX_CUSTOMIZE@
