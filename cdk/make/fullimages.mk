# Name for images/filesystems:
#
# $partition-$gui-$rootfilesystem.$type

# where 
# $type in { cramfs, squashfs, jffs2, img1x, img2x, flfs1x, flfs2x }
# $partition in { root, var } (empty by full images)
# $gui in { neutrino, enigma } 

# Public targets that build one or more images (*.img*x)

################################################################
# Targets for building user images (*.img*x)
#
# They all depend on two or three $partition-$gui.$fstype, defined in the
# partition-images.mk.

# Note the difference between $partition-$gui-$fstype (directory) and 
# $partition-$gui.$fstype (filesystem image of type $fstype).

TUXBOX_CHECKIMAGE = \
	$(hostprefix)/bin/checkImage $@ $(subst x,,$(subst .flfs,-,$(suffix $(word 1,$+)))) || mv $@ $@_bad

$(flashprefix)/neutrino-cramfs.img1x $(flashprefix)/neutrino-cramfs.img2x: \
$(flashprefix)/neutrino-cramfs.img%: \
		$(flashprefix)/cramfs.flfs% \
		$(flashprefix)/root-neutrino.cramfs \
		$(flashprefix)/var-neutrino.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/neutrino-squashfs.img1x $(flashprefix)/neutrino-squashfs.img2x:\
$(flashprefix)/neutrino-squashfs.img%: \
		$(flashprefix)/squashfs.flfs% \
		$(flashprefix)/root-neutrino.squashfs \
		$(flashprefix)/var-neutrino.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/neutrino-squashfs_nolzma.img1x $(flashprefix)/neutrino-squashfs_nolzma.img2x:\
$(flashprefix)/neutrino-squashfs_nolzma.img%: \
		$(flashprefix)/squashfs_nolzma.flfs% \
		$(flashprefix)/root-neutrino.squashfs_nolzma \
		$(flashprefix)/var-neutrino.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/neutrino-jffs2.img1x $(flashprefix)/neutrino-jffs2.img2x: \
$(flashprefix)/neutrino-jffs2.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-neutrino.jffs2 \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	@FILESIZE=$$(stat -c%s $@); \
	if [ $$FILESIZE -gt 8257536 ]; \
		then echo "fatal error: File $@ too large ($$FILESIZE > 8257536)"; \
		rm $@; exit 1; \
	fi
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/neutrino-jffs2_lzma.img1x $(flashprefix)/neutrino-jffs2_lzma.img2x: \
$(flashprefix)/neutrino-jffs2_lzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-neutrino.jffs2_lzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	@FILESIZE=$$(stat -c%s $@); \
	if [ $$FILESIZE -gt 8257536 ]; \
		then echo "fatal error: File $@ too large ($$FILESIZE > 8257536)"; \
		rm $@; exit 1; \
	fi
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/neutrino-jffs2_lzma_klzma.img1x $(flashprefix)/neutrino-jffs2_lzma_klzma.img2x: \
$(flashprefix)/neutrino-jffs2_lzma_klzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-neutrino.jffs2_lzma_klzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	@FILESIZE=$$(stat -c%s $@); \
	if [ $$FILESIZE -gt 8257536 ]; \
		then echo "fatal error: File $@ too large ($$FILESIZE > 8257536)"; \
		rm $@; exit 1; \
	fi
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

######################

$(flashprefix)/radiobox-cramfs.img1x $(flashprefix)/radiobox-cramfs.img2x: \
$(flashprefix)/radiobox-cramfs.img%: \
		$(flashprefix)/cramfs.flfs% \
		$(flashprefix)/root-radiobox.cramfs \
		$(flashprefix)/var-radiobox.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/radiobox-squashfs.img1x $(flashprefix)/radiobox-squashfs.img2x:\
$(flashprefix)/radiobox-squashfs.img%: \
		$(flashprefix)/squashfs.flfs% \
		$(flashprefix)/root-radiobox.squashfs \
		$(flashprefix)/var-radiobox.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/radiobox-jffs2.img1x $(flashprefix)/radiobox-jffs2.img2x: \
