
BUSYBOX_M4 = -D$(BOXTYPE)
BUSYBOX_M4 += -Dcustomizationsdir=$(customizationsdir)

if ENABLE_IDEMMC
BUSYBOX_M4 += -Dide
if !ENABLE_FDISK_STANDALONE
BUSYBOX_M4 += -Dfdisk
endif
endif
if !ENABLE_MOUNT_STANDALONE
BUSYBOX_M4 += -Dmount
endif
if ENABLE_EXTFS
BUSYBOX_M4 += -Dextfs
endif
if KERNEL26
BUSYBOX_M4 += -Dkernel26
endif
if ENABLE_FS_CIFS
BUSYBOX_M4 += -Dcifs
endif
# this option is not used for yadd builds, they need nfs for booting
if ENABLE_FS_NFS
BUSYBOX_M4 += -Dnfs
endif

if ENABLE_OPENVPN
BUSYBOX_M4 += -Dopenvpn
endif

if ENABLE_CRON
BUSYBOX_M4 += -Dcron
endif

# if standalone procps is not enabled, use the busybox applets instead
if !ENABLE_PROCPS
BUSYBOX_M4 += -Dprocps
endif

# if standalone ether-wake is not enabled, use the busybox applet instead
if !ENABLE_ETHERWAKE
BUSYBOX_M4 += -Detherwake
endif

# if standalone wget is not enabled, use the busybox applet instead
if !ENABLE_WGET
BUSYBOX_M4 += -Dwget
endif

if ENABLE_DEBUG
BUSYBOX_M4 += -Ddebug
endif

# this variable is needed because cdk/rules.pl assumes *.patch can be found in cdk/Patches
#BUSYBOX_PATCHES =

$(DEPDIR)/busybox: bootstrap @DEPENDS_busybox@ $(BUSYBOX_PATCHES) $(busybox_conf)
	@PREPARE_busybox@
if BOXTYPE_DREAMBOX
	cd @DIR_busybox@ && patch -p1 -E -i $(buildprefix)/Patches/busybox-dream.diff
endif
	m4 $(BUSYBOX_M4) -Dyadd -DPREFIX=\`\"$(targetprefix)\"\' $(busybox_conf) > @DIR_busybox@/.config
	cd @DIR_busybox@ && \
		LDFLAGS="$(TARGET_LDFLAGS) -fwhole-program" \
		$(MAKE) all \
			CROSS_COMPILE=$(target)- \
			CONFIG_EXTRA_CFLAGS="$(TARGET_CFLAGS)" && \
		$(MAKE) install \
			CROSS_COMPILE=$(target)- \
			CONFIG_EXTRA_CFLAGS="$(TARGET_CFLAGS)"
	@CLEANUP_busybox@
	touch $@

flash-busybox: bootstrap $(flashprefix)/root @DEPENDS_busybox@ $(BUSYBOX_PATCHES) $(busybox_conf)
	@PREPARE_busybox@
if BOXTYPE_DREAMBOX
	cd @DIR_busybox@ && patch -p1 -E -i $(buildprefix)/Patches/busybox-dream.diff
endif
	m4 $(BUSYBOX_M4) -Dflash -DPREFIX=\`\"$(flashprefix)/root\"\' $(busybox_conf) > @DIR_busybox@/.config
	cd @DIR_busybox@ && \
		LDFLAGS="$(TARGET_LDFLAGS) -fwhole-program" \
		$(MAKE) all \
			CROSS_COMPILE=$(target)- \
			CONFIG_EXTRA_CFLAGS="$(TARGET_CFLAGS)" && \
		$(MAKE) install \
			CROSS_COMPILE=$(target)- \
			CONFIG_EXTRA_CFLAGS="$(TARGET_CFLAGS)"
	@CLEANUP_busybox@
	@FLASHROOTDIR_MODIFIED@

.PHONY: flash-busybox
