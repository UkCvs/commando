AC_INIT([tuxbox-cdk],[0])
AC_PREREQ(2.57a)

AC_CANONICAL_BUILD
AC_SUBST(build)

AC_ARG_WITH(toolchain,
	[  --with-toolchain=PATH   do not build a toolchain but use the one at PATH],
	[TOOLCHAIN_PATH="$withval"],[TOOLCHAIN_PATH="notset"])
AM_CONDITIONAL(USE_FOREIGN_TOOLCHAIN,test "$TOOLCHAIN_PATH" != "notset")
AC_SUBST(TOOLCHAIN_PATH)

AC_ARG_ENABLE(uclibc,
	AS_HELP_STRING(--enable-uclibc,	enable rules for creating uclibc linked targets),
	,[enable_uclibc=no])
AM_CONDITIONAL(ENABLE_UCLIBC,test "$enable_uclibc" = "yes")

TUXBOX_BOXTYPE

if test "$target_alias" = "" ; then
	if test "$enable_uclibc" = "yes"; then
		target_alias="powerpc-tuxbox-linux-uclibc"
	else
		target_alias="powerpc-tuxbox-linux-gnu"
	fi
fi

AC_CANONICAL_TARGET
if test "$target" = "sh4-unknown-linux-gnu" ; then
        target="sh4-linux"
fi
AC_SUBST(target)

# automake1.10 comes with verbose warning output, but we don't want them
AM_INIT_AUTOMAKE([-Wno-portability])
AM_MAINTAINER_MODE

AC_PROG_CC

AC_ARG_WITH(gcc4,
	[  --with-gcc4             dBox2-only compile image using gcc 4.1.2],
	[
		if test "$BOXTYPE" = "dbox2"; then
			dbox2_gcc4="yes"
		else
			dbox2_gcc4="no"
		fi
	],
	[dbox2_gcc4="no"])
AM_CONDITIONAL(DBOX2_GCC4,test "$dbox2_gcc4" = "yes")

AC_ARG_WITH(gcc47,
	[  --with-gcc47            dBox2-only compile image using gcc 4.7/kernel 2.6],
	[
		if test "$BOXTYPE" = "dbox2"; then
			dbox2_gcc47="yes"
			enable_kernel26="yes"
		else
			dbox2_gcc47="no"
		fi
	],
	[dbox2_gcc47="no"])
AM_CONDITIONAL(DBOX2_GCC47,test "$dbox2_gcc47" = "yes")

AC_MSG_CHECKING(kernel26)
AC_ARG_ENABLE(kernel26,
	AS_HELP_STRING(--enable-kernel26,set up the CDK to use the 2.6 kernel),
        [enable_kernel26=$enableval],
        [enable_kernel26="no"])

if test "$force_kernel26" = "yes"; then
	enable_kernel26="yes"
fi

AC_MSG_RESULT($enable_kernel26)

AM_CONDITIONAL(KERNEL26,test "$enable_kernel26" = "yes")
if test "$enable_kernel26" = "yes"; then
	KERNELVERSION="\$(VERSION_linux)"
else
	KERNELVERSION="\$(VERSION_linux24)"
fi

AC_ARG_ENABLE(nptl,
	AS_HELP_STRING(--disable-nptl,do not use native posix threads for glibc (only meaningful for kernel 2.6.x, default for 2.4.x)),
	,[
		if test "$enable_kernel26" = "yes"; then 
			enable_nptl=yes
		else
			enable_nptl=no
		fi
	])
if test "$enable_nptl" = "yes"; then
	if test "$enable_kernel26" != "yes"; then
		AC_MSG_ERROR([Option --enable-nptl (native posix threads) is only supported for a 2.6.x kernel])
	fi
	GLIBC_PTHREADS="nptl"
else
	GLIBC_PTHREADS="linuxthreads"
fi
AC_SUBST(GLIBC_PTHREADS)

AC_ARG_WITH(assume-kernelsources-old,
	[  --with-assume-kernelsources-old
                          Do not recompile due to new kernel sources],
	[assume_kernelsources_old="yes"],[assume_kernelsources_old="no"])
AM_CONDITIONAL(ASSUME_KERNELSOURCES_OLD,test "$assume_kernelsources_old" = "yes")

AC_ARG_ENABLE(debug,
	AS_HELP_STRING(--enable-debug,	enable debug code to be used with gdb),
	,[enable_debug=no])
AM_CONDITIONAL(ENABLE_DEBUG,test "$enable_debug" = "yes")

if test -z "$CFLAGS" ; then
	CFLAGS="-O2"
fi

if test -z "$TARGET_CFLAGS" -o -z "$TARGET_CXXFLAGS"; then
	TARGET_CFLAGS="-pipe"
	if test "$enable_debug" = "yes"; then
		TARGET_CFLAGS="$TARGET_CFLAGS -O2 -g3 -gdwarf-2"
		TARGET_LDFLAGS=""
	else
		case "${BOXTYPE}" in
			dreambox)
				TARGET_CFLAGS="$TARGET_CFLAGS -Os -mcpu=$CPU_MODEL -msoft-float -mmultiple -mstring -meabi -ggdb3"
				TARGET_LDFLAGS=""
				;;
			*)
				TARGET_CFLAGS="$TARGET_CFLAGS -Os"
				TARGET_LDFLAGS="-Wl,-O1"
				;;
		esac
	fi
	if test "$TOOLCHAIN_PATH" != "notset"; then
		TARGET_LDFLAGS="$TARGET_LDFLAGS -L\$(targetprefix)/lib"
		TARGET_CFLAGS="$TARGET_CFLAGS -I\$(targetprefix)/include -mcpu=$CPU_MODEL -msoft-float"
	fi
	TARGET_CXXFLAGS="$TARGET_CFLAGS"
fi

if test "$enable_uclibc" = "yes"; then
	CFLAGS="$CFLAGS -fPIC"
	TARGET_CFLAGS="$TARGET_CFLAGS -fPIC"
	TARGET_CXXFLAGS="$TARGET_CXXFLAGS -fPIC"
	LIBGETTEXT=libgettext
fi
AC_SUBST(LIBGETTEXT)

if test "$dbox2_gcc47" = "yes"; then
	TARGET_CFLAGS="$TARGET_CFLAGS -flto -fuse-linker-plugin"
	TARGET_CXXFLAGS="$TARGET_CXXFLAGS -flto -fuse-linker-plugin"
	TARGET_LDFLAGS="$TARGET_LDFLAGS -flto -fuse-linker-plugin"
fi

AC_ARG_ENABLE(german_keymaps,
	AS_HELP_STRING(--enable-german-keymaps,	include loadkey and German keymaps in yadds and images),
	[case "${enableval}" in
		yes) CONSOLE_TOOLS=kbd ;;
	esac],[enable_german_keymaps=no])
AM_CONDITIONAL(ENABLE_GERMAN_KEYMAPS,test "$enable_german_keymaps" = "yes")
AC_SUBST(CONSOLE_TOOLS)

# we need to use $enable_use_mmc instead of $enable_mmc because
# --disable-mmc would be ignored thanks to autoconf
AC_ARG_ENABLE(ide,
	AS_HELP_STRING(--enable-ide,	include ide/mmc and ext2/ext3 drivers in yadds and images),
	[case "${enableval}" in
		yes) enable_use_mmc=yes; DEFAULT_FS_FSTAB="ext3" ;;
		no)  enable_use_mmc=no ;;
	esac],[enable_ide=no; enable_use_mmc=no])

AC_ARG_ENABLE(ext3,
	AS_HELP_STRING(--disable-ext3,	exclude ext2/ext3 drivers in yadds and images),
	[case "${enableval}" in
		yes) enable_ext2=yes
		     enable_ext3=yes ;;
		no)  enable_ext2=no
		     DEFAULT_FS_FSTAB="" ;;
	esac],[if test "$enable_ide" = "yes"; then
		    enable_ext2=yes
		    enable_ext3=yes
		else
		    enable_ext2=no
		    enable_ext3=no
		fi])

AC_ARG_ENABLE(xfs,
	AS_HELP_STRING(--enable-xfs,	include xfs drivers in yadds and images),
	[case "${enableval}" in
		yes) [[ "$DEFAULT_FS_FSTAB" = "" ] && DEFAULT_FS_FSTAB="xfs" || /bin/true ] ;;
	esac],[enable_xfs=no])

AC_ARG_ENABLE(mmc,
	AS_HELP_STRING(--enable-mmc,	include mmc drivers in yadds and images - you need to activate a filesystem),
	[case "${enableval}" in
		yes)	enable_use_mmc=yes ;;
		no)	enable_use_mmc=no ;;
	esac],[])

# transfer current defaults to new configuration data
enable_fs_nfs=yes
enable_fs_cifs=yes
enable_fs_smbfs=no
enable_fs_lufs=yes
enable_vfat=no
enable_reiserfs=no

AC_ARG_WITH(filesystems,[  --with-filesystems=LIST comma seperated list of filesystems to be used,
                          first disk filesystem will be used as default for /hdd entry in /etc/fstab
                          allowed values: nfs (always enabled in yadd), cifs, smbfs,
                                          lufs (ftpfs), xfs (not kernel2.4/uClibc),
                                          ext2, ext3, extfs (ext2/3), vfat, reiserfs],[
if test x"${withval+set}" = xset; then
    enable_filesystems=`echo "${withval}" | sed -e 's/[[ 	,]][[ 	,]]*/,/g' -e 's/,$//' -e 's/,/ /g'`
    enable_fs_nfs=no
    enable_fs_cifs=no
    enable_fs_lufs=no
    enable_ext2=no
    enable_ext3=no
    enable_xfs=no
    enable_vfat=no
    enable_reiserfs=no
    DEFAULT_FS_FSTAB=""
    for i in $enable_filesystems; do
	case $i in
	    nfs)		enable_fs_nfs=yes ;;
	    cifs)		enable_fs_cifs=yes ;;
	    smbfs)		enable_fs_smbfs=yes ;;
	    lufs|ftpfs)		enable_fs_lufs=yes ;;
	    vfat)		enable_vfat=yes
#				activate IDE support if MMC is not enabled
				[[ "$enable_use_mmc" != "yes" ] && enable_ide=yes ]
				[[ "$DEFAULT_FS_FSTAB" = "" ] && DEFAULT_FS_FSTAB="vfat" || /bin/true ] ;;
	    xfs)		enable_xfs=yes
				[[ "$enable_use_mmc" != "yes" ] && enable_ide=yes ]
				[[ "$DEFAULT_FS_FSTAB" = "" ] && DEFAULT_FS_FSTAB="xfs" || /bin/true ] ;;
	    reiserfs)		enable_reiserfs=yes
				[[ "$enable_use_mmc" != "yes" ] && enable_ide=yes ]
				[[ "$DEFAULT_FS_FSTAB" = "" ] && DEFAULT_FS_FSTAB="reiserfs" || /bin/true ] ;;
	    extfs)		enable_ext2=yes; enable_ext3=yes
				[[ "$enable_use_mmc" != "yes" ] && enable_ide=yes ]
				[[ "$DEFAULT_FS_FSTAB" = "" ] && DEFAULT_FS_FSTAB="ext3" || /bin/true ] ;;
	    ext3)		enable_ext3=yes
				[[ "$enable_use_mmc" != "yes" ] && enable_ide=yes ]
				[[ "$DEFAULT_FS_FSTAB" = "" ] && DEFAULT_FS_FSTAB="ext3" || /bin/true ] ;;
	    ext2)		enable_ext2=yes
				[[ "$enable_use_mmc" != "yes" ] && enable_ide=yes ]
				[[ "$DEFAULT_FS_FSTAB" = "" ] && DEFAULT_FS_FSTAB="ext2" || /bin/true ] ;;
	    *)			AC_MSG_ERROR([bad value $i for --with-filesystems]) ;;
	esac
    done
