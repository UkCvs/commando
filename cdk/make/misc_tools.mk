# misc/tools

$(appsdir)/misc/tools/config.status: bootstrap libpng
	cd $(appsdir)/misc/tools && $(CONFIGURE)

misc_tools: $(appsdir)/misc/tools/config.status
	$(MAKE) -C $(appsdir)/misc/tools all
	$(MAKE) -C $(appsdir)/misc/tools install


flash-misc_tools: $(appsdir)/misc/tools/config.status
	$(MAKE) -C $(appsdir)/misc/tools all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

flash-eraseall: $(flashprefix)/root/sbin/eraseall

$(flashprefix)/root/sbin/eraseall: misc_tools | $(flashprefix)/root
	$(INSTALL) $(appsdir)/misc/tools/mtd/eraseall $@
	@FLASHROOTDIR_MODIFIED@

flash-fcp: $(flashprefix)/root/sbin/fcp

$(flashprefix)/root/sbin/fcp: misc_tools | $(flashprefix)/root
	$(INSTALL) $(appsdir)/misc/tools/mtd/fcp $@
	@FLASHROOTDIR_MODIFIED@

flash-dboxshot: $(flashprefix)/root/bin/dboxshot

$(flashprefix)/root/bin/dboxshot: $(appsdir)/misc/tools/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/dboxshot all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/dboxshot install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

flash-fbshot: $(flashprefix)/root/bin/fbshot

$(flashprefix)/root/bin/fbshot: $(appsdir)/misc/tools/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/fbshot all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/fbshot install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

flash-ether-wake: $(flashprefix)/root/bin/ether-wake

$(flashprefix)/root/bin/ether-wake: $(appsdir)/misc/tools/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/etherwake all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/etherwake install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

flash-rtc: $(flashprefix)/root/bin/hwrtc

$(flashprefix)/root/bin/hwrtc:  $(appsdir)/misc/tools/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/rtc all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/rtc install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

flash-makedevices: $(flashprefix)/root/bin/makedevices

$(flashprefix)/root/bin/makedevices:  $(appsdir)/misc/tools/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/makedevices all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/misc/tools/makedevices install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@
