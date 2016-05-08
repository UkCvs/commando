fx2_lib: $(appsdir)/tuxbox/plugins/config.status
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/lib all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/lib install

flash-fx2_lib: $(appsdir)/tuxbox/plugins/config.status fx2_lib | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/lib all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/lib install prefix=$(flashprefix)/root

if BOXTYPE_DBOX2

fx2pluginsdbox2 = outdoor sudoku

fx2-c64emu: $(appsdir)/tuxbox/plugins/config.status fx2_lib @DEPENDS_tuxfrodo@
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/$(patsubst fx2-%,%,$@) all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/$(patsubst fx2-%,%,$@) install
	@PREPARE_tuxfrodo@
	tar -C $(targetprefix)/lib/tuxbox/plugins/c64emu/ -xjvf @DIR_tuxfrodo@/hdd/c64emu/roms.tar.bz2
	@CLEANUP_tuxfrodo@

flash-c64emu-fx2: $(appsdir)/tuxbox/plugins/config.status flash-fx2_lib | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/$(patsubst flash-%-fx2,%,$@) all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/$(patsubst flash-%-fx2,%,$@) install prefix=$(flashprefix)/root
	@PREPARE_tuxfrodo@
	tar -C $(flashprefix)/root/lib/tuxbox/plugins/c64emu/ -xjvf @DIR_tuxfrodo@/hdd/c64emu/roms.tar.bz2
	@CLEANUP_tuxfrodo@
	@FLASHROOTDIR_MODIFIED@

else

fx2pluginsdbox2 =

endif

fx2pluginsgeneric = $(fx2pluginsdbox2) bouquet lcdcirc lemm master mines pac satfind snake sokoban sol solitair tank tetris vierg yahtzee

$(patsubst %,fx2-%,$(fx2pluginsgeneric)): $(appsdir)/tuxbox/plugins/config.status fx2_lib
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/$(patsubst fx2-%,%,$@) all
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/$(patsubst fx2-%,%,$@) install

$(patsubst %,flash-%-fx2,$(fx2pluginsgeneric)): $(appsdir)/tuxbox/plugins/config.status flash-fx2_lib | $(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/$(patsubst flash-%-fx2,%,$@) all prefix=$(flashprefix)/root
	$(MAKE) -C $(appsdir)/tuxbox/plugins/fx2/$(patsubst flash-%-fx2,%,$@) install prefix=$(flashprefix)/root
	@FLASHROOTDIR_MODIFIED@