fi],[])

# now we need to test some constellations which are not possible,
# i.e. IDE or MMC support without disc filesystems
if test "$enable_ide" = "no" -a "$enable_mmc" = "no"; then
	enable_ext2=no
	enable_ext3=no
	enable_xfs=no
	enable_reiserfs=no
fi

# we need to check vfat support seperately because it
# can be used either with HDD or MMC
if test "$enable_use_mmc" != "yes" -a "$enable_ide" != "yes" -a "$enable_vfat" = "yes"; then
	enable_vfat=no
fi

# if no disc filesystem is activated, deactivate IDE and MMC support
if ! test "$enable_ext2" = "yes" -o "$enable_ext3" = "yes" -o "$enable_xfs" = "yes" -o "$enable_reiserfs" = "yes" -o "$enable_vfat" = "yes"; then
	DEFAULT_FS_FSTAB=""
	enable_ide=no
	enable_use_mmc=no
	enable_ext2=no
	enable_ext3=no
	enable_xfs=no
	enable_reiserfs=no
	enable_vfat=no
	disable_hdd_fs=yes
else
	disable_hdd_fs=no
fi

AM_CONDITIONAL(ENABLE_FS_CIFS,test "$enable_fs_cifs" = "yes")
AM_CONDITIONAL(ENABLE_FS_SMBFS,test "$enable_fs_smbfs" = "yes")
AM_CONDITIONAL(ENABLE_FS_LUFS,test "$enable_fs_lufs" = "yes")
if test "$enable_fs_lufs" = "yes"; then
	LUFS=lufs
fi
AC_SUBST(LUFS)

AM_CONDITIONAL(ENABLE_VFAT,test "$enable_vfat" = "yes")
AM_CONDITIONAL(ENABLE_IDE,test "$enable_ide" = "yes")
AM_CONDITIONAL(ENABLE_MMC,test "$enable_use_mmc" = "yes")
AM_CONDITIONAL(ENABLE_IDEMMC,test "$enable_ide" = "yes" -o "$enable_use_mmc" = "yes")
AM_CONDITIONAL(ENABLE_EXT2,test "$enable_ext2" = "yes")
AM_CONDITIONAL(ENABLE_EXT3,test "$enable_ext3" = "yes")
AM_CONDITIONAL(ENABLE_EXTFS,test "$enable_ext2" = "yes" -o "$enable_ext3" = "yes")
AM_CONDITIONAL(ENABLE_XFS,test "$enable_xfs" = "yes")
AM_CONDITIONAL(ENABLE_REISERFS,test "$enable_reiserfs" = "yes")
AC_SUBST(DEFAULT_FS_FSTAB)

# enable all needed filesystem tools by default
enable_fs_fdisk=no
enable_fs_mount=no
enable_fs_e2fsprogs=no
enable_fs_xfsprogs=no
enable_fs_reiserfsprogs=no
enable_fs_dosfstools=no
if test "$enable_ext2" = "yes" -o "$enable_ext3" = "yes"; then
	enable_fs_e2fsprogs=yes
fi
if test "$enable_xfs" = "yes"; then
	enable_fs_xfsprogs=yes
fi
if test "$enable_reiserfs" = "yes"; then
	enable_fs_reiserfsprogs=yes
fi
if test "$enable_vfat" = "yes"; then
	enable_fs_dosfstools=yes
fi

AC_ARG_WITH(filesystem-tools,[  --with-filesystem-tools=LIST 
                          comma seperated list of filesystem tools to be used,
                          allowed values: e2fsprogs, reiserfsprogs, dosfstools, xfsprogs, fdisk/mount (standalone)],[
if test x"${withval+set}" = xset; then
    enable_filesystem_tools=`echo "${withval}" | sed -e 's/[[ 	,]][[ 	,]]*/,/g' -e 's/,$//' -e 's/,/ /g'`
    enable_fs_e2fsprogs=no
    enable_fs_xfsprogs=no
    enable_fs_reiserfsprogs=no
    enable_fs_dosfstools=no
    for i in $enable_filesystem_tools; do
	case $i in
	    e2fsprogs)		if test "$enable_ext2" = "yes" -o "$enable_ext3" = "yes"; then
				    enable_fs_e2fsprogs=yes
				fi ;;
	    xfsprogs)		if test "$enable_xfs" = "yes"; then
				    enable_fs_xfsprogs=yes
				fi ;;
	    reiserfsprogs)	if test "$enable_reiserfs" = "yes"; then
				    enable_fs_reiserfsprogs=yes
				fi ;;
	    dosfstools)		if test "$enable_vfat" = "yes"; then
				    enable_fs_dosfstools=yes
				fi ;;
	    fdisk)		if test "$enable_ide" = "yes" -o "$enable_mmc" = "yes"; then
				    enable_fs_fdisk=yes
				fi ;;
	    mount)		enable_fs_mount=yes
				;;
	    *)			AC_MSG_ERROR([bad value $i for --with-filesystem-tools]) ;;
	esac
    done
fi],[])

AM_CONDITIONAL(ENABLE_E2FSPROGS,test "$enable_fs_e2fsprogs" = "yes")
AM_CONDITIONAL(ENABLE_XFSPROGS,test "$enable_fs_xfsprogs" = "yes")
AM_CONDITIONAL(ENABLE_REISERFSPROGS,test "$enable_fs_reiserfsprogs" = "yes")
AM_CONDITIONAL(ENABLE_DOSFSTOOLS,test "$enable_fs_dosfstools" = "yes")
AM_CONDITIONAL(ENABLE_MOUNT_STANDALONE,test "$enable_fs_mount" = "yes")

# neutrino drive-gui setup
AC_ARG_ENABLE(drive-gui,
   AS_HELP_STRING(--enable-drive-gui,   enable neutrino gui-setup for ide hdd mmc administration - uses utillinux fdisk instead of busybox),
   [case "${enableval}" in
      yes) if test "$enable_ide" = "yes" -o "$enable_mmc" = "yes" ; then
		if test "$enable_kernel26" = "yes"; then 
			enable_drive_gui=no
		else
			enable_drive_gui=yes
		fi
	   else
	     enable_drive_gui=no
	   fi ;;
      *) enable_drive_gui=no
   esac],[if test "$enable_ide" = "yes" -o "$enable_mmc" = "yes"; then
	     	if test "$enable_kernel26" = "yes"; then 
			enable_drive_gui=no
		else
			enable_drive_gui=yes
		fi
	   else
	     enable_drive_gui=no
	   fi])
AM_CONDITIONAL(ENABLE_DRIVE_GUI,test "$enable_drive_gui" = "yes")

# force standalone fdisk when Neutrino drive GUI is enabled
if test "$enable_drive_gui" = "yes"; then
	enable_fs_fdisk=yes
fi
AM_CONDITIONAL(ENABLE_FDISK_STANDALONE,test "$enable_fs_fdisk" = "yes")

if test "$enable_uclibc" = "yes" -a "$enable_xfs" = "yes" -a "$enable_kernel26" != "yes"; then
	echo
	echo "******************************************************************************"
	echo "***** XFS does not work with uClibc/kernel 2.4 yet - use \"--disable-xfs\" *****"
	echo "******************************************************************************"
	echo
	exit 1
fi

AC_ARG_ENABLE(nfsserver,
	AS_HELP_STRING(--enable-nfsserver,	enable the dBox NFS server),
	[case "${enableval}" in
		yes) NFSSERVER=nfsserver; enable_fs_nfs=yes ;;
	esac],[enable_nfsserver=no])
AM_CONDITIONAL(ENABLE_NFSSERVER,test "$enable_nfsserver" = "yes")
AM_CONDITIONAL(ENABLE_FS_NFS,test "$enable_fs_nfs" = "yes")
AC_SUBST(NFSSERVER)

# mark if no network filesystem is activated
# please note that NFS is always activated for yadd images, but
# a deactivated automounter will not be reactivated for yadd
if ! test "$enable_fs_cifs" = "yes" -o "$enable_fs_smbfs" = "yes" -o "$enable_fs_lufs" = "yes" -o "$enable_fs_nfs" = "yes"; then
	disable_net_fs=yes
else
	disable_net_fs=no
fi

AC_ARG_ENABLE(automount,
	AS_HELP_STRING(--enable-automount,	enable automount daemon),
	[case "${enableval}" in
		yes) enable_automount=yes ;;
		no) enable_automount=no ;;
	esac],[enable_automount=yes])

AC_ARG_ENABLE(gui-mount,
	AS_HELP_STRING(--disable-gui-mount,disable GUI mount functionality),
	,[enable_gui_mount=yes])

# if no filesystem is active, deactivate mounting
if test "$disable_net_fs" = "yes" -a "$disable_hdd_fs" = "yes"; then
	enable_automount=no
	enable_gui_mount=no
fi

# enable GUI mount if at least one filesystem is used and automount is disabled
if test "$disable_net_fs" = "no" -o "$disable_hdd_fs" = "no"; then
	if test "$enable_automount" = "no"; then
		enable_gui_mount=yes
	fi
fi

# always enable GUI mount for lufs/ftpfs because automount can not be used with lufs
if test "$enable_fs_lufs" = "yes"; then
	enable_gui_mount=yes
fi

if test "$enable_automount" = "yes"; then
	AUTOMOUNT=automount
fi
AC_SUBST(AUTOMOUNT)
AM_CONDITIONAL(ENABLE_AUTOMOUNT,test "$enable_automount" = "yes")
AM_CONDITIONAL(ENABLE_GUI_MOUNT,test "$enable_gui_mount" = "yes")

AC_ARG_ENABLE(sambaserver,
	AS_HELP_STRING(--enable-sambaserver,    enable the dBox samba server),
	[case "${enableval}" in
		yes) SAMBASERVER=sambaserver ;;
	esac],[enable_sambaserver=no])
AM_CONDITIONAL(ENABLE_SAMBASERVER,test "$enable_sambaserver" = "yes")
AC_SUBST(SAMBASERVER)

AC_ARG_ENABLE(upnp,
	AS_HELP_STRING(--enable-upnp,include upnp support - depends on audioplayer),
	,[enable_upnp=no])

AC_ARG_ENABLE(flac,
	AS_HELP_STRING(--enable-flac,include Neutrino flac audio support  - depends on audioplayer),
	[case "${enableval}" in
		yes) FLACLIB=libFLAC ;;
	esac],[enable_flac=no])

AC_ARG_ENABLE(audioplayer,
	AS_HELP_STRING(--disable-audioplayer,include Neutrino audioplayer/internetradio),
	,[enable_audioplayer=yes])
if test "$enable_audioplayer" = "yes"; then
	NEUTRINO_AUDIOPLAYER_DEPS="libid3tag libmad libvorbisidec $FLACLIB"
else
	enable_upnp=no
	enable_flac=no
fi
AM_CONDITIONAL(ENABLE_UPNP,test "$enable_upnp" = "yes")
AM_CONDITIONAL(ENABLE_FLAC,test "$enable_flac" = "yes")
AM_CONDITIONAL(ENABLE_AUDIOPLAYER,test "$enable_audioplayer" = "yes")
AC_SUBST(NEUTRINO_AUDIOPLAYER_DEPS)