$(flashprefix)/radiobox-jffs2.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-radiobox.jffs2 \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/radiobox-jffs2_lzma.img1x $(flashprefix)/radiobox-jffs2_lzma.img2x: \
$(flashprefix)/radiobox-jffs2_lzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-radiobox.jffs2_lzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/radiobox-jffs2_lzma_klzma.img1x $(flashprefix)/radiobox-jffs2_lzma_klzma.img2x: \
$(flashprefix)/radiobox-jffs2_lzma_klzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-radiobox.jffs2_lzma_klzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

################################################################
$(flashprefix)/enigma-cramfs.img1x $(flashprefix)/enigma-cramfs.img2x: \
$(flashprefix)/enigma-cramfs.img%: \
		$(flashprefix)/cramfs.flfs% \
		$(flashprefix)/root-enigma.cramfs \
		$(flashprefix)/var-enigma.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/enigma-squashfs.img1x $(flashprefix)/enigma-squashfs.img2x: \
$(flashprefix)/enigma-squashfs.img%: \
		$(flashprefix)/squashfs.flfs% \
		$(flashprefix)/root-enigma.squashfs \
		$(flashprefix)/var-enigma.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/enigma-squashfs_nolzma.img1x $(flashprefix)/enigma-squashfs_nolzma.img2x: \
$(flashprefix)/enigma-squashfs_nolzma.img%: \
		$(flashprefix)/squashfs_nolzma.flfs% \
		$(flashprefix)/root-enigma.squashfs_nolzma \
		$(flashprefix)/var-enigma.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

if BOXTYPE_DREAMBOX
$(flashprefix)/enigma-squashfs.dream: \
		$(flashprefix)/root \
		$(flashprefix)/root-enigma-squashfs \
		$(flashprefix)/boot-cramfs.img $(flashprefix)/root-squashfs.img $(flashprefix)/complete.img
		@TUXBOX_CUSTOMIZE@

$(flashprefix)/boot-cramfs.img: $(KERNEL_BUILD_FILENAME)\
		$(flashprefix)/boot
	$(INSTALL) $(KERNEL_BUILD_FILENAME) $(flashprefix)/boot/root/platform/kernel/os
if BOXMODEL_DM7000
	$(hostprefix)/bin/mkcramfs-e -eb $(flashprefix)/boot $(flashprefix)/boot-cramfs.img
else
	mv $(flashprefix)/boot/root/platform/kernel/bild .
	$(hostprefix)/bin/mkcramfs-e -eb $(flashprefix)/boot $(flashprefix)/boot-cramfs.img
	mv ./bild $(flashprefix)/boot/root/platform/kernel
endif
	@if [ `stat -c %s $(flashprefix)/boot-cramfs.img` -gt 1048576 ]; then \
		echo "ERROR: CramFS part is too big for image (max. allowed 1048576 bytes)"; \
		rm -f $(flashprefix)/boot-cramfs.img.too-big 2> /dev/null || /bin/true; \
		mv $(flashprefix)/boot-cramfs.img $(flashprefix)/boot-cramfs.img.too-big; \
		exit 1; \
	fi

$(flashprefix)/root-squashfs.img: \
		$(flashprefix)/root \
		$(flashprefix)/root-enigma-squashfs
	-rm $@
	$(hostprefix)/bin/mksquashfs-dream $(flashprefix)/root-enigma-squashfs $@ -be -all-root
	@if [ `stat -c %s $@` -gt 3276800 ]; then \
		echo "ERROR: SquashFS part is too big for image (max. allowed 3276800 bytes)"; \
		rm -f $(flashprefix)/root-squashfs.img.too-big 2> /dev/null || /bin/true; \
		mv $@ $(flashprefix)/root-squashfs.img.too-big; \
		exit 1; \
	fi

$(flashprefix)/complete.img: $(flashprefix)/boot-cramfs.img $(flashprefix)/root-squashfs.img
	cp $(flashprefix)/boot-cramfs.img $(flashprefix)/complete.img; \
	dd if=$(flashprefix)/root-squashfs.img of=$(flashprefix)/complete.img bs=1024 seek=1024

