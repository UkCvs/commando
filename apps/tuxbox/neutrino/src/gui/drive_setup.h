/*
	$Id: drive_setup.h,v 1.40 2012/11/01 19:44:37 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	hdd setup implementation, fdisk frontend for Neutrino gui

	based upon ideas for the neutrino ide_setup by Innuendo and riker

	Copyright (C) 2009 Thilo Graf (dbt)
	http://www.dbox2-tuning.de

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	Special thx for hardware support by stingray www.dbox2.com and gurgel www.hallenberg.com !

*/

#ifndef __drive_setup__
#define __drive_setup__

#include <configfile.h>

#include <gui/widget/menue.h>
#include <gui/widget/dirchooser.h>
#include <gui/widget/stringinput.h>

#define ETC_DIR				"/etc"
#define VAR_ETC_DIR 			"/var/etc"

#ifdef ENABLE_SAMBASERVER
#include "gui/sambaserver_setup.h"

#define INIT_SAMBA_SCRIPT_NAME 		"31sambaserver"
#define INIT_SAMBA_SCRIPT_FILE 		INIT_D_DIR  "/"  INIT_SAMBA_SCRIPT_NAME
#define INIT_SAMBA_VAR_SCRIPT_FILE 	INIT_D_VAR_DIR "/" INIT_SAMBA_SCRIPT_NAME

#define SMBCONFDIR			ETC_DIR 
#define SMBCONFDIR_VAR			VAR_ETC_DIR 

#define SMBCONF 			SMBCONFDIR "/smb.conf"
#define SMBCONF_VAR 			SMBCONFDIR_VAR "/smb.conf"
#endif /*ENABLE_SAMBASERVER*/


#include <driver/framebuffer.h>
#include <system/settings.h>
#include <system/setting_helpers.h>

#include <string>
#include <vector>

// maximal count of usable devices
#define MAXCOUNT_DRIVE 3
// possible count of partitions per device
#define MAXCOUNT_PARTS 4
// maximal count of possible supported mmc driver modules
#define MAXCOUNT_MMC_MODULES 3
// possible supported fstypes for mkfs.X and fsck.x
#define MAXCOUNT_FSTYPES 7