AC_ARG_ENABLE(pictureviewer,
	AS_HELP_STRING(--disable-pictureviewer,include Neutrino pictureviewer),
	,[enable_pictureviewer=yes])
AM_CONDITIONAL(ENABLE_PICTUREVIEWER,test "$enable_pictureviewer" = "yes")
if test "$enable_pictureviewer" = "yes"; then
	NEUTRINO_PICTUREVIEWER_DEPS="libjpeg libpng"
fi
AC_SUBST(NEUTRINO_PICTUREVIEWER_DEPS)

AC_ARG_ENABLE(movieplayer,
	AS_HELP_STRING(--disable-movieplayer,include Neutrino movieplayer),
	,[enable_movieplayer=yes])
AM_CONDITIONAL(ENABLE_MOVIEPLAYER,test "$enable_movieplayer" = "yes")

AC_ARG_ENABLE(radiotext,
	AS_HELP_STRING(--disable-radiotext,include Neutrino Radiotext support),
	,[enable_radiotext=yes])
AM_CONDITIONAL(ENABLE_RADIOTEXT,test "$enable_radiotext" = "yes")

AC_ARG_ENABLE(epgplus,
	AS_HELP_STRING(--disable-epgplus,include Neutrino EPG plus),
	,[enable_epgplus=yes])
AM_CONDITIONAL(ENABLE_EPGPLUS,test "$enable_epgplus" = "yes")

if test "$enable_uclibc" = "yes" && test "$enable_flac" = "yes"; then
	echo
	echo "*********************************************************************"
	echo "***** FLAC does not work with uClibc yet - use \"--disable-flac\" *****"
	echo "*********************************************************************"
	echo
	exit 1
fi

# choose set of tools/plugins to be part of flash images
AC_ARG_ENABLE(aformat,
	AS_HELP_STRING(--enable-aformat,enable aformat - dbox2-only),
	[case "${enableval}" in
		yes) 	if ! test "$BOXTYPE" = "dbox2"; then
				enable_aformat="boxtype not supported"
			fi ;;
	esac],[enable_aformat=no])
AM_CONDITIONAL(ENABLE_AFORMAT,test "$enable_aformat" = "yes")

AC_ARG_ENABLE(aio-grab,
	AS_HELP_STRING(--disable-aio-grab,disable aio-grab screenshot),
	,[enable_aio_grab=yes])
AM_CONDITIONAL(ENABLE_AIO_GRAB,test "$enable_aio_grab" = "yes")

AC_ARG_ENABLE(cdkVcInfo,
	AS_HELP_STRING(--enable-cdkVcInfo,include cdkVcInfo in yadds and images),
	,[enable_cdkVcInfo=no])
AM_CONDITIONAL(ENABLE_CDKVCINFO,test "$enable_cdkVcInfo" = "yes")

AC_ARG_ENABLE(blockads,
	AS_HELP_STRING(--enable-blockads,enable blockads),
	,[enable_blockads=no])
AM_CONDITIONAL(ENABLE_BLOCKADS,test "$enable_blockads" = "yes")

AC_ARG_ENABLE(clock,
	AS_HELP_STRING(--enable-clock,enable clock),
	,[enable_clock=no])
AM_CONDITIONAL(ENABLE_CLOCK,test "$enable_clock" = "yes")

AC_ARG_ENABLE(cron,
	AS_HELP_STRING(--enable-cron,enable cron),
	,[enable_cron=no])
AM_CONDITIONAL(ENABLE_CRON,test "$enable_cron" = "yes")

AC_ARG_ENABLE(dboxshot,
	AS_HELP_STRING(--enable-dboxshot,enable dboxshot),
	,[enable_dboxshot=no])
AM_CONDITIONAL(ENABLE_DBOXSHOT,test "$enable_dboxshot" = "yes")

AC_ARG_ENABLE(dropbear,
	AS_HELP_STRING(--enable-dropbear,enable dropbear),
	,[enable_dropbear=no])
AM_CONDITIONAL(ENABLE_DROPBEAR,test "$enable_dropbear" = "yes")

AC_ARG_ENABLE(dvbsnoop,
	AS_HELP_STRING(--enable-dvbsnoop,enable dvbsnoop),
	,[enable_dvbsnoop=no])
AM_CONDITIONAL(ENABLE_DVBSNOOP,test "$enable_dvbsnoop" = "yes")

AC_ARG_ENABLE(dvbsub,
	AS_HELP_STRING(--disable-dvbsub,disable dvbsub - Neutrino only),
	,[enable_dvbsub=yes])
AM_CONDITIONAL(ENABLE_DVBSUB,test "$enable_dvbsub" = "yes")

AC_ARG_ENABLE(eraseall,
	AS_HELP_STRING(--enable-eraseall,include eraseall in yadds and images),
	,[enable_eraseall=no])
AM_CONDITIONAL(ENABLE_ERASEALL,test "$enable_eraseall" = "yes")

AC_ARG_ENABLE(esd,
	AS_HELP_STRING(--enable-esd,enable esound),
	[case "${enableval}" in
		yes) ESOUND=esound ;;
	esac],[enable_esd=no])
AM_CONDITIONAL(ENABLE_ESD,test "$enable_esd" = "yes")
AC_SUBST(ESOUND)

AC_ARG_ENABLE(fbshot,
	AS_HELP_STRING(--enable-fbshot,enable fbshot),
	,[enable_fbshot=no])
AM_CONDITIONAL(ENABLE_FBSHOT,test "$enable_fbshot" = "yes")

AC_ARG_ENABLE(gdbserver,
	AS_HELP_STRING(--enable-gdbserver,enable gdbserver),
	,[enable_gdbserver=no])
AM_CONDITIONAL(ENABLE_GDBSERVER,test "$enable_gdbserver" = "yes")

AC_ARG_ENABLE(getrc,
	AS_HELP_STRING(--enable-getrc,enable getrc),
	,[enable_getrc=no])
AM_CONDITIONAL(ENABLE_GETRC,test "$enable_getrc" = "yes")

AC_ARG_ENABLE(hddtemp,
	AS_HELP_STRING(--enable-hddtemp,include hddtemp in yadds and images - depends on IDE support),
	[case "${enableval}" in
		yes) 	if ! test "$enable_ide" = "yes"; then
				enable_hddtemp="disabled due to missing IDE support"
			fi ;;
	esac],[enable_hddtemp=no])
AM_CONDITIONAL(ENABLE_HDDTEMP,test "$enable_hddtemp" = "yes")

AC_ARG_ENABLE(htop,
	AS_HELP_STRING(--enable-htop,enable htop),
	,[enable_htop=no])
AM_CONDITIONAL(ENABLE_HTOP,test "$enable_htop" = "yes")

AC_ARG_ENABLE(inadyn-mt,
	AS_HELP_STRING(--enable-inadyn-mt,enable inadyn-mt),
	,[enable_inadyn_mt=no])
AM_CONDITIONAL(ENABLE_INADYN_MT,test "$enable_inadyn_mt" = "yes")

AC_ARG_ENABLE(input,
	AS_HELP_STRING(--enable-input,enable the tool named input),
	,[enable_input=no])
AM_CONDITIONAL(ENABLE_INPUT,test "$enable_input" = "yes")

AC_ARG_ENABLE(ipkg,
	AS_HELP_STRING(--enable-ipkg,include ipkg in yadds and images),
	,[enable_ipkg=no])
AM_CONDITIONAL(ENABLE_IPKG,test "$enable_ipkg" = "yes")

AC_ARG_ENABLE(lcshot,
	AS_HELP_STRING(--enable-lcshot,enable lcshot - dbox2-only),
	[case "${enableval}" in
		yes) 	if ! test "$BOXTYPE" = "dbox2"; then
				enable_lcshot="boxtype not supported"
			fi ;;
	esac],[enable_lcshot=no])
AM_CONDITIONAL(ENABLE_LCSHOT,test "$enable_lcshot" = "yes")

AC_ARG_ENABLE(links,
	AS_HELP_STRING(--enable-links,include text mode links browser in yadds and images),
	,[enable_links=no])
AM_CONDITIONAL(ENABLE_LINKS,test "$enable_links" = "yes")

AC_ARG_ENABLE(links-g,
	AS_HELP_STRING(--enable-links-g,include graphic mode links browser in yadds and images),
	[case "${enableval}" in
		yes) 	enable_kb2rcd="yes" ;;
	esac],[enable_links_g=no])
AM_CONDITIONAL(ENABLE_LINKS_G,test "$enable_links_g" = "yes")

AC_ARG_ENABLE(kb2rcd,
	AS_HELP_STRING(--enable-kb2rcd,include kb2rcd in yadds and images),
	,[enable_kb2rcd=no])
AM_CONDITIONAL(ENABLE_KB2RCD,test "$enable_kb2rcd" = "yes")

AC_ARG_ENABLE(lirc,
	AS_HELP_STRING(--enable-lirc,include lirc in yadds and images - dbox2-only),
	[case "${enableval}" in
		yes) 	if ! test "$BOXTYPE" = "dbox2"; then
				enable_lirc="boxtype not supported"
			fi ;;
	esac],[enable_lirc=no])
AM_CONDITIONAL(ENABLE_LIRC,test "$enable_lirc" = "yes")

AC_ARG_ENABLE(logomask,
	AS_HELP_STRING(--enable-logomask,enable logomask),
	,[enable_logomask=no])
AM_CONDITIONAL(ENABLE_LOGOMASK,test "$enable_logomask" = "yes")

AC_ARG_ENABLE(msgbox,
	AS_HELP_STRING(--enable-msgbox,enable msgbox),
	,[enable_msgbox=no])
AM_CONDITIONAL(ENABLE_MSGBOX,test "$enable_msgbox" = "yes")

AC_ARG_ENABLE(netio,
	AS_HELP_STRING(--enable-netio,enable netio),
	,[enable_netio=no])
AM_CONDITIONAL(ENABLE_NETIO,test "$enable_netio" = "yes")

AC_ARG_ENABLE(openntpd,
	AS_HELP_STRING(--enable-openntpd,include OpenVPN in yadds and images),
	,[enable_openntpd=no])
AM_CONDITIONAL(ENABLE_OPENNTPD,test "$enable_openntpd" = "yes")

AC_ARG_ENABLE(openvpn,
	AS_HELP_STRING(--enable-openvpn,include OpenVPN in yadds and images and build tun kernel module),
	,[enable_openvpn=no])
AM_CONDITIONAL(ENABLE_OPENVPN,test "$enable_openvpn" = "yes")

AC_ARG_WITH(procps,
	AS_HELP_STRING(--with-procps,replace Busybox applets ps and top with procps versions),
	[enable_procps="$withval"],[enable_procps=no])
AM_CONDITIONAL(ENABLE_PROCPS,test "$enable_procps" = "yes")

AC_ARG_ENABLE(rtc,
	AS_HELP_STRING(--disable-rtc,disable rtc hardware support - dbox2-only),
	[case "${enableval}" in
		yes) 	if ! test "$BOXTYPE" = "dbox2"; then
				enable_rtc="boxtype not supported"
			fi ;;
	esac],[	if test "$BOXTYPE" = "dbox2"; then
			enable_rtc=yes
		else
			enable_rtc="boxtype not supported"
		fi])
AM_CONDITIONAL(ENABLE_RTC,test "$enable_rtc" = "yes")

