if BOXTYPE_DREAMBOX
KERNEL_DEPENDS = @DEPENDS_linux_dream@
KERNEL_DIR = @DIR_linux_dream@
KERNEL_PREPARE = @PREPARE_linux_dream@
KERNEL_BUILD_FILENAME = @DIR_linux_dream@/arch/ppc/boot/images/zImage.treeboot
else
if BOXTYPE_IPBOX
KERNEL_DEPENDS = @DEPENDS_linux_ipbox@
KERNEL_DIR = @DIR_linux_ipbox@
KERNEL_PREPARE = @PREPARE_linux_ipbox@
KERNEL_BUILD_FILENAME = @DIR_linux_ipbox@/vmlinux
if BOXMODEL_IP200
IPBOX_UBOOT_TARGET = relook100s
IPBOX_FLASH_MAP = $(buildprefix)/config/flash_map.mutant200s
IPBOX_DRIVER = dgstationdriver_mutant200s
IPBOX_DRIVER_MODDIR = $(flashprefix)/root-squashfs/lib/modules/@VERSION_linux_ipbox@-mutant200s
IPBOX_DRIVER_DEPENDS = @DEPENDS_dgstationdriver_mutant200s@
IPBOX_DRIVER_DIR = @DIR_dgstationdriver_mutant200s@
IPBOX_DRIVER_PREPARE = @PREPARE_dgstationdriver_mutant200s@
IPBOX_HW_MODEL=0x00010000
endif
if BOXMODEL_IP250
IPBOX_UBOOT_TARGET = relook200s
IPBOX_FLASH_MAP = $(buildprefix)/config/flash_map.relook200s
IPBOX_DRIVER = dgstationdriver_cubecafe
IPBOX_DRIVER_MODDIR = $(flashprefix)/root-squashfs/lib/modules/@VERSION_linux_ipbox@-cubecafe
IPBOX_DRIVER_DEPENDS = @DEPENDS_dgstationdriver_cubecafe@
IPBOX_DRIVER_DIR = @DIR_dgstationdriver_cubecafe@
IPBOX_DRIVER_PREPARE = @PREPARE_dgstationdriver_cubecafe@
IPBOX_HW_MODEL=0x00020000
endif
if BOXMODEL_IP350
IPBOX_UBOOT_TARGET = relook210
IPBOX_FLASH_MAP = $(buildprefix)/config/flash_map.relook210
IPBOX_DRIVER = dgstationdriver_prime
IPBOX_DRIVER_MODDIR = $(flashprefix)/root-squashfs/lib/modules/@VERSION_linux_ipbox@-cubecafe-prime
IPBOX_DRIVER_DEPENDS = @DEPENDS_dgstationdriver_prime@
IPBOX_DRIVER_DIR = @DIR_dgstationdriver_prime@
IPBOX_DRIVER_PREPARE = @PREPARE_dgstationdriver_prime@
IPBOX_HW_MODEL=0x00021000
endif
if BOXMODEL_IP400
IPBOX_UBOOT_TARGET = relook400s
IPBOX_FLASH_MAP = $(buildprefix)/config/flash_map.relook400s
IPBOX_DRIVER_MODDIR = $(flashprefix)/root-squashfs/lib/modules/@VERSION_linux_ipbox@-relook400
IPBOX_DRIVER_DEPENDS = @DEPENDS_dgstationdriver_relook400s@
IPBOX_DRIVER_DIR = @DIR_dgstationdriver_relook400s@
IPBOX_DRIVER_PREPARE = @PREPARE_dgstationdriver_relook400s@
IPBOX_HW_MODEL=0x00040000
endif
else
if BOXTYPE_COOL
KERNEL_DEPENDS = @DEPENDS_linux_cool@
KERNEL_DIR = @DIR_linux_cool@
KERNEL_PREPARE = @PREPARE_linux_cool@
KERNEL_BUILD_FILENAME = @DIR_linux_cool@/arch/arm/boot/images/uImage
else
if KERNEL26
KERNEL_DEPENDS = @DEPENDS_linux@
KERNEL_DIR = @DIR_linux@
KERNEL_PREPARE = @PREPARE_linux@
KERNEL_BUILD_FILENAME = @DIR_linux@/arch/ppc/boot/images/uImage
else
KERNEL_DEPENDS = @DEPENDS_linux24@
KERNEL_DIR = @DIR_linux24@
KERNEL_PREPARE = @PREPARE_linux24@
KERNEL_BUILD_FILENAME = @DIR_linux24@/arch/ppc/boot/images/vmlinux.gz
endif
endif
endif
endif

