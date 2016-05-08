# tuxbox/plugins

$(appsdir)/tuxbox/plugins/config.status: bootstrap libfreetype libcurl libz libsigc libpng libjpeg $(targetprefix)/lib/pkgconfig/tuxbox.pc $(targetprefix)/lib/pkgconfig/tuxbox-xmltree.pc $(targetprefix)/lib/pkgconfig/tuxbox-tuxtxt.pc
	cd $(appsdir)/tuxbox/plugins && $(CONFIGURE)

plugins: neutrino-plugins enigma-plugins

neutrino-plugins: $(targetprefix)/include/tuxbox/plugin.h tuxmail tuxtxt tuxcom tuxcal vncviewer dvbsub shellexec tuxwetter sysinfo clock logomask blockads

enigma-plugins: @LIBGETTEXT@ $(appsdir)/tuxbox/plugins/config.status $(targetprefix)/include/tuxbox/plugin.h
	$(MAKE) -C $(appsdir)/tuxbox/plugins/enigma all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/enigma install

$(targetprefix)/include/tuxbox/plugin.h \
$(targetprefix)/lib/pkgconfig/tuxbox-plugins.pc: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/include all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/include install
	cp $(appsdir)/tuxbox/plugins/tuxbox-plugins.pc $(targetprefix)/lib/pkgconfig/tuxbox-plugins.pc

tuxmail: libcrypto $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxmail all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxmail install

flash-tuxmail: libcrypto $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxmail all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxmail install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

pluginx: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/pluginx all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/pluginx install

flash-pluginx: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/pluginx all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/pluginx install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

tuxtxt: $(appsdir)/tuxbox/plugins/config.status $(targetprefix)/include/tuxbox/plugin.h
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxtxt all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxtxt install

flash-tuxtxt: $(appsdir)/tuxbox/plugins/config.status tuxtxt | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxtxt install  prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

tuxcom: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxcom all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxcom install

flash-tuxcom: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxcom all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxcom install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

tuxcal: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxcal all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxcal install

flash-tuxcal: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxcal all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxcal install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

tuxclock: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxclock all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxclock install

flash-tuxclock: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxclock all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxclock install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

vncviewer: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/vncviewer all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/vncviewer install

flash-vncviewer: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/vncviewer all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/vncviewer install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

pip: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/pip all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/pip install

flash-pip: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/pip all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/pip install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

mosaic: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/mosaic all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/mosaic install

flash-mosaic: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/mosaic all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/mosaic install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

shellexec: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/shellexec all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/shellexec install
if ENABLE_BLOCKADS
	$(MAKE) -C $(appsdir)/tuxbox/plugins/blockads/shellexec all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/blockads/shellexec install
endif
if ENABLE_CLOCK
	$(MAKE) -C $(appsdir)/tuxbox/plugins/clock/shellexec all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/clock/shellexec install
endif
if ENABLE_LOGOMASK
	$(MAKE) -C $(appsdir)/tuxbox/plugins/logomask/shellexec all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/logomask/shellexec install
endif
if ENABLE_TUXWETTER
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxwetter/shellexec all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxwetter/shellexec install
endif

flash-shellexec: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/shellexec all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/shellexec install prefix=$(flashprefix)/root
if ENABLE_BLOCKADS
	$(MAKE) -C $(appsdir)/tuxbox/plugins/blockads/shellexec all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/blockads/shellexec install prefix=$(flashprefix)/root
endif
if ENABLE_CLOCK
	$(MAKE) -C $(appsdir)/tuxbox/plugins/clock/shellexec all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/clock/shellexec install prefix=$(flashprefix)/root
endif
if ENABLE_LOGOMASK
	$(MAKE) -C $(appsdir)/tuxbox/plugins/logomask/shellexec all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/logomask/shellexec install prefix=$(flashprefix)/root
endif
if ENABLE_TUXWETTER
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxwetter/shellexec all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxwetter/shellexec install prefix=$(flashprefix)/root
endif
	@FLASHROOTDIR_MODIFIED@

tuxwetter: giflib input $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxwetter all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxwetter install

flash-tuxwetter: giflib flash-input $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxwetter all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/tuxwetter install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

sysinfo: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/sysinfo all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/sysinfo install

flash-sysinfo: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/sysinfo all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/sysinfo install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

blockads: input $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/blockads all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/blockads install

flash-blockads: flash-input $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/blockads all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/blockads install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

clock: input $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/clock all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/clock install

flash-clock: flash-input $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/clock all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/clock install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

logomask: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/logomask all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/logomask install

flash-logomask: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/logomask all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/logomask install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

dvbsub: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/dvbsub all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/dvbsub install

flash-dvbsub: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/dvbsub all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/dvbsub install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@

$(DEPDIR)/links-plugin: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/links all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/links install
	touch $@

flash-links-plugin: $(appsdir)/tuxbox/plugins/config.status | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/links all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/links install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@