AC_ARG_ENABLE(satfind,
	AS_HELP_STRING(--enable-satfind,enable satfind),
	,[enable_satfind=no])
AM_CONDITIONAL(ENABLE_SATFIND,test "$enable_satfind" = "yes")

AC_ARG_ENABLE(shellexec,
	AS_HELP_STRING(--enable-shellexec,enable shellexec),
	,[enable_shellexec=no])
AM_CONDITIONAL(ENABLE_SHELLEXEC,test "$enable_shellexec" = "yes")

AC_ARG_ENABLE(sqlite,
	AS_HELP_STRING(--enable-sqlite,enable sqlite),
	,[enable_sqlite=no])
AM_CONDITIONAL(ENABLE_SQLITE,test "$enable_sqlite" = "yes")

AC_ARG_ENABLE(strace,
	AS_HELP_STRING(--enable-strace,enable strace),
	,[enable_strace=no])
AM_CONDITIONAL(ENABLE_STRACE,test "$enable_strace" = "yes")

AC_ARG_ENABLE(sysinfo,
	AS_HELP_STRING(--enable-sysinfo,enable sysinfo),
	,[enable_sysinfo=no])
AM_CONDITIONAL(ENABLE_SYSINFO,test "$enable_sysinfo" = "yes")

AC_ARG_ENABLE(tuxcal,
	AS_HELP_STRING(--enable-tuxcal,enable tuxcal),
	,[enable_tuxcal=no])
AM_CONDITIONAL(ENABLE_TUXCAL,test "$enable_tuxcal" = "yes")

AC_ARG_ENABLE(tuxcom,
	AS_HELP_STRING(--disable-tuxcom,disable tuxcom),
	,[enable_tuxcom=yes])
AM_CONDITIONAL(ENABLE_TUXCOM,test "$enable_tuxcom" = "yes")

AC_ARG_ENABLE(tuxmail,
	AS_HELP_STRING(--disable-tuxmail,disable tuxmail),
	,[enable_tuxmail=yes])
AM_CONDITIONAL(ENABLE_TUXMAIL,test "$enable_tuxmail" = "yes")

AC_ARG_ENABLE(tuxtxt,
	AS_HELP_STRING(--disable-tuxtxt,disable tuxtxt completely),
	,[enable_tuxtxt=yes])

AC_ARG_ENABLE(internal-tuxtxt,
	AS_HELP_STRING(--disable-internal-tuxtxt,[disable internal Tuxtxt cache, use only external Tuxtxt plugin instead]),
	,[enable_internal_tuxtxt=yes])
if ! test "$enable_tuxtxt" = "yes"; then
	enable_internal_tuxtxt=no
fi
if test "$BOXTYPE" = "spark"; then
	enable_tuxtxt=no
	enable_internal_tuxtxt=no
fi
AM_CONDITIONAL(ENABLE_TUXTXT,test "$enable_tuxtxt" = "yes")
AM_CONDITIONAL(ENABLE_INTERNAL_TUXTXT,test "$enable_internal_tuxtxt" = "yes")

AC_ARG_ENABLE(tuxwetter,
	AS_HELP_STRING(--enable-tuxwetter,enable tuxwetter),
	,[enable_tuxwetter=no])
AM_CONDITIONAL(ENABLE_TUXWETTER,test "$enable_tuxwetter" = "yes")

AC_ARG_ENABLE(vncviewer,
	AS_HELP_STRING(--enable-vncviewer,enable vncviewer),
	,[enable_vncviewer=no])
AM_CONDITIONAL(ENABLE_VNCVIEWER,test "$enable_vncviewer" = "yes")

AC_ARG_WITH(etherwake,
	AS_HELP_STRING(--with-etherwake,replace Busybox applet ether-wake with original version),
	[enable_etherwake="$withval"],[enable_etherwake=no])
AM_CONDITIONAL(ENABLE_ETHERWAKE,test "$enable_etherwake" = "yes")

AC_ARG_WITH(wget,
	AS_HELP_STRING(--with-wget,replace Busybox applet wget with original version),
	[enable_wget="$withval"],[enable_wget=no])
AM_CONDITIONAL(ENABLE_WGET,test "$enable_wget" = "yes")

AC_ARG_WITH(ssl,
	AS_HELP_STRING(--with-ssl,build image with OpenSSL - use it for standalone wget & links),
	[enable_ssl="$withval"],[enable_ssl=no])
if test "$enable_tuxmail" = "yes"; then
	if test "$enable_ssl" = "no"; then
		summary_plugins="/ tuxmail"
	fi
	enable_ssl=yes
fi
AM_CONDITIONAL(ENABLE_SSL,test "$enable_ssl" = "yes")

AC_ARG_ENABLE(enigma-dbswitch,
	AS_HELP_STRING(--disable-enigma-dbswitch,disable Enigma dbswitch),
	,[enable_enigma_dbswitch=yes])
AM_CONDITIONAL(ENABLE_ENIGMA_DBSWITCH,test "$enable_enigma_dbswitch" = "yes")

AC_ARG_ENABLE(enigma-movieplayer,
	AS_HELP_STRING(--disable-enigma-movieplayer,disable Enigma movieplayer - needs expert webif),
	,[enable_enigma_movieplayer=yes])
AM_CONDITIONAL(ENABLE_ENIGMA_MOVIEPLAYER,test "$enable_enigma_movieplayer" = "yes")

AC_ARG_ENABLE(enigma-ngrab,
	AS_HELP_STRING(--disable-enigma-ngrab,disable Enigma ngrab),
	,[enable_enigma_ngrab=yes])
AM_CONDITIONAL(ENABLE_ENIGMA_NGRAB,test "$enable_enigma_ngrab" = "yes")

AC_ARG_ENABLE(enigma-rss,
	AS_HELP_STRING(--disable-enigma-rss,disable Enigma rss),
	,[enable_enigma_rss=yes])
AM_CONDITIONAL(ENABLE_ENIGMA_RSS,test "$enable_enigma_rss" = "yes")

AC_ARG_ENABLE(enigma-script,
	AS_HELP_STRING(--disable-enigma-script,disable Enigma script),
	,[enable_enigma_script=yes])
AM_CONDITIONAL(ENABLE_ENIGMA_SCRIPT,test "$enable_enigma_script" = "yes")

AC_ARG_ENABLE(enigma-weather,
	AS_HELP_STRING(--enable-enigma-weather,enable Enigma weather),
	,[enable_enigma_weather=no])
AM_CONDITIONAL(ENABLE_ENIGMA_WEATHER,test "$enable_enigma_weather" = "yes")

# games and fx2 stuff
FX2_DEPS=""

AC_ARG_ENABLE(fx2-bouquet,
	AS_HELP_STRING(--enable-fx2-bouquet,FX2: enable bouquet),
	[ FX2_DEPS="$FX2_DEPS fx2-bouquet"]
	,[enable_fx2_bouquet=no])
AM_CONDITIONAL(ENABLE_FX2_BOUQUET,test "$enable_fx2_bouquet" = "yes")

AC_ARG_ENABLE(fx2-c64emu,
	AS_HELP_STRING(--enable-fx2-c64emu,FX2: enable c64emu - dbox2-only),
	[case "${enableval}" in
		yes) 	if ! test "$BOXTYPE" = "dbox2"; then
				enable_fx2_c64emu="boxtype not supported"
			else
				[ FX2_DEPS="$FX2_DEPS fx2-c64emu"]
			fi ;;
	esac],[enable_fx2_c64emu=no])
AM_CONDITIONAL(ENABLE_FX2_C64EMU,test "$enable_fx2_c64emu" = "yes")

AC_ARG_ENABLE(fx2-lcdcirc,
	AS_HELP_STRING(--enable-fx2-lcdcirc,FX2: enable lcdcirc),
	[ FX2_DEPS="$FX2_DEPS fx2-lcdcirc"]
	,[enable_fx2_lcdcirc=no])
AM_CONDITIONAL(ENABLE_FX2_LCDCIRC,test "$enable_fx2_lcdcirc" = "yes")

AC_ARG_ENABLE(fx2-lemm,
	AS_HELP_STRING(--enable-fx2-lemm,FX2: enable lemmings),
	[ FX2_DEPS="$FX2_DEPS fx2-lemm"]
	,[enable_fx2_lemm=no])
AM_CONDITIONAL(ENABLE_FX2_LEMM,test "$enable_fx2_lemm" = "yes")

AC_ARG_ENABLE(fx2-master,
	AS_HELP_STRING(--enable-fx2-master,FX2: enable master),
	[ FX2_DEPS="$FX2_DEPS fx2-master"]
	,[enable_fx2_master=no])
AM_CONDITIONAL(ENABLE_FX2_MASTER,test "$enable_fx2_master" = "yes")

AC_ARG_ENABLE(fx2-mines,
	AS_HELP_STRING(--enable-fx2-mines,FX2: enable mines),
	[ FX2_DEPS="$FX2_DEPS fx2-mines"]
	,[enable_fx2_mines=no])
AM_CONDITIONAL(ENABLE_FX2_MINES,test "$enable_fx2_mines" = "yes")

AC_ARG_ENABLE(fx2-outdoor,
	AS_HELP_STRING(--enable-fx2-outdoor,FX2: enable outdoor - dbox2-only),
	[case "${enableval}" in
		yes) 	if ! test "$BOXTYPE" = "dbox2"; then
				enable_fx2_outdoor="boxtype not supported"
			else
				[ FX2_DEPS="$FX2_DEPS fx2-outdoor"]
			fi ;;
	esac],[enable_fx2_outdoor=no])
AM_CONDITIONAL(ENABLE_FX2_OUTDOOR,test "$enable_fx2_outdoor" = "yes")

AC_ARG_ENABLE(fx2-pac,
	AS_HELP_STRING(--enable-fx2-pac,FX2: enable pac),
	[ FX2_DEPS="$FX2_DEPS fx2-pac"]
	,[enable_fx2_pac=no])
AM_CONDITIONAL(ENABLE_FX2_PAC,test "$enable_fx2_pac" = "yes")

AC_ARG_ENABLE(fx2-satfind,
	AS_HELP_STRING(--enable-fx2-satfind,FX2: enable satfind),
	[ FX2_DEPS="$FX2_DEPS fx2-satfind"]
	,[enable_fx2_satfind=no])
AM_CONDITIONAL(ENABLE_FX2_SATFIND,test "$enable_fx2_satfind" = "yes")

AC_ARG_ENABLE(fx2-snake,
	AS_HELP_STRING(--enable-fx2-snake,FX2: enable snake),
	[ FX2_DEPS="$FX2_DEPS fx2-snake"]
	,[enable_fx2_snake=no])
AM_CONDITIONAL(ENABLE_FX2_SNAKE,test "$enable_fx2_snake" = "yes")

AC_ARG_ENABLE(fx2-sokoban,
	AS_HELP_STRING(--enable-fx2-sokoban,FX2: enable sokoban),
	[ FX2_DEPS="$FX2_DEPS fx2-sokoban"]
	,[enable_fx2_sokoban=no])
AM_CONDITIONAL(ENABLE_FX2_SOKOBAN,test "$enable_fx2_sokoban" = "yes")

AC_ARG_ENABLE(fx2-sol,
	AS_HELP_STRING(--enable-fx2-sol,FX2: enable sol),
	[ FX2_DEPS="$FX2_DEPS fx2-sol"]
	,[enable_fx2_sol=no])
AM_CONDITIONAL(ENABLE_FX2_SOL,test "$enable_fx2_sol" = "yes")