if ENABLE_FS_CIFS
KERNEL_DEPENDS += $(archivedir)/cifs-1.20c-2.4.tar.gz $(buildprefix)/Patches/linux-2.4-cifs_gcc4.diff
endif

if ENABLE_AUTOMOUNT
KERNEL_DEPENDS += $(archivedir)/autofs4-2.4-module-20050404.tar.gz
endif

$(DEPDIR)/linuxdir: $(KERNEL_DEPENDS) @DEPENDS_liblzma465@ directories
	$(KERNEL_PREPARE)
	@PREPARE_liblzma465@
if KERNEL26
if BOXTYPE_DREAMBOX
	cp $(KERNEL_DIR)/arch/ppc/configs/$(BOXMODEL)_defconfig $(KERNEL_DIR)/.config
else
if BOXTYPE_COOL
# do nothing here
	touch $(KERNEL_DIR)/.config
else
	m4 $(KERNEL_M4) $(kernel_conf) > $(KERNEL_DIR)/.config
endif
endif
if BOXTYPE_DBOX2
	$(INSTALL) -d $(KERNEL_DIR)/lib/lzma/
	$(INSTALL) -d $(KERNEL_DIR)/include/linux/lzma/
	mv @DIR_liblzma465@/C/Lz*.c $(KERNEL_DIR)/lib/lzma/
	mv @DIR_liblzma465@/C/*.h $(KERNEL_DIR)/include/linux/lzma/
	cd $(KERNEL_DIR) && patch -p1 -E -i $(buildprefix)/Patches/linux-2.6-jffs2_lzma.diff
endif
else
	m4 $(KERNEL_M4) $(kernel_conf) > $(KERNEL_DIR)/.config
if ENABLE_FS_LUFS
	cd $(KERNEL_DIR) && patch -p1 -E -i $(buildprefix)/Patches/linux-2.4.33-dbox2-lufs.diff
endif
if ENABLE_FS_CIFS
	gunzip -cd $(archivedir)/cifs-1.20c-2.4.tar.gz | TAPE=- tar -xh
	cd $(KERNEL_DIR) && patch -p1 -E -i ./cifs_24.patch
	cd $(KERNEL_DIR) && patch -p1 -E -i $(buildprefix)/Patches/linux-2.4-cifs_gcc4.diff
endif
if ENABLE_AUTOMOUNT
	cd $(KERNEL_DIR) && gunzip -cd $(archivedir)/autofs4-2.4-module-20050404.tar.gz | TAPE=- tar -x
	cd $(KERNEL_DIR) && patch -p1 -E -i ./autofs4-2.4/module-patches/autofs4-2.4.29.patch
endif
	cd $(KERNEL_DIR) && \
	  bzcat $(archivedir)/linux-2.4.34-dbox2-mtd0305.diff.bz2 > linux-2.4.34-dbox2-mtd0305.diff && \
	  patch -p1 -E -i $(buildprefix)/Patches/linux-2.4.37.11-dbox2-mtd0305.diff && \
	patch -p1 -E -i linux-2.4.34-dbox2-mtd0305.diff
	mv @DIR_liblzma465@/C/Lz* $(KERNEL_DIR)/fs/jffs2/
	mv @DIR_liblzma465@/C/Types.h $(KERNEL_DIR)/fs/jffs2/
	cd $(KERNEL_DIR) && patch -p1 -E -i $(buildprefix)/Patches/linux-2.4-jffs2_lzma.diff
endif
	@CLEANUP_liblzma465@
if !BOXTYPE_COOL
	$(MAKE) -C $(KERNEL_DIR) oldconfig \
		ARCH=ppc
if KERNEL26
	$(MAKE) -C $(KERNEL_DIR) include/asm \
		ARCH=ppc
endif
	$(MAKE) -C $(KERNEL_DIR) include/linux/version.h \
		ARCH=ppc
endif
	rm -f $(KERNEL_DIR)/.config
	touch $@

if !USE_FOREIGN_TOOLCHAIN
if BOXTYPE_DREAMBOX
$(DEPDIR)/binutils: @DEPENDS_binutils_dream@ directories 
	@PREPARE_binutils_dream@
	cd @DIR_binutils_dream@ && \
		CC=$(CC) \
		CFLAGS="$(CFLAGS) -D_FORTIFY_SOURCE=0" \
		@CONFIGURE_binutils_dream@ \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--disable-nls \
			--nfp && \
		$(MAKE) all all-gprof && \
		@INSTALL_binutils_dream@
	@CLEANUP_binutils_dream@
	touch $@

$(DEPDIR)/bootstrap_gcc: @DEPENDS_bootstrap_gcc_dream@ binutils linuxdir
	@PREPARE_bootstrap_gcc_dream@
	$(INSTALL) -d $(hostprefix)/$(target)/sys-include
	ln -sf $(buildprefix)/linux/include/{asm,linux} $(hostprefix)/$(target)/sys-include/
	cd @DIR_bootstrap_gcc_dream@ && \
		CC=$(CC) CFLAGS="$(CFLAGS) -D_FORTIFY_SOURCE=0" \
		@CONFIGURE_bootstrap_gcc_dream@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-cpu=$(CPU_MODEL) \
			--enable-target-optspace \
			--enable-languages="c" \
			--disable-shared \
			--disable-nls \
			--disable-threads \
			--with-float=soft && \
		$(MAKE) all && \
		@INSTALL_bootstrap_gcc_dream@
	rm -rf $(hostprefix)/$(target)/sys-include
	@CLEANUP_bootstrap_gcc_dream@
	touch $@


$(DEPDIR)/libc: @DEPENDS_glibc_dream@ bootstrap_gcc install-linux-headers
	@PREPARE_glibc_dream@
	cd @DIR_glibc_dream@ && \
		CC=$(target)-gcc \
		AR=$(target)-ar \
		RANLIB=$(target)-ranlib \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		@CONFIGURE_glibc_dream@ \
			--build=$(build) \
			--host=$(target) \
			--prefix= \
			--with-headers=$(buildprefix)/linux/include \
			--enable-kernel=2.6.5 \
			--disable-profile \
			--enable-shared \
			--enable-add-ons=nptl \
			--with-__thread \
			--without-fp \
			--with-tls && \
		$(MAKE) all && \
		@INSTALL_glibc_dream@
	@CLEANUP_glibc_dream@
	rm $(hostprefix)/$(target)/include/asm-generic 2> /dev/null || /bin/true;
	rm $(buildprefix)/linux-libc-headers 2> /dev/null || /bin/true
	touch $@

$(DEPDIR)/gcc: @DEPENDS_gcc_dream@ libc
	@PREPARE_gcc_dream@
	$(INSTALL) -d $(hostprefix)/$(target)/sys-include
	@if [ -s $(hostprefix)/$(target)/lib/nof ] ; then \
		rm $(hostprefix)/$(target)/lib/nof; \
	fi
	cp -p $(hostprefix)/$(target)/include/limits.h $(hostprefix)/$(target)/sys-include/
	cd @DIR_gcc_dream@ && \
		CC=$(CC) CFLAGS="$(CFLAGS) -D_FORTIFY_SOURCE=0" \
		@CONFIGURE_gcc_dream@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-cpu=$(CPU_MODEL) \
			--enable-target-optspace \
			--enable-languages="c,c++" \
			--enable-shared \
			--enable-threads=posix \
			--disable-nls \
			--with-float=soft \
			--enable-__cxa_atexit && \
		$(MAKE) all && \
		@INSTALL_gcc_dream@
	rm -rf $(hostprefix)/$(target)/sys-include
	@for i in `find $(hostprefix)/$(target)/lib/nof -type f` ; do mv $$i $(hostprefix)/$(target)/lib; done
	@for i in `find $(hostprefix)/$(target)/lib/nof -type l` ; do mv $$i $(hostprefix)/$(target)/lib; done
	rm -rf $(hostprefix)/$(target)/lib/nof
	ln -sf $(hostprefix)/$(target)/lib $(hostprefix)/$(target)/lib/nof
	@CLEANUP_gcc_dream@
	touch $@
else
if BOXTYPE_COOL
BINUTILS_OPTS= \
--disable-multilib \
--with-gmp=$(hostprefix) \
--with-mpfr=$(hostprefix) \
--with-float=soft
$(DEPDIR)/binutils: @DEPENDS_binutils@ directories libmpfr_host
else
BINUTILS_OPTS= \
--without-fp
BINUTILS_PATCHES= \
$(archivedir)/300-001_ld_makefile_patch.patch \
$(archivedir)/300-012_check_ldrunpath_length.patch
$(DEPDIR)/binutils: @DEPENDS_binutils@ $(BINUTILS_PATCHES) directories
endif
	@PREPARE_binutils@
	for i in ld gas intl binutils; do \
		mkdir -p @DIR_binutils@/$$i; \
		echo "ac_cv_func_setlocale=no" > @DIR_binutils@/$$i/config.cache; \
	done
	cd @DIR_binutils@ && \
		unset CONFIG_SITE && \
		CC=$(CC) \
		CFLAGS="$(CFLAGS)" \
		@CONFIGURE_binutils@ \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--disable-nls \
			--disable-werror \
			$(BINUTILS_OPTS) && \
		$(MAKE) configure-host && \
		$(MAKE) all && \
		$(MAKE) all-gprof && \
		@INSTALL_binutils@
	@CLEANUP_binutils@
	touch $@

if !BOXTYPE_COOL
#
# gcc first stage without glibc
#
if !DBOX2_GCC4
if ASSUME_KERNELSOURCES_OLD
$(DEPDIR)/bootstrap_gcc: @DEPENDS_bootstrap_gcc@ binutils | linuxdir
else
$(DEPDIR)/bootstrap_gcc: @DEPENDS_bootstrap_gcc@ binutils linuxdir
endif
	@PREPARE_bootstrap_gcc@
	$(INSTALL) -d $(hostprefix)/$(target)/sys-include
	ln -sf $(buildprefix)/linux/include/{asm,linux} $(hostprefix)/$(target)/sys-include/
	cd @DIR_bootstrap_gcc@ && \
		unset CONFIG_SITE && \
		CC=$(CC) CFLAGS="$(CFLAGS)" \
		@CONFIGURE_bootstrap_gcc@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-cpu=$(CPU_MODEL) \
			--enable-target-optspace \
			--enable-languages="c" \
			--disable-shared \
			--disable-threads \
			--disable-nls \
			--without-fp && \
		$(MAKE) all && \
		@INSTALL_bootstrap_gcc@
	rm -rf $(hostprefix)/$(target)/sys-include
	@CLEANUP_bootstrap_gcc@
	touch $@
else
if ASSUME_KERNELSOURCES_OLD
$(DEPDIR)/bootstrap_gcc: @DEPENDS_bootstrap_gcc41@ binutils | linuxdir
else
$(DEPDIR)/bootstrap_gcc: @DEPENDS_bootstrap_gcc41@ binutils linuxdir
endif
	@PREPARE_bootstrap_gcc41@
	$(INSTALL) -d $(hostprefix)/$(target)/sys-include
	ln -sf $(buildprefix)/linux/include/{asm,linux} $(hostprefix)/$(target)/sys-include/
	cd @SOURCEDIR_bootstrap_gcc41@ && \
		bunzip2 -cd $(archivedir)/gcc-4.1.2-patches-1.3.tar.bz2 | TAPE=- tar -x && \
		for i in patch/*.patch; do ( patch -p0 -f -i $$i || patch -p1 -f -i $$i ); done && cd .. && \
	cd @DIR_bootstrap_gcc41@ && \
		unset CONFIG_SITE && \
		CC=$(CC) CFLAGS="$(CFLAGS)" \
		@CONFIGURE_bootstrap_gcc41@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-cpu=$(CPU_MODEL) \
			--enable-target-optspace \
			--enable-languages="c" \
			--disable-shared \
			--disable-threads \
			--disable-nls \
			--without-headers \
			--with-newlib \
			--disable-libgomp \
			--disable-libmudflap \
			--disable-libssp \
			--without-fp && \
		$(MAKE) all && \
		@INSTALL_bootstrap_gcc41@
	rm -rf $(hostprefix)/$(target)/sys-include
	@CLEANUP_bootstrap_gcc41@
	touch $@
endif
endif

UCLIBC_M4 =

if !BOXTYPE_COOL
if ENABLE_UCLIBC

if ENABLE_DEBUG
UCLIBC_M4 += -Ddebug
endif

if BOXTYPE_DBOX2
if KERNEL26
UCLIBC_M4 += -DKHEADERS=\`\"$(buildprefix)/$(KERNEL_DIR)/usr/include\"\'
else
UCLIBC_M4 += -DKHEADERS=\`\"$(buildprefix)/$(KERNEL_DIR)/include\"\'
endif
else
UCLIBC_M4 += -DKHEADERS=\`\"$(hostprefix)/$(target)/include\"\'
endif

if ASSUME_KERNELSOURCES_OLD
$(DEPDIR)/libc: @DEPENDS_uclibc@ bootstrap_gcc | install-linux-headers
else
$(DEPDIR)/libc: @DEPENDS_uclibc@ bootstrap_gcc install-linux-headers
endif
	@PREPARE_uclibc@
	m4 $(UCLIBC_M4) config/uclibc.config.m4 > @DIR_uclibc@/.config
	$(MAKE) -C @DIR_uclibc@ oldconfig ARCH=ppc
	cd @DIR_uclibc@ && \
		$(BUILDENV) \
		$(MAKE) \
		PREFIX= \
		HOSTCC=$(CC) \
		all && \
		@INSTALL_uclibc@
	sed -i -e 's,/lib/,$(targetprefix)/lib/,g' $(targetprefix)/lib/libc.so
	@CLEANUP_uclibc@
	touch $@

else
if USE_FOREIGN_TOOLCHAIN
$(DEPDIR)/libc: directories
	cp -a $(TOOLCHAIN_PATH)/$(target)/lib $(targetprefix)/
	touch $@
else

if ASSUME_KERNELSOURCES_OLD
$(DEPDIR)/libc: @DEPENDS_glibc@ bootstrap_gcc | install-linux-headers
else
$(DEPDIR)/libc: @DEPENDS_glibc@ bootstrap_gcc install-linux-headers
endif
	@PREPARE_glibc@
	touch @DIR_glibc@/config.cache
	@if [ $(GLIBC_PTHREADS) = "nptl" ]; then \
		cp @SOURCEDIR_glibc@/nptl/sysdeps/pthread/pthread.h @DIR_glibc@ && \
		cp @SOURCEDIR_glibc@/nptl/sysdeps/unix/sysv/linux/powerpc/bits/pthreadtypes.h @DIR_glibc@ && \
		echo "libc_cv_forced_unwind=yes" > @DIR_glibc@/config.cache && \
		echo "libc_cv_c_cleanup=yes" >> @DIR_glibc@/config.cache; \
	fi
if CPUMODEL_405
	rm @SOURCEDIR_glibc@/sysdeps/powerpc/powerpc32/strncmp.S
	cd @SOURCEDIR_glibc@ && patch -p1 -E -i $(buildprefix)/Patches/glibc_ppc4xx_ibmstropt.diff
	cd @SOURCEDIR_glibc@ && patch -p1 -E -i $(buildprefix)/Patches/glibc-ibmppc4xx_fp_perflib.diff
endif
	cd @DIR_glibc@ && \
		$(BUILDENV) \
		@CONFIGURE_glibc@ \
			--build=$(build) \
			--host=$(target) \
			--prefix= \
			--with-headers=$(targetprefix)/include \
			--disable-profile \
			--disable-debug  \
			--enable-shared \
			--without-gd \
			--with-tls \
			--with-__thread \
			--enable-add-ons=$(GLIBC_PTHREADS) \
			--enable-clocale=gnu \
			--without-fp \
			--cache-file=config.cache \
			$(GLIBC_EXTRA_FLAGS) && \
		$(MAKE) all && \
		@INSTALL_glibc@
	@CLEANUP_glibc@
	sed -e's, /lib/, $(targetprefix)/lib/,g' < $(targetprefix)/lib/libc.so > $(targetprefix)/lib/libc.so.new
	mv $(targetprefix)/lib/libc.so.new $(targetprefix)/lib/libc.so
	sed -e's, /lib/, $(targetprefix)/lib/,g' < $(targetprefix)/lib/libpthread.so > $(targetprefix)/lib/libpthread.so.new
	mv $(targetprefix)/lib/libpthread.so.new $(targetprefix)/lib/libpthread.so
	touch $@
endif
endif

#
# gcc second stage
#
if USE_FOREIGN_TOOLCHAIN
$(DEPDIR)/gcc: libc linuxdir
	touch $@
else
if !DBOX2_GCC4
$(DEPDIR)/gcc: @DEPENDS_gcc@ libc
# if we have a symlink inside the libdir (in case gcc has already been built)
# we remove it here
	@if [ -h $(hostprefix)/$(target)/lib/nof ]; then \
		rm -f $(hostprefix)/$(target)/lib/nof; \
	fi
	@PREPARE_gcc@
if ENABLE_UCLIBC
	cd @SOURCEDIR_gcc@ && patch -p1 -E -i $(buildprefix)/Patches/gcc-uclibc.diff
endif
if CPUMODEL_405
	cd @SOURCEDIR_gcc@ && patch -p1 -E -i ../Patches/gcc-g++-ppc4xx.diff
	cd @SOURCEDIR_gcc@ && patch -p1 -E -i ../Patches/gcc-ibmppc4xx_fp_perflib.diff
endif
	$(INSTALL) -d $(hostprefix)/$(target)/sys-include
	cp -p $(hostprefix)/$(target)/include/limits.h $(hostprefix)/$(target)/sys-include/
	cd @DIR_gcc@ && \
		unset CONFIG_SITE && \
		CC=$(CC) CFLAGS="$(CFLAGS)" \
		@CONFIGURE_gcc@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-cpu=$(CPU_MODEL) \
			--enable-target-optspace \
			--enable-languages="c,c++" \
			--enable-shared \
			--enable-threads \
			--disable-nls \
			--without-fp && \
		$(MAKE) all && \
		@INSTALL_gcc@
	rm -rf $(hostprefix)/$(target)/sys-include
	for i in `find $(hostprefix)/$(target)/lib/nof` ; do mv $$i $(hostprefix)/$(target)/lib; done
	rm -rf $(hostprefix)/$(target)/lib/nof
	ln -sf $(hostprefix)/$(target)/lib $(hostprefix)/$(target)/lib/nof
	@CLEANUP_gcc@
	touch $@
else
if ENABLE_UCLIBC
$(DEPDIR)/gcc: @DEPENDS_gcc41@ libc $(archivedir)/100-uclibc-conf.patch $(archivedir)/200-uclibc-locale.patch
else
$(DEPDIR)/gcc: @DEPENDS_gcc41@ libc
endif
# if we have a symlink inside the libdir (in case gcc has already been built)
# we remove it here
	@if [ -h $(hostprefix)/$(target)/lib/nof ]; then \
		rm -f $(hostprefix)/$(target)/lib/nof; \
	fi
	@PREPARE_gcc41@
	$(INSTALL) -d $(hostprefix)/$(target)/sys-include
	cp -p $(hostprefix)/$(target)/include/limits.h $(hostprefix)/$(target)/sys-include/
	cd @SOURCEDIR_gcc41@ && \
		bunzip2 -cd $(archivedir)/gcc-4.1.2-patches-1.3.tar.bz2 | TAPE=- tar -x && \
		for i in patch/*.patch; do ( patch -p0 -f -i $$i || patch -p1 -f -i $$i ); done
if ENABLE_UCLIBC
	-cd @SOURCEDIR_gcc41@ && patch -p1 -f -E -i $(archivedir)/100-uclibc-conf.patch
	cd @SOURCEDIR_gcc41@ && patch -p1 -f -E -i $(archivedir)/200-uclibc-locale.patch
endif
	cd @DIR_gcc41@ && \
		unset CONFIG_SITE && \
		CC=$(CC) CFLAGS="$(CFLAGS)" \
		@CONFIGURE_gcc41@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-cpu=$(CPU_MODEL) \
			--enable-target-optspace \
			--enable-languages="c,c++" \
			--enable-shared \
			--enable-threads \
			--disable-nls \
			--disable-libgomp \
			--disable-libmudflap \
			--disable-libssp \
			--without-fp && \
		$(MAKE) all && \
		@INSTALL_gcc41@
	rm -rf $(hostprefix)/$(target)/sys-include
	for i in `find $(hostprefix)/$(target)/lib/nof` ; do mv $$i $(hostprefix)/$(target)/lib; done
	rm -rf $(hostprefix)/$(target)/lib/nof
	ln -sf $(hostprefix)/$(target)/lib $(hostprefix)/$(target)/lib/nof
	ln -s libgcc_s.so.1 $(hostprefix)/$(target)/lib/libgcc_s_nof.so.1
	ln -s libgcc_s.so.1 $(hostprefix)/$(target)/lib/libgcc_s_nof.so
	@CLEANUP_gcc41@
	touch $@
endif
endif

else

$(DEPDIR)/bootstrap_gcc_static_cool: @DEPENDS_bootstrap_gcc_static_cool@ binutils install-linux-headers
	@PREPARE_bootstrap_gcc_static_cool@
	$(INSTALL) -d $(hostprefix)/$(target)/sys-include
	ln -sf $(buildprefix)/linux/include/{asm,linux} $(hostprefix)/$(target)/sys-include/
	cd @DIR_bootstrap_gcc_static_cool@ && \
		CC=$(CC) CFLAGS="$(CFLAGS)" \
		@CONFIGURE_bootstrap_gcc_static_cool@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--disable-multilib \
			--with-newlib \
			--enable-threads=no \
			--disable-shared \
			--with-float=soft \
			--with-gmp=$(hostprefix) \
			--with-mpfr=$(hostprefix) \
			--enable-__cxa_atexit \
			--disable-nls \
			--enable-symvers=gnu \
			--enable-languages=c \
			--enable-target-optspace && \
		$(MAKE) all-gcc && \
		@INSTALL_bootstrap_gcc_static_cool@
	rm -rf $(hostprefix)/$(target)/sys-include
	@CLEANUP_bootstrap_gcc_static_cool@
	touch $@

$(DEPDIR)/bootstrap_gcc_shared_cool: @DEPENDS_bootstrap_gcc_shared_cool@ bootstrap_eglibc_cool
	@PREPARE_bootstrap_gcc_shared_cool@
	$(INSTALL) -d $(hostprefix)/$(target)/sys-include
	ln -sf $(buildprefix)/linux/include/{asm,linux} $(hostprefix)/$(target)/sys-include/
	cd @DIR_bootstrap_gcc_shared_cool@ && \
		CC=$(CC) CFLAGS="$(CFLAGS)" \
		@CONFIGURE_bootstrap_gcc_shared_cool@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--disable-multilib \
			--enable-shared \
			--with-float=soft \
			--with-gmp=$(hostprefix) \
			--with-mpfr=$(hostprefix) \
			--enable-__cxa_atexit \
			--disable-nls \
			--enable-symvers=gnu \
			--enable-languages=c \
			--enable-target-optspace && \
		$(MAKE) configure-gcc configure-libcpp configure-build-libiberty && \
		$(MAKE) all-libcpp all-build-libiberty && \
		$(MAKE) configure-libdecnumber && \
		$(MAKE) -C libdecnumber libdecnumber.a && \
		$(MAKE) -C gcc libgcc.mvars && \
		$(MAKE) all-gcc all-target-libgcc && \
		@INSTALL_bootstrap_gcc_shared_cool@
	rm -rf $(hostprefix)/$(target)/sys-include
	@CLEANUP_bootstrap_gcc_shared_cool@
	touch $@

$(DEPDIR)/bootstrap_eglibc_cool: $(archivedir)/eglibc-2_8.tar.bz2 bootstrap_gcc_static_cool
	@PREPARE_bootstrap_eglibc_cool@
	cd @DIR_bootstrap_eglibc_cool@ && \
		@CONFIGURE_bootstrap_eglibc_cool@ \
			--build=$(build) \
			--host=$(target) \
			--prefix= \
			--with-headers=$(targetprefix)/include \
			--disable-profile \
			--without-gd \
			--without-cvs \
			--enable-add-ons && \
		$(MAKE) install-headers install_root=$(targetprefix) install-bootstrap-headers=yes && \
		$(MAKE) csu/subdir_lib && \
		cp csu/crt1.o csu/crti.o csu/crtn.o $(targetprefix)/lib && \
		$(target)-gcc -nostdlib -nostartfiles -shared -x c /dev/null -o $(targetprefix)/lib/libc.so
	@CLEANUP_bootstrap_eglibc_cool@
	touch $@

$(DEPDIR)/eglibc_cool: $(archivedir)/eglibc-2_8.tar.bz2 bootstrap_gcc_shared_cool
	@PREPARE_eglibc_cool@
	cd @DIR_eglibc_cool@ && \
		@CONFIGURE_eglibc_cool@ \
			--build=$(build) \
			--host=$(target) \
			--prefix= \
			--with-headers=$(targetprefix)/include \
			--disable-profile \
			--without-gd \
			--without-cvs \
			--enable-kernel=2.6.26 \
			--with-__thread \
			--with-tls \
			--enable-shared \
			--without-fp \
			--enable-add-ons=nptl,ports \
			--enable-kernel=2.6.18 && \
		$(MAKE) && \
		$(MAKE) install install_root=$(targetprefix)
	sed -e's, /lib/, $(targetprefix)/lib/,g' < $(targetprefix)/lib/libc.so > $(targetprefix)/lib/libc.so.new
	mv $(targetprefix)/lib/libc.so.new $(targetprefix)/lib/libc.so
	sed -e's, /lib/, $(targetprefix)/lib/,g' < $(targetprefix)/lib/libpthread.so > $(targetprefix)/lib/libpthread.so.new
	mv $(targetprefix)/lib/libpthread.so.new $(targetprefix)/lib/libpthread.so
	@CLEANUP_eglibc_cool@
	touch $@

$(DEPDIR)/gcc: @DEPENDS_gcc_cool@ eglibc_cool
	@PREPARE_gcc_cool@
	cd @DIR_gcc_cool@ && \
		CC=$(CC) CFLAGS="$(CFLAGS)" \
		@CONFIGURE_gcc_cool@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--enable-languages=c,c++ \
			--disable-multilib \
			--with-float=soft \
			--with-gmp=$(hostprefix) \
			--with-mpfr=$(hostprefix) \
			--enable-__cxa_atexit \
			--with-local-prefix=$(targetprefix) \
			--disable-nls \
			--enable-threads=posix \
			--enable-symvers=gnu \
			--enable-c99 \
			--enable-long-long \
			--enable-target-optspace \
			--enable-shared && \
		$(MAKE) all && \
		$(MAKE) install
	@CLEANUP_gcc_cool@
	touch $@

$(archivedir)/eglibc-2_8.tar.bz2:
	rm -rf $(buildprefix)/eglibc_svn && \
	mkdir $(buildprefix)/eglibc_svn && \
	cd $(buildprefix)/eglibc_svn && \
	svn export --force -r HEAD svn://svn.eglibc.org/branches/eglibc-2_8 . && \
	mv libc eglibc-2_8 && \
	tar cjf eglibc-2_8.tar.bz2 eglibc-2_8 && \
	tar cjf eglibc-linuxthreads-2_8.tar.bz2 linuxthreads && \
	tar cjf eglibc-localedef-2_8.tar.bz2 localedef && \
	tar cjf eglibc-ports-2_8.tar.bz2 ports && \
	for i in eglibc eglibc-linuxthreads eglibc-localedef eglibc-ports; do \
		mv $$i-2_8.tar.bz2 $(archivedir)/; \
	done && \
	cd $(buildprefix) && \
	rm -rf $(buildprefix)/eglibc_svn

$(archivedir)/eglibc-linuxthreads-2_8.tar.bz2 \
$(archivedir)/eglibc-localedef-2_8.tar.bz2 \
$(archivedir)/eglibc-ports-2_8.tar.bz2: \
$(archivedir)/eglibc-2_8.tar.bz2

endif
endif
else
$(DEPDIR)/bootstrap_gcc:
	touch $@
endif
