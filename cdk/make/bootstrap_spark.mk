KERNEL_DEPENDS = @DEPENDS_linux_spark@
KERNEL_DIR = @SOURCEDIR_linux_spark@
KERNEL_PREPARE = @PREPARE_linux_spark@
KERNEL_BUILD_FILENAME = @SOURCEDIR_linux_spark@/arch/sh/boot/uImage

$(DEPDIR)/binutils: @DEPENDS_binutils_spark@ directories
	@PREPARE_binutils_spark@
	cp -Ppr @SOURCEDIR_binutils_spark@/opt/STM/STLinux-2.4/devkit/sh4/* $(hostprefix)/
	@CLEANUP_binutils_spark@
	touch $@

$(DEPDIR)/linux_spark_kernel_headers: @DEPENDS_linux_spark_kernel_headers@ binutils
	@PREPARE_linux_spark_kernel_headers@
	cp -Ppr @SOURCEDIR_linux_spark_kernel_headers@/opt/STM/STLinux-2.4/devkit/sh4/* $(targetprefix)/
	@CLEANUP_linux_spark_kernel_headers@
	touch $@

$(DEPDIR)/glibc: @DEPENDS_glibc_spark@ linux_spark_kernel_headers
	@PREPARE_glibc_spark@
	cp -Ppr @SOURCEDIR_glibc_spark@/opt/STM/STLinux-2.4/devkit/sh4/* $(targetprefix)/
	@INSTALL_glibc_spark@
	@CLEANUP_glibc_spark@
	touch $@

# dummy target
$(DEPDIR)/bootstrap_gcc:
	touch $@

$(DEPDIR)/gcc: @DEPENDS_gcc_spark@ glibc bootstrap_gcc
	@PREPARE_gcc_spark@
	cp -Ppr @SOURCEDIR_gcc_spark@/opt/STM/STLinux-2.4/devkit/sh4/* $(hostprefix)/
# finally link some stuff into the right places
	ln -s $(targetprefix)/target/lib $(targetprefix)/lib
	ln -s $(targetprefix)/target/usr/include $(targetprefix)/include
	ln -s $(targetprefix)/target/usr/lib/libstdc++.la $(targetprefix)/lib/libstdc++.la
	ln -s $(targetprefix)/target/usr/lib/libstdc++.so $(targetprefix)/lib/libstdc++.so
	ln -s $(targetprefix)/target $(hostprefix)/target
	@CLEANUP_gcc_spark@
	touch $@

LINUXKERNEL_PATCHES= \
$(archivedir)/linux-sh4-linuxdvb_stm24_0209.patch \
$(archivedir)/linux-sh4-sound_stm24_0209.patch \
$(archivedir)/linux-sh4-time_stm24_0209.patch \
$(archivedir)/linux-sh4-init_mm_stm24_0209.patch \
$(archivedir)/linux-sh4-copro_stm24_0209.patch \
$(archivedir)/bpa2_procfs_stm24_0209.patch \
$(archivedir)/linux-sh4-stmmac_stm24_0209.patch \
$(archivedir)/linux-sh4-lmb_stm24_0209.patch \
$(archivedir)/linux-sh4-spark_setup_stm24_0209.patch \
$(archivedir)/linux-sh4-linux_yaffs2_stm24_0209.patch \
$(archivedir)/0001-spark-fix-buffer-overflow-in-lirc_stm.patch

$(DEPDIR)/linuxdir: @DEPENDS_linux_spark@ $(LINUXKERNEL_PATCHES) $(archivedir)/kernel.config-spark bootstrap
	@PREPARE_linux_spark@
	tar -xf $(buildprefix)/@SOURCEDIR_linux_spark@/linux-2.6.32.tar.bz2 && \
	cd @SOURCEDIR_linux_spark@ && \
	bzcat linux-2.6.32.46.patch.bz2 | patch -p1 && \
	bzcat linux-2.6.32.46_stm24_sh4_0209.patch.bz2 | patch -p1 && \
	for i in $(LINUXKERNEL_PATCHES); do \
		patch -p1 -i $$i; \
	done && \
	cp -v $(archivedir)/kernel.config-spark .config && \
	make ARCH=sh oldconfig
	touch $@

FFMPEG_PATCHES= \
$(archivedir)/ffmpeg-0.6-avoid-UINT64_C.diff \
$(archivedir)/ffmpeg-0.10-remove-buildtime.diff

$(DEPDIR)/ffmpeg: @DEPENDS_ffmpeg@ $(FFMPEG_PATCHES) bootstrap
	@PREPARE_ffmpeg@
	cd @DIR_ffmpeg@ && \
		$(BUILDENV) \
		./configure \
			--disable-encoders \
                        --disable-muxers \
                        --disable-ffplay \
                        --disable-ffserver \
                        --arch=sh4 \
                        --enable-ffmpeg \
                        --enable-demuxers \
                        --enable-parser=mjpeg \
                        --enable-demuxer=mjpeg \
                        --enable-decoder=mjpeg \
                        --enable-encoder=mpeg2video \
                        --enable-muxer=mpeg2video \
                        --disable-bsfs \
                        --enable-decoder=dvbsub \
                        --enable-demuxer=mpegps \
                        --disable-devices \
                        --disable-mmx \
                        --disable-altivec \
                        --disable-iwmmxt \
                        --disable-protocols \
                        --enable-protocol=file \
                        --disable-zlib \
                        --enable-bzlib \
                        --disable-network \
                        --disable-ffprobe \
                        --disable-static \
                        --enable-shared \
                        --enable-cross-compile \
                        --cross-prefix=sh4-linux- \
                        --target-os=linux \
                        --enable-debug \
                        --enable-stripping \
                        --prefix= && \
                $(MAKE) && \
                @INSTALL_ffmpeg@
	@CLEANUP_ffmpeg@
	touch $@

$(DEPDIR)/opkg: @DEPENDS_opkg@ bootstrap
	@PREPARE_opkg@
	cd @DIR_opkg@ && \
		echo ac_cv_func_realloc_0_nonnull=yes >> config.cache && \
		$(BUILDENV) \
		./configure \
			--build=i686-pc-linux-gnu \
			--host=sh4-linux \
			-prefix= \
	                --disable-curl \
	                --disable-gpg \
	                --disable-shared \
	                --config-cache \
	                --with-opkglibdir=/var/lib && \
                make all exec_prefix= && \
	        @INSTALL_opkg@
	@CLEANUP_opkg@
	touch $@

$(DEPDIR)/opkg_host: @DEPENDS_opkg_host@ directories
	@PREPARE_opkg_host@
	cd @DIR_opkg_host@ && \
		./configure \
			-prefix= \
	                --disable-curl \
	                --disable-gpg \
	                --disable-shared \
	                --with-opkg_hostlibdir=/var/lib && \
                make all exec_prefix= && \
	        @INSTALL_opkg_host@
	@CLEANUP_opkg_host@
	touch $@