AC_ARG_ENABLE(fx2-solitair,
	AS_HELP_STRING(--enable-fx2-solitair,FX2: enable solitair),
	[ FX2_DEPS="$FX2_DEPS fx2-solitair"]
	,[enable_fx2_solitair=no])
AM_CONDITIONAL(ENABLE_FX2_SOLITAIR,test "$enable_fx2_solitair" = "yes")

AC_ARG_ENABLE(fx2-sudoku,
	AS_HELP_STRING(--enable-fx2-sudoku,FX2: enable sudoku - dbox2-only),
	[case "${enableval}" in
		yes) 	if ! test "$BOXTYPE" = "dbox2"; then
				enable_fx2_sudoku="boxtype not supported"
			else
				[ FX2_DEPS="$FX2_DEPS fx2-sudoku"]
			fi ;;
	esac],[enable_fx2_sudoku=no])
AM_CONDITIONAL(ENABLE_FX2_SUDOKU,test "$enable_fx2_sudoku" = "yes")

AC_ARG_ENABLE(fx2-tank,
	AS_HELP_STRING(--enable-fx2-tank,FX2: enable tank),
	[ FX2_DEPS="$FX2_DEPS fx2-tank"]
	,[enable_fx2_tank=no])
AM_CONDITIONAL(ENABLE_FX2_TANK,test "$enable_fx2_tank" = "yes")

AC_ARG_ENABLE(fx2-tetris,
	AS_HELP_STRING(--enable-fx2-tetris,FX2: enable tetris),
	[ FX2_DEPS="$FX2_DEPS fx2-tetris"]
	,[enable_fx2_tetris=no])
AM_CONDITIONAL(ENABLE_FX2_TETRIS,test "$enable_fx2_tetris" = "yes")

AC_ARG_ENABLE(fx2-vierg,
	AS_HELP_STRING(--enable-fx2-vierg,FX2: enable vierg),
	[ FX2_DEPS="$FX2_DEPS fx2-vierg"]
	,[enable_fx2_vierg=no])
AM_CONDITIONAL(ENABLE_FX2_VIERG,test "$enable_fx2_vierg" = "yes")

AC_ARG_ENABLE(fx2-yahtzee,
	AS_HELP_STRING(--enable-fx2-yahtzee,FX2: enable yahtzee),
	[ FX2_DEPS="$FX2_DEPS fx2-yahtzee"]
	,[enable_fx2_yahtzee=no])
AM_CONDITIONAL(ENABLE_FX2_YAHTZEE,test "$enable_fx2_yahtzee" = "yes")

AC_SUBST(FX2_DEPS)

AC_ARG_ENABLE(audioplay,
	AS_HELP_STRING(--enable-audioplay,enable tool audioplay),
	,[enable_audioplay=no])
AM_CONDITIONAL(ENABLE_AUDIOPLAY,test "$enable_audioplay" = "yes")

AC_ARG_ENABLE(aviaext,
	AS_HELP_STRING(--enable-aviaext,enable tool aviaext),
	,[enable_aviaext=no])
AM_CONDITIONAL(ENABLE_AVIAEXT,test "$enable_aviaext" = "yes")

AC_ARG_ENABLE(aviafbtool,
	AS_HELP_STRING(--enable-aviafbtool,enable tool aviafbtool),
	,[enable_aviafbtool=no])
AM_CONDITIONAL(ENABLE_AVIAFBTOOL,test "$enable_aviafbtool" = "yes")

AC_ARG_ENABLE(avswitch,
	AS_HELP_STRING(--enable-avswitch,enable tool avswitch),
	,[enable_avswitch=no])
AM_CONDITIONAL(ENABLE_AVSWITCH,test "$enable_avswitch" = "yes")

AC_ARG_ENABLE(fbclear,
	AS_HELP_STRING(--enable-fbclear,enable tool fbclear),
	,[enable_fbclear=no])
AM_CONDITIONAL(ENABLE_FBCLEAR,test "$enable_fbclear" = "yes")

AC_ARG_ENABLE(lcddump,
	AS_HELP_STRING(--enable-lcddump,enable tool lcddump),
	,[enable_lcddump=no])
AM_CONDITIONAL(ENABLE_LCDDUMP,test "$enable_lcddump" = "yes")

AC_ARG_ENABLE(pluginx,
	AS_HELP_STRING(--enable-pluginx,enable tool pluginx),
	,[enable_pluginx=no])
AM_CONDITIONAL(ENABLE_PLUGINX,test "$enable_pluginx" = "yes")

AC_ARG_ENABLE(rcinfo,
	AS_HELP_STRING(--enable-rcinfo,enable tool rcinfo),
	,[enable_rcinfo=no])
AM_CONDITIONAL(ENABLE_RCINFO,test "$enable_rcinfo" = "yes")

AC_ARG_ENABLE(rcsim,
	AS_HELP_STRING(--disable-rcsim,disable tool rcsim),
	,[enable_rcsim=yes])
AM_CONDITIONAL(ENABLE_RCSIM,test "$enable_rcsim" = "yes")

AC_ARG_ENABLE(saa,
	AS_HELP_STRING(--disable-saa,disable tool saa),
	,[enable_saa=yes])
AM_CONDITIONAL(ENABLE_SAA,test "$enable_saa" = "yes")

AC_ARG_ENABLE(showptsdiff,
	AS_HELP_STRING(--enable-showptsdiff,enable tool showptsdiff),
	,[enable_showptsdiff=no])
AM_CONDITIONAL(ENABLE_SHOWPTSDIFF,test "$enable_showptsdiff" = "yes")

AC_ARG_ENABLE(switch,
	AS_HELP_STRING(--disable-switch,disable tool switch),
	,[enable_switch=yes])
AM_CONDITIONAL(ENABLE_SWITCH,test "$enable_switch" = "yes")

AC_ARG_WITH(rootpartitionsize,
	[  --with-rootpartitionsize=SIZE
                          size of the root partition],
	[ROOT_PARTITION_SIZE="$withval"],[ROOT_PARTITION_SIZE="0x660000"])
AC_SUBST(ROOT_PARTITION_SIZE)

AC_ARG_WITH(flashfstype,
	[  --with-flashfstype=FS_TYPE
                          type of flash root filesystem partition for the yadd kernel (squashfs+lzma/squashfs)],
	[FLASH_FS_TYPE="$withval"],[FLASH_FS_TYPE="squashfs+lzma"])
AC_SUBST(FLASH_FS_TYPE)

AC_ARG_WITH(defaultlocale,
	[  --with-defaultlocale=LOCALE
                          default locale],
	[DEFAULTLOCALE="$withval"],[DEFAULTLOCALE="deutsch"])
AC_SUBST(DEFAULTLOCALE)

AC_SUBST(CFLAGS)
AC_SUBST(TARGET_CFLAGS)
AC_SUBST(TARGET_CXXFLAGS)
AC_SUBST(TARGET_LDFLAGS)

AC_PREFIX_DEFAULT(/dbox2)

if test "$prefix" = "NONE"; then
	prefix=/dbox2
fi

AC_ARG_WITH(targetprefix,
	[  --with-targetprefix=DIR prefix for target files [[PREFIX/cdkroot]]],
	[targetprefix="$with_targetprefix"],[targetprefix="${prefix}/cdkroot"])

AC_ARG_WITH(hostprefix,
	[  --with-hostprefix=DIR   prefix for host files [[PREFIX/cdk]]],
	[hostprefix="$with_hostprefix"],[hostprefix="${prefix}/cdk"])

AC_ARG_WITH(bootprefix,
	[  --with-bootprefix=DIR   prefix for boot files [[PREFIX/tftpboot]]],
	[bootprefix="$with_bootprefix"],[bootprefix="${prefix}/tftpboot"])

AC_ARG_WITH(flashprefix,
	[  --with-flashprefix=DIR  prefix for flash files [[PREFIX/cdkflash]] (only used for flash building)],
	[flashprefix="$with_flashprefix"],[flashprefix="${prefix}/cdkflash"])

AC_ARG_WITH(serversupport,
	[  --with-serversupport=DIR  
                          prefix for server file templates [[PREFIX/serversupport]]],
	[serversupport="$with_serversupport"],[serversupport="${prefix}/serversupport"])

AC_ARG_WITH(ucodesdir,
	[  --with-ucodesdir=DIR    optional directory containing ucodes (dbox2 only) [[NONE]]],
	[ucodesdir="$with_ucodesdir"],[ucodesdir="NONE"])

if test "$BOXTYPE" != "dbox2"; then
	ucodesdir="NONE"
fi

AC_ARG_WITH(logosdir,
	[  --with-logosdir=DIR     optional directory containing logos [[[CVS/]logos]]],
	[logosdir="$with_logosdir"],[logosdir="`pwd`/logos"])

AC_ARG_WITH(customizationsdir,
	[  --with-customizationsdir=DIR  
                          optional directory containing customization scripts [[[CVS]]]],
	[customizationsdir="$with_customizationsdir"],[customizationsdir="`pwd`"])

AC_ARG_WITH(updatehttpprefix,
	[  --with-updatehttpprefix=URL  
                          optional URL containing the URL of a directory with update images
                          "disable" completely disables internet update code [[NONE]]],
	[updatehttpprefix="$with_updatehttpprefix"],[updatehttpprefix=disable])

AC_ARG_WITH(busybox_conf,
	[  --with-busybox-conf=M4_FILE 
                          personalized optional m4 busybox configuration file [[config/busybox.config.m4]]],
	[busybox_conf="$with_busybox_conf"],[busybox_conf="config/busybox.config.m4"])
AC_SUBST(busybox_conf)

AC_ARG_WITH(kernel_conf,
	[  --with-kernel-conf=FILE optional personalized linux kernel config file [[config/dbox2_kernel-KERNEL_VERSION.config.m4]]],
	[kernel_conf="$with_kernel_conf"],[kernel_conf="NONE"])
AC_SUBST(kernel_conf)

buildprefix=`pwd`
AC_SUBST(targetprefix)
AC_SUBST(hostprefix)
AC_SUBST(buildprefix)
AC_SUBST(bootprefix)
AC_SUBST(flashprefix)
AC_SUBST(serversupport)
AC_SUBST(ucodesdir)
AC_SUBST(logosdir)
AC_SUBST(customizationsdir)
AC_SUBST(updatehttpprefix)
AM_CONDITIONAL(DISABLE_UPDATE_HTTP,test "$updatehttpprefix" = "disable")
flashprefix_ro=${flashprefix}/ro
AC_SUBST(flashprefix_ro)
flashprefix_rw=${flashprefix}/rw
AC_SUBST(flashprefix_rw)

AC_ARG_WITH(archivedir,
	[  --with-archivedir=DIR   where to store downloaded files [[Archive/]]],
	[archivedir="$with_archivedir"],[archivedir="$buildprefix/Archive"])

AC_ARG_WITH(cvsdir,
	[  --with-cvsdir=DIR       where to find the cvs],
	[cvsdir="$with_cvsdir"],[cvsdir="NONE"])

AC_ARG_WITH(appsdir,
	[  --with-appsdir=DIR      apps dir from cvs [[[CVS/]apps/]]],
	[appsdir="$with_appsdir"],[appsdir="NONE"])

AC_ARG_WITH(bootdir,
	[  --with-bootdir=DIR      boot dir from cvs [[[CVS/]boot/]]],
	[bootdir="$with_bootdir"],[bootdir="NONE"])