$(flashprefix)/neutrino-squashfs.dream: \
		$(flashprefix)/root \
		$(flashprefix)/root-neutrino-squashfs \
		$(flashprefix)/boot-cramfs.img \
		$(flashprefix)/root-neutrino-squashfs.img \
		$(flashprefix)/var-neutrino-jffs2.img \
		$(flashprefix)/complete-neutrino.img
		@TUXBOX_CUSTOMIZE@

$(flashprefix)/root-neutrino-squashfs.img: \
		$(flashprefix)/root \
		$(flashprefix)/root-neutrino-squashfs
	-rm $@
	m4 -D flashprefix=$(flashprefix)/root-neutrino-squashfs \
		root-neutrino-squashfs-ignore-list.m4 > $(buildprefix)/root-neutrino-squashfs-ignore-list
	$(hostprefix)/bin/mksquashfs-dream \
	$(flashprefix)/root-neutrino-squashfs $@ -be -all-root -ef \
	$(buildprefix)/root-neutrino-squashfs-ignore-list
	@if [ `stat -c %s $@` -gt 3276800 ]; then \
		echo "ERROR: SquashFS part is too big for image (max. allowed 3276800 bytes)"; \
		rm -f $(flashprefix)/root-neutrino-squashfs.img.too-big 2> /dev/null || /bin/true; \
		mv $@ $(flashprefix)/root-neutrino-squashfs.img.too-big; \
		exit 1; \
	fi

$(flashprefix)/cramfs-squashfs-neutrino.img: \
		$(flashprefix)/boot-cramfs.img \
		$(flashprefix)/root-neutrino-squashfs.img
	cp $(flashprefix)/boot-cramfs.img $(flashprefix)/cramfs-squashfs-neutrino.img; \
	dd if=$(flashprefix)/root-neutrino-squashfs.img of=$(flashprefix)/cramfs-squashfs-neutrino.img bs=1024 seek=1024

$(flashprefix)/var-neutrino-jffs2.img: \
		$(flashprefix)/root \
		$(flashprefix)/root-neutrino-squashfs
	-rm $@
	mkfs.jffs2 -b -U -e 0x20000 --pad=3801088 -r $(flashprefix)/root-neutrino-squashfs/var_init -o $@
	@if [ `stat -c %s $@` -gt 3801088 ]; then \
		echo "ERROR: jffs2 part is too big for image (max. allowed 3801088 bytes)"; \
		rm -f $(flashprefix)/var-neutrino-jffs2.img.too-big 2> /dev/null || /bin/true; \
		mv $@ $(flashprefix)/var-neutrino-jffs2.img.too-big; \
		exit 1; \
	fi

$(flashprefix)/complete-neutrino.img: \
		$(flashprefix)/boot-cramfs.img \
		$(flashprefix)/root-neutrino-squashfs.img \
		$(flashprefix)/cramfs-squashfs-neutrino.img \
		$(flashprefix)/var-neutrino-jffs2.img
	cp $(flashprefix)/cramfs-squashfs-neutrino.img $(flashprefix)/complete-neutrino.img; \
	dd if=$(flashprefix)/var-neutrino-jffs2.img of=$(flashprefix)/complete-neutrino.img bs=1024 seek=4224
	@if [ `stat -c %s $@` -ne 8126464 ]; then \
		echo "ERROR: complete image is not flash (w/o bootloader) size (flash. size 8126464 bytes)"; \
		rm -f $(flashprefix)/complete-neutrino.img.bad.size 2> /dev/null || /bin/true; \
		mv $@ $(flashprefix)/complete-neutrino.img.bad.size; \
		exit 1; \
	fi
endif

$(flashprefix)/enigma-jffs2.img1x $(flashprefix)/enigma-jffs2.img2x: \
$(flashprefix)/enigma-jffs2.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-enigma.jffs2 \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	@FILESIZE=$$(stat -c%s $@); \
	if [ $$FILESIZE -gt 8257536 ]; \
		then echo "fatal error: File $@ too large ($$FILESIZE > 8257536)"; \
		rm $@; exit 1; \
	fi
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/enigma-jffs2_lzma.img1x $(flashprefix)/enigma-jffs2_lzma.img2x: \
$(flashprefix)/enigma-jffs2_lzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-enigma.jffs2_lzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	@FILESIZE=$$(stat -c%s $@); \
	if [ $$FILESIZE -gt 8257536 ]; \
		then echo "fatal error: File $@ too large ($$FILESIZE > 8257536)"; \
		rm $@; exit 1; \
	fi
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/enigma-jffs2_lzma_klzma.img1x $(flashprefix)/enigma-jffs2_lzma_klzma.img2x: \
$(flashprefix)/enigma-jffs2_lzma_klzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-enigma.jffs2_lzma_klzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	@FILESIZE=$$(stat -c%s $@); \
	if [ $$FILESIZE -gt 8257536 ]; \
		then echo "fatal error: File $@ too large ($$FILESIZE > 8257536)"; \
		rm $@; exit 1; \
	fi
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

