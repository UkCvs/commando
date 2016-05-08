#### Targets for building $partition-$gui.$fstype

####### var-$gui.jffs2
$(flashprefix)/var-radiobox.jffs2 \
$(flashprefix)/var-neutrino.jffs2 \
$(flashprefix)/var-enigma+neutrino.jffs2 \
$(flashprefix)/var-enigma.jffs2: \
$(flashprefix)/var-%.jffs2: $(flashprefix)/var-% $(hostprefix)/bin/mkfs.jffs2
	$(hostprefix)/bin/mkfs.jffs2 -x lzma -b -U -e 131072 -p -r $< -o $@


####### root-$gui.$fstype
$(flashprefix)/root-radiobox.cramfs \
$(flashprefix)/root-neutrino.cramfs \
$(flashprefix)/root-enigma.cramfs: \
$(flashprefix)/root-%.cramfs: $(flashprefix)/root-%-cramfs $(MKCRAMFS)
	$(MKCRAMFS) -p -n "0106`date +%Y%m%d%H%M`" $< $@
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/root-radiobox.squashfs \
$(flashprefix)/root-neutrino.squashfs \
$(flashprefix)/root-enigma+neutrino.squashfs \
$(flashprefix)/root-enigma.squashfs: \
$(flashprefix)/root-%.squashfs: \
$(flashprefix)/root-%-squashfs \
$(hostprefix)/bin/mksquashfs-lzma \
$(hostprefix)/bin/checkImage
	rm -f $@
	$(hostprefix)/bin/mksquashfs-lzma $< $@ -be -all-root
if BOXTYPE_DBOX2
	trycount=1; \
	while [ $$trycount -le "20" ]; do \
		$(hostprefix)/bin/checkImage $@; \
		if [ $$? = "0" -o $$? = "3" ]; then break; fi; \
		echo "$$trycount/20 try to repair image"; \
		dd if=/dev/urandom of=$</bad_magic_payload count=$$trycount bs=`expr 256 \* $$trycount`; \
		rm -f $@; \
		$(hostprefix)/bin/mksquashfs-lzma $< $@ -be -all-root; \
		trycount=`expr $$trycount + 1`; \
	done
endif
	chmod 644 $@
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/root-radiobox.squashfs_nolzma \
$(flashprefix)/root-neutrino.squashfs_nolzma \
$(flashprefix)/root-enigma+neutrino.squashfs_nolzma \
$(flashprefix)/root-enigma.squashfs_nolzma: \
$(flashprefix)/root-%.squashfs_nolzma: \
$(flashprefix)/root-%-squashfs_nolzma \
$(hostprefix)/bin/mksquashfs-nolzma \
$(hostprefix)/bin/checkImage
	rm -f $@
	$(hostprefix)/bin/mksquashfs-nolzma $< $@ -be -all-root
if BOXTYPE_DBOX2
	trycount=1; \
	while [ $$trycount -le "20" ]; do \
		$(hostprefix)/bin/checkImage $@; \
		if [ $$? = "0" -o $$? = "3" ]; then break; fi; \
		echo "$$trycount/20 try to repair image"; \
		dd if=/dev/urandom of=$</bad_magic_payload count=$$trycount bs=`expr 256 \* $$trycount`; \
		rm -f $@; \
		$(hostprefix)/bin/mksquashfs-nolzma $< $@ -be -all-root; \
		trycount=`expr $$trycount + 1`; \
	done
endif
	chmod 644 $@
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/root-radiobox.jffs2 \
$(flashprefix)/root-neutrino.jffs2 \
$(flashprefix)/root-enigma.jffs2 \
$(flashprefix)/root-lcars.jffs2 \
$(flashprefix)/root-null.jffs2: \
$(flashprefix)/root-%.jffs2: \
$(flashprefix)/root-%-jffs2 \
$(hostprefix)/bin/mkfs.jffs2 \
$(hostprefix)/bin/checkImage
	$(hostprefix)/bin/mkfs.jffs2 -x lzma -b -U -e 0x20000 --pad=0x7c0000 -r $< -o $@
if BOXTYPE_DBOX2
	trycount=1; \
	while [ $$trycount -le "20" ]; do \
		$(hostprefix)/bin/checkImage $@; \
		if [ $$? = "0" -o $$? = "3" ]; then break; fi; \
		echo "$$trycount/20 try to repair image"; \
		dd if=/dev/urandom of=$</bad_magic_payload count=$$trycount bs=`expr 256 \* $$trycount`; \
		rm -f $@; \
		$(hostprefix)/bin/mkfs.jffs2 -x lzma -b -U -e 0x20000 --pad=0x7c0000 -r $< -o $@; \
		trycount=`expr $$trycount + 1`; \
	done
endif

$(flashprefix)/root-radiobox.jffs2_lzma \
$(flashprefix)/root-neutrino.jffs2_lzma \
$(flashprefix)/root-enigma+neutrino.jffs2_lzma \
$(flashprefix)/root-enigma.jffs2_lzma \
$(flashprefix)/root-lcars.jffs2_lzma \
$(flashprefix)/root-null.jffs2_lzma: \
$(flashprefix)/root-%.jffs2_lzma: \
$(flashprefix)/root-%-jffs2_lzma \
$(hostprefix)/bin/mkfs.jffs2 \
$(hostprefix)/bin/checkImage
	$(hostprefix)/bin/mkfs.jffs2 -b -U -e 0x20000 --pad=0x7c0000 -r $< -o $@