AC_ARG_WITH(driverdir,
	[  --with-driverdir=DIR    driver dir from cvs [[[CVS/]driver]]],
	[driverdir="$with_driverdir"],[driverdir="NONE"])

AC_ARG_WITH(hostappsdir,
	[  --with-hostappsdir=DIR  hostapps dir from cvs [[[CVS/]hostapps]]],
	[hostappsdir="$with_hostappsdir"],[hostappsdir="NONE"])

AC_ARG_WITH(gnuserver,
        [  --with-gnuserver=ADDRESS
                          the gnu server for gnu-stuff (without ftp://)],
        [gnuserver="$with_gnuserver"],[gnuserver=ftp.gwdg.de/pub/gnu/ftp/gnu])

AC_ARG_WITH(defaultserver,
        [  --with-defaultserver=ADDRESS
                          the server that is taken if no server is given/works (without http://)],
        [defaultserver="$with_defaultserver"],[defaultserver=www.i-have-a-dreambox.com/Archive])

if test "$cvsdir" = "NONE"; then
	if test "$appsdir" = "NONE"; then
		appsdir="$buildprefix/apps"
	fi
	if test "$bootdir" = "NONE"; then
		bootdir="$buildprefix/boot"
	fi
	if test "$driverdir" = "NONE"; then
		driverdir="$buildprefix/driver"
	fi
	if test "$hostappsdir" = "NONE"; then
		hostappsdir="$buildprefix/hostapps"
	fi
else
	if test "$appsdir" = "NONE"; then
		appsdir="$cvsdir/apps"
	fi
	if test "$bootdir" = "NONE"; then
		bootdir="$cvsdir/boot"
	fi
	if test "$driverdir" = "NONE"; then
		driverdir="$cvsdir/driver"
	fi
	if test "$hostappsdir" = "NONE"; then
		hostappsdir="$cvsdir/hostapps"
	fi
fi

case "${BOXTYPE}" in
	dreambox)
		startscriptdir="$cvsdir/cdk/root_dream"
		;;
	*)
		startscriptdir="$cvsdir/cdk/root"
		MKIMAGE=$hostprefix/bin/mkimage
		;;
esac
AC_SUBST(MKIMAGE)

dircheck="ok"
AC_MSG_CHECKING([for apps directory])
if test ! -f "$appsdir/dvb/zapit/configure.ac" ; then
	AC_MSG_WARN([appsdir $appsdir not found])
	appsdir="missing_appsir"
	dircheck="fail"
else
	AC_MSG_RESULT([yes])
fi

AC_MSG_CHECKING([for boot directory])
if test ! -d "$bootdir/u-boot-tuxbox" ; then
	AC_MSG_WARN([bootdir $bootdir not found])
	bootdir="missing_bootdir"
	dircheck="fail"
else
	AC_MSG_RESULT([yes])
fi

AC_MSG_CHECKING([for driver directory])
if test ! -f "$driverdir/Makefile" ; then
	AC_MSG_WARN([driverdir $driverdir not found])
	driverdir="missing_driverdir"
	dircheck="fail"
else
	AC_MSG_RESULT([yes])
fi

AC_MSG_CHECKING([for hostapps directory])
if test ! -f "$hostappsdir/configure.ac" ; then
	AC_MSG_WARN([hostappsdir $hostappsdir not found])
	hostappsdir="missing_hostappsdir"
	dircheck="fail"
else
	AC_MSG_RESULT([yes])
fi

AC_SUBST(appsdir)
AC_SUBST(bootdir)
AC_SUBST(driverdir)
AC_SUBST(hostappsdir)
AC_SUBST(startscriptdir)
AC_SUBST(gnuserver)
AC_SUBST(defaultserver)
AC_SUBST(archivedir)

AC_PATH_PROG(M4,m4,no)
if test "$M4" = "no"; then
	AC_MSG_WARN(No m4 found)
fi

AC_PROG_CXX
if test "$CXX" = "g++" -a "$GXX" != "yes"; then
	AC_MSG_ERROR(No g++ found)
fi

AC_PROG_LEX
if test "$LEX" != "flex"; then
	AC_MSG_ERROR(No flex found)
fi

AC_PROG_YACC
if test "$YACC" != "bison -y"; then
	AC_MSG_ERROR(No bison found)
fi

if test "$enable_uclibc" = "yes"; then
	MKLIBS="$hostappsdir/mklibs/mklibs_uclibc.py"
else
	MKLIBS="$hostappsdir/mklibs/mklibs.py"
fi
[[ -x "$MKLIBS" ] && true || chmod +x "$MKLIBS" ]
AC_SUBST(MKLIBS)

AC_PATH_PROG(MKCRAMFS,mkcramfs,no)
if test "$MKCRAMFS" = "no"; then
	AC_MSG_WARN(using tuxbox cramfs)
	MKCRAMFS=$hostprefix/bin/mkcramfs
fi

################################################################
# ccache
#
AC_ARG_ENABLE(ccache,
   AS_HELP_STRING(--enable-ccache,   enable ccache supported compiling ),
   [case "${enableval}" in
      yes) CCACHE=ccache ;;
   esac],[enable_ccache=no])
AM_CONDITIONAL(ENABLE_CCACHE,test "$enable_ccache" = "yes")
AC_SUBST(CCACHE)

AC_ARG_WITH(curlversion,
	[  --with-curlversion=old/current],
	[case "${withval}" in
		current)  with_curlversion=current ;;
		*)	  with_curlversion=old ;;
	esac],[with_curlversion=old])
AM_CONDITIONAL(CURL_CURRENT,test "$with_curlversion" = "current")

hostccachedir=$(dirname `which ccache` 2>/dev/null)
if  [[ -e $hostccachedir/ccache ]]; then
	ccachedir=$hostccachedir
	AM_CONDITIONAL(USE_CCACHEHOST, true)
else
	ccachedir=$hostprefix/bin
	AM_CONDITIONAL(USE_CCACHEHOST, false)
fi
AC_SUBST(ccachedir)

# optional ccache settings
AC_ARG_WITH(maxcachesize,
	[  --with-maxcachesize=SIZE 
                          maximal ccachesize for ccache ],
	[maxcachesize="$with_maxcachesize"],[maxcachesize="-1"])
AC_SUBST(maxcachesize)

AC_ARG_WITH(maxcachefiles,
	[  --with-maxcachefiles=COUNT 
                          maximal count of cachefiles for ccache ],
	[maxcachefiles="$with_maxcachefiles"],[maxcachefiles="-1"])
AC_SUBST(maxcachefiles)

if test "$enable_ccache" = "yes"; then
	if  [[ -e $ccachedir/ccache ]]; then
		if test $maxcachesize != -1; then
			$ccachedir/ccache -M $maxcachesize
		fi
		if test $maxcachefiles != -1; then
			$ccachedir/ccache -F $maxcachefiles
		fi
		ccacheinfo="
----------------------------------------
ccache installdir:			$ccachedir
$($ccachedir/ccache -s)
----------------------------------------"
	else
		ccacheinfo="
----------------------------------------
ccache installdir:		$ccachedir
ccache is not yet installed, but installation starts on first build of cdk environment or you can run make ccache
----------------------------------------"
	fi
fi
# UK FreeSat EPG #########################
AC_ARG_ENABLE(freesatepg,
	AS_HELP_STRING(--enable-freesatepg,enable UK FreeSat EPG),
	,[enable_freesatepg=no])
AM_CONDITIONAL(ENABLE_FREESATEPG,test "$enable_freesatepg" = "yes")
#
################################################################

# neutrino movieplayer2 (experimental) #########################
AC_ARG_ENABLE(movieplayer2,
	AS_HELP_STRING(--enable-movieplayer2,enable experimental neutrino movieplayer2),
	,[enable_movieplayer2=no])
AM_CONDITIONAL(ENABLE_MOVIEPLAYER2,test "$enable_movieplayer2" = "yes")

################################################################

# enigma specific stuff #########################
AC_ARG_WITH(webif,
	[  --with-webif=NAME       Enigma webif [[standard,expert]]],
	[WEBIF="$withval"],[WEBIF="standard"])

AC_SUBST(WEBIF)
AM_CONDITIONAL(WEBIF_STANDARD,test "$WEBIF" = "standard")
AM_CONDITIONAL(WEBIF_EXPERT,test "$WEBIF" = "expert")

AC_ARG_WITH(epg,
	[  --with-epg=NAME         dreambox epg [[standard,private]]],
	[EPG="$withval"],[EPG="standard"])

AC_SUBST(EPG)
AM_CONDITIONAL(EPG_STANDARD,test "$EPG" = "standard")
AM_CONDITIONAL(EPG_PRIVATE,test "$EPG" = "private")

AC_ARG_WITH(mhw-epg,
	[  --with-mhw-epg          enable capture of mhw epg (default off)],
	[MHW_EPG="$withval"],[MHW_EPG="no"])

AC_SUBST(MHW_EPG)
AM_CONDITIONAL(MHW_EPG_NO,test "$MHW_EPG" = "no")
AM_CONDITIONAL(MHW_EPG_YES,test "$MHW_EPG" = "yes")

AC_ARG_WITH(flashtool,
	[  --with-flashtool=NAME   dreambox flashtool [[standard,expert]]],
	[FLASHTOOL="$withval"],[FLASHTOOL="standard"])

AC_SUBST(FLASHTOOL)
AM_CONDITIONAL(FLASHTOOL_STANDARD,test "$FLASHTOOL" = "standard")
AM_CONDITIONAL(FLASHTOOL_EXPERT,test "$FLASHTOOL" = "expert")

AC_ARG_WITH(ext-flashtool,
	[  --with-ext-flashtool=NAME 
                          dreambox ext-flashtool [[yes,no]]],
	[EXTFLASHTOOL="$withval"],[EXTFLASHTOOL="yes"])

AC_ARG_WITH(enigma-debug,
	[  --with-enigma-debug=NAME 
                          dreambox enigma-debug [[yes,no]]],
	[ENIGMA_DEBUG="$withval"],[ENIGMA_DEBUG="no"])

AC_SUBST(ENIGMA_DEBUG)

#
################################################################

FONTDIR="/share/fonts"
AC_SUBST(FONTDIR)
UCODEDIR="/var/tuxbox/ucodes"
AC_SUBST(UCODEDIR)

CLEANUP="rm -rf"
AC_SUBST(CLEANUP)

DEPSCLEANUP="rm -f"
AC_SUBST(DEPSCLEANUP)

#
# core
#

