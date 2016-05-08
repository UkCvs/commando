KERNEL_DEPENDS = @DEPENDS_linux@
KERNEL_DIR = @DIR_linux@
KERNEL_PREPARE = @PREPARE_linux@
KERNEL_BUILD_FILENAME = @DIR_linux@/arch/ppc/boot/images/uImage

$(DEPDIR)/linuxdir: $(KERNEL_DEPENDS) @DEPENDS_liblzma465@ directories
	$(KERNEL_PREPARE)
	@PREPARE_liblzma465@
	m4 $(KERNEL_M4) $(kernel_conf) > $(KERNEL_DIR)/.config
	$(INSTALL) -d $(KERNEL_DIR)/lib/lzma/
	$(INSTALL) -d $(KERNEL_DIR)/include/linux/lzma/
	mv @DIR_liblzma465@/C/Lz*.c $(KERNEL_DIR)/lib/lzma/
	mv @DIR_liblzma465@/C/*.h $(KERNEL_DIR)/include/linux/lzma/
	cd $(KERNEL_DIR) && patch -p1 -E -i $(buildprefix)/Patches/linux-2.6-jffs2_lzma.diff
	@CLEANUP_liblzma465@
	$(MAKE) -C $(KERNEL_DIR) oldconfig \
		ARCH=ppc
	$(MAKE) -C $(KERNEL_DIR) include/asm \
		ARCH=ppc
	$(MAKE) -C $(KERNEL_DIR) include/linux/version.h \
		ARCH=ppc
	rm -f $(KERNEL_DIR)/.config
	touch $@

BINUTILS_PATCHES= \
$(archivedir)/300-001_ld_makefile_patch.patch \
$(archivedir)/300-012_check_ldrunpath_length.patch

$(DEPDIR)/binutils: @DEPENDS_binutils@ $(BINUTILS_PATCHES) directories libcloog_host
	@PREPARE_binutils@
	for i in ld gas intl binutils; do \
		mkdir -p @DIR_binutils@/$$i; \
		echo "ac_cv_func_setlocale=no" > @DIR_binutils@/$$i/config.cache; \
	done
	cd @DIR_binutils@ && \
		CC=$(CC) \
		CFLAGS="-pipe" \
		CXXFLAGS="-pipe" \
		@CONFIGURE_binutils@ \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-sysroot=$(targetprefix) \
			--with-gmp=$(hostprefix) \
			--with-mpfr=$(hostprefix) \
			--disable-werror \
			--enable-ld=default \
			--enable-gold=yes \
			--enable-threads \
			--enable-plugins \
			--enable-lto \
			--disable-multilib \
			--disable-nls \
			--with-float=soft && \
		$(MAKE) configure-host && \
		$(MAKE) all && \
		$(MAKE) all-gprof && \
		@INSTALL_binutils@
	@CLEANUP_binutils@
	touch $@

GCC_BOOTSTRAP_PATCHES= \
$(archivedir)/410-libgcc_eh.a.patch

$(DEPDIR)/bootstrap_gcc_static: @DEPENDS_bootstrap_gcc47_static@ $(GCC_BOOTSTRAP_PATCHES) binutils
	@PREPARE_bootstrap_gcc47_static@
	cd @DIR_bootstrap_gcc47_static@ && \
		CC=$(CC) \
		CFLAGS="-pipe" \
		LDFLAGS="" \
		@CONFIGURE_bootstrap_gcc47_static@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-sysroot=$(targetprefix) \
			--disable-libmudflap \
			--with-newlib \
			--disable-threads \
			--disable-shared \
			--with-cpu=$(CPU_MODEL) \
			--with-tune=$(CPU_MODEL) \
			--with-float=soft \
			--enable-__cxa_atexit \
			--with-gmp=$(hostprefix) \
			--with-mpfr=$(hostprefix) \
			--with-ppl=$(hostprefix) \
			--with-cloog=$(hostprefix) \
			--enable-cloog-backend=isl \
			--with-host-libstdcxx="-static-libgcc -Wl,-Bstatic,-lstdc++,-Bdynamic -lm" \
			--enable-target-optspace \
			--enable-lto \
			--disable-libgomp \
			--disable-nls \
			--enable-languages=c && \
		$(MAKE) all-gcc && \
		@INSTALL_bootstrap_gcc47_static@
	@CLEANUP_bootstrap_gcc47_static@
	touch $@

$(DEPDIR)/eglibc_version_check: directories
	mkdir -p eglibc_svn
	cd eglibc_svn && \
	LC_ALL=C svn info svn://svn.eglibc.org/trunk eglibc | grep -i "Last Changed Rev" | cut -f2 -d: | sed -e 's/ //g' > eglibc_version && \
	diff -uNr eglibc_version $(archivedir)/eglibc_version > /dev/null || ( \
		echo "downloading current eglibc snapshot..." && \
		svn export --force -r HEAD svn://svn.eglibc.org/trunk . && \
		mv libc eglibc-svn && \
		tar cjf eglibc.tar.bz2 eglibc-svn && \
		mv -f eglibc.tar.bz2 $(archivedir)/ && \
		mv -f eglibc_version $(archivedir)/ \
	) || true
	rm -rf eglibc_svn
	touch $@

EGLIBC_PATCHES= \
$(archivedir)/100-powerpc-8xx-CPU15-errata.patch

$(DEPDIR)/eglibc_headers: eglibc_version_check @DEPENDS_eglibc@ $(EGLIBC_PATCHES) bootstrap_gcc_static install-linux-headers
	@PREPARE_eglibc@
	cd @SOURCEDIR_eglibc@ && \
		cp -v Makeconfig{,.orig} && \
		sed -e 's/-lgcc_eh//g' Makeconfig.orig > Makeconfig
	echo "libc_cv_forced_unwind=yes" > @DIR_eglibc@/config.cache
	echo "libc_cv_c_cleanup=yes" >> @DIR_eglibc@/config.cache
	echo "libc_cv_ppc_machine=yes" >> @DIR_eglibc@/config.cache
	echo "libc_cv_gnu89_inline=yes" >> @DIR_eglibc@/config.cache
	echo "libc_cv_ssp=no" >> @DIR_eglibc@/config.cache
	cp config/eglibc.config @DIR_eglibc@/option-groups.config
	cd @DIR_eglibc@ && \
		BUILD_CC=$(CC) \
		CC="$(target)-gcc" \
		AR="$(target)-ar" \
		RANLIB="$(target)-ranlib" \
		CFLAGS="-U_FORTIFY_SOURCE -mcpu=823 -mtune=823 -msoft-float -O2" \
		@CONFIGURE_eglibc@ \
			--build=$(build) \
			--host=$(target) \
			--prefix= \
			--cache-file=config.cache \
			--without-cvs \
			--disable-profile \
			--without-gd \
			--enable-kernel=2.6.26 \
			--with-__thread \
			--with-tls \
			--enable-shared \
			--enable-obsolete-rpc \
			--without-fp \
			--enable-add-ons=nptl,ports && \
		$(MAKE) install_root=$(targetprefix) install-bootstrap-headers=yes ASFLAGS=-DBROKEN_PPC_8xx_CPU15 install-headers && \
		$(MAKE) ASFLAGS=-DBROKEN_PPC_8xx_CPU15 csu/subdir_lib && \
		for i in crt1 crti crtn; do \
			cp csu/$$i.o $(targetprefix)/lib; \
		done && \
		$(target)-gcc -nostdlib -nostartfiles -shared -x c /dev/null -o $(targetprefix)/lib/libc.so
	@CLEANUP_eglibc@
	touch $@

$(DEPDIR)/bootstrap_gcc: @DEPENDS_bootstrap_gcc47_shared@ $(GCC_BOOTSTRAP_PATCHES) eglibc_headers
	@PREPARE_bootstrap_gcc47_shared@
	cd @DIR_bootstrap_gcc47_shared@ && \
		CC=$(CC) \
		CFLAGS="-pipe" \
		LDFLAGS="" \
		@CONFIGURE_bootstrap_gcc47_shared@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-sysroot=$(targetprefix) \
			--with-native-system-header-dir=/include \
			--disable-libmudflap \
			--enable-shared \
			--with-cpu=$(CPU_MODEL) \
			--with-tune=$(CPU_MODEL) \
			--with-float=soft \
			--enable-__cxa_atexit \
			--with-gmp=$(hostprefix) \
			--with-mpfr=$(hostprefix) \
			--with-ppl=$(hostprefix) \
			--with-cloog=$(hostprefix) \
			--enable-cloog-backend=isl \
			--with-host-libstdcxx="-static-libgcc -Wl,-Bstatic,-lstdc++,-Bdynamic -lm" \
			--enable-target-optspace \
			--enable-lto \
			--disable-libgomp \
			--disable-nls \
			--disable-multilib \
			--enable-languages=c && \
	        $(MAKE) configure-gcc configure-libcpp configure-build-libiberty && \
	        $(MAKE) all-libcpp all-build-libiberty && \
	        $(MAKE) configure-libdecnumber && \
	        $(MAKE) -C libdecnumber libdecnumber.a && \
	        $(MAKE) -C gcc libgcc.mvars && \
	        sed -r -i -e 's@-lc@@g' gcc/libgcc.mvars && \
	        $(MAKE) all-gcc all-target-libgcc && \
		@INSTALL_bootstrap_gcc47_shared@
	@CLEANUP_bootstrap_gcc47_shared@
	touch $@

$(DEPDIR)/eglibc: eglibc_version_check @DEPENDS_eglibc@ $(EGLIBC_PATCHES) bootstrap_gcc
	@PREPARE_eglibc@
	echo "libc_cv_forced_unwind=yes" > @DIR_eglibc@/config.cache
	echo "libc_cv_c_cleanup=yes" >> @DIR_eglibc@/config.cache
	echo "libc_cv_ppc_machine=yes" >> @DIR_eglibc@/config.cache
	echo "libc_cv_gnu89_inline=yes" >> @DIR_eglibc@/config.cache
	echo "libc_cv_ssp=no" >> @DIR_eglibc@/config.cache
	cp config/eglibc.config @DIR_eglibc@/option-groups.config
	cd @DIR_eglibc@ && \
		BUILD_CC=$(CC) \
		CC="$(target)-gcc" \
		AR="$(target)-ar" \
		RANLIB="$(target)-ranlib" \
		CFLAGS="-U_FORTIFY_SOURCE -mcpu=823 -mtune=823 -msoft-float -O2" \
		@CONFIGURE_eglibc@ \
			--build=$(build) \
			--host=$(target) \
			--prefix= \
			--cache-file=config.cache \
			--without-cvs \
			--disable-profile \
			--without-gd \
			--enable-kernel=2.6.26 \
			--with-__thread \
			--with-tls \
			--enable-shared \
			--enable-obsolete-rpc \
			--without-fp \
			--enable-add-ons=nptl,ports && \
		$(MAKE) ASFLAGS=-DBROKEN_PPC_8xx_CPU15 all && \
		$(MAKE) ASFLAGS=-DBROKEN_PPC_8xx_CPU15 install_root=$(targetprefix) install
	@CLEANUP_eglibc@
	touch $@

GCC_PATCHES= \
$(GCC_BOOTSTRAP_PATCHES)

$(DEPDIR)/gcc: @DEPENDS_gcc47@ $(GCC_PATCHES) eglibc
	@PREPARE_gcc47@
	cd @DIR_gcc47@ && \
		CC=$(CC) \
		CFLAGS="-pipe" \
		LDFLAGS="" \
		CFLAGS_FOR_TARGET="-mcpu=823 -mtune=823 -msoft-float" \
		CXXFLAGS_FOR_TARGET="-mcpu=823 -mtune=823 -msoft-float" \
		@CONFIGURE_gcc47@ \
			--build=$(build) \
			--host=$(build) \
			--target=$(target) \
			--prefix=$(hostprefix) \
			--with-sysroot=$(targetprefix) \
			--with-native-system-header-dir=/include \
			--enable-languages=c,c++ \
			--with-cpu=$(CPU_MODEL) \
			--with-float=soft \
			--enable-__cxa_atexit \
			--disable-libmudflap \
			--disable-libgomp \
			--disable-libssp \
			--with-gmp=$(hostprefix) \
			--with-mpfr=$(hostprefix) \
			--with-ppl=$(hostprefix) \
			--with-cloog=$(hostprefix) \
			--enable-cloog-backend=isl \
			--with-host-libstdcxx="-static-libgcc -Wl,-Bstatic,-lstdc++,-Bdynamic -lm" \
			--enable-threads=posix \
			--enable-target-optspace \
			--enable-lto \
			--disable-nls \
			--enable-clocale=generic \
			--disable-multilib \
			--enable-c99 \
			--enable-long-long \
			--enable-symvers=gnu \
			--enable-shared && \
		$(MAKE) all && \
		@INSTALL_gcc47@
		rm $(hostprefix)/bin/$(target)-ld
		ln -s $(target)-ld.gold $(hostprefix)/bin/$(target)-ld
	@CLEANUP_gcc47@
	touch $@