if BOXTYPE_DBOX2
	trycount=1; \
	while [ $$trycount -le "20" ]; do \
		$(hostprefix)/bin/checkImage $@; \
		if [ $$? = "0" -o $$? = "3" ]; then break; fi; \
		echo "$$trycount/20 try to repair image"; \
		dd if=/dev/urandom of=$</bad_magic_payload count=$$trycount bs=`expr 256 \* $$trycount`; \
		rm -f $@; \
		$(hostprefix)/bin/mkfs.jffs2 -b -U -e 0x20000 --pad=0x7c0000 -r $< -o $@; \
		trycount=`expr $$trycount + 1`; \
	done
endif

$(flashprefix)/root-radiobox.jffs2_lzma_klzma \
$(flashprefix)/root-neutrino.jffs2_lzma_klzma \
$(flashprefix)/root-enigma+neutrino.jffs2_lzma_klzma \
$(flashprefix)/root-enigma.jffs2_lzma_klzma \
$(flashprefix)/root-lcars.jffs2_lzma_klzma \
$(flashprefix)/root-null.jffs2_lzma_klzma: \
$(flashprefix)/root-%.jffs2_lzma_klzma: \
$(flashprefix)/root-%-jffs2_lzma_klzma \
$(hostprefix)/bin/mkfs.jffs2 \
$(hostprefix)/bin/checkImage
	$(hostprefix)/bin/mkfs.jffs2 -b -U -e 0x20000 --pad=0x7c0000 -r $< -o $@
if BOXTYPE_DBOX2
	trycount=1; \
	while [ $$trycount -le "20" ]; do \
		$(hostprefix)/bin/checkImage $@; \
		if [ $$? = "0" -o $$? = "3" ]; then break; fi; \
		echo "$$trycount/20 try to repair image"; \
		dd if=/dev/urandom of=$</bad_magic_payload count=$$trycount bs=`expr 256 \* $$trycount`; \
		rm -f $@; \
		$(hostprefix)/bin/mkfs.jffs2 -b -U -e 0x20000 --pad=0x7c0000 -r $< -o $@; \
		trycount=`expr $$trycount + 1`; \
	done
endif

################ $fs-to-boot.flfs*x
$(flashprefix)/cramfs.flfs1x $(flashprefix)/cramfs.flfs2x: \
$(hostprefix)/bin/mkflfs config/u-boot.dbox2.h.m4 \
| $(flashprefix)
	m4 --define=uboottype=cramfs --define=rootsize=$(ROOT_PARTITION_SIZE) \
		config/u-boot.dbox2.h.m4 > $(bootdir)/u-boot-config/u-boot.config
	$(MAKE) @DIR_uboot@/u-boot.stripped
	$(hostprefix)/bin/mkflfs 1x -o $(flashprefix)/cramfs.flfs1x @DIR_uboot@/u-boot.stripped
	$(hostprefix)/bin/mkflfs 2x -o $(flashprefix)/cramfs.flfs2x @DIR_uboot@/u-boot.stripped
	@CLEANUP_uboot@
	rm $(bootdir)/u-boot-config/u-boot.config

$(flashprefix)/squashfs.flfs1x $(flashprefix)/squashfs.flfs2x: \
$(hostprefix)/bin/mkflfs config/u-boot.dbox2.h.m4  \
| $(flashprefix)
	m4 --define=uboottype=squashfs --define=rootsize=$(ROOT_PARTITION_SIZE) --define=lzma \
		config/u-boot.dbox2.h.m4 > $(bootdir)/u-boot-config/u-boot.config
	$(MAKE) @DIR_uboot@/u-boot.stripped
	$(hostprefix)/bin/mkflfs 1x -o $(flashprefix)/squashfs.flfs1x @DIR_uboot@/u-boot.stripped
	$(hostprefix)/bin/mkflfs 2x -o $(flashprefix)/squashfs.flfs2x @DIR_uboot@/u-boot.stripped
	@CLEANUP_uboot@
	rm $(bootdir)/u-boot-config/u-boot.config

$(flashprefix)/squashfs_nolzma.flfs1x $(flashprefix)/squashfs_nolzma.flfs2x: \
$(hostprefix)/bin/mkflfs config/u-boot.dbox2.h.m4  \
| $(flashprefix)
	m4 --define=uboottype=squashfs --define=rootsize=$(ROOT_PARTITION_SIZE) \
		config/u-boot.dbox2.h.m4 > $(bootdir)/u-boot-config/u-boot.config
	$(MAKE) @DIR_uboot@/u-boot.stripped
	$(hostprefix)/bin/mkflfs 1x -o $(flashprefix)/squashfs_nolzma.flfs1x @DIR_uboot@/u-boot.stripped
	$(hostprefix)/bin/mkflfs 2x -o $(flashprefix)/squashfs_nolzma.flfs2x @DIR_uboot@/u-boot.stripped
	@CLEANUP_uboot@
	rm $(bootdir)/u-boot-config/u-boot.config

$(flashprefix)/jffs2.flfs1x $(flashprefix)/jffs2.flfs2x: \
$(hostprefix)/bin/mkflfs config/u-boot.dbox2.h.m4 \
| $(flashprefix)
	m4 --define=uboottype=jffs2 \
		config/u-boot.dbox2.h.m4 > $(bootdir)/u-boot-config/u-boot.config
	$(MAKE) @DIR_uboot@/u-boot.stripped
	$(hostprefix)/bin/mkflfs 1x -o $(flashprefix)/jffs2.flfs1x @DIR_uboot@/u-boot.stripped
	$(hostprefix)/bin/mkflfs 2x -o $(flashprefix)/jffs2.flfs2x @DIR_uboot@/u-boot.stripped
	@CLEANUP_uboot@
	rm $(bootdir)/u-boot-config/u-boot.config