case "${BOXTYPE}" in
	dreambox)
		TUXBOX_RULES_MAKE(linux_dream)
		TUXBOX_RULES_MAKE(linux_dream_kernel_headers)
		TUXBOX_RULES_MAKE(dreamfiles)
		TUXBOX_RULES_MAKE(dreamdriver_dm7000)
		TUXBOX_RULES_MAKE(dreamdriver_dm56x0)
		TUXBOX_RULES_MAKE(dreamdriver_dm500)
		TUXBOX_RULES_MAKE_EXDIR(binutils_dream)
		TUXBOX_RULES_MAKE_EXDIR(bootstrap_gcc_dream)
		TUXBOX_RULES_MAKE_EXDIR(glibc_dream)
		TUXBOX_RULES_MAKE_EXDIR(gcc_dream)
		;;
	ipbox)
		TUXBOX_RULES_MAKE(uboot_ipbox)
		TUXBOX_RULES_MAKE(linux_ipbox)
		TUXBOX_RULES_MAKE(linux_ipbox_kernel_headers)
		TUXBOX_RULES_MAKE(dgstationdriver_cubecafe)
		TUXBOX_RULES_MAKE(dgstationdriver_mutant200s)
		TUXBOX_RULES_MAKE(dgstationdriver_prime)
		TUXBOX_RULES_MAKE(dgstationdriver_relook400s)
		TUXBOX_RULES_MAKE_EXDIR(bootstrap_gcc)
		TUXBOX_RULES_MAKE_EXDIR(gcc)
		TUXBOX_RULES_MAKE_EXDIR(glibc)
		;;
	coolstream)
		TUXBOX_RULES_MAKE(linux_cool)
		TUXBOX_RULES_MAKE_EXDIR(bootstrap_gcc_static_cool)
		TUXBOX_RULES_MAKE_EXDIR(bootstrap_gcc_shared_cool)
		TUXBOX_RULES_MAKE_EXDIR(bootstrap_eglibc_cool)
		TUXBOX_RULES_MAKE_EXDIR(eglibc_cool)
		TUXBOX_RULES_MAKE_EXDIR(gcc_cool)
		;;
	spark)
		TUXBOX_RULES_MAKE_EXDIR(linux_spark)
		TUXBOX_RULES_MAKE_EXDIR(linux_spark_kernel_headers)
		TUXBOX_RULES_MAKE_EXDIR(binutils_spark)
		TUXBOX_RULES_MAKE_EXDIR(glibc_spark)
		TUXBOX_RULES_MAKE_EXDIR(gcc_spark)
		TUXBOX_RULES_MAKE(ffmpeg)
		TUXBOX_RULES_MAKE(opkg)
		TUXBOX_RULES_MAKE(opkg_host)
		;;
	*)
		TUXBOX_RULES_MAKE(linux)
		if test "$dbox2_gcc47" = "yes"; then
			TUXBOX_RULES_MAKE_EXDIR(bootstrap_gcc)
			TUXBOX_RULES_MAKE_EXDIR(bootstrap_gcc47_static)
			TUXBOX_RULES_MAKE_EXDIR(bootstrap_gcc47_shared)
			TUXBOX_RULES_MAKE_EXDIR(eglibc)
			TUXBOX_RULES_MAKE_EXDIR(gcc47)
			TUXBOX_RULES_MAKE(libppl_host)
		else
			if test "$dbox2_gcc4" = "yes"; then
				TUXBOX_RULES_MAKE_EXDIR(bootstrap_gcc41)
				TUXBOX_RULES_MAKE_EXDIR(gcc41)
			else
				TUXBOX_RULES_MAKE_EXDIR(bootstrap_gcc)
				TUXBOX_RULES_MAKE_EXDIR(gcc)
			fi
			TUXBOX_RULES_MAKE(linux24)
			TUXBOX_RULES_MAKE_EXDIR(glibc)
			TUXBOX_RULES_MAKE(uclibc)
		fi
		;;
esac

TUXBOX_RULES_MAKE(uboot)
TUXBOX_RULES_MAKE_EXDIR(binutils)

#
# root
#
TUXBOX_RULES_MAKE(busybox)
TUXBOX_RULES_MAKE(automount)
TUXBOX_RULES_MAKE(nfs_utils)
TUXBOX_RULES_MAKE(fuse)
if test "$enable_kernel26" != "yes"; then
TUXBOX_RULES_MAKE(fusekernel)
fi
TUXBOX_RULES_MAKE(djmount)
TUXBOX_RULES_MAKE(ftpd)
TUXBOX_RULES_MAKE(module_init_tools)
TUXBOX_RULES_MAKE(modutils)
TUXBOX_RULES_MAKE(portmap)
TUXBOX_RULES_MAKE(procps)
TUXBOX_RULES_MAKE(udev)
TUXBOX_RULES_MAKE(watchdog)
TUXBOX_RULES_MAKE(mrouted)
TUXBOX_RULES_MAKE(samba)
TUXBOX_RULES_MAKE(cramfs)
TUXBOX_RULES_MAKE(squashfs)
#
# contrib libs
#
TUXBOX_RULES_MAKE(libboost)
TUXBOX_RULES_MAKE(libcommoncplusplus)
TUXBOX_RULES_MAKE(libcrypto)
TUXBOX_RULES_MAKE(libcurl)
TUXBOX_RULES_MAKE(libcurl_current)
TUXBOX_RULES_MAKE(libdirectfb)
TUXBOX_RULES_MAKE(libdirectfbpp)
TUXBOX_RULES_MAKE(libppdirectfb)
TUXBOX_RULES_MAKE(libdvb)
TUXBOX_RULES_MAKE(libdvbpsi)
TUXBOX_RULES_MAKE(libexpat)
TUXBOX_RULES_MAKE(libffi)
TUXBOX_RULES_MAKE(libfreetype)
TUXBOX_RULES_MAKE(libfribidi)
TUXBOX_RULES_MAKE(libid3tag)
TUXBOX_RULES_MAKE(libgmp)
TUXBOX_RULES_MAKE(libmad)
TUXBOX_RULES_MAKE(libncurses)
TUXBOX_RULES_MAKE(libjpeg)
TUXBOX_RULES_MAKE(giflib)
TUXBOX_RULES_MAKE(libpcap)
TUXBOX_RULES_MAKE(libpng)
TUXBOX_RULES_MAKE(libreadline)
TUXBOX_RULES_MAKE(libsdl)
TUXBOX_RULES_MAKE(libsigc)
TUXBOX_RULES_MAKE(libvorbisidec)
TUXBOX_RULES_MAKE(libxml2)
TUXBOX_RULES_MAKE(libz)
TUXBOX_RULES_MAKE(libglib)
TUXBOX_RULES_MAKE(libiconv)
TUXBOX_RULES_MAKE(libtool)
TUXBOX_RULES_MAKE(libFLAC)
TUXBOX_RULES_MAKE(libgettext)
TUXBOX_RULES_MAKE(liblzma465)
TUXBOX_RULES_MAKE(libfaad2)
TUXBOX_RULES_MAKE(libgmp_host)
TUXBOX_RULES_MAKE(libmpfr_host)
TUXBOX_RULES_MAKE(libcloog_host)
TUXBOX_RULES_MAKE(libmpc_host)

#
# contrib apps
#
TUXBOX_RULES_MAKE(bzip2)
TUXBOX_RULES_MAKE(console_data)
TUXBOX_RULES_MAKE(console_tools)
TUXBOX_RULES_MAKE(kbd)
TUXBOX_RULES_MAKE(directfb_examples)
TUXBOX_RULES_MAKE(fbset)
TUXBOX_RULES_MAKE(lirc)
TUXBOX_RULES_MAKE(lsof)
TUXBOX_RULES_MAKE(ssh)
TUXBOX_RULES_MAKE(tcpdump)
TUXBOX_RULES_MAKE(bonnie)
TUXBOX_RULES_MAKE(vdr)
TUXBOX_RULES_MAKE(lufs)
TUXBOX_RULES_MAKE(dropbear)
TUXBOX_RULES_MAKE(kermit)
TUXBOX_RULES_MAKE(wget)
TUXBOX_RULES_MAKE(ncftp)
TUXBOX_RULES_MAKE(screen)
TUXBOX_RULES_MAKE(links)
TUXBOX_RULES_MAKE(links_g)
TUXBOX_RULES_MAKE(ntpd)
TUXBOX_RULES_MAKE(ntpclient)
TUXBOX_RULES_MAKE(openntpd)
TUXBOX_RULES_MAKE(esound)
TUXBOX_RULES_MAKE(python)
TUXBOX_RULES_MAKE(ser2net)
TUXBOX_RULES_MAKE(ucl)
TUXBOX_RULES_MAKE(upx_host)
TUXBOX_RULES_MAKE(lzma)
TUXBOX_RULES_MAKE(lzma_host)
TUXBOX_RULES_MAKE(openvpn)
TUXBOX_RULES_MAKE(ipkg)
TUXBOX_RULES_MAKE(htop)
TUXBOX_RULES_MAKE(netio)
TUXBOX_RULES_MAKE(netio_host)
TUXBOX_RULES_MAKE(inadyn_mt)

#
# ide apps
#
TUXBOX_RULES_MAKE(hdparm)
TUXBOX_RULES_MAKE(e2fsprogs)
TUXBOX_RULES_MAKE(utillinux)
TUXBOX_RULES_MAKE(parted)
TUXBOX_RULES_MAKE(hddtemp)
TUXBOX_RULES_MAKE(xfsprogs)
TUXBOX_RULES_MAKE(reiserfsprogs)
TUXBOX_RULES_MAKE(smartmontools)
TUXBOX_RULES_MAKE(dosfstools)

#
# ccache
#
TUXBOX_RULES_MAKE(ccache)

#
# msttf
#
TUXBOX_RULES_MAKE(cabextract)
TUXBOX_RULES_MAKE(msttf)

#
# additional
#
TUXBOX_RULES_MAKE(gdb)
TUXBOX_RULES_MAKE(valgrind)
TUXBOX_RULES_MAKE(insight)
TUXBOX_RULES_MAKE(ltrace)
TUXBOX_RULES_MAKE(strace)
TUXBOX_RULES_MAKE(nano)
TUXBOX_RULES_MAKE(mc)
TUXBOX_RULES_MAKE(joe)
TUXBOX_RULES_MAKE(bash)

#
# java
#
TUXBOX_RULES_MAKE(kaffe)
TUXBOX_RULES_MAKE(kaffeh)

#
# fun stuff
#
TUXBOX_RULES_MAKE(gnuboy)
TUXBOX_RULES_MAKE(scummvm)
TUXBOX_RULES_MAKE(sdldoom)
TUXBOX_RULES_MAKE(tinygl)
TUXBOX_RULES_MAKE(tuxfrodo)

#
# dvb apps
#
TUXBOX_RULES_MAKE(dvbdate)
TUXBOX_RULES_MAKE(dvbstream)
TUXBOX_RULES_MAKE(dvbtext)
TUXBOX_RULES_MAKE(dvbtune)
TUXBOX_RULES_MAKE(vls)

#
# bluetooth
#
TUXBOX_RULES_MAKE(bluez_hcidump)
TUXBOX_RULES_MAKE(bluez_libs)
TUXBOX_RULES_MAKE(bluez_pan)
TUXBOX_RULES_MAKE(bluez_sdp)
TUXBOX_RULES_MAKE(bluez_utils)

#
# Database
#
TUXBOX_RULES_MAKE(sqlite)

#
# internal
#
AC_SUBST_FILE(ARCHIVE)
ARCHIVE=Makefile-archive
AC_CONFIG_COMMANDS_PRE([
AC_MSG_NOTICE([creating Makefile-archive])
${srcdir}/rules-archive.pl ${srcdir}/rules-archive > Makefile-archive
])

#
# customized linux kernel configs
#

case "${BOXTYPE}" in
	ipbox)
		if test "$kernel_conf" = "NONE"; then
			kernel_conf="config/ipbox_kernel-2.6.17.config.m4"
		fi
		;;
	*)
		if test "$enable_kernel26" = "yes"; then
			if test "$kernel_conf" = "NONE"; then
				kernel_conf="config/dbox2_kernel-2.6.config.m4"
			fi
		else
			if test "$kernel_conf" = "NONE"; then
				kernel_conf="config/dbox2_kernel-2.4.config.m4"
			fi
		fi
		;;
esac

# hack
AC_ARG_ENABLE(dreambox-serial-console,
	AS_HELP_STRING(--enable-dreambox-serial-console, enable serial console on dream),
	,[enable_dreambox_serial_console=no])
