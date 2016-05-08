# dvb/config

# cables.xml, satellites.xml

$(appsdir)/dvb/config/config.status:
	cd $(appsdir)/dvb/config && $(CONFIGURE)

config: $(appsdir)/dvb/config/config.status
	$(MAKE) -C $(appsdir)/dvb/config all
	$(MAKE) -C $(appsdir)/dvb/config install

flash-config: $(appsdir)/dvb/config/config.status $(flashprefix)/root
	$(MAKE) -C $(appsdir)/dvb/config all datadir=$(flashprefix)/root/share
	$(MAKE) -C $(appsdir)/dvb/config install datadir=$(flashprefix)/root/share
	@FLASHROOTDIR_MODIFIED@
