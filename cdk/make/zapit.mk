# Zapit

$(appsdir)/dvb/zapit/src/zapit:
	$(MAKE) zapit

$(appsdir)/dvb/zapit/config.status: bootstrap $(targetprefix)/lib/pkgconfig/tuxbox-xmltree.pc $(targetprefix)/lib/pkgconfig/tuxbox-tuxtxt.pc
	cd $(appsdir)/dvb/zapit && $(CONFIGURE_BIN)

zapit: $(appsdir)/dvb/zapit/config.status
	$(MAKE) -C $(appsdir)/dvb/zapit all
	$(MAKE) -C $(appsdir)/dvb/zapit install

.PHONY: zapit