AM_CONDITIONAL(DREAMBOX_ENABLE_SERIAL_CONSOLE,test "$enable_dreambox_serial_console" = "yes")

#
# summary
#

if test "$enable_kernel26" == "yes"; then
	used_kernelversion="2.6"
else
	used_kernelversion="2.4"
fi

if test "$enable_uclibc" == "yes"; then
	used_libc="uclibc"
else
	if test "$dbox2_gcc47" = "yes"; then
		used_gcc="/gcc 4.7"
		used_libc="eglibc"
	else
		if test "$dbox2_gcc4" = "yes"; then
    			used_gcc="/gcc 4.1.2"
		else
			used_gcc="/gcc 3.4.6"
		fi
		used_libc="glibc"
	fi
fi

if test "$BOXTYPE" = "coolstream"; then
	used_libc="eglibc"
fi

if test "$enable_fs_e2fsprogs" = "yes" -a "$enable_ext2" = "yes" ; then
	summary_e2fsprogs_ext2="/ e2fsprogs"
fi
if test "$enable_fs_e2fsprogs" = "yes" -a "$enable_ext3" = "yes" ; then
	summary_e2fsprogs_ext3="/ e2fsprogs"
fi
if test "$enable_fs_xfsprogs" = "yes"; then
	summary_xfsprogs="/ xfsprogs"
fi
if test "$enable_fs_reiserfsprogs" = "yes"; then
	summary_reiserfsprogs="/ reiserfsprogs"
fi
if test "$enable_fs_dosfstools" = "yes"; then
	summary_dosfstools="/ dosfstools"
fi

AC_MSG_RESULT([
summary:
----------------------------------------
target cpu/boxtype:		$CPU_ARCH$CPU_MODEL/$BOXTYPE $BOXMODEL
kernel:				$used_kernelversion
kernel config:			$kernel_conf
libc:				$used_libc$used_gcc
threads:			$GLIBC_PTHREADS
target:				$target
host:				$host
debug code:			$enable_debug
target prefix:			$targetprefix
host prefix:			$hostprefix
boot prefix:			$bootprefix
build prefix:			$buildprefix
root partition size:		$ROOT_PARTITION_SIZE
default locale:			$DEFAULTLOCALE
serversupport:			$serversupport
checkImage:			$CHECKIMAGE_BEHAVIOR
target apps directory:		$appsdir
boot directory:			$bootdir
start script directory:		$startscriptdir
driver directory:		$driverdir
host apps directory:		$hostappsdir
logo directory:			$logosdir
customization directory:	$customizationsdir
ucode directory:		$ucodesdir
archive directory:		$archivedir
GNU server:			$gnuserver
default server:			$defaultserver
update http prefix:		$updatehttpprefix
used busybox-m4-config:		$busybox_conf
IDE support:			$enable_ide
MMC support:			$enable_use_mmc
fdisk standalone:		$enable_fs_fdisk
mount standalone:		$enable_fs_mount
fstab default fs:		$DEFAULT_FS_FSTAB
IDE/MMC: Ext2 support		$enable_ext2 $summary_e2fsprogs_ext2
IDE/MMC: Ext3 support		$enable_ext3 $summary_e2fsprogs_ext3
IDE/MMC: XFS support		$enable_xfs $summary_xfsprogs
IDE/MMC: REISERFS support	$enable_reiserfs $summary_reiserfsprogs
IDE/MMC: VFAT support		$enable_vfat $summary_dosfstools
CIFS kernel module:		$enable_fs_cifs
SMBFS kernel module:		$enable_fs_smbfs
LUFS kernel module:		$enable_fs_lufs
NFS kernel module:		$enable_fs_nfs
NFS server:			$enable_nfsserver
Samba server:			$enable_sambaserver
Automount:			$enable_automount
Neutrino UPnP-support:		$enable_upnp
Neutrino Audioplayer:		$enable_audioplayer
Neutrino Movieplayer:		$enable_movieplayer
Neutrino Pictureviewer:		$enable_pictureviewer
Neutrino Mount:			$enable_gui_mount
Neutrino Drive-Setup:		$enable_drive_gui
Neutrino dvbsub			$enable_dvbsub
Neutrino EPG plus		$enable_epgplus
Radiotext support:		$enable_radiotext
FLAC support:			$enable_flac
CURL version:			$with_curlversion
OpenSSL:			$enable_ssl $summary_plugins
German keymaps:			$enable_german_keymaps

aformat				$enable_aformat
aio-grab			$enable_aio_grab
cdkVcInfo			$enable_cdkVcInfo
blockads			$enable_blockads
clock				$enable_clock
cron				$enable_cron
dboxshot			$enable_dboxshot
dropbear			$enable_dropbear
dvbsnoop			$enable_dvbsnoop
eraseall			$enable_eraseall
Esound support:			$enable_esd
ether-wake			$enable_etherwake
fbshot				$enable_fbshot
gdbserver			$enable_gdbserver
getrc				$enable_getrc
hddtemp				$enable_hddtemp
htop				$enable_htop
inadyn-mt			$enable_inadyn_mt
input				$enable_input
ipkg				$enable_ipkg
kb2rcd				$enable_kb2rcd
LIRC				$enable_lirc
lcshot				$enable_lcshot
links text mode browser		$enable_links
links graphics mode browser	$enable_links_g
logomask			$enable_logomask
msgbox				$enable_msgbox
netio				$enable_netio
openntpd			$enable_openntpd
OpenVPN:			$enable_openvpn
procps				$enable_procps
rtc				$enable_rtc
satfind				$enable_satfind
shellexec			$enable_shellexec
sqlite3 storage:		$enable_sqlite
strace				$enable_strace
sysinfo				$enable_sysinfo
tuxcal				$enable_tuxcal
tuxcom				$enable_tuxcom
tuxmail				$enable_tuxmail
tuxtxt (internal)		$enable_tuxtxt ($enable_internal_tuxtxt)
tuxwetter			$enable_tuxwetter
vncviewer			$enable_vncviewer
wget				$enable_wget

Enigma: dbswitch		$enable_enigma_dbswitch
Enigma: movieplayer		$enable_enigma_movieplayer
Enigma: ngrab			$enable_enigma_ngrab
Enigma: rss			$enable_enigma_rss
Enigma: script			$enable_enigma_script
Enigma: weather			$enable_enigma_weather

FX2: C64 Emulator		$enable_fx2_c64emu
FX2: lcdcirc			$enable_fx2_lcdcirc
FX2: Lemmings			$enable_fx2_lemm
FX2: Mastermind			$enable_fx2_master
FX2: MineSweeper		$enable_fx2_mines
FX2: Outdoor			$enable_fx2_outdoor
FX2: PacMan			$enable_fx2_pac
FX2: Satfind			$enable_fx2_satfind
FX2: Snake			$enable_fx2_snake
FX2: Sokoban			$enable_fx2_sokoban
FX2: Sol			$enable_fx2_sol
FX2: Solitair			$enable_fx2_solitair
FX2: Sudoko			$enable_fx2_sudoku
FX2: Tank			$enable_fx2_tank
FX2: Tetris			$enable_fx2_tetris
FX2: Vier gewinnt		$enable_fx2_vierg
FX2: Yahtzee			$enable_fx2_yahtzee

Tools: audioplay		$enable_audioplay
Tools: aviaext			$enable_aviaext
Tools: aviafbtool		$enable_aviafbtool
Tools: avswitch			$enable_avswitch
Tools: fbclear			$enable_fbclear
Tools: lcddump			$enable_lcddump
Tools: pluginx			$enable_pluginx
Tools: rcinfo			$enable_rcinfo
Tools: rcsim			$enable_rcsim
Tools: saa			$enable_saa
Tools: showptsdiff		$enable_showptsdiff
Tools: switch			$enable_switch

ccache support:			$enable_ccache
$ccacheinfo
])

if test "$dircheck" = "fail"; then
AC_MSG_RESULT([
******************************************************************
*                       !!!WARNING!!!                            *
* one or more of your CVS source directories could not be found, *
* (for details see above), did you maybe supply an incorrect     *
*  --with-cvsdir argument to configure?                          *
*                       !!!WARNING!!!                            *
******************************************************************
])
fi

## just error out now instead of mysteriously failing later.
if test -n "$CONFIG_SITE"; then
AC_MSG_ERROR([
***************************************************************
*                        !!!ERROR!!!                          *
* The environment variable CONFIG_SITE is set on your system. *
* This will likely break your build. Please rerun with unset  *
* CONFIG_SITE.                                                *
*                        !!!ERROR!!!                          *
***************************************************************
])
fi

TUXBOX_CUSTOMIZE='[[ -x $(customizationsdir)/$(notdir $@)-local.sh ] && KERNEL='$used_kernelversion' KERNELVERSION=$(KERNELVERSION) LIBC='$used_libc' BOXTYPE=$(BOXTYPE) BOXMODEL=$(BOXMODEL) $(customizationsdir)/$(notdir $@)-local.sh $(flashprefix) $(buildprefix) || true]'
AC_SUBST(TUXBOX_CUSTOMIZE)
TUXBOX_YADD_CUSTOMIZE='[[ -x $(customizationsdir)/$(notdir $@)-local.sh ] && KERNEL='$used_kernelversion' KERNELVERSION=$(KERNELVERSION) LIBC='$used_libc' BOXTYPE=$(BOXTYPE) BOXMODEL=$(BOXMODEL) $(customizationsdir)/$(notdir $@)-local.sh $(targetprefix) $(buildprefix) || true]'
AC_SUBST(TUXBOX_YADD_CUSTOMIZE)
FLASHROOTDIR_MODIFIED='touch $(flashprefix)/root'
AC_SUBST(FLASHROOTDIR_MODIFIED)


AC_CONFIG_FILES([
Makefile
rules-downcheck.pl
])

case "${BOXTYPE}" in
	dreambox)
	AC_CONFIG_FILES([
root_dream/Makefile
root_dream/bin/Makefile
root_dream/etc/Makefile
root_dream/etc/init.d/Makefile
root_dream/root/Makefile
root_dream/root/platform/Makefile
root_dream/root/platform/kernel/Makefile
root_dream/share/Makefile
root_dream/share/udhcpc/Makefile
root_dream/share/zoneinfo/Makefile
root_dream/var_init/Makefile
root_dream/var_init/bin/Makefile
root_dream/var_init/bin/emu/Makefile
root_dream/var_init/etc/Makefile
root_dream/var_init/etc/init.d/Makefile
root_dream/var_init/etc/network/Makefile
root_dream/var_init/httpd/Makefile
root_dream/var_init/keys/Makefile
root_dream/var_init/lib/Makefile
root_dream/var_init/spool/Makefile
root_dream/var_init/spool/cron/Makefile
root_dream/var_init/spool/cron/crontabs/Makefile
root_dream/var_init/tuxbox/Makefile
root_dream/var_init/tuxbox/config/Makefile
root_dream/var_init/tuxbox/config/enigma/Makefile
root_dream/var_init/tuxbox/config/zapit/Makefile
])
	;;
	*)
	AC_CONFIG_FILES([
root/Makefile
root/etc/Makefile
root/etc/init.d/Makefile
root/etc/network/Makefile
root/share/Makefile
root/share/udhcpc/Makefile
])
	;;
esac

AC_CONFIG_COMMANDS([rules-downcheck-chmod],[chmod +x rules-downcheck.pl])

AC_OUTPUT
