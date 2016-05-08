include $(KERNEL_LOCATION)/Rules.make

.PHONY: modules
modules: $(patsubst %, _mod_%, $(SUBDIRS))

depend dep: $(KERNEL_LOCATION)/scripts/mkdep fastdep

clean:
	find . \( -name '*.[oas]' -o -name '.*.flags' \) -type f -print | xargs rm -f

mrproper: clean
	find . \( -size 0 -o -name .depend \) -type f -print | xargs rm -f

distclean: mrproper
	rm -f `find . \( -not -type d \) -and \
		\( -name '*.orig' -o -name '*.rej' -o -name '*~' \
		-o -name '*.bak' -o -name '#*#' -o -name '.*.orig' \
		-o -name '.*.rej' -o -name '.SUMS' -o -size 0 \) -type f -print`

.PHONY: install
install: modules_install_misc

.PHONY: $(patsubst %, _mod_%, $(SUBDIRS))
$(patsubst %, _mod_%, $(SUBDIRS)):
	$(MAKE) -C $(patsubst _mod_%, %, $@) modules

.PHONY: modules_install_misc
modules_install_misc: _modinst_misc_ $(patsubst %,_modinst_misc_%,$(MOD_DIRS))

.PHONY: _modinst_misc_
_modinst_misc_: dummy
ifneq "$(strip $(ALL_MOBJS))" ""
	mkdir -p $(MODLIB)/misc
	for f in $(ALL_MOBJS) ; do \
		$(OBJCOPY) -R __ksymtab -R .comment -R .note -x \
		`$(NM) $$f | cut -f3- -d' ' | sed -n \
			-e 's/__module_parm_\(.*\)/-K \1/p' \
			-e 's/__ks..tab_\(.*\)/-K \1/p'` \
		$$f $(MODLIB)/misc/`basename $$f`; \
	done
endif

ifneq "$(strip $(MOD_DIRS))" ""
.PHONY: $(patsubst %,_modinst_misc_%,$(MOD_DIRS))
$(patsubst %,_modinst_misc_%,$(MOD_DIRS)) : dummy
	$(MAKE) -C $(patsubst _modinst_misc_%,%,$@) modules_install_misc
endif

