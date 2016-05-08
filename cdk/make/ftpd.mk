$(DEPDIR)/ftpd: bootstrap @DEPENDS_ftpd@
	-rm -rf $(targetprefix)/share/empty
	@PREPARE_ftpd@
if KERNEL26
	cd @DIR_ftpd@ && patch -p1 < $(buildprefix)/Patches/ftpd26.diff
endif
if CPUMODEL_405
	cd @DIR_ftpd@ && sed -i "s/823/405/" Makefile
endif
if DBOX2_GCC47
	cd @DIR_ftpd@ && \
		cp -v sysdeputil.c{,.orig} && \
		sed "s/#define VSF_SYSDEP_HAVE_UTMPX/#undef VSF_SYSDEP_HAVE_UTMPX/g" sysdeputil.c.orig > sysdeputil.c
endif
	cd @DIR_ftpd@ && \
		CC=$(target)-gcc \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		$(MAKE) && \
		@INSTALL_ftpd@
	@CLEANUP_ftpd@
	touch $@

# Remark: the install target in the Makefile in in.ftpd is not GNU-conformant,
# therefor the silly install command.
flash-ftpd: | $(flashprefix)/root @DEPENDS_ftpd@
	-rm -rf $(flashprefix)/root/share/empty
	@PREPARE_ftpd@
if KERNEL26
	cd @DIR_ftpd@ && patch -p1 < $(buildprefix)/Patches/ftpd26.diff
endif
if CPUMODEL_405
	cd @DIR_ftpd@ && sed -i "s/823/405/" Makefile
endif
if DBOX2_GCC47
	cd @DIR_ftpd@ && \
		cp -v sysdeputil.c{,.orig} && \
		sed "s/#define VSF_SYSDEP_HAVE_UTMPX/#undef VSF_SYSDEP_HAVE_UTMPX/g" sysdeputil.c.orig > sysdeputil.c
endif
	cd @DIR_ftpd@ && \
		CC=$(target)-gcc \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		$(MAKE) && \
		$(INSTALL) -m755 vsftpd $(flashprefix)/root/sbin/in.ftpd && \
		$(INSTALL) -m644 vsftpd-dbox2.conf $(flashprefix)/root/etc/vsftpd.conf && \
		$(INSTALL) -d $(flashprefix)/root/share/empty
	@CLEANUP_ftpd@
	@FLASHROOTDIR_MODIFIED@
	@TUXBOX_CUSTOMIZE@

.PHONY: flash-ftpd