// drive settings
struct SDriveSettings
{
	std::string 	drive_partition_mountpoint[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
	std::string 	drive_advanced_modul_command_load_options;
	std::string 	drive_modul_dir;
	std::string 	drive_mmc_modul_parameter[MAXCOUNT_MMC_MODULES];

	int 	drive_use_fstab;
	int	drive_use_fstab_auto_fs;
	int	drive_warn_jffs;

	int 	drive_activate_ide;
	int 	drive_write_cache[MAXCOUNT_DRIVE];
	int 	drive_partition_activ[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
	char	drive_partition_fstype[MAXCOUNT_DRIVE][MAXCOUNT_PARTS][9];	// "reiserfs\0"
	char 	drive_spindown[MAXCOUNT_DRIVE][5];	// "1200\0"
	char 	drive_partition_size[MAXCOUNT_DRIVE][MAXCOUNT_PARTS][8];
	char 	drive_mmc_module_name[10];
	char 	drive_fs_format_option[MAXCOUNT_FSTYPES][36];

#ifdef ENABLE_NFSSERVER
	std::string 	drive_partition_nfs_host_ip[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
	int 	drive_partition_nfs[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
#endif /*ENABLE_NFSSERVER*/

#ifdef ENABLE_SAMBASERVER
	std::string 	drive_partition_samba_share_name[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
	std::string 	drive_partition_samba_share_comment[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
	int 	drive_partition_samba[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
 	int 	drive_partition_samba_ro[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
 	int 	drive_partition_samba_public[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];
#endif /*ENABLE_SAMBASERVER*/

};


enum ON_OFF_NUM	
{
	OFF,
	ON
};
// switch on/off Option
#define OPTIONS_ON_OFF_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_ON_OFF_OPTIONS[OPTIONS_ON_OFF_OPTION_COUNT] =
{
	{ OFF, LOCALE_OPTIONS_OFF  },
	{ ON, LOCALE_OPTIONS_ON }
};


enum YES_NO_NUM	
{
	NO,
	YES
};
// switch enable/disable partition
#define OPTIONS_YES_NO_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_YES_NO_OPTIONS[OPTIONS_YES_NO_OPTION_COUNT] =
{
	{ NO, LOCALE_DRIVE_SETUP_PARTITION_ACTIVATE_NO  },
	{ YES, LOCALE_DRIVE_SETUP_PARTITION_ACTIVATE_YES }
};


// modes count for enum IDE_DRIVERMODES collection
enum IDE_DRIVERMODES
{
	IDE_OFF,
	IDE_ACTIVE,
	IDE_ACTIVE_IRQ6
};
// switch activate/deactivate ide interface
#define OPTIONS_IDE_ACTIVATE_OPTION_COUNT 3
const CMenuOptionChooser::keyval OPTIONS_IDE_ACTIVATE_OPTIONS[OPTIONS_IDE_ACTIVATE_OPTION_COUNT] =
{
	{ IDE_OFF, LOCALE_OPTIONS_OFF  },
	{ IDE_ACTIVE, LOCALE_DRIVE_SETUP_IDE_ACTIVATE_ON },
	{ IDE_ACTIVE_IRQ6, LOCALE_DRIVE_SETUP_IDE_ACTIVATE_IRQ6 }
};


class CDriveSetup : public CMenuTarget
{
	private:
		
		typedef enum 	
		{
			ADD,
			DELETE,
			DELETE_CLEAN
		} action_int_t;

		// set nums for commands collection, used in v_init_ide_L_cmds, this is also the order of commands in the init file
		// commands count for enum INIT_COMMANDS collection
		#define INIT_COMMANDS_COUNT 6
		typedef enum 	
		{
			LOAD_IDE_CORE,
			LOAD_DBOXIDE,
			LOAD_IDE_DETECT,
			LOAD_IDE_DISK,
			SET_MASTER_HDPARM_OPTIONS,
			SET_SLAVE_HDPARM_OPTIONS
		} INIT_COMMANDS;

		typedef enum 	
		{
			DEVICE,
			MOUNTPOINT,
			FS,
			OPTIONS
		} MTAB_INFO_NUM;
		
		#define SWAP_INFO_NUM_COUNT 5
		typedef enum 	
		{
			FILENAME,
			TYPE,
			SIZE,
			USED,
			PRIORITY
		} SWAP_INFO_NUM;

		typedef enum 	
		{
			START_CYL,
			END_CYL,
			SIZE_BLOCKS,
			ID,
			SIZE_CYL,
			COUNT_CYL,
			PART_SIZE
		} PARTINFO_TYPE_NUM;

		typedef enum   //column numbers	
		{
			FDISK_INFO_START_CYL	= 1,
			FDISK_INFO_END_CYL	= 2,
			FDISK_INFO_SIZE_BLOCKS	= 3,
			FDISK_INFO_ID		= 4
		} fdisk_info_uint_t;

		#define INIT_FILE_TYPE_NUM_COUNT 2
		typedef enum 	
		{
			INIT_FILE_MODULES,
			INIT_FILE_MOUNTS
		} INIT_FILE_TYPE_NUM;

		typedef enum   //mount types	
		{
			MOUNT_STAT_MOUNT_AND_SWAP	= 0,
			MOUNT_STAT_MOUNT_ONLY 		= 1,
			MOUNT_STAT_MOUNT_AND_SHARE 	= 2
		} mount_stat_uint_t;


		CFrameBuffer 			*frameBuffer;
		CConfigFile			configfile;
		SDriveSettings			d_settings;
		COnOffNotifier			*fstabNotifier;
		CDirChooser			*dirchooser_moduldir;
		CStringInputSMS 		*insmod_load_options;
		COnOffNotifier			*mmc_notifier;
		std::vector<CStringInputSMS*> 	v_input_fs_options, v_input_mmc_parameters;

		int x, y, width, height, hheight, mheight, selected_main;
		int pb_x, pb_y, pb_w, pb_h;
		int msg_timeout; 	// timeout for messages
		int exit_res;

		//stuff for settings handlers
		typedef struct settings_int_t
		{
			int old_val;
			int *p_val;
		};
		std::vector<settings_int_t> v_int_settings;
		
		typedef struct settings_string_t
		{
			std::string old_val;
			std::string *p_val;
		};
		std::vector<settings_string_t> v_string_settings;

		void 	handleSetting(int *setting);
		void 	handleSetting(std::string *setting);
		std::string old_drive_mmc_module_name;
		std::string old_drive_spindown[MAXCOUNT_DRIVE];
		std::string old_drive_fs_format_option[MAXCOUNT_FSTYPES];

		void 	restoreSettings();
		bool  	haveChangedSettings();
		bool	haveChangedMounts();

	
		const char* msg_icon; 	// icon for all hdd setup windows
		char part_num_actionkey[MAXCOUNT_PARTS][17];
		std::string make_part_actionkey[MAXCOUNT_PARTS]; //action key strings for make_partition_$
		std::string mount_partition[MAXCOUNT_PARTS]; //action key strings for mount_partition_$
		std::string unmount_partition[MAXCOUNT_PARTS]; //action key strings for unmount_partition_$
		std::string delete_partition[MAXCOUNT_PARTS]; //action key strings for delete_partition_$
		std::string check_partition[MAXCOUNT_PARTS]; //action key strings for check_partition_$
		std::string format_partition[MAXCOUNT_PARTS]; //action key strings for format_partition_$
		std::string sel_device_num_actionkey[MAXCOUNT_DRIVE]; //"sel_device_0 ... sel_device_n""
		std::string s_init_mmc_cmd; // system load command for mmc modul
		std::string mmc_modules[MAXCOUNT_MMC_MODULES]; //all supported mmc modules
		std::string moduldir[4]; //possible dirs of modules
		std::string k_name; //kernel name
		std::string partitions[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];

		//error messages
		#define ERROR_DESCRIPTIONS_NUM_COUNT 31 
		typedef enum 	
		{
			ERR_CHKFS,
			ERR_FORMAT_PARTITION,
			ERR_HDPARM,
			ERR_INIT_FSDRIVERS,
			ERR_INIT_IDEDRIVERS,
			ERR_INIT_MMCDRIVER,
			ERR_INIT_MODUL,
			ERR_LINK_INITFILES,
			ERR_LINK_SMBINITFILES,
			ERR_MK_PARTITION,
			ERR_MK_EXPORTS,
			ERR_MK_FS,
			ERR_MK_FSTAB,
			ERR_MK_MOUNTS,
			ERR_MK_SMBCONF,
			ERR_MK_SMBINITFILE,
			ERR_MOUNT_ALL,
			ERR_MOUNT_PARTITION,
			ERR_MOUNT_DEVICE,
			ERR_RESET,
			ERR_SAVE_DRIVE_SETUP,
			ERR_UNLINK_SMBINITLINKS,
			ERR_UNLOAD_FSDRIVERS,
			ERR_UNLOAD_IDEDRIVERS,
			ERR_UNLOAD_MMC_DRIVERS,
			ERR_UNLOAD_MODUL,
			ERR_UNMOUNT_ALL,
			ERR_UNMOUNT_PARTITION,
			ERR_UNMOUNT_DEVICE,
			ERR_WRITE_INITFILES,
			ERR_WRITE_SETTINGS
		} errnum_uint_t;
		std::string err[ERROR_DESCRIPTIONS_NUM_COUNT];
		bool have_apply_errors;
	
		bool have_fsdrivers;
		
		int current_device; 	//MASTER || SLAVE || MMCARD, current edit device
		int hdd_count; 		// count of hdd drives
 		int part_count[MAXCOUNT_DRIVE /*MASTER || SLAVE || MMCARD*/]; //count of partitions at device
		int next_part_number;// number of next free partition that can be added from device 1...4
		int count_Partitions; // needed for creating the proper sequence in scriptfile

		unsigned long long start_cylinder;
		unsigned long long end_cylinder;
		unsigned long long part_size;

		void setStartCylinder();

		bool device_isActive[MAXCOUNT_DRIVE /*MASTER || SLAVE || MMCARD*/];
		bool have_mmc_modul[MAXCOUNT_MMC_MODULES];	//collection of mmc modules state true=available false=not available 

		std::vector<std::string> v_mmc_modules;		//collection of available mmc modules
		std::vector<std::string> v_mmc_modules_opts;	//collection of available mmc module options, must be synchronized with v_mmc_modules !!!
		std::vector<std::string> v_model_name;		//collection of names of models
		std::vector<std::string> v_fs_modules;		//collection of available fs modules
		std::vector<std::string> v_init_ide_L_cmds; 	//collection of ide load commands
		std::vector<std::string> v_init_fs_L_cmds; 	//collection of fs load commands
		std::vector<std::string> v_init_fs_U_cmds; 	//collection of fs unload commands
		std::vector<std::string> v_device_temp;  	//collection of temperature of devices
		std::vector<std::string> v_hdparm_cmds;		//collection of available hdparm commands

		unsigned long long device_size[MAXCOUNT_DRIVE];		// contains sizes of all devices
		unsigned long long device_cylcount[MAXCOUNT_DRIVE]; 	// contains count of devices for all devices
		unsigned long long device_cyl_size[MAXCOUNT_DRIVE]; 	// contains bytes of one cylinder for all devices in bytes
		unsigned long long device_heads_count[MAXCOUNT_DRIVE];	 // contains count of heads
		unsigned long long device_sectors_count[MAXCOUNT_DRIVE]; // contains count of sectors

		const char *getFsTypeStr(long &fs_type_const);

		bool foundHdd(const std::string& mountpoint);
		bool foundMmc();
		bool loadFdiskPartTable(const int& device_num /*MASTER || SLAVE || MMCARD*/, bool use_extra = false /*using extra funtionality table*/);
		bool isActivePartition(const std::string& partname);
		bool isModulLoaded(const std::string& modulname);
		bool isMountedPartition(const std::string& partname);
		bool isSwapPartition(const std::string& partname);
		bool isUsedFsModul(const std::string& fs_name);
		bool initFsDrivers(bool do_unload_first = true);
		bool loadHddParams(const bool do_reset = false);
		bool initIdeDrivers(const bool irq6 = false);
		bool initModul(const std::string& modul_name, bool do_unload_first = true, const std::string& options = "");
		bool mountPartition(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number,  const std::string& fs_name, const std::string& mountpoint, const bool force_mount = true);
		bool mountDevice(const int& device_num, const bool force_mount = true);
		bool mountAll(const bool force_mount = false);
		bool unmountPartition(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number);
		bool unmountDevice(const int& device_num);
		bool unmountAll();
		bool saveHddSetup();
		bool unloadFsDrivers();
		bool unloadMmcDrivers();
		bool initMmcDriver();
		bool unloadIdeDrivers();
		bool unloadModul(const std::string& modulname);
		bool writeInitFile(const bool clear = false);
		bool mkMounts();
		bool mkFstab();
	#ifdef ENABLE_NFSSERVER
		bool mkExports();
	#endif
	#ifdef ENABLE_SAMBASERVER
		bool mkSambaInitFile();
		bool unlinkSmbInitLinks();
		bool linkSmbInitFiles();
		std::string getInitSmbFilePath();
		void setSambaMode();
	#endif
		bool haveSwap();
		bool haveMounts(const int& device_num, mount_stat_uint_t moun_stat = MOUNT_STAT_MOUNT_AND_SWAP);
		bool isMmcActive();
		bool isMmcEnabled();
		bool isIdeInterfaceActive();
		bool linkInitFiles();
		bool haveActiveParts(const int& device_num);
		bool Reset();
		bool ApplySetup(const bool show_msg = true);
		
		bool mkPartition(const int& device_num /*MASTER || SLAVE || MMCARD*/, const action_int_t& action, const int& part_number, const unsigned long long& start_cyl = 0, const unsigned long long& size = 0);
		bool mkFs(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number,  const std::string& fs_name);
		bool chkFs(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number,  const std::string& fs_name);
		bool formatPartition(const int& device_num, const int& part_number);
		void showStatus(const int& progress_val, const std::string& msg, const int& max = 5); // helper

		unsigned int getFirstUnusedPart(const int& device_num);
		unsigned long long getPartData(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number, const int& info_t_num /*START||END*/, bool refresh_table = true);
		unsigned long long getPartSize(const int& device_num /*MASTER || SLAVE || MMCARD*/, const int& part_number = -1);
	
		void Init();
		
		void loadHddCount();
		void loadHddModels();
		void loadFsModulList();
		void loadMmcModulList();
		void loadFdiskData();
		void loadDriveTemps();
		void loadModulDirs();
						
		void showHddSetupMain();
		void showHddSetupSub();
		void showHelp();
		void showExtMenu(CMenuWidget * extsettings);
		void showMMCParameterMenu(CMenuWidget* w_mmc);
		
		bool writeDriveSettings();
		void loadDriveSettings();

		void generateAllUsableDataOfDevice(const int& device_num /*MASTER || SLAVE || MMCARD*/);
		unsigned long long getFreeDiskspace(const char *mountpoint);
		unsigned long long getUnpartedDeviceSize(const int& device_num /*MASTER || SLAVE || MMCARD*/);
		unsigned long long getFileEntryLong(const char* filename, const std::string& filter_entry, const int& column_num);

		unsigned long long free_size[MAXCOUNT_DRIVE][MAXCOUNT_PARTS]; // contains free unused size of disc in MB
		unsigned long long calcCyl(const int& device_num /*MASTER || SLAVE || MMCARD*/, const unsigned long long& bytes);

		long long getDeviceInfo(const char *mountpoint/*/hdd...*/, const int& device_info /*KB_BLOCKS,
											KB_AVAILABLE,
											PERCENT_USED,
											PERCENT_FREE,
											FILESYSTEM...*/);
// 		examples: 	getFsTypeStr(getDeviceInfo("/hdd", FILESYSTEM));
// 				getDeviceInfo("/hdd", PERCENT_USED);

		
		std::string getMountInfo(const std::string& partname /*HDA1...HDB4*/, const int& mtab_info_num /*MTAB_INFO_NUM*/);
		std::string getSwapInfo(const std::string& partname /*HDA1...HDB4*/, const int& swap_info_num  /*SWAP_INFO_NUM*/);
		std::string getFileEntryString(const char* filename, const std::string& filter_entry, const int& column_num);
		std::string convertByteString(const unsigned long long& byte_size);
		std::string getUsedMmcModulName();
		std::string getFilePath(const char* main_file, const char* default_file);
		std::string getInitIdeFilePath();
		std::string getInitMountFilePath();
		std::string getFstabFilePath();
	#ifdef ENABLE_NFSSERVER
		std::string getExportsFilePath();
	#endif
		std::string getDefaultSysMounts();
		std::string getDefaultFstabEntries();
		std::string getTimeStamp();
		std::string getInitFileHeader(std::string & filename);
		std::string getInitFileMountEntries();
		std::string getInitFileModulEntries(bool with_unload_entries = false);
		std::string getInitModulLoadStr(const std::string& modul_name);
		std::string getPartEntryString(std::string& partname);
		std::string getMountPoint(const int& device_num, const int& part_number);

		//helper
		std::string iToString(int int_val);

		int exec(CMenuTarget* parent, const std::string & actionKey);
		
	//char options
	#ifdef ENABLE_NFSSERVER
		#define NFS_OPTS_COUNT 2 
	#else
		#define NFS_OPTS_COUNT 0 
	#endif /*ENABLE_NFSSERVER*/

	#ifdef ENABLE_SAMBASERVER
		#define SMB_OPTS_COUNT 5
	#else
		#define SMB_OPTS_COUNT 0  
	#endif /*ENABLE_SAMBASERVER*/

	#define CHAR_OPTIONS_NUM_COUNT 9 + NFS_OPTS_COUNT + SMB_OPTS_COUNT
	
		typedef enum 
		{
			OPT_SWAPSIZE,
			OPT_MMC_PARAMETER,
			OPT_MOUNTPOINT,
			OPT_SPINDOWN,
			OPT_PARTSIZE,
			OPT_FSTYPE,
			OPT_FS_FORMAT_OPTION,
			OPT_WRITECACHE,
			OPT_ACTIV_PARTITION
	#ifdef ENABLE_NFSSERVER
			,
			OPT_SHARE_FOR_NFS,
			OPT_SHARE_NFS_CLIENT_IP
	#endif /*ENABLE_NFSSERVER*/
	#ifdef ENABLE_SAMBASERVER
			,
			OPT_SHARE_FOR_SAMBA,
			OPT_SHARE_SAMBA_RO,
			OPT_SHARE_SAMBA_PUBLIC,
			OPT_SHARE_SAMBA_NAME,
			OPT_SHARE_SAMBA_COMMENT	
	#endif /*ENABLE_SAMBASERVER*/
		}c_option_uint_t;
		char c_opt[CHAR_OPTIONS_NUM_COUNT][64];

	public:
		enum DRIVE_NUM	
		{
			MASTER,
			SLAVE,
			MMCARD
		};
		
		enum MMC_NUM	
		{
			MMC,
			MMC2,
			MMCCOMBO
		};

	#define DEVICE_INFO_COUNT 5
		enum DEVICE_INFO	
		{
			KB_BLOCKS,
			KB_USED,
			KB_AVAILABLE,
			PERCENT_USED,
			PERCENT_FREE,
			FILESYSTEM,
			FREE_HOURS
		};
		
		enum PARTSIZE_TYPE_NUM	
		{
			KB,
			MB
		};
		
 		CDriveSetup();
		~CDriveSetup();

		static CDriveSetup* getInstance();

		std::string getHddTemp(const int& device_num /*MASTER || SLAVE || MMCARD*/); //hdd temperature
		std::string getModelName(const std::string& mountpoint);
		std::string getDriveSetupVersion();
		std::string getErrMsg();
	#ifdef ENABLE_SAMBASERVER
		std::string getSmbConfFilePath();
		bool haveMountedSmbShares();
		bool mkSmbConf();
	#endif

};

#endif
