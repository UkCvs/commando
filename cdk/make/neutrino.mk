# tuxbox/neutrino

if ENABLE_MOVIEPLAYER2
CONFIGURE_OPTS_MP2 = \
	--enable-movieplayer2
endif

$(appsdir)/tuxbox/neutrino/config.status: bootstrap $(appsdir)/dvb/zapit/src/zapit libboost libcurl libfreetype @ESOUND@ @NEUTRINO_AUDIOPLAYER_DEPS@ @NEUTRINO_PICTUREVIEWER_DEPS@ $(targetprefix)/lib/pkgconfig/tuxbox-tuxtxt.pc $(targetprefix)/include/tuxbox/plugin.h
	cd $(appsdir)/tuxbox/neutrino && $(CONFIGURE_BIN) $(CONFIGURE_OPTS_MP2)

neutrino: $(appsdir)/tuxbox/neutrino/config.status
	$(MAKE) -C $(appsdir)/tuxbox/neutrino all
	$(MAKE) -C $(appsdir)/tuxbox/neutrino install
	$(MAKE) neutrino-additional-fonts
	$(MAKE) var-httpd-styles

flash-neutrino: $(flashprefix)/root-neutrino

$(flashprefix)/root-neutrino: $(appsdir)/tuxbox/neutrino/config.status
	$(MAKE) -C $(appsdir)/tuxbox/neutrino all prefix=$@
	$(MAKE) -C $(appsdir)/tuxbox/neutrino install prefix=$@
	$(MAKE) -C $(appsdir)/dvb/zapit install prefix=$@
	$(MAKE) neutrino-additional-fonts targetprefix=$@
	$(MAKE) var-httpd-styles targetprefix=$@
if ENABLE_ESD
	$(INSTALL) $(targetprefix)/bin/esd $@/bin
endif
if ENABLE_FDISK_STANDALONE
	$(MAKE) flash-fdisk
endif
if ENABLE_MOUNT_STANDALONE
	$(MAKE) flash-mount
endif
if ENABLE_DVBSUB
	$(MAKE) flash-dvbsub
endif
	touch $@
	@TUXBOX_CUSTOMIZE@

# This is really an ugly kludge. Neutrino and the plugins should
# install the fonts they need in their own Makefiles.
neutrino-additional-fonts:
	cp $(appsdir)/tuxbox/enigma/data/fonts/bluebold.ttf $(targetprefix)/share/fonts
	cp $(appsdir)/tuxbox/enigma/data/fonts/bluehigh.ttf $(targetprefix)/share/fonts
	cp $(appsdir)/tuxbox/enigma/data/fonts/pakenham.ttf $(targetprefix)/share/fonts
	cp $(appsdir)/tuxbox/enigma/data/fonts/unmrs.pfa    $(targetprefix)/share/fonts

var-httpd-styles:
	install -d $(targetprefix)/var/httpd
	install -d $(targetprefix)/var/httpd/styles
	cp $(appsdir)/tuxbox/neutrino/daemons/nhttpd/web/styles/Y_Dist.css $(targetprefix)/var/httpd
	ln -sf /var/httpd/Y_Dist.css $(targetprefix)/share/tuxbox/neutrino/httpd-y/Y_Dist.css