################################################################
$(flashprefix)/lcars-jffs2.img1x $(flashprefix)/lcars-jffs2.img2x: \
$(flashprefix)/lcars-jffs2.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-lcars.jffs2 \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

################################################################
$(flashprefix)/null-jffs2.img1x $(flashprefix)/null-jffs2.img2x: \
$(flashprefix)/null-jffs2.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-null.jffs2 \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/null-jffs2_lzma.img1x $(flashprefix)/null-jffs2_lzma.img2x: \
$(flashprefix)/null-jffs2_lzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-null.jffs2_lzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/null-jffs2_lzma_klzma.img1x $(flashprefix)/null-jffs2_lzma_klzma.img2x: \
$(flashprefix)/null-jffs2_lzma_klzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-null.jffs2_lzma_klzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

################################################################
$(flashprefix)/enigma+neutrino-squashfs.img1x $(flashprefix)/enigma+neutrino-squashfs.img2x:\
$(flashprefix)/enigma+neutrino-squashfs.img%: \
		$(flashprefix)/squashfs.flfs% \
		$(flashprefix)/root-enigma+neutrino.squashfs \
		$(flashprefix)/var-enigma+neutrino.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/enigma+neutrino-squashfs_nolzma.img1x $(flashprefix)/enigma+neutrino-squashfs_nolzma.img2x:\
$(flashprefix)/enigma+neutrino-squashfs_nolzma.img%: \
		$(flashprefix)/squashfs_nolzma.flfs% \
		$(flashprefix)/root-enigma+neutrino.squashfs_nolzma \
		$(flashprefix)/var-enigma+neutrino.jffs2 \
		$(hostprefix)/bin/checkImage
	$(hostappsdir)/flash/flashmanage.pl -i $@ -o build \
		--rootsize=$(ROOT_PARTITION_SIZE) \
		--part ppcboot=$< \
		--part root=$(word 2,$+) \
		--part var=$(word 3,$+)
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/enigma+neutrino-jffs2_lzma.img1x $(flashprefix)/enigma+neutrino-jffs2_lzma.img2x: \
$(flashprefix)/enigma+neutrino-jffs2_lzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-enigma+neutrino.jffs2_lzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	@FILESIZE=$$(stat -c%s $@); \
	if [ $$FILESIZE -gt 8257536 ]; \
		then echo "fatal error: File $@ too large ($$FILESIZE > 8257536)"; \
		rm $@; exit 1; \
	fi
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

$(flashprefix)/enigma+neutrino-jffs2_lzma_klzma.img1x $(flashprefix)/enigma+neutrino-jffs2_lzma_klzma.img2x: \
$(flashprefix)/enigma+neutrino-jffs2_lzma_klzma.img%: \
		$(flashprefix)/jffs2.flfs% \
		$(flashprefix)/root-enigma+neutrino.jffs2_lzma_klzma \
		$(hostprefix)/bin/checkImage
	cat $< $(word 2,$+) > $@
	@FILESIZE=$$(stat -c%s $@); \
	if [ $$FILESIZE -gt 8257536 ]; \
		then echo "fatal error: File $@ too large ($$FILESIZE > 8257536)"; \
		rm $@; exit 1; \
	fi
	$(TUXBOX_CHECKIMAGE) && \
	@TUXBOX_CUSTOMIZE@

# target yadd-enigma+neutrino makes no sense, use
# make yadd-neutrino yadd-enigma lcd
# instead
enigma+neutrino: neutrino enigma

flash-enigma+neutrino: $(flashprefix)/root-neutrino $(flashprefix)/root-enigma
