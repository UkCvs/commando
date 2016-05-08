/*
	$Id: drive_setup.cpp,v 1.94 2012/11/01 19:44:37 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	drive setup implementation, fdisk frontend for Neutrino gui

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

NOTE: 	This is only beta. There is a lot to do

TODO:
	- cleanups


*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gui/drive_setup.h"
#include "gui/imageinfo.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/progressbar.h>
#include <gui/widget/helpbox.h>

#include <driver/screen_max.h>

#include <zapit/client/zapittools.h>

#include <system/debug.h>
#include <system/helper.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <ios>
#include <dirent.h>
#include <sys/vfs.h> // statfs
#include <sys/utsname.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/swap.h>
#include <errno.h>

// Paths of system and init files
#define INIT_D_DIR 			ETC_DIR "/init.d"
#define INIT_D_VAR_DIR 			VAR_ETC_DIR "/init.d"
#define INIT_IDE_SCRIPT_NAME 		"06hdd"
#define INIT_IDE_SCRIPT_PATH 		INIT_D_DIR "/" INIT_IDE_SCRIPT_NAME
#define INIT_IDE_VAR_SCRIPT_PATH 	INIT_D_VAR_DIR "/" INIT_IDE_SCRIPT_NAME
#define INIT_MOUNT_SCRIPT_NAME 		"07mounts"
#define INIT_MOUNT_SCRIPT_FILE 		INIT_D_DIR  "/"  INIT_MOUNT_SCRIPT_NAME
#define INIT_MOUNT_VAR_SCRIPT_FILE 	INIT_D_VAR_DIR "/" INIT_MOUNT_SCRIPT_NAME

#ifdef ENABLE_NFSSERVER
#define EXPORTS 			ETC_DIR "/exports"
#define EXPORTS_VAR 			VAR_ETC_DIR "/exports"
#endif /*ENABLE_NFSSERVER*/

#define TEMP_SCRIPT			"/tmp/drive_setup"
#define PREPARE_SCRIPT_FILE		"/tmp/prepare_opt"
#define PART_TABLE			"/tmp/part_table"
#define DRV_CONFIGFILE			CONFIGDIR "/drivesetup.conf"

#define PROC 				"/proc"
#define PROC_MODULES 			PROC "/modules"
#define PROC_PARTITIONS			PROC "/partitions"
#define PROC_MOUNTS			PROC "/self/mounts"
#define PROC_SWAPS 			PROC "/swaps"
#define FSTAB				ETC_DIR "/fstab"
#define FSTAB_VAR			VAR_ETC_DIR "/fstab"

// system commands
#define LOAD		"insmod "
#define UNLOAD		"rmmod "
#define SWAPON		"swapon "
#define SWAPOFF		"swapoff "
#define MKSWAP		"mkswap "
#define MKFSPREFIX	"mkfs."
#define CKFSPREFIX	"fsck."
#define DISCTOOL	"fdisk "
#define HDDTEMP		"hddtemp "
#define MOUNT		"mount "
#define UMOUNT		"umount "
#define HDPARM		"hdparm "

#define DEVNULL 	" 2>/dev/null "

// modul type
#define M_TYPE		".o"
// module names
#define DBOXIDE 	"dboxide"
#define IDE_DETECT 	"ide-detect"
#define IDE_CORE 	"ide-core"
#define IDE_DISK 	"ide-disk"
#define M_MMC		"mmc"
#define M_MMC2		"mmc2"
#define M_MMCCOMBO	"mmccombo"

// default modul dirs
#define MOUDULDIR	"/lib/modules"
#define VAR_MOUDULDIR	"/var/lib/modules"

// proc devices
#define IDE0HDA		"/proc/ide/ide0/hda"
#define IDE0HDB		"/proc/ide/ide0/hdb"
#define MMC0DISC	""

// devices
#define HDA 		"/dev/ide/host0/bus0/target0/lun0/disc"
#define HDB 		"/dev/ide/host0/bus0/target1/lun0/disc"
#define MMCA 		"/dev/mmc/disc0/disc"

// partitions templates
#define HDA_PARTS 	"/dev/ide/host0/bus0/target0/lun0/part"
#define HDB_PARTS 	"/dev/ide/host0/bus0/target1/lun0/part"
#define MMC_PARTS 	"/dev/mmc/disc0/part"

#define NO_REFRESH	0 /*false*/

// actionkey patterns
#define MAKE_PARTITION 		"make_partition_"
#define MOUNT_PARTITION 	"mount_partition_"
#define UNMOUNT_PARTITION 	"unmount_partition_"
#define DELETE_PARTITION 	"delete_partition_"
#define CHECK_PARTITION 	"check_partition_"
#define FORMAT_PARTITION 	"format_partition_"


using namespace std;

typedef struct drives_t
{
	const string device;
	const string proc_device;
} drives_struct_t;

const drives_struct_t drives[MAXCOUNT_DRIVE] =
{
	{HDA, IDE0HDA},		//MASTER
	{HDB, IDE0HDB},		//SLAVE
 	{MMCA, MMC0DISC}, 	//MMCARD
};

typedef struct data_cyl_t
{
	unsigned long long start_cyl;
	unsigned long long end_cyl;
	unsigned long long used_size;
	unsigned long long free_size;
};

struct data_cyl_t data_partition[MAXCOUNT_DRIVE][MAXCOUNT_PARTS] = 
{
	{ { 0, 0, 0, 0 } }
};

CDriveSetup::CDriveSetup():configfile('\t')
{
	frameBuffer 		= CFrameBuffer::getInstance();
	fstabNotifier 		= NULL;
	dirchooser_moduldir 	= NULL;
	insmod_load_options 	= NULL;
	mmc_notifier		= NULL;
	for (size_t i=0; i<MAXCOUNT_FSTYPES; i++)
		v_input_fs_options.push_back(NULL);
	for (size_t i=0; i<MAXCOUNT_MMC_MODULES; i++)
		v_input_mmc_parameters.push_back(NULL);
	

	width 	= w_max (600, 50);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height 	= h_max (hheight+13*mheight+10, 0);
	selected_main = -1;
	x	= getScreenStartX (width);
	y	= getScreenStartY (height);

	pb_w = width/2;
	pb_h = 50;
	pb_x = x+pb_w/2;
	pb_y = y+height/2-pb_h/2;

	msg_timeout 	= g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR];
	msg_icon 	= NEUTRINO_ICON_PARTITION;

	//generate action key strings for device selection
	for (int i = 0; i < MAXCOUNT_DRIVE; i++)
	{
		string s_i = iToString(i);
		sel_device_num_actionkey[i] = "sel_device_" + s_i;
	}

	//generate action key strings for partition operations
	for (int i = 0; i<MAXCOUNT_PARTS; i++)
	{ 
		make_part_actionkey[i]		= MAKE_PARTITION + iToString(i);
		mount_partition[i] 		= MOUNT_PARTITION + iToString(i);
		unmount_partition[i] 		= UNMOUNT_PARTITION + iToString(i);
		delete_partition[i]		= DELETE_PARTITION + iToString(i);
		check_partition[i]		= CHECK_PARTITION + iToString(i);
		format_partition[i]		= FORMAT_PARTITION + iToString(i);
		
		sprintf(part_num_actionkey[i], "edit_partition_%d", i);
	}
	
	//add possible supported mmc modules
	mmc_modules[MMC] 	= M_MMC;
	mmc_modules[MMC2] 	= M_MMC2;
	mmc_modules[MMCCOMBO] 	= M_MMCCOMBO;

	//kernel info
	struct utsname u;
	if (!uname(&u))
		k_name = u.release;

	//partition names
	string part_pattern[MAXCOUNT_DRIVE] ={	HDA_PARTS,	//MASTER
						HDB_PARTS,	//SLAVE
 						MMC_PARTS};	//MMCARD
	for (uint i=0; i < MAXCOUNT_DRIVE; i++) 
	{
		for (uint j=0; j < MAXCOUNT_PARTS; j++) 
		{
			partitions[i][j] = part_pattern[i] + iToString(j+1);
		}
	}

	have_apply_errors = false;
	exit_res = menu_return::RETURN_REPAINT;
}

CDriveSetup* CDriveSetup::getInstance()
{
	static CDriveSetup* drivesetup = NULL;

	if(!drivesetup)
	{
		drivesetup = new CDriveSetup();
		printf("[drive_setup] Instance created\n");
	}

	return drivesetup;
}

CDriveSetup::~CDriveSetup()
{
	delete fstabNotifier;
	delete dirchooser_moduldir;
	delete insmod_load_options;
	delete mmc_notifier;
	for (size_t i=0; i<v_input_fs_options.size(); i++)
		delete v_input_fs_options[i];
	for (size_t i=0; i<v_input_mmc_parameters.size(); i++)
		delete v_input_mmc_parameters[i];
}

int CDriveSetup::exec(CMenuTarget* parent, const string &actionKey)
{	
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	if (actionKey=="apply")
	{
		if (ApplySetup(NO/*no message*/))
		{
			Init();
			return menu_return::RETURN_EXIT;
		}

		return res;
	}
	else if (actionKey == "mount_device_partitions")
 	{

		if (!mountDevice(current_device)) 
		{
			DisplayErrorMessage(err[ERR_MOUNT_DEVICE].c_str());
			return res;
		}
		showHddSetupSub();
		return menu_return::RETURN_EXIT;
 	}
	else if (actionKey == "unmount_device_partitions")
 	{
		if (!unmountDevice(current_device)) 
		{
			DisplayErrorMessage(err[ERR_UNMOUNT_DEVICE].c_str());
			return res;
		}
			showHddSetupSub();
			return menu_return::RETURN_EXIT;
 	}
	else if (actionKey == "make_swap")
 	{
		bool do_format = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_PARTITION_CREATE, LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_INFO, width, 5) == CMessageBox::mbrYes);

		if (do_format) 
		{
			strcpy(d_settings.drive_partition_fstype[current_device][next_part_number], "swap"); 
			d_settings.drive_partition_mountpoint[current_device][next_part_number] = "none";
			strncpy(d_settings.drive_partition_size[current_device][next_part_number], c_opt[OPT_SWAPSIZE], 4);

			if (!formatPartition(current_device, next_part_number))
				DisplayErrorMessage(err[ERR_FORMAT_PARTITION].c_str());
			else // success
				return menu_return::RETURN_EXIT_ALL;
		}
		
		return res;
 	}
	else if (actionKey == "reset_drive_setup")
 	{
		if (Reset())
		{
			Init();
			return menu_return::RETURN_EXIT_ALL;
		}
		else
			DisplayErrorMessage(err[ERR_RESET].c_str());

		return res;
	}
#ifdef ENABLE_SAMBASERVER
	else if (actionKey == "missing_samba")
	{
		DisplayInfoMessage(g_Locale->getText(LOCALE_SAMBASERVER_SETUP_MSG_NOT_INSTALLED));
		return res;
	}
#endif
	//using generated actionkeys for...
	for (int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		if (actionKey == sel_device_num_actionkey[i]) //...select device
		{
			current_device = i;
			showHddSetupSub();
			return res;
		}
		for (int ii = 0; ii < MAXCOUNT_PARTS; ii++)	
		{
			if (actionKey == make_part_actionkey[ii])//...make partition
			{ 
				bool do_format = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_PARTITION_CREATE, LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_INFO, width, 5) == CMessageBox::mbrYes);
	
				if (do_format) 
				{
					if (formatPartition(current_device, ii)) 
					{ // success
						return menu_return::RETURN_EXIT_ALL;
					}
					else // formating failed
						DisplayErrorMessage(err[ERR_FORMAT_PARTITION].c_str());
				}
				return res;
			}
			else if (actionKey == mount_partition[ii]) //...mount partition
			{
				if (mountPartition(current_device, ii, d_settings.drive_partition_fstype[current_device][ii], d_settings.drive_partition_mountpoint[current_device][ii])) 
				{
					showHddSetupSub();
					return menu_return::RETURN_EXIT_ALL;
				}
				else 
				{
					DisplayErrorMessage(err[ERR_MOUNT_PARTITION].c_str());
					return menu_return::RETURN_EXIT_ALL; 
				}
			}
			else if (actionKey == unmount_partition[ii]) //...unmount partition
			{
				if (unmountPartition(current_device, ii)) 
				{
					showHddSetupSub();
					return menu_return::RETURN_EXIT_ALL;
				}
				else 
				{
					DisplayErrorMessage(err[ERR_UNMOUNT_PARTITION].c_str());
					return menu_return::RETURN_EXIT_ALL; 
				}
			}
			else if (actionKey == delete_partition[ii]) //...delete partition
			{
				bool delete_part = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_PARTITION_DELETE, LOCALE_DRIVE_SETUP_MSG_PARTITION_DELETE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_ERROR, width, 5) == CMessageBox::mbrYes);
		
				if (delete_part) 
				{
					if (!mkPartition(current_device, DELETE_CLEAN, ii))
					{ // delete is failed
						DisplayErrorMessage(err[ERR_MK_PARTITION].c_str());
						return res;
					}
					else 
					{ // delete was successfull
						ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_DRIVE_SETUP_MSG_PARTITION_DELETE_OK, width, msg_timeout, NEUTRINO_ICON_INFO);
						showHddSetupSub();
						return menu_return::RETURN_EXIT_ALL;
					}
				}
				else
					return res;		
			}
			else if (actionKey == check_partition[ii]) //...check partition
			{
				bool check_part = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_PARTITION_CHECK, LOCALE_DRIVE_SETUP_MSG_PARTITION_CHECK, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_ERROR, width, 5) == CMessageBox::mbrYes);
		
				if (check_part) 
				{
					if (unmountPartition(current_device, ii)) 
					{
						string fstype = d_settings.drive_partition_fstype[current_device][ii];
						if (!chkFs(current_device, ii, fstype)){ // check is failed
							DisplayErrorMessage(err[ERR_CHKFS].c_str());
							return res;
						}
						else 
						{ // check was successfull
							//after check, mounting!
							if (!mountPartition(current_device, ii, fstype,d_settings.drive_partition_mountpoint[current_device][ii], true ))
								 DisplayErrorMessage(err[ERR_MOUNT_PARTITION].c_str());
							ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_DRIVE_SETUP_MSG_PARTITION_CHECK_OK, width, msg_timeout, NEUTRINO_ICON_INFO);
							return menu_return::RETURN_EXIT;
						}
					}
				}
				else
					return res;
			}
			else if (actionKey == format_partition[ii]) //...format partition
			{
				string fstype = d_settings.drive_partition_fstype[current_device][ii];
				char msg[255];
				sprintf(msg, g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_FS), fstype.c_str()); 

				bool format_part = (ShowMsgUTF(LOCALE_DRIVE_SETUP_PARTITION_FORMAT, msg, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_ERROR, width, 5) == CMessageBox::mbrYes); //UTF-8
		
				if (format_part) 
				{
					if (unmountPartition(current_device, ii)) 
					{
						if (!mkFs(current_device, ii, fstype))// mkfs is failed
						{
							DisplayErrorMessage(err[ERR_MK_FS].c_str());
							return res;
						}
						else 
						{ // format was successfull
							//after mkFs, check!
							if (!chkFs(current_device, ii, fstype)) // check is failed
								DisplayErrorMessage(err[ERR_CHKFS].c_str());
							else
							{
							ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_DRIVE_SETUP_MSG_PARTITION_FORMAT_OK, width, msg_timeout, NEUTRINO_ICON_INFO);
							return menu_return::RETURN_EXIT;
							}
						}
					}
					else
					{
						DisplayErrorMessage(err[ERR_UNMOUNT_PARTITION].c_str());
						return res;
					}
				}
				else
					return res;
			}
			else if (actionKey == "show_help")
			{
				showHelp();
				return res;
			}
		}
	}

	Init();
	res = exit_res;

	//left menue with user message
	if (ApplySetup())
	{
		//on any error, relaod menue
		if (have_apply_errors)
		{
			Init();
			return menu_return::RETURN_EXIT;
		}		
	}
		
	return res;
}

// init menue
void CDriveSetup::Init()
{
 	cout<<"[drive_setup] init drive setup " << getDriveSetupVersion()<<endl;
		
	CProgressBar pb(false);

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

 	void (CDriveSetup::*pMember[])(void) = {&CDriveSetup::loadDriveSettings,
						&CDriveSetup::loadModulDirs,
						&CDriveSetup::loadHddCount,
						&CDriveSetup::loadHddModels,
						&CDriveSetup::loadFsModulList,
						&CDriveSetup::loadMmcModulList,
						&CDriveSetup::loadFdiskData,
						&CDriveSetup::loadDriveTemps,
						&CDriveSetup::showHddSetupMain};

	frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);

	unsigned int max_members = (sizeof(pMember) / sizeof(pMember[0]));

	for (unsigned int i = 0; i < max_members; i++) 
	{
		pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, max_members, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, "loading menue...", COL_MENUCONTENT);
		(*this.*pMember[i])();
	}

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	//unload unused fs modules on left menu
	for (unsigned int i = 0; i < v_fs_modules.size(); i++) 
	{
		if (!isUsedFsModul(v_fs_modules[i]))
		{
			if (!unloadModul(v_fs_modules[i]))
				cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<err[ERR_UNLOAD_MODUL]<<endl;
		}
	}

}

//applying settings wit user message, returns true if applied anything
bool CDriveSetup::ApplySetup(const bool show_msg)
{
	bool do_apply = false;
	have_apply_errors = false;

	//observ settings
	if (haveChangedSettings())
	{
		if (show_msg)
		{
			if (!ShowLocalizedMessage(LOCALE_DRIVE_SETUP_SAVESETTINGS, LOCALE_DRIVE_SETUP_MSG_SAVESETTINGS_FOUND_CHANGES, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_QUESTION, width, 5) == CMessageBox::mbrYes)
			{
				restoreSettings();
				return false;
			}
			else
				do_apply = true;
		}
		else
			do_apply = true;
	}
	
	//observe current mounts and mount settings only
	if (!do_apply && haveChangedMounts())
	{
		if (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_HEAD, LOCALE_DRIVE_SETUP_MSG_MOUNT, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_QUESTION, width, 5) == CMessageBox::mbrYes)
			do_apply = true;
	}

	//save
	if (do_apply) 
	{
		if (!saveHddSetup()) 
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<": errors while applying settings..."<<endl;
			string 	err_msg = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_FAILED);
				err_msg += "\n" + err[ERR_SAVE_DRIVE_SETUP];
			DisplayErrorMessage(err_msg.c_str());
			have_apply_errors = true;
		}
		return true;
	}
	
	return false;
}

//names and properities for supported filesystems
typedef struct fstype_t
{
	const string fsname;
	const string mkfs_options;
	const string fsck_options;
} fstype_struct_t;

//Note: mkfs options are default values for settings
const fstype_struct_t fstype[MAXCOUNT_FSTYPES] =
{
	{"ext2", 	"-T largefile -v -m0 -q -I 128", 	"-y -v"},
	{"ext3",	"-T largefile -v -m0 -q -I 128", 	"-y -v"},
	{"msdos", 	"", 			"-y"},
	{"vfat", 	"", 			"-y"},
	{"reiserfs", 	"-f", 			"-f -y "},
	{"xfs", 	"-l version=2 -f -q", 	"-v"},
	{"swap", 	"", 			""},
};

typedef struct mn_data_t
{
	const neutrino_locale_t entry_locale;
	const neutrino_msg_t 	rcinput_key;
	const char 		*active_icon;
} mn_data_struct_t;

const mn_data_struct_t mn_data[MAXCOUNT_DRIVE] =
{
	{LOCALE_DRIVE_SETUP_HDD_MASTER	, CRCInput::RC_green	, NEUTRINO_ICON_BUTTON_GREEN},	//MASTER
	{LOCALE_DRIVE_SETUP_HDD_SLAVE	, CRCInput::RC_yellow	, NEUTRINO_ICON_BUTTON_YELLOW},	//SLAVE
 	{LOCALE_DRIVE_SETUP_MMC		, CRCInput::RC_blue	, NEUTRINO_ICON_BUTTON_BLUE},	//MMCARD
};


// shows the main drive setup menue
void CDriveSetup::showHddSetupMain()
{
	// have no fsdrivers found
	if (!have_fsdrivers)
		DisplayErrorMessage(g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_NO_FSDRIVER_FOUND));
	
	// mmc active ?
	device_isActive[MMCARD] = isMmcActive();
	
	// main menue
	CMenuWidget m(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);
	m.setPreselected(selected_main);
	
	// apply
	CMenuForwarder *m1 = new CMenuForwarder(LOCALE_DRIVE_SETUP_SAVESETTINGS, true, NULL, this, "apply", CRCInput::RC_red);
	
	// help
	CMenuForwarder *m_help = new CMenuForwarder(LOCALE_SETTINGS_HELP, true, NULL, this, "show_help", CRCInput::RC_help);
	
	// activate/deactivate ide interface
	if (isIdeInterfaceActive()){
		// get the current state of ide modul and set it to current settings, informations comes from init file and neutrino settings
		// TODO get status direct from driver
		string init_file = getInitIdeFilePath();
		string irq6_opt = getFileEntryString(init_file.c_str(), DBOXIDE, 3);
		d_settings.drive_activate_ide = (irq6_opt == "irq6=1") ? IDE_ACTIVE_IRQ6 : IDE_ACTIVE;
	}
	else
		d_settings.drive_activate_ide = IDE_OFF;
	CMenuOptionChooser *m2	= new CMenuOptionChooser(LOCALE_DRIVE_SETUP_IDE_ACTIVATE, &d_settings.drive_activate_ide, OPTIONS_IDE_ACTIVATE_OPTIONS, OPTIONS_IDE_ACTIVATE_OPTION_COUNT, true);
	
	/************add main menue entries***********/
	// intro entries
	m.addIntroItems();
	// show apply button/entry
	m.addItem(m1);
	m.addItem(GenericMenuSeparatorLine);
	//---------------------------------------------
	m.addItem(m2); // add ide options on/off
	//---------------------------------------------
	
	// mmc: prepare mmc settings  and paint mmc option item only if is any mmc modul available...
	CMenuWidget  *w_mmc_parameter = NULL;
	if (!v_mmc_modules.empty()){
		//mmc options separator
		m.addItem(new CMenuSeparator(CMenuSeparator::ALIGN_CENTER | CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_MMC)); //show mmc options separator if supported
		
		//options item enabled/disabled
		bool mmc_opt_activ = (string)d_settings.drive_mmc_module_name == g_Locale->getText(LOCALE_OPTIONS_OFF) ? false : device_isActive[MMCARD];
		//current modul name on open menue
		sprintf(d_settings.drive_mmc_module_name, "%s", getUsedMmcModulName().c_str());
		
		//prepare sub menue an item for mmc parameters
		w_mmc_parameter = new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);
		showMMCParameterMenu(w_mmc_parameter);
		
		//prepare mmc options...
		CMenuForwarder  *fw_mmc_parameter = new CMenuForwarder(LOCALE_DRIVE_SETUP_MMC_MODUL_PARAMETERS, mmc_opt_activ, NULL, w_mmc_parameter, NULL, CRCInput::RC_0);
		
		//select mmc modules
		if (mmc_notifier == NULL)
			mmc_notifier = new COnOffNotifier(0x61757300 /*off*/); //modify mmc load options (off: 0x61757300, mmc: 0x6D6D6300, mmc2: 0x6D6D6332, mmccombo: 0x6D6D6363)
		mmc_notifier->addItem(fw_mmc_parameter);
		CMenuOptionStringChooser *oj_mmc_chooser = new CMenuOptionStringChooser(LOCALE_DRIVE_SETUP_MMC_USED_DRIVER, d_settings.drive_mmc_module_name, true, mmc_notifier);
		for (size_t i=0; i < v_mmc_modules.size(); i++)
			oj_mmc_chooser->addOption(v_mmc_modules[i].c_str());
		oj_mmc_chooser->addOption(g_Locale->getText(LOCALE_OPTIONS_OFF));//... and add mmc option "off"
		
		//mmc: add select item
		m.addItem (oj_mmc_chooser);	//show mmc name chooser
		m.addItem (fw_mmc_parameter);	//show item mmc options
	}
	else	//set mmc-name option to "off", if no mmc modul is loaded
		strcpy(d_settings.drive_mmc_module_name, g_Locale->getText(LOCALE_OPTIONS_OFF));
		
	m.addItem(GenericMenuSeparatorLine);
	
	//extended settings
	CMenuWidget w_extsettings(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);
	m.addItem (new CMenuForwarder(LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS, true, NULL, &w_extsettings, NULL, CRCInput::RC_1));
	showExtMenu(&w_extsettings);
	
	//drives:
	//show select separator, only visible if any device activ
	if (hdd_count>0 || foundMmc()) 	
		m.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_SELECT));
	
	//capacity
	string s_cap[MAXCOUNT_DRIVE];
	//generate forwarders for model, capacity and temperature
	for (int i = 0; i<MAXCOUNT_DRIVE; i++)	{
		if (device_isActive[i]){
			//model
			m.addItem(new CMenuForwarder(mn_data[i].entry_locale, true, v_model_name[i], this, sel_device_num_actionkey[i].c_str(), mn_data[i].rcinput_key, mn_data[i].active_icon));
			//capacity
			s_cap[i] = convertByteString(device_size[i]);
			m.addItem( new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_CAPACITY, false, s_cap[i]));
			//temperature, do nothing if no value available
			if (v_device_temp[i] !="0")
				m.addItem( new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_TEMP, false, v_device_temp[i].c_str()));
		}
	}
	
	//help
	m.addItem(m_help);
	
	exit_res = m.exec (NULL, "");
	selected_main = m.getSelected();
	if (fstabNotifier != NULL)
		fstabNotifier->removeItems();
	if (mmc_notifier != NULL)
		mmc_notifier->removeItems();
	delete w_mmc_parameter;
}

//init extended settings sub menu
void CDriveSetup::showExtMenu(CMenuWidget *extsettings)
{
	//extended settings: fstab settings
	CMenuOptionChooser *oj_auto_fs = new CMenuOptionChooser(LOCALE_DRIVE_SETUP_FSTAB_USE_AUTO_FS, &d_settings.drive_use_fstab_auto_fs, OPTIONS_ON_OFF_OPTIONS, OPTIONS_ON_OFF_OPTION_COUNT, d_settings.drive_use_fstab);
	if (fstabNotifier == NULL)
		fstabNotifier = new COnOffNotifier(); //enable disable entry for fstab options
	fstabNotifier->addItem(oj_auto_fs);
	CMenuOptionChooser *oj_fstab = new CMenuOptionChooser(LOCALE_DRIVE_SETUP_FSTAB_USE, &d_settings.drive_use_fstab, OPTIONS_ON_OFF_OPTIONS, OPTIONS_ON_OFF_OPTION_COUNT, true, fstabNotifier);

	//extended settings: insmod load options
	CMenuSeparator * sep_load_options = new CMenuSeparator(CMenuSeparator::ALIGN_CENTER | CMenuSeparator::LINE | CMenuSeparator::STRING);
	sep_load_options->setString(LOAD);
	if (insmod_load_options == NULL)
		insmod_load_options = new CStringInputSMS(LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_MODUL_LOADCMD_OPTIONS_INPUT, &d_settings.drive_advanced_modul_command_load_options, 20,
							  false, LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_MODUL_LOADCMD_OPTIONS_INPUT_L1, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz- ");
	CMenuForwarder * fw_input_load_options = new CMenuForwarder(LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_MODUL_LOADCMD_OPTIONS_ENTRY, true, d_settings.drive_advanced_modul_command_load_options, insmod_load_options);

	//extended settings: custom modul dir
	CMenuSeparator * sep_modules = new CMenuSeparator(CMenuSeparator::ALIGN_CENTER | CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_MODUL);
	if (dirchooser_moduldir == NULL)
		dirchooser_moduldir = new CDirChooser(&d_settings.drive_modul_dir);
	CMenuForwarder * fw_moduldir = new CMenuForwarder(LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_CUSTOM_MODULDIR, true, d_settings.drive_modul_dir, dirchooser_moduldir);

	//extended settings: reset settings
	CMenuForwarder *fw_reset = new CMenuForwarder(LOCALE_DRIVE_SETUP_RESET, true, NULL, this, "reset_drive_setup", CRCInput::RC_red);

	//extended settings: filesystem format options
	vector<CMenuForwarder*> v_fs_opts_items;
	for (size_t i=0; i<MAXCOUNT_FSTYPES; i++){
		for (size_t j = 0; j<v_fs_modules.size(); j++){
			if (v_fs_modules[j] != fstype[i].fsname)
				continue;
			//create items, but don't create new input instances
			if (v_input_fs_options[i] == NULL )
				v_input_fs_options[i] = new CStringInputSMS(LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_MODUL_LOADCMD_OPTIONS_INPUT, d_settings.drive_fs_format_option[i], 32,
								LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_MODUL_LOADCMD_OPTIONS_INPUT_L1, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz- ");
			CMenuForwarder * fw_item= NULL;
			fw_item = new CMenuForwarder(fstype[i].fsname.c_str(), true, d_settings.drive_fs_format_option[i], v_input_fs_options[i]);
			v_fs_opts_items.push_back(fw_item);
		}
	}

	//extended settings: add items
	extsettings->addIntroItems(LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS, LOCALE_DRIVE_SETUP_FSTAB);	//intro items
	// -----------------------------------------
	extsettings->addItem (oj_fstab);		//option fstab on/off
	extsettings->addItem (oj_auto_fs);		//option auto fs on/off
	// -----------------------------------------
	extsettings->addItem (sep_load_options);	//separator insmod/modprobe
	extsettings->addItem (fw_input_load_options);	//input options
	extsettings->addItem (sep_modules);		//separator modul
	extsettings->addItem (fw_moduldir);		//select prefered modul directory
	// -----------------------------------------
	if (!v_fs_opts_items.empty())			//separator format options
		extsettings->addItem(new CMenuSeparator(CMenuSeparator::ALIGN_CENTER | CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_FORMAT_OPTIONS));
	for (size_t i = 0; i<v_fs_opts_items.size(); i++)	//format options
		extsettings->addItem(v_fs_opts_items[i]);
	//------------------------------------------
	extsettings->addItem(GenericMenuSeparatorLine);	//separator
	extsettings->addItem (fw_reset);		//reset
}

void CDriveSetup::showMMCParameterMenu(CMenuWidget* w_mmc)
{
	//prepare mmc parameter input
	std::vector<CMenuForwarder*> v_fw_mmc_load_parameters;
	for (uint i=0; i < MAXCOUNT_MMC_MODULES; i++){
		if (v_input_mmc_parameters[i] == NULL)
			v_input_mmc_parameters[i] = new CStringInputSMS(LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_MODUL_LOADCMD_OPTIONS_INPUT, &d_settings.drive_mmc_modul_parameter[i], 20, false,
										LOCALE_DRIVE_SETUP_ADVANCED_SETTINGS_MODUL_MMC_OPTIONS_INPUT_L1, NONEXISTANT_LOCALE, "1234567890abcdefghijklmnopqrstuvwxyz-= ");

		string m_name = have_mmc_modul[i] ? mmc_modules[i]: mmc_modules[i] + " " + g_Locale->getText(LOCALE_DRIVE_SETUP_MMC_MODUL_NOT_INSTALLED);
		v_fw_mmc_load_parameters.push_back(new CMenuForwarder(m_name.c_str(), have_mmc_modul[i], d_settings.drive_mmc_modul_parameter[i], v_input_mmc_parameters[i]));
	}

	//mmc:paint submenue mmc parameters
	w_mmc->addIntroItems(LOCALE_DRIVE_SETUP_MMC_MODUL_PARAMETERS);	//intro items
	// -----------------------------------------
	for (size_t i=0; i < v_fw_mmc_load_parameters.size(); i++)
		w_mmc->addItem(v_fw_mmc_load_parameters[i]); //show selectable mmc modules to edit
}


//helper: generate part entries for showHddSetupSubMenue
string CDriveSetup::getPartEntryString(string& partname)
{
	string p_name = partname;

	string s_mountpoint, s_size, s_type, s_fs, s_options;

	ostringstream str_size, str_hours;

	if (isSwapPartition(p_name))
	{ // found swap partition
		string s_mp 		= getSwapInfo(p_name, FILENAME);
		int mp_len		= s_mp.length();
		s_mountpoint		= (mp_len > 10) ? (s_mp.substr(0, 9) + "..." + s_mp.substr(mp_len-6)) : s_mp;
		s_type			= "swap " + getSwapInfo(p_name, TYPE);
		long long l_size 	= atoi(getSwapInfo(p_name, SIZE).c_str())/1024; //MB
		// convert size to string
		str_size << l_size;
		string s_space(str_size.str());
		s_size		= s_space + " MB";
	}
	else if ((!isSwapPartition(p_name)) && (isActivePartition(p_name))) 
	{
		s_mountpoint 		= getMountInfo(p_name, MOUNTPOINT);
		long long l_space 	= getDeviceInfo(s_mountpoint.c_str(), KB_AVAILABLE)*1024; //Bytes
		long long l_hours	= getDeviceInfo(s_mountpoint.c_str(), FREE_HOURS);
		s_fs 			= getMountInfo(p_name, FS);
		s_options 		= getMountInfo(p_name, OPTIONS);

		//convert bytes
		string s_space = convertByteString(l_space);

		string hours_left;
		if (l_hours > 1) 
		{
			str_hours << l_hours;
			string s_hours(str_hours.str());
			hours_left = "(ca. " + s_hours + "h)";
		}	
		else
			hours_left = "(< 1h)";

		s_size 	= s_space + char(32) + hours_left;
	}

	string 	s_entry = s_mountpoint;
		s_entry += char(32);
		s_entry += s_fs;
		s_entry += char(32);
		s_entry += s_type;
		s_entry += s_options;
		s_entry += char(32);
		s_entry += s_size;

	if (s_mountpoint.empty()) // no active partition mounted
		s_entry = g_Locale->getText(LOCALE_DRIVE_SETUP_PARTITION_NOT_ACTIVE);

	if (!isActivePartition(p_name)) // no active partition found
		s_entry = g_Locale->getText(LOCALE_DRIVE_SETUP_PARTITION_NOT_CREATED);

	return s_entry;
}

// shows the sub setup menue with informations about partitions and primary settings
void CDriveSetup::showHddSetupSub()
{
	//menue sub
	CMenuWidget 	*sub = new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);

	//menue add
	CMenuWidget 	*sub_add = new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);

#if defined ENABLE_NFSSERVER || defined ENABLE_SAMBASERVER
	//menue add shares
	CMenuWidget 	sub_add_share(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);

#ifdef ENABLE_NFSSERVER
	//nfs separator
	CMenuSeparator 	*srv_nfs_sep = new CMenuSeparator(CMenuSeparator::ALIGN_CENTER | CMenuSeparator::LINE | CMenuSeparator::STRING);
	srv_nfs_sep->setString("NFS");
#endif

#ifdef ENABLE_SAMBASERVER
	//samba separator
	CMenuSeparator 	*srv_smb_sep = new CMenuSeparator(CMenuSeparator::ALIGN_CENTER | CMenuSeparator::LINE | CMenuSeparator::STRING);
	srv_smb_sep->setString("Samba");
	CSambaSetup smb;
	bool have_samba = smb.haveSambaSupport();
#endif
#endif /*defined ENABLE_NFSSERVER || defined ENABLE_SAMBASERVER*/

	//menue add_swap
	CMenuWidget 	*sub_add_swap = new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);

	//menue partitions
	CMenuWidget 	*part[MAXCOUNT_PARTS];
	for (int i = 0; i<MAXCOUNT_PARTS; i++)
	{
	 	part[i]= new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);
	}

#if defined ENABLE_NFSSERVER || defined ENABLE_SAMBASERVER
	//menue server shares
	CMenuWidget 	*part_srv_shares[MAXCOUNT_PARTS];
	for (int i = 0; i<MAXCOUNT_PARTS; i++)
	{
	 	part_srv_shares[i]= new CMenuWidget(LOCALE_DRIVE_SETUP_HEAD, msg_icon, width);
	}
#endif /*ENABLE_NFSSERVER || definied ENABLE_SAMBASERVER*/

	//menue sub: prepare sub head
	string dev_name = g_Locale->getText(mn_data[current_device].entry_locale);

	//menue sub: generate part items
	CMenuForwarder *sub_part_entry[MAXCOUNT_PARTS];
	string partname[MAXCOUNT_PARTS];
	string item_name[MAXCOUNT_PARTS];
	for (uint i = 0; i<MAXCOUNT_PARTS; i++) 
	{

 		partname[i] = partitions[current_device][i];

		item_name[i] = getPartEntryString(partname[i]).c_str(), isActivePartition(partname[i]);
		sub_part_entry[i] = new CMenuForwarder(item_name[i].c_str(), isActivePartition(partname[i]), NULL, part[i], NULL/*part_num_actionkey[i]*/, CRCInput::convertDigitToKey(i+1));
	}
	// generate all usable DATA from Device
	generateAllUsableDataOfDevice(current_device);

	//menue sub:
	next_part_number = getFirstUnusedPart(current_device); //also used from swap_add
	unsigned long long ll_free_device_size 		= getUnpartedDeviceSize(current_device);
	unsigned long long ll_next_free_part_size 	= data_partition[current_device][next_part_number].free_size;

	// disable entry if we have no free partition or not enough size
	bool add_activate;
	if (count_Partitions < MAXCOUNT_PARTS && ll_free_device_size > 0xA00000)
		add_activate = true;
	else 
		add_activate = false;
	
	//menue sub: prepare separator: hdparms
	CMenuSeparator *sep_jobs = new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_HDD_JOBS);
 	
	//menue sub: prepare item: add swap
	bool add_swap_active;
	if ((!haveSwap()) && (add_activate)) // disable add item if we have already a swap, no free partition or not enough size
		add_swap_active = true;
	else 
		add_swap_active = false;

	//add swap
	CMenuForwarder *swap_add = new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_ADD_SWAP_PARTITION, add_swap_active, NULL, sub_add_swap, NULL/*"add_swap_partition"*/, CRCInput::RC_red);

	//add part
	CMenuForwarder *part_add = new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_ADD_PARTITION, add_activate, NULL, sub_add, NULL/*"add_partition"*/, CRCInput::RC_green);

	//menue add: prepare subhead: add part
	string add_subhead_txt = dev_name + " >> " + iToString(next_part_number+1) + ". " + g_Locale->getText(LOCALE_DRIVE_SETUP_HDD_ADD_PARTITION);
	CMenuSeparator *add_subhead = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING);
	add_subhead->setString(add_subhead_txt);


	//menue add swap:
	//prepare swap sizes
	long long ll_swap_sizes[2] = 	{(ll_next_free_part_size < 0x4000000 ? ll_next_free_part_size : 0x4000000 /*64MB*/), 
					(ll_next_free_part_size < 0x8000000 ?  ll_next_free_part_size : 0x8000000 /*128MB*/)};

	//set default swap size to 128 MB or available max size
	long long ll_max_swap_size = max(ll_swap_sizes[0], ll_swap_sizes[1]);
	
	string s_swap_size[2];
	for (uint i=0; i < 2; i++)
		s_swap_size[i] = iToString(ll_swap_sizes[i]/1024/1024);
	
	string s_max_swap_size = iToString(ll_max_swap_size/1024/1024);
	
	strncpy(c_opt[OPT_SWAPSIZE], s_max_swap_size.c_str(), 4);	

	CMenuOptionStringChooser *add_swap_size = new CMenuOptionStringChooser(LOCALE_DRIVE_SETUP_PARTITION_SIZE, c_opt[OPT_SWAPSIZE], true );
	for (uint i=0; i < 2; i++) 
		add_swap_size->addOption(s_swap_size[i].c_str());


	//menue add_swap: prepare subhead: add swap
	string add_swap_subhead_txt = dev_name + " >> " + g_Locale->getText(LOCALE_DRIVE_SETUP_HDD_ADD_SWAP_PARTITION);
	CMenuSeparator *add_swap_subhead = new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING);
	add_swap_subhead->setString(add_swap_subhead_txt);

	CMenuForwarder *make_swap = new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_FORMAT_PARTITION, true, NULL, this, "make_swap", CRCInput::RC_red);

	//menue add: prepare start_cylinder for add partition
	setStartCylinder();
	string s_add_start_cyl = iToString(start_cylinder);
	CMenuForwarder *fw_add_start_cyl = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_START_CYLINDER, false, s_add_start_cyl.c_str());
	
	//menue sub: set mountstatus of devices for enable/disable menue items
	bool have_mounts = haveMounts(current_device);
	bool have_parts = haveActiveParts(current_device);
	
	//menue sub: prepare item: mount all partitions
 	CMenuForwarder *mount_all =  new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_MOUNT_NOW_DEVICE, (!have_mounts ? have_parts:false), NULL, this, "mount_device_partitions", CRCInput::RC_yellow);

	//menue sub: prepare item: unmount all partitions
 	CMenuForwarder *ummount_all =  new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_UNMOUNT_NOW_DEVICE, (have_mounts ? true:false), NULL, this, "unmount_device_partitions", CRCInput::RC_blue);

	//menue sub: prepare separator: partlist
	CMenuSeparator *separator = new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_HDD_EDIT_PARTITION);

	//menue partitions:
	CMenuSeparator *p_subhead[MAXCOUNT_PARTS];

	//menue partitions: subhead text
	string sh_txt[MAXCOUNT_PARTS];

	//menue partitions: prepare information about possible size, cylinders
	string s_free_sizeOfDevice = convertByteString(ll_free_device_size);

	//menue partitions: size of current and free partition

	unsigned long long ll_cur_part_size[MAXCOUNT_PARTS];
	string p_size[MAXCOUNT_PARTS];
	string s_size[MAXCOUNT_PARTS];
	string s_sizes_Of_Part_and_Whole[MAXCOUNT_PARTS];
	CMenuForwarder *freesizeOfPart[MAXCOUNT_PARTS];
	CMenuForwarder *partsize[MAXCOUNT_PARTS];

	//sub menue main
	sub->addIntroItems(mn_data[current_device].entry_locale, (current_device != MMCARD) ? LOCALE_DRIVE_SETUP_HDD_PARAMETERS : LOCALE_DRIVE_SETUP_HDD_JOBS);	//intro items
	//------------------------
	CStringInput hdd_sleep(LOCALE_DRIVE_SETUP_HDD_SLEEP, d_settings.drive_spindown[current_device], 3, LOCALE_DRIVE_SETUP_HDD_SLEEP_STD, LOCALE_DRIVE_SETUP_HDD_SLEEP_HELP, "0123456789 ");
	if (current_device != MMCARD) 	//not for mmc!
	{
		sub->addItem(new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_SLEEP, true, d_settings.drive_spindown[current_device], &hdd_sleep )); //spindown
		sub->addItem(new CMenuOptionChooser(LOCALE_DRIVE_SETUP_HDD_CACHE, &d_settings.drive_write_cache[current_device], OPTIONS_ON_OFF_OPTIONS, OPTIONS_ON_OFF_OPTION_COUNT, true )); //writecache
		sub->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_HDD_JOBS)); //separator jobs
	}
	//------------------------
	sub->addItem(swap_add); 	//add swap
	sub->addItem(part_add); 	//add partition
	sub->addItem(mount_all); 	//mount
	sub->addItem(ummount_all); 	//unmount
	//------------------------
	sub->addItem(separator); 	//separator partlist
	//------------------------

	// start cylinder
	unsigned long long start_cyl[MAXCOUNT_PARTS];
	// end cylinder
	unsigned long long end_cyl[MAXCOUNT_PARTS];

	// cylinders
	CMenuForwarder *fw_cylinders[MAXCOUNT_PARTS];

	//menue partitions: prepare separator: settings
	CMenuSeparator *sep_settings = new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVE_SETUP_PARTITION_SETTINGS);

	//choose aktivate/deactivate partition
	CMenuOptionChooser *activate[MAXCOUNT_PARTS];

	//choose mountpoint
	CMenuForwarder * mp_chooser[MAXCOUNT_PARTS];
	CDirChooser * mountdir[MAXCOUNT_PARTS];
	bool item_activ[MAXCOUNT_PARTS];

	//disable item for add swap if we have already a swap or no free partition or not enough size
	if ((!haveSwap()) && (add_activate)) 
		add_swap_active = true;
	else 
		add_swap_active = false;
	
	//choose filesystem
	CMenuOptionStringChooser * fs_chooser[MAXCOUNT_PARTS];
	
	//fs notifier
	COnOffNotifier * fsNotifier[MAXCOUNT_PARTS];

	//size input
	CStringInput * input_part_size[MAXCOUNT_PARTS];
	CMenuForwarder * input_size[MAXCOUNT_PARTS];

#if defined ENABLE_NFSSERVER || defined ENABLE_SAMBASERVER
	//prepare submenue forwarder for server shares
	CMenuForwarder * part_srv_fw[MAXCOUNT_PARTS];
	//prepare forwarder with current mountpoint as shared path
	CMenuForwarder * srv_path_fw[MAXCOUNT_PARTS];
	#ifdef ENABLE_NFSSERVER
		//choose nfs mode
		COnOffNotifier * nfsHostNotifier[MAXCOUNT_PARTS];
		CMenuOptionChooser * nfs_chooser[MAXCOUNT_PARTS];
	
		//menue partitions: host ip input for nfs exports
		CIPInput * nfs_host_ip[MAXCOUNT_PARTS];
		CMenuForwarder * nfs_host_ip_fw[MAXCOUNT_PARTS]; 
	#endif
	#ifdef ENABLE_SAMBASERVER
		//global samba settings
		CMenuForwarder * srv_smb_globals[MAXCOUNT_PARTS];
		COnOffNotifier * sambaNotifier[MAXCOUNT_PARTS];
		CMenuOptionChooser * smb_chooser[MAXCOUNT_PARTS];
		
		CMenuForwarder * smb_share_name_fw[MAXCOUNT_PARTS];
		CStringInputSMS * smb_share_name_input[MAXCOUNT_PARTS];
		CMenuForwarder * smb_share_comment_fw[MAXCOUNT_PARTS];
		CMenuOptionChooser * smb_ro_chooser[MAXCOUNT_PARTS];
		CMenuOptionChooser * smb_public_chooser[MAXCOUNT_PARTS];
	#endif /*ENABLE_SAMBASERVER*/
#endif /*ENABLE_NFSSERVER || definied ENABLE_SAMBASERVER*/

	//make partition
	CMenuForwarder * mkpart[MAXCOUNT_PARTS];

	//mount/unmount partition
	CMenuForwarder * mount_umount[MAXCOUNT_PARTS];
	neutrino_locale_t locale_mount_umount[MAXCOUNT_PARTS];

	//delete partition
	CMenuForwarder * delete_part[MAXCOUNT_PARTS];

	//check partition
	CMenuForwarder * check_part[MAXCOUNT_PARTS];

	//format partition
	CMenuForwarder * format_part[MAXCOUNT_PARTS];

	//action key strings
	string ak_make_partition[MAXCOUNT_PARTS];
	string ak_mount_umount_partition[MAXCOUNT_PARTS];
	string ak_delete_partition[MAXCOUNT_PARTS];
	string ak_check_partition[MAXCOUNT_PARTS];
	string ak_format_partition[MAXCOUNT_PARTS];

	//count of cylinders in edit view
	string ed_cylinders[MAXCOUNT_PARTS];

	//stat of partition
	bool is_mounted[MAXCOUNT_PARTS];

	//menue partitions: edit mode
	for (int i = 0; i<MAXCOUNT_PARTS; i++)
	{
		sub->addItem (sub_part_entry[i]); //possible parts 1-4 for menue partitions:

		//prepare sub head text
		sh_txt[i] 		= dev_name + " >> " + iToString(i+1) + ". " + g_Locale->getText(LOCALE_DRIVE_SETUP_HDD_EDIT_PARTITION);
		p_subhead[i] 		= new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING);
		p_subhead[i]->setString(sh_txt[i]);

		//prepare current partsizes and current freesize
		s_sizes_Of_Part_and_Whole[i] = convertByteString(data_partition[current_device][i].free_size) + " / " + s_free_sizeOfDevice;
		freesizeOfPart[i] 	= new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_FREE_SIZE, false, s_sizes_Of_Part_and_Whole[i].c_str());

		ll_cur_part_size[i] 	= getPartSize(current_device, i);
		p_size[i] 		= convertByteString(ll_cur_part_size[i]);
		s_size[i] 		= iToString(ll_cur_part_size[i]/1024/1024);
		partsize[i] 		= new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_CURRENT_SIZE, false, p_size[i].c_str());

		//prepare cylinders
		start_cyl[i] 		= getPartData(current_device, i, START_CYL, NO_REFRESH);
		end_cyl[i] 		= getPartData(current_device, i, END_CYL, NO_REFRESH);
		ed_cylinders[i] 	= iToString(start_cyl[i]) + " / " + iToString(end_cyl[i]);
		fw_cylinders[i] 	= new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_CURRENT_CYLINDERS, false, ed_cylinders[i].c_str());

		//enable/disable partition
		activate[i] = new CMenuOptionChooser(LOCALE_DRIVE_SETUP_PARTITION_ACTIVATE, &d_settings.drive_partition_activ[current_device][i], OPTIONS_YES_NO_OPTIONS, OPTIONS_YES_NO_OPTION_COUNT, true, NULL, CRCInput::RC_standby);

		//set mountstatus for enable/disable menue items
		if (isMountedPartition(partname[i]) || isSwapPartition(partname[i]))
			is_mounted[i] = true;
		else
			is_mounted[i] = false ;

		item_activ[i] = is_mounted[i] ? false : true;

 		if (isMountedPartition(partname[i]))
		{
			//get mountpoint primary from system, if available and write it to settings for this partition
		}

 		if (isSwapPartition(partname[i]) || (string)d_settings.drive_partition_fstype[current_device][i] == "swap") 
 		{	
 			item_activ[i] = false;
			//if found swap, set mountpoint to "none" in settings for this partition
			d_settings.drive_partition_mountpoint[current_device][i] = "none";
 		}


		//prepare option mointpoint
		mountdir[i] 	= new CDirChooser(&d_settings.drive_partition_mountpoint[current_device][i]);
		mp_chooser[i] 	= new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_MOUNTPOINT, item_activ[i], d_settings.drive_partition_mountpoint[current_device][i], mountdir[i]);

#if defined ENABLE_NFSSERVER || defined ENABLE_SAMBASERVER
		bool share_chooser_activ = ((string)d_settings.drive_partition_fstype[current_device][i] == "swap" ? false : true);
		//prepare submenue for server shares
		part_srv_fw[i] 	= new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_SERVER_SHARE, share_chooser_activ, NULL, part_srv_shares[i], NULL, CRCInput::RC_0);

		//forwarder with current mountpoint as shared path
		srv_path_fw[i] 	= new CMenuForwarder(LOCALE_SAMBASERVER_SETUP_SHARES_PATH, false, d_settings.drive_partition_mountpoint[current_device][i]);

		#ifdef ENABLE_NFSSERVER
			//prepare option host input
			nfs_host_ip[i] = new CIPInput(LOCALE_DRIVE_SETUP_PARTITION_NFS_HOST_IP , d_settings.drive_partition_nfs_host_ip[current_device][i]);
	
			//prepare option nfs	
			nfs_host_ip_fw[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_NFS_HOST_IP, d_settings.drive_partition_nfs[current_device][i], d_settings.drive_partition_nfs_host_ip[current_device][i], nfs_host_ip[i], NULL, CRCInput::RC_1);
	
			//prepare option nfs chooser
			nfsHostNotifier[i] = new COnOffNotifier(); //enable disable entry for input nfs host ip
			nfsHostNotifier[i]->addItem(nfs_host_ip_fw[i]);
			nfs_chooser[i] = new CMenuOptionChooser(LOCALE_DRIVE_SETUP_PARTITION_NFS, &d_settings.drive_partition_nfs[current_device][i], OPTIONS_YES_NO_OPTIONS, OPTIONS_YES_NO_OPTION_COUNT, share_chooser_activ, nfsHostNotifier[i], CRCInput::RC_red);
		#endif /*ENABLE_NFSSERVER*/
		
		#ifdef ENABLE_SAMBASERVER
 			srv_smb_globals[i]= new CMenuForwarder(LOCALE_SAMBASERVER_SETUP, d_settings.drive_partition_samba[current_device][i], NULL, new CSambaSetup(LOCALE_DRIVE_SETUP_HEAD, msg_icon), NULL, CRCInput::RC_0);

			//prepare share name
			smb_share_name_input[i] = new CStringInputSMS(LOCALE_SAMBASERVER_SETUP_SHARES_NAME, &d_settings.drive_partition_samba_share_name[current_device][i], 20, false, LOCALE_SAMBASERVER_SETUP_SHARES_NAME_HINT1, LOCALE_SAMBASERVER_SETUP_SHARES_NAME_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789!""\xA7$%&/()=?-_. ");
			smb_share_name_fw[i] = new CMenuForwarder(LOCALE_SAMBASERVER_SETUP_SHARES_NAME, d_settings.drive_partition_samba[current_device][i], d_settings.drive_partition_samba_share_name[current_device][i], smb_share_name_input[i], NULL, CRCInput::RC_2);
			
			//prepare comment and set a default comment if no comment was found
			if (d_settings.drive_partition_samba_share_comment[current_device][i].empty())
				d_settings.drive_partition_samba_share_comment[current_device][i] = "P" + iToString(i+1) + "@" + v_model_name[current_device];
			smb_share_comment_fw[i] = new CMenuForwarder(LOCALE_SAMBASERVER_SETUP_SHARES_COMMENT, false, d_settings.drive_partition_samba_share_comment[current_device][i]);

			//prepare option read only
			smb_ro_chooser[i] = new CMenuOptionChooser(LOCALE_SAMBASERVER_SETUP_SHARES_RO, &d_settings.drive_partition_samba_ro[current_device][i], OPTIONS_YES_NO_OPTIONS, OPTIONS_YES_NO_OPTION_COUNT, d_settings.drive_partition_samba[current_device][i], NULL, CRCInput::RC_3);

			//prepare option guest ok 
			smb_public_chooser[i] = new CMenuOptionChooser(LOCALE_SAMBASERVER_SETUP_SHARES_PUBLIC, &d_settings.drive_partition_samba_public[current_device][i], OPTIONS_YES_NO_OPTIONS, OPTIONS_YES_NO_OPTION_COUNT, d_settings.drive_partition_samba[current_device][i], NULL, CRCInput::RC_4);

			//prepare on off use
			//only active if samba binaries are available or if no samba installed, show info message
			sambaNotifier[i] = new COnOffNotifier(); //enable disable entries for samba shares
			sambaNotifier[i]->addItem(srv_smb_globals[i]);
			sambaNotifier[i]->addItem(smb_share_name_fw[i]);
			sambaNotifier[i]->addItem(smb_ro_chooser[i]);
			sambaNotifier[i]->addItem(smb_public_chooser[i]);
			smb_chooser[i] = new CMenuOptionChooser(LOCALE_DRIVE_SETUP_PARTITION_SAMBA, &d_settings.drive_partition_samba[current_device][i], OPTIONS_YES_NO_OPTIONS, OPTIONS_YES_NO_OPTION_COUNT, have_samba, sambaNotifier[i], CRCInput::RC_green);
		#endif /*ENABLE_SAMBASERVER*/
#endif /*ENABLE_NFSSERVER || definied ENABLE_SAMBASERVER*/

		//prepare size input, show size
		strcpy( d_settings.drive_partition_size[current_device][i], s_size[i].c_str() ); //set real size to settings
		input_part_size[i] = new CStringInput(LOCALE_DRIVE_SETUP_PARTITION_SIZE, d_settings.drive_partition_size[current_device][i], 8, LOCALE_DRIVE_SETUP_PARTITION_SIZE_HELP, LOCALE_DRIVE_SETUP_PARTITION_SIZE_STD, "0123456789 ");
		input_size[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_SIZE, item_activ[i], d_settings.drive_partition_size[current_device][i], input_part_size[i] );

		//select filesystem
		fsNotifier[i] = new COnOffNotifier(0x73776170 /*swap*/); //enable disable entry for selecting mountpoint
		fsNotifier[i]->addItem(mp_chooser[i]);
		fsNotifier[i]->addItem(input_size[i]);
#if defined ENABLE_NFSSERVER || defined ENABLE_SAMBASERVER
		fsNotifier[i]->addItem(part_srv_fw[i]);
#endif		
		bool fs_chooser_activ = (string)d_settings.drive_partition_fstype[current_device][i] == "swap" ? false : (is_mounted[i] ? false : true);
	 	fs_chooser[i] = new CMenuOptionStringChooser(LOCALE_DRIVE_SETUP_PARTITION_FS, d_settings.drive_partition_fstype[current_device][i],fs_chooser_activ, fsNotifier[i]);
		for (uint n=0; n < v_fs_modules.size(); n++) 
		{
			if ((v_fs_modules[n] != "jbd") && (v_fs_modules[n] != "fat")) 
				fs_chooser[i]->addOption(v_fs_modules[n].c_str());
		}

		//prepare make partition
		ak_make_partition[i] = MAKE_PARTITION + iToString(i);
		mkpart[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_HDD_FORMAT_PARTITION, true, NULL, this, ak_make_partition[i].c_str(), CRCInput::RC_red);

		//prepare format partition
		ak_format_partition[i] = FORMAT_PARTITION + iToString(i);
		format_part[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_FORMAT, true, NULL, this, ak_format_partition[i].c_str(), CRCInput::RC_red);

		//prepare mount/unmount partition, swap caption and actionkey strings
		if(is_mounted[i])
		{
			ak_mount_umount_partition[i] = UNMOUNT_PARTITION + iToString(i);
			locale_mount_umount[i] = LOCALE_DRIVE_SETUP_PARTITION_UNMOUNT_NOW;
		}
		else
		{
			ak_mount_umount_partition[i] = MOUNT_PARTITION + iToString(i);
			locale_mount_umount[i] = LOCALE_DRIVE_SETUP_PARTITION_MOUNT_NOW;
		}
		mount_umount[i] = new CMenuForwarder(locale_mount_umount[i], true, NULL, this, ak_mount_umount_partition[i].c_str(), CRCInput::RC_green);

		//prepare delete partition
		ak_delete_partition[i] = DELETE_PARTITION + iToString(i);
		delete_part[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_DELETE, true, NULL, this, ak_delete_partition[i].c_str(), CRCInput::RC_yellow);

		//prepare check partition
		ak_check_partition[i] = CHECK_PARTITION + iToString(i);
		check_part[i] = new CMenuForwarder(LOCALE_DRIVE_SETUP_PARTITION_CHECK, true, NULL, this, ak_check_partition[i].c_str(), CRCInput::RC_blue);

		//edit partition
		part[i]->addItem(p_subhead[i]);			//subhead
		//------------------------
		part[i]->addIntroItems(NONEXISTANT_LOCALE, LOCALE_DRIVE_SETUP_PARTITION_INFO);	//intro items
		//------------------------
//		part[i]->addItem(freesizeOfPart[i]);		//freesize of Part and / whole freesize, it is not very useful to show more, than size of partition in edit mode!
		part[i]->addItem(partsize[i]);			//partsize
		part[i]->addItem(fw_cylinders[i]);		//cylinders
		//------------------------
		part[i]->addItem(sep_settings);			//separator settings
		//------------------------
		part[i]->addItem(activate[i]);			//enable/disable partition
		part[i]->addItem(fs_chooser[i]);		//select filesystem
		part[i]->addItem(mp_chooser[i]);		//select mountpoint
// 		part[i]->addItem(input_size[i]);		//input part size , it's sufficient to show only the used size, since subsequent changes are not foreseen at the moment!
		//------------------------
		part[i]->addItem(sep_jobs);			//separator jobs
		//------------------------
		part[i]->addItem(format_part[i]);		//format partition
		part[i]->addItem(mount_umount[i]);		//mount/unmount partition
		part[i]->addItem(delete_part[i]);		//delete partition
		part[i]->addItem(check_part[i]);		//check partition
#if defined ENABLE_NFSSERVER || defined ENABLE_SAMBASERVER
		part[i]->addItem(GenericMenuSeparatorLine);	//separator
		part[i]->addItem(part_srv_fw[i]);		//sub menue server shares
			//------------------------
			part_srv_shares[i]->addIntroItems(LOCALE_DRIVE_SETUP_PARTITION_SERVER_SHARE);	//intro items
			part_srv_shares[i]->addItem(srv_path_fw[i]);		//show shared mountpoint
		#ifdef ENABLE_NFSSERVER
			part_srv_shares[i]->addItem(srv_nfs_sep);		//nfs separator
			part_srv_shares[i]->addItem(nfs_chooser[i]);		//nfs on/off
			part_srv_shares[i]->addItem(nfs_host_ip_fw[i]);		//nfs host ip input
			//------------------------
		#endif
		#ifdef ENABLE_SAMBASERVER
			part_srv_shares[i]->addItem(srv_smb_sep);		//samba separator
			if (!have_samba)
				part_srv_shares[i]->addItem(new CMenuForwarder(LOCALE_MESSAGEBOX_INFO, true, NULL, this, "missing_samba", CRCInput::RC_help));//samba info
			part_srv_shares[i]->addItem(smb_chooser[i]);		//samba on/off
			//------------------------
			part_srv_shares[i]->addItem(GenericMenuSeparatorLine);	//separator
			part_srv_shares[i]->addItem(srv_smb_globals[i]);	//samba globals
			//------------------------
			part_srv_shares[i]->addItem(GenericMenuSeparatorLine);	//separator
			part_srv_shares[i]->addItem(smb_share_comment_fw[i]);	//samba share comment
			part_srv_shares[i]->addItem(smb_share_name_fw[i]);	//samba share name
			part_srv_shares[i]->addItem(smb_ro_chooser[i]);		//samba readonly
			part_srv_shares[i]->addItem(smb_public_chooser[i]);	//samba guest ok
			//------------------------
		#endif
#endif /*ENABLE_NFSSERVER || definied ENABLE_SAMBASERVER*/
	
	}
	
	//add
	sub_add->addItem(add_subhead); 	//add partition-subhead
	//------------------------
	sub_add->addIntroItems();	//intro items
	//------------------------
	sub_add->addItem(freesizeOfPart[next_part_number]); // freesize of Part
	sub_add->addItem(fw_add_start_cyl);		//start_cylinder
	sub_add->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add->addItem(activate[next_part_number]);	//enable/disable partition
	sub_add->addItem(fs_chooser[next_part_number]);	//select filesystem
	sub_add->addItem(mp_chooser[next_part_number]);	//select mountpoint
	sub_add->addItem(input_size[next_part_number]);	//input part size
	//------------------------
	sub_add->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add->addItem(mkpart[next_part_number]);	//make partition
#if defined ENABLE_NFSSERVER || defined ENABLE_SAMBASERVER
	sub_add->addItem(GenericMenuSeparatorLine);	//separator
	sub_add->addItem(part_srv_fw[next_part_number]);//sub menue server shares
		//------------------------
		sub_add_share.addIntroItems(LOCALE_DRIVE_SETUP_PARTITION_SERVER_SHARE);	//intro items
		sub_add_share.addItem(srv_path_fw[next_part_number]); 	//separator
	#ifdef ENABLE_NFSSERVER
		sub_add_share.addItem(srv_nfs_sep);				//nfs separator
		sub_add_share.addItem(nfs_chooser[next_part_number]);		//nfs
		sub_add_share.addItem(nfs_host_ip_fw[next_part_number]);	//nfs host ip input
		//------------------------
	#endif
	#ifdef ENABLE_SAMBASERVER
		sub_add_share.addItem(srv_smb_sep);				//samba separator
		sub_add_share.addItem(smb_chooser[next_part_number]);		//samba on/off
		sub_add_share.addItem(GenericMenuSeparatorLine);		//separator
		//------------------------
		sub_add_share.addItem(srv_smb_globals[next_part_number]);	//samba globals
		sub_add_share.addItem(GenericMenuSeparatorLine);		//separator
		//------------------------
		sub_add_share.addItem(smb_share_comment_fw[next_part_number]);	//samba share comment
		sub_add_share.addItem(smb_share_name_fw[next_part_number]);	//samba share name
		sub_add_share.addItem(smb_ro_chooser[next_part_number]);	//samba readonly
		sub_add_share.addItem(smb_public_chooser[next_part_number]);	//samba guest ok
		//------------------------
	#endif
#endif /*ENABLE_NFSSERVER || definied ENABLE_SAMBASERVER*/


	//add swap
	sub_add_swap->addItem(add_swap_subhead); 		//add swap-subhead
	//------------------------
	sub_add_swap->addIntroItems();	//intro items
	//------------------------
	sub_add_swap->addItem(freesizeOfPart[next_part_number]); // freesize of Part
	sub_add_swap->addItem(fw_add_start_cyl);		//start_cylinder
	sub_add_swap->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add_swap->addItem(activate[next_part_number]);	//enable/disable partition
	sub_add_swap->addItem(add_swap_size);			//swap size
	//------------------------
	sub_add_swap->addItem(GenericMenuSeparatorLine);	//separator
	//------------------------
	sub_add_swap->addItem(make_swap);			//make swap partition



	sub->exec (NULL, "");
	delete sub;
}

//calc the current start cylinder for selected partition at device
void CDriveSetup::setStartCylinder()
{
	if (next_part_number == 0)
		start_cylinder = getPartData(current_device, 0, START_CYL, NO_REFRESH) + 1;
	else 
		start_cylinder = getPartData(current_device, next_part_number-1, END_CYL, NO_REFRESH) + 1;
}


// show progress status
void CDriveSetup::showStatus(const int& progress_val, const string& msg, const int& max)
{
	CProgressBar pb(false);
	string s_info = msg;
	cout<<"[drive setup] "<<msg<<endl;
	frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
	pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, progress_val, max, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, msg.c_str(), COL_MENUCONTENT);
}

bool CDriveSetup::formatPartition(const int& device_num, const int& part_number)
// make all steps to create, formating and mount partitions
{
	err[ERR_FORMAT_PARTITION] = "";
	bool ret = true;
	unsigned long long raw_size = atol(d_settings.drive_partition_size[device_num][part_number]);
	if (raw_size > 0)
		part_size = raw_size*1024*1024; //bytes
	else
		part_size = 0;

	string fs = d_settings.drive_partition_fstype[device_num][part_number]; // filesystem
	string mp = d_settings.drive_partition_mountpoint[device_num][part_number]; // mountpoint

	
	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
	

	int i = 1;
 	showStatus(0, "formating...", 7);

	if (unmountDevice(device_num))
		showStatus(i++, "unmount device...");
		

	string partname = partitions[device_num][part_number];

	if ((isActivePartition(partname)) || (isSwapPartition(partname))) 
	{
		showStatus(i++, "deleting partition...");
			if(mkPartition(device_num, DELETE, part_number, start_cylinder, part_size /*bytes*/))
				showStatus(i, "deleting partition...ok");
	}

	showStatus(i, "creating partition...");
	if (mkPartition(device_num, ADD, part_number, start_cylinder, part_size /*bytes*/))
	{
		i++;
		showStatus(i, "formating...");
		if (mkFs(device_num, part_number, fs)) /*filesystem*/ 
		{			
			i++;
	 		showStatus(i, "checking filesystem...");
	 		if (chkFs(device_num, part_number, fs)) 
			{/*checking fs*/
				i++;
				showStatus(i, "mounting...");
				if (!d_settings.drive_partition_activ[device_num][part_number]) 
				{
					showStatus(i, "partition not mounted, please activate..."); 
					ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_DRIVE_SETUP_MSG_PARTITION_NOT_MOUNTED_PLEASE_ACTIVATE, width, msg_timeout, NEUTRINO_ICON_INFO);
				}	
				else if (mountDevice(device_num)) 
				{ /*mounting*/
					showStatus(i, "partitions mounted...");
					ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_SUCCESS, width, msg_timeout, NEUTRINO_ICON_PARTITION);
				}
				else 
				{
					err[ERR_FORMAT_PARTITION] = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_FAILED);
					ret = false;
				}		
			}
			else
			{
				err[ERR_FORMAT_PARTITION] = err[ERR_CHKFS];
				ret = false;
			} 
 		}
 		else
		{
			err[ERR_FORMAT_PARTITION] = err[ERR_MK_FS];
 			ret = false;
		}
	}
	else
	{
		err[ERR_FORMAT_PARTITION] = err[ERR_MK_PARTITION];
		ret = false;
	}


	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	return ret;
}

// loads basicly data from devices generated by fdisk
void CDriveSetup::loadFdiskData()
{
 	// cleanup vectors before

	for (int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		if(access(drives[i].device.c_str(), R_OK) ==0) 
		{
			// generate fdisk part table
			if (loadFdiskPartTable(i))
			{
				// device size
				device_size[i] = getFileEntryLong(PART_TABLE, drives[i].device, 4);

				// count of cylinders
				device_cylcount[i] = getFileEntryLong(PART_TABLE, "heads", 4);
	
				// count of heads
				device_heads_count[i] = getFileEntryLong(PART_TABLE, "heads", 0);
	
				// count of sectors
				device_sectors_count[i] = getFileEntryLong(PART_TABLE, "sectors", 2);

				// sizes of cylinder
				device_cyl_size[i] = getFileEntryLong(PART_TABLE, "Units", 8);	
			}
		}
	}

	remove(PART_TABLE);
}

// returns the first possible unused partition number 0...MAXCOUNT_PARTS-1 (not the real numbers 1...n)
unsigned int CDriveSetup::getFirstUnusedPart(const int& device_num)
{
	unsigned int ret = 0;

	for (unsigned int i = 0; i<MAXCOUNT_PARTS; i++) 
	{
		if (!isActivePartition(partitions[device_num][i])) 
		{
			ret = i;
			break;
		}
	}

	return ret;
}

// unmount all mounted partition or swaps
bool CDriveSetup::unmountAll()
{
	bool ret = true;
	string 	err_msg;
	err[ERR_UNMOUNT_ALL] = "";

	for (unsigned int i=0; i < MAXCOUNT_DRIVE; i++) 
	{
		if(!unmountDevice(i /*MASTER||SLAVE||MMCARD*/))
		{
			err_msg += "\n";
			err_msg	+= err[ERR_UNMOUNT_DEVICE];
			ret = false;
		}
	}

	if (!ret)
		err[ERR_UNMOUNT_ALL] = err_msg;

	return ret;
}

// unmount all mounted partitions or swaps from device
bool CDriveSetup::unmountDevice(const int& device_num)
{
	bool ret = true;
	int i = device_num;
	string err_msg;
	err[ERR_UNMOUNT_DEVICE] = "";

	for (unsigned int ii=0; ii < MAXCOUNT_PARTS; ii++) 
	{
		if(!unmountPartition(i, ii))
		{
			err_msg += "\n";
			err_msg += err[ERR_UNMOUNT_PARTITION]; 
			err_msg += "\n";
			ret = false;
		}
	}

	if (!ret)
	{
		err[ERR_UNMOUNT_DEVICE] = g_Locale->getText(mn_data[i].entry_locale); 
		err[ERR_UNMOUNT_DEVICE] += "\n"; 
		err[ERR_UNMOUNT_DEVICE] += g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_UNMOUNT_DEVICE) + err_msg;
	}

	return ret;
}

// unmount single mounted partition or swap
bool CDriveSetup::unmountPartition(const int& device_num /*MASTER||SLAVE||MMCARD*/, const int& part_number)
{
	string partname = partitions[device_num][part_number];
	err[ERR_UNMOUNT_PARTITION] = "";

	//executing user script if available before unmounting
	/* $ echo -n "/var/tuxbox/config/before_unmount_1_1.sh"|wc -c
	 * 40
	 */
	char user_script[64];
	snprintf(user_script, 64, "%s/before_unmount_%d_%d.sh", CONFIGDIR, device_num, part_number);
	user_script[63] = '\0'; /* ensure termination... */
	if (my_system(user_script) != 0)
		perror(user_script);

	if((access(partname.c_str(), R_OK) !=0) || (!isActivePartition(partname))) // exit if no available
	{ 
 		return true;
	}
	else 
	{
		string swapoff_cmd = SWAPOFF + partname;

		if (isSwapPartition(partname)) 
		{ // unmount swap
			if (swapoff(partname.c_str()) !=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": error while swapoff "<<partname<<" "<< strerror(errno)<< endl;
				err[ERR_UNMOUNT_PARTITION] = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_UNMOUNT_SWAP);
				return false;
			}
			else
			{ 
				if (mount("tmpfs", "/tmp" , "tmpfs", MS_REMOUNT , "size=50%")!=0)
				{
					cerr<<"[drive setup] "<<__FUNCTION__ <<":  mount: "<<strerror(errno)<< " " << "tmpfs"<<endl;
					err[ERR_UNMOUNT_PARTITION] += "\nError while remounting /tmp!";
				}
				else 
					return true;
			}
		}
		else if (isMountedPartition(partname)) 
		{ // unmount partition

		#ifdef ENABLE_SAMBASERVER
			// stop samba if it's necessary 
			if (d_settings.drive_partition_samba[device_num][part_number])
			{	CSambaSetup smb;
				smb.killSamba();
			}
		#endif
		
			string mp = getMountInfo(partname, MOUNTPOINT);

			if (umount(mp.c_str()) !=0)
			{ 
				cerr<<"[drive setup] "<<__FUNCTION__ <<": error while unmount "<<partname<<" "<< strerror(errno)<< endl;
				char msg[255];
				switch (errno)
				{
					case 16 /*EBUSY*/:
						sprintf(msg, "%s", g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_UNMOUNT_PARTITION_BUSY));
						break;
					default: 
						sprintf(msg, "%s\n%s: %s", g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_UNMOUNT_PARTITION), g_Locale->getText(LOCALE_MESSAGEBOX_ERROR),  strerror(errno));
						break;		
				}

				char err_msg[255];
				sprintf(err_msg, "%d %s", part_number+1, msg); 
				err[ERR_UNMOUNT_PARTITION] = err_msg;

				return false;
			}
			else 
			{
				return true;
			}
		}
	}

	return true;
}


// returns true if modul/driver is loaded, false if unloaded e.g: isModulLoaded("dboxide")
bool CDriveSetup::isModulLoaded(const string& modulname)
{
	string temp = "";
	set <string> modules;
	ifstream input(PROC_MODULES);

	while( input >> temp ) 
	{
		modules.insert(temp);
		getline(input,temp);
	}

	return modules.count(modulname);
}


typedef struct ide_modules_t
{
	const int modul_num;
	const string modul;
} ide_modules_struct_t;

#define IDE_MODULES_COUNT 4
const ide_modules_struct_t ide_modules[IDE_MODULES_COUNT] =
{
	{0, IDE_CORE	},
	{1, DBOXIDE	},
	{2, IDE_DETECT	},
	{3, IDE_DISK	}
};

// load/apply/testing modules and returns true if it sucessfully
bool CDriveSetup::initIdeDrivers(const bool irq6)
{
	err[ERR_INIT_IDEDRIVERS] = "";

	if (unloadIdeDrivers()) // unload previously loaded modules
		printf("[drive setup] ide modules unloaded...\n");

	CProgressBar pb(false);
	bool ret = true;
	string err_msg;

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	v_init_ide_L_cmds.clear();
	
	// exec, test commands and add commands to vector
	for (unsigned int i = 0; i < IDE_MODULES_COUNT; i++)
	{
		string modulname = ide_modules[i].modul;
		
		string load_cmd = getInitModulLoadStr(modulname);

		if (i != LOAD_DBOXIDE)
			v_init_ide_L_cmds.push_back(load_cmd);
		else
			v_init_ide_L_cmds.push_back(((!irq6) ? load_cmd : load_cmd + " irq6=1")); // observe irq6 option!)


		if (!isModulLoaded(modulname))
		{
			if (CNeutrinoApp::getInstance()->execute_sys_command(v_init_ide_L_cmds[i].c_str())!=0) 
				cerr<<"[drive setup] "<<__FUNCTION__ <<": loading " << modulname << "...failed " << strerror(errno)<<endl;
		}

		// TESTING loaded modul
		if (!isModulLoaded(modulname)) 
		{
			err_msg = "modul " +  modulname + " not loaded"; 
			cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<err[ERR_INIT_IDEDRIVERS]<<endl;
			ret = false;
		}
			// show load progress on screen
			frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
			pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, IDE_MODULES_COUNT-1, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, modulname.c_str(), COL_MENUCONTENT);
	}

	// refreshing 
	loadHddCount();

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	if (!ret)
		err[ERR_INIT_IDEDRIVERS] = err_msg;

	return ret;
}

// load/apply/testing modules and returns true on sucess
bool CDriveSetup::initFsDrivers(bool do_unload_first)
{
	err[ERR_INIT_FSDRIVERS] = "";

	// reset
	v_init_fs_L_cmds.clear();

	CProgressBar pb(false);

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	bool ret = true;
	unsigned int modul_count = v_fs_modules.size();
	string err_msg;

	// exec commands
	for (unsigned int i = 0; i < modul_count; i++)
	{
		// exit for, if we use swap
		if (v_fs_modules[i]=="swap")
			 break; 

		// testing init command / init modul
		if (!initModul(v_fs_modules[i], do_unload_first)) 
		{
			err_msg += "\n";
			err_msg += err[ERR_INIT_MODUL];
			ret = false;
		}
		else
		{
			// add fs modules to init list only if needed
			if (isUsedFsModul(v_fs_modules[i]))
			{
				// add dependent modul first and push_back to command list
				if (v_fs_modules[i]=="ext3")
					v_init_fs_L_cmds.push_back(getInitModulLoadStr("jbd"));
		
				if (v_fs_modules[i]=="vfat")
					v_init_fs_L_cmds.push_back(getInitModulLoadStr("fat"));
		
				// add modul to vector
				v_init_fs_L_cmds.push_back(getInitModulLoadStr(v_fs_modules[i]));
			}
		}



		// show load progress on screen
		string 	screen_msg = "load ";
			screen_msg += v_fs_modules[i];
		frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
		pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, modul_count, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, screen_msg.c_str(), COL_MENUCONTENT);
	}

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	if (!ret)
		err[ERR_INIT_FSDRIVERS] = err_msg;

	return ret;
}

// unload mmc modul and returns true on success
bool CDriveSetup::unloadMmcDrivers()
{
	err[ERR_UNLOAD_MMC_DRIVERS] = "";
	unsigned int i = 0;
	while (i < v_mmc_modules.size())
	{
		if (isModulLoaded(v_mmc_modules[i])) 
		{
			if (!unloadModul(v_mmc_modules[i]))
			{
				err[ERR_UNLOAD_MMC_DRIVERS] = err[ERR_UNLOAD_MODUL];
				return false;
			}
		}
		i++;
	}

	// set entry for init file
	s_init_mmc_cmd = "";

	return true;
}

#define MAXCOUNT_MMC_MODULES 3
// load/apply/testing mmc modul and returns true on success
bool CDriveSetup::initMmcDriver()
{
	err[ERR_INIT_MMCDRIVER] = "";

	// unload first
	if (!unloadMmcDrivers())
		return false;

	string modul_name = (string)d_settings.drive_mmc_module_name;
	
	string opts;

	if (modul_name == g_Locale->getText(LOCALE_OPTIONS_OFF))
		return true;

	for (uint i = 0; i < v_mmc_modules.size(); i++)
	{
		if (modul_name == v_mmc_modules[i])
		{
			opts = char(32) + v_mmc_modules_opts[i];
			// set entry for init file
			s_init_mmc_cmd = getInitModulLoadStr(modul_name) + opts;
			break;
		}
	}

	// exec command
	if (!initModul(modul_name, false, opts)) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": loading "<<modul_name<<opts<< " failed..."<<endl;
		err[ERR_INIT_MMCDRIVER] = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_LOAD_MMC_DRIVER_FAILED);
		return false;
	}
	
	return true;
}

// unload modules and returns true on sucess
bool CDriveSetup::unloadFsDrivers()
{
	err[ERR_INIT_FSDRIVERS] = "";

	CProgressBar pb(false);

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	bool ret = true;
	unsigned int modul_count = v_fs_modules.size();
	string err_msg;

	// exec commands
	for (unsigned int i = 0; i < modul_count; i++)
	{
		if (!unloadModul(v_fs_modules[i]))
		{
			err_msg += "\n";
			err_msg += err[ERR_UNLOAD_MODUL];
			ret = false;
		}

		// show unload progress on screen
		string 	screen_msg = "unload ";
			screen_msg += v_fs_modules[i];
		frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
		pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, modul_count, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, screen_msg.c_str(), COL_MENUCONTENT);
	}

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	if (!ret)
		err[ERR_UNLOAD_FSDRIVERS] = err_msg;

	return ret;
}

// unloads ide modules, returns true on success
bool CDriveSetup::unloadIdeDrivers()
{
	err[ERR_UNLOAD_IDEDRIVERS] = "";

	CProgressBar pb(false);

	bool ret = true;
	string err_msg;

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	// exec all commands
	int i = IDE_MODULES_COUNT-1;
	while (i > -1)
	{
		if (!unloadModul(ide_modules[i].modul))
		{
			err_msg += "\n" + ide_modules[i].modul;
			ret = false;
			i = -1; // exit while
		}
		// painting load progress on screen
		frameBuffer->paintBoxRel(pb_x, pb_y, pb_w, pb_h, COL_MENUCONTENT_PLUS_0, RADIUS_MID);
		pb.paintProgressBar(pb_x+10, pb_y+pb_h-20-SHADOW_OFFSET, pb_w-20, 16, i, IDE_MODULES_COUNT-1, 0, 0, COL_SILVER, COL_INFOBAR_SHADOW, ide_modules[i].modul.c_str(), COL_MENUCONTENT);
		i--;
	}

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}
	
	if (!ret)
		err[ERR_UNLOAD_IDEDRIVERS] = "Can't unload " + err_msg;

	return ret;
}

//creates possible module paths
void CDriveSetup::loadModulDirs()
{
	string k_path = "/" + k_name;

	//possible paths of modules
	moduldir[0] = MOUDULDIR + k_path + "/misc";
	moduldir[1] = MOUDULDIR + k_path + "/kernel/drivers/ide"; 
	moduldir[2] = MOUDULDIR + k_path + "/kernel/fs/";
	moduldir[3] = d_settings.drive_modul_dir;
}

//helper: get init string for any modulename depends of it's path in root or var
//default paths are in /lib/modules/[KERNEL_NAME]/kernel/fs/ and /lib/modules/[KERNEL_NAME]/kernel/misc
//the default command string for these paths is insmod [modulname] or modprobe [modulname] but the
//prefered path is /var/lib/modules/
//if we found a module in the prefered path, than returns load command with full path else returns the default load command string
string CDriveSetup::getInitModulLoadStr(const string& modul_name)
{
	loadModulDirs();
	string m_filename = "/" + modul_name + M_TYPE;

	//using .erase(0,1) to remove the 1st slash in modul path (MOUDULDIR), we need a command string without this slash! 
	string modul_path[] = {	moduldir[3] + m_filename, /*d_settings.drive_modul_dir*/ 
				moduldir[0].erase(0,1) + m_filename,
				moduldir[1].erase(0,1) + m_filename,
				moduldir[2].erase(0,1) + modul_name + m_filename}; 

	string cmd = LOAD + d_settings.drive_advanced_modul_command_load_options + char(32);

	string load_str = "";
	
	for (uint i=0; i<(sizeof(modul_path) / sizeof(modul_path[0])) ; i++)
	{
 		if (access(modul_path[i].c_str(), R_OK)==0)
		{
 			load_str = (i > 0) ? cmd + modul_name : cmd + modul_path[i];
			return load_str;
		}		
	}
	
	cerr<<"[drive setup] "<<__FUNCTION__ <<": can't found modul "<<modul_name<<endl;

	return load_str;
}

// load module, returns true on success
bool CDriveSetup::initModul(const string& modul_name, bool do_unload_first, const string& options)
{
	err[ERR_INIT_MODUL] = "";

	// load any dependent modules
	if (modul_name == "ext3")
		initModul("jbd", false);

	if (modul_name == "vfat")
		initModul("fat", false);

	
	string 	load_cmd =  getInitModulLoadStr(modul_name) + options;

	if (do_unload_first) 
	{
		if (unloadModul(modul_name)) 
		{// ensure that the module is currently unloaded
			if (!isModulLoaded(modul_name))
			{
				if (CNeutrinoApp::getInstance()->execute_sys_command(load_cmd.c_str()) !=0) 
				{
					cerr<<"[drive setup] "<<__FUNCTION__ <<": load "<<modul_name<< "...failed "<<endl;
					err[ERR_INIT_MODUL] = "Can't load " + modul_name;
				}
			}
			if (!isModulLoaded(modul_name)) 
			{ // check loaded modules
				cerr<<"[drive setup] "<<__FUNCTION__ <<": modul "<<modul_name<< " not loaded"<<endl;
				err[ERR_INIT_MODUL] = "Can't load " + modul_name;
				return false;
			}
		}
	}
	else 
	{
		if (!isModulLoaded(modul_name))
		{
			if (CNeutrinoApp::getInstance()->execute_sys_command(load_cmd.c_str()) !=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": load "<<modul_name<< "...failed "<<endl;
			}
		}
			if (!isModulLoaded(modul_name)) 
			{ // check loaded modules
				cerr<<"[drive setup] "<<__FUNCTION__ <<": modul "<<modul_name<< " not loaded"<<endl;
				err[ERR_INIT_MODUL] = "Can't load " + modul_name;
				return false;
			}
	}

	return true;
}

// unload module, returns true on success
bool CDriveSetup::unloadModul(const string& modulname)
{
	string 	unload_cmd = UNLOAD + modulname;
	int retval = 0;
	err[ERR_UNLOAD_MODUL] = "";

	if (isModulLoaded(modulname))
	{
		//unload any dependent modules first
		if (modulname=="jbd")
			unloadModul("ext3");
		if (modulname=="fat")
			unloadModul("vfat");
		
		retval = CNeutrinoApp::getInstance()->execute_sys_command(unload_cmd.c_str());		

		if (retval !=0)
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<": unload "<<modulname<< "...failed "<<endl;
			//get modul status
			string m_stat = getFileEntryString(PROC_MODULES, modulname.c_str(), 3);
			//send message
			if ((retval == 139) || (m_stat == "(deleted)"))
			{
				string msg_txt = unload_cmd + " " + g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_FATAL);
				bool msg = (ShowMsgUTF(LOCALE_MESSAGEBOX_ERROR, msg_txt, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_ERROR, width, 5) == CMessageBox::mbrYes);
				if (msg)
				{
					unlink(getInitIdeFilePath().c_str());
					unlink(getInitMountFilePath().c_str());
				}
			}
			err[ERR_UNLOAD_MODUL] = "Can't unload " + modulname;
			return false;
		}
		else
			return true;
	}
	else
		return true;
}

// returns true if fs module is needed
bool CDriveSetup::isUsedFsModul(const string& fs_name)
{
	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		for (unsigned int j = 0; j < MAXCOUNT_PARTS; j++)
		{ 
			string fs = d_settings.drive_partition_fstype[i][j];
			if (fs == fs_name && !fs.empty() && (d_settings.drive_partition_activ[i][j]))
				return true;
		}	
	}
	
	return false;
}

// saves settings: load/unload modules and writes init file
bool CDriveSetup::saveHddSetup()
{
	bool ide_disabled = true;
	bool ret = true;
	vector<string> v_errors;
	err[ERR_SAVE_DRIVE_SETUP] = "";

	//unmount first
	if (!unmountAll())
	{
		v_errors.push_back(err[ERR_UNMOUNT_ALL]);
		ret = false;
	}

	//ide
	if (d_settings.drive_activate_ide == IDE_ACTIVE) 
		ide_disabled = (!initIdeDrivers() ? true : false);
	
	if (d_settings.drive_activate_ide == IDE_ACTIVE_IRQ6) 
		ide_disabled = (!initIdeDrivers(true) ? true : false);

	if (d_settings.drive_activate_ide == IDE_OFF)
	{
		if (!unloadIdeDrivers())
		{
			ide_disabled = false;
			v_errors.push_back(err[ERR_UNLOAD_IDEDRIVERS]);
			ret = false;
		}
		else
			ide_disabled = true;
	}

	//check ide status
	if (d_settings.drive_activate_ide != IDE_OFF && !isIdeInterfaceActive()) 
	{
		v_errors.push_back(g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_ACTIVATE_INTERFACE));
		v_errors.push_back(err[ERR_INIT_IDEDRIVERS]);
		ret = false;
	}

	// hdparm: show error messages, but without effect for settings
	if (ide_disabled)
	{
		loadHddParams(true); //reset hdparm parameters
	}
	else 
	{
		if (!loadHddParams(false))
			DisplayInfoMessage(err[ERR_HDPARM].c_str());
	}

	// mmc stuff
	if (isMmcEnabled())
	{ 
		if (!initMmcDriver())
		{ 
			v_errors.push_back(err[ERR_INIT_MMCDRIVER]),
			ret = false;
		}
	}
	else
	{
		if (!unloadMmcDrivers())
		{
			v_errors.push_back(err[ERR_UNLOAD_MMC_DRIVERS]),
			ret = false;
		}
	}

	//fs modules
	if (ide_disabled && !isMmcEnabled())
	{
		if (!unloadFsDrivers())
		{
			v_errors.push_back(err[ERR_UNLOAD_FSDRIVERS]);
			ret = false;
		}
	}
	else
	{
		if (!initFsDrivers())
		{
			v_errors.push_back(err[ERR_INIT_FSDRIVERS]);
			ret = false;
		}
	}

	
	//mount all parts
	if (!mkMounts())
	{
		v_errors.push_back(err[ERR_MK_MOUNTS]);
		ret = false;
	}


	//fstab
	if (ret) 
	{
		if (!mkFstab())
		{
			v_errors.push_back(err[ERR_MK_FSTAB]);
			ret = false;
		}
	}
	

	// write and linking init and config files
#ifdef ENABLE_SAMBASERVER
	
	setSambaMode();	

	// write samba files
	if (ret)
	{
		//smb.conf
		if (!mkSmbConf())
		{
			v_errors.push_back(err[ERR_MK_SMBCONF]);
			ret = false;
		}
		//31sambaserver
		if (!mkSambaInitFile())
		{
			v_errors.push_back(err[ERR_MK_SMBINITFILE]);
			ret = false;
		}
		// unlink old symlinks to 31sambaserver
		if (!unlinkSmbInitLinks())
		{
			v_errors.push_back(err[ERR_UNLINK_SMBINITLINKS]);
			ret = false;
		}
		//create new links
		if (!linkSmbInitFiles())
		{
			v_errors.push_back(err[ERR_LINK_SMBINITFILES]);
			ret = false;
		}

	}

	// start samba if it's enabled
	if (ret)
	{
		if (g_settings.smb_setup_samba_on_off && haveMountedSmbShares())
		{
			CSambaSetup smb;
			if (!smb.startSamba())
			{
				v_errors.push_back(smb.getErrMsg());
				ret = false;
			}
		}
	}
#endif

#ifdef ENABLE_NFSSERVER
	// exports
	if (ret)
	{
		if (!mkExports())
		{
			v_errors.push_back(err[ERR_MK_EXPORTS]);
			ret = false;
		}
	}
#endif

	// initfiles for ide, fs, mmc, and mounts
	if (ret)
	{
		if (!writeInitFile(ide_disabled))
		{
			v_errors.push_back(err[ERR_WRITE_INITFILES]);
			ret = false;
		}	
	}

	// linking initfiles
	if (ret)
	{
		if (!linkInitFiles())
		{
			v_errors.push_back(err[ERR_LINK_INITFILES]);
			ret = false;
		}	
	}

	//save settings
	if (ret)
	{
		if (!writeDriveSettings())
		{
			v_errors.push_back(err[ERR_WRITE_SETTINGS]);
			ret = false;
		}
	}

	// create result message
	if (ret)
	{
		neutrino_locale_t msg_locale;
	
		if (ide_disabled && !isMmcActive())
			msg_locale = LOCALE_DRIVE_SETUP_MSG_SAVED_DISABLED;
		else
			msg_locale = LOCALE_DRIVE_SETUP_MSG_SAVED;
	
			ShowLocalizedHint(LOCALE_DRIVE_SETUP_HEAD, msg_locale, width, msg_timeout, NEUTRINO_ICON_INFO);
	}
	else 	//create summary error message
	{
		for (uint i=0; i<(v_errors.size()) ; i++)
		{
			err[ERR_SAVE_DRIVE_SETUP] += v_errors[i] + "\n";
		}
		return false;
	}

	return true;
}

// writes init files from line collection, parameter clear==true will remove command entries, == disabled interface
bool CDriveSetup::writeInitFile(const bool clear)
{
	bool ret = true;
	err[ERR_WRITE_INITFILES] = "Error while writing initfiles:\n";

	string init_file [INIT_FILE_TYPE_NUM_COUNT] = {getInitIdeFilePath(), getInitMountFilePath()};

	ofstream initfile[INIT_FILE_TYPE_NUM_COUNT] ;

	// write init files
	for (uint i=0; i<(INIT_FILE_TYPE_NUM_COUNT) ; ++i) 
	{
		initfile[i].open(init_file[i].c_str());

		if (!initfile[i]) 
		{ // Error while open
			err[ERR_WRITE_INITFILES] += init_file[i] + "\n" + strerror(errno) + "\n";
       			cerr << "[drive setup] "<<__FUNCTION__ <<":  write error "<<init_file[i]<<", please check permissions..."<< strerror(errno)<<endl;
			return false;
		}
		// add head lines
		initfile[i] << getInitFileHeader(init_file[i]) <<endl;
	}	

	if (clear && !isMmcEnabled()) 
	{ // interface is disabled 
		initfile[INIT_FILE_MODULES] << "echo "<<char(34)<<"ide-interface/mmc disabled"<<char(34)<<endl; // write disable tag


	}
	else 
	{
 		// write commands for loading the filesystem modules and hdparm commands
		string md_txt = getInitFileModulEntries();
		initfile[INIT_FILE_MODULES] << (md_txt)<<endl;

		// write mount entries
		string 	m_txt =  getInitFileMountEntries();
		initfile[INIT_FILE_MOUNTS] <<(m_txt)<<endl;
	}


	for (uint i=0; i<(INIT_FILE_TYPE_NUM_COUNT) ; ++i) 
	{
 		initfile[i].close();
	
		// INIT_IDE_SCRIPT_PATH must be executable
		int chmod_ret = chmod(init_file[i].c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

		if ( chmod_ret !=0 )
		{
			err[ERR_WRITE_INITFILES] += init_file[i] + "\n" + strerror(chmod_ret) + "\n";
			cerr<<"[drive setup] "<<__FUNCTION__ <<": Error while setting permissions for "<<init_file[i]<<" "<<strerror(chmod_ret)<<endl;
			ret = false;
		}
		else
			cout<<"[drive setup] "<<__FUNCTION__ <<": writing "<<init_file[i]<< " ...ok"<<endl;
	}
	
	return ret;
}

// returns true if found hdd, disc is eg: "/proc/ide/ide0/hda"
bool CDriveSetup::foundHdd(const string& disc)
{
	string hdd = disc/* + "/model"*/;
	if(access(hdd.c_str(), R_OK) == 0)
		return true;
	else
		return false;
}

// returns true if found mmc
bool CDriveSetup::foundMmc()
{
	if(access(MMCA, R_OK) == 0)
		return true;
	else
		return false;
}

// returns the model name of hdd, disc is eg: "/proc/ide/ide0/hda"
string CDriveSetup::getModelName(const string& disc)
{
	string filename = disc + "/model";
	string model;

	if (foundHdd(disc))
	{
		ifstream f(filename.c_str());
		char ch;
		while(f.get(ch)) {
			model += ch;
		}
		f.close();
		return model;
	}
	return g_Locale->getText(LOCALE_DRIVE_SETUP_MODEL_UNKNOWN);
}

// converts size string to a viable unit e.g. MB or GB, usable in menue entries, as std::string
string CDriveSetup::convertByteString(const unsigned long long& byte_size /*bytes*/)
{
	string ret = "";
	unsigned long long size = byte_size;
	unsigned long long l_size[] = {	size/1024 /*kb*/,
					size/1024/1024 /*MB*/,
					size/1024/1024/1024 /*GB*/,
					size/1024/1024/1024/1024 /*TB*/};

	unsigned int i_size = sizeof(l_size) / sizeof(l_size[0]);

	string s_size[i_size];
				
	ostringstream str_size[i_size];

	for (unsigned int i = 0; i < i_size; i++)
	{
		str_size[i] << l_size[i];
		string s_temp_size(str_size[i].str());
		s_size[i] = s_temp_size;
	}

	if (size < 0x400 /*1MB*/) 
	{
		ret = s_size[0] + " kB";
	}
	else if ((size > 0x400 /*1MB*/) && (size < 0x40000000 /*1GB*/)) 
	{
		ret = s_size[1] ;
		ret += " MB";
	}
	else if ((size > 0x40000000 /*1GB*/) && (size < 0x280000000LL /*10GB*/)) 
	{
		ret = s_size[2] ;
		ret += " GB (" + s_size[1] + " MB)";
	}
	else if ((size > 0x280000000LL /*10GB*/) && (size < 0x10000000000LL /*1TB*/)) 
	{
		ret = s_size[2] ;
		ret += " GB";
	}
	else if (size >= 0x10000000000LL /*1TB*/)
	{
		ret = s_size[3] ;
		ret += " TB";
	}
	else
		ret += "0";
		
	
	return ret;
}

// collects temperatures of all devices to vector v_device_temp
void CDriveSetup::loadDriveTemps()
{
	v_device_temp.clear();

	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++)
	{
		if (i !=MMCARD)
			v_device_temp.push_back(getHddTemp(i));
		else
			v_device_temp.push_back("0"); // we don't need temperatur for mmc
	}	
}

// returns free disc temperature from disc via hddtemp, device_num is MASTER||SLAVE
string CDriveSetup::getHddTemp(const int& device_num)
{
	int readtemp = 0;
	string disc = drives[device_num].device;

		string	cmdtemp  = HDDTEMP;
			cmdtemp += " -n -w -q ";
			cmdtemp += disc;
			cmdtemp += " ";
			cmdtemp += "> /tmp/hdtemp";
			cmdtemp += " ";
			cmdtemp += "2>/dev/null";

	int cmd_res = CNeutrinoApp::getInstance()->execute_sys_command(cmdtemp.c_str());

	if (cmd_res!=0)
	{
		string cerr_content = "[drive setup] " + (string)__FUNCTION__ + ": executing " +  cmdtemp + " ...failed! ";
		
		if (cmd_res == 127) 
			cerr<<cerr_content<<HDDTEMP" not installed"<<endl;
		else
			cerr<<cerr_content<<endl;

		return "0";
	}

	FILE *f=fopen( "/tmp/hdtemp", "r");
	if (!f)
		return "0";

	fscanf(f, "%d", &readtemp );
	fclose(f);
	unlink("/tmp/hdtmp");
	
	ostringstream Str;
	Str << readtemp;
	string temp(Str.str());
	
	return temp;
}

// set/apply/testing hdparm commands and returns true on sucess, parameter "reset = true" sets no commands
bool CDriveSetup::loadHddParams(const bool do_reset)
{
	string str_hdparm_cmd[hdd_count/*MASTER, SLAVE*/];
	v_hdparm_cmds.clear();
	bool ret = true;
	err[ERR_HDPARM] = "";

	// do nothing on reset
	if (!do_reset)
	{
		char opt_hdparm[hdd_count/*MASTER, SLAVE*/][15];

		//test/define all commands
		for (int i = 0; i < hdd_count; i++)
		{
			sprintf(opt_hdparm[i],"-S%d -W%d -c1 ", atoi(d_settings.drive_spindown[i])/5, d_settings.drive_write_cache[i]);
			str_hdparm_cmd[i] =((device_isActive[i]) ? HDPARM + (string)opt_hdparm[i]  + drives[i].device : "");

			if (device_isActive[i])
			{
				int cmd_res = CNeutrinoApp::getInstance()->execute_sys_command(str_hdparm_cmd[i].c_str());
								
				if (cmd_res !=0)
				{ 
					string cerr_content = "[drive setup] " + (string)__FUNCTION__  + ": executing " + str_hdparm_cmd[i] + " ...failed! ";
					string 	err_msg = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_HDPARM);
						err_msg += "\nProblem: ";	

					if (cmd_res == 127)
					{ 
						cerr<<cerr_content<<HDPARM" not installed"<<endl;
						err_msg += HDPARM;
						err_msg += g_Locale->getText(LOCALE_DRIVE_SETUP_MMC_MODUL_NOT_INSTALLED);
					}
					else if (cmd_res == 5)
					{
						err_msg += g_Locale->getText(mn_data[i].entry_locale);
						err_msg += "\n";
						err_msg += g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_HDPARM_NOT_COMPATIBLE);
					}
					else
					{
						cerr<<cerr_content<<endl;
						err_msg += iToString(cmd_res);
					}

					err[ERR_HDPARM] = err_msg;
					ret = false;
				}
			}
			// add to collection
			v_hdparm_cmds.push_back(str_hdparm_cmd[i]);
		}
	}

	return ret;
}

/* returns mode of partitions, true=active, false=not active, usage: bool partActive = isActivePartition("/dev/hda1") */
bool CDriveSetup::isActivePartition(const string& partname)
{
 	string part_tab;
	string partx = partname;

	if(access(partx.c_str(), R_OK) !=0) {// exit if no available
 		//cerr<<"[drive setup] "<<partname<<" not found..."<< endl;
		return false;
	}

	if(access(PROC_PARTITIONS, R_OK) !=0) 
	{// exit if no available
		cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<PROC_PARTITIONS<<" not found..."<< endl;
		return false;
	}

	// get parts
	ifstream f(PROC_PARTITIONS);
	if (!f)	
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while open "<<PROC_PARTITIONS<<" "<< strerror(errno)<<endl;
		return false;
	}
	char ch;
	while(f.get(ch)) 
	{
		part_tab += ch;
	}
	f.close();

	//normalize partion name, remove "/dev/"
	partx.replace(partx.find("/dev/"), 5 ,"");

	//search matching entries and return result
	string::size_type loc = part_tab.find( partx, 0 );
	return (( loc != string::npos ) ? true : false);
}

// generate fstab file, returns true on success NOTE: All partitions must be mounted!
bool CDriveSetup::mkFstab()
{
	// set fstab path
	string fstab = getFstabFilePath();
	string timestamp = getTimeStamp();
	err[ERR_MK_FSTAB] = "";

	vector<string> v_fstab_entries;
	v_fstab_entries.push_back("# " + fstab + " generated from neutrino ide/mmc/hdd drive-setup\n #" +  getDriveSetupVersion() + " " +  timestamp );

	//remove fstab if not needed
	if (!d_settings.drive_use_fstab) 
		remove(fstab.c_str());
	else
	{
		// collecting mount settings
		for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
		{
			for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
			{
				string partname = partitions[i][ii];
				string mount_entry;
				if (isSwapPartition(partname)) 
				{
					string swap_size = iToString(getPartSize(i,ii)/1024/1024);
					mount_entry = partname;
					mount_entry += " none swap sw 0 0\n"; // TODO setup for fstab swap options
					mount_entry += "tmpfs /tmp tmpfs size=";
					mount_entry += swap_size ; //use real size of partition for swap size, not setting /*d_settings.drive_partition_size[i][ii]*/
					mount_entry += "M,remount 0 0\n";
					v_fstab_entries.push_back(mount_entry);
				}
				else if (isMountedPartition(partname)) 
				{
					string mp = getMountInfo(partname, MOUNTPOINT);
					string fs = (d_settings.drive_use_fstab_auto_fs ? "auto" : d_settings.drive_partition_fstype[i][ii]);
					mount_entry = partname;
					mount_entry += char(32);
					mount_entry += mp + char(32) + fs +" rw,sync 0 0"; // TODO setup for fstab mount options
					v_fstab_entries.push_back(mount_entry);
				}
			}
		}
	
		// write fstab
		ofstream str_fstab(fstab.c_str());
		if (!str_fstab) 
		{ // Error while open
			cerr << "[drive setup] "<<__FUNCTION__ <<": write error "<<fstab<<", please check permissions..." << strerror(errno)<<endl;
			err[ERR_MK_FSTAB] = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_MAKE_FSTAB);
			return false;
		}
		else 
		{
			for (unsigned int i = 0; i < v_fstab_entries.size(); i++) 
			{
				str_fstab << v_fstab_entries[i] <<endl;
			}
		}
		str_fstab.close();

		cout<<"[drive setup] "<<__FUNCTION__ <<": writing "<<fstab<< "...ok"<<endl;
	}

	return true;
}

//mount all devices, if any device activated
bool CDriveSetup::mkMounts()
{
	err[ERR_MK_MOUNTS] = "";

	if (d_settings.drive_activate_ide != IDE_OFF || isMmcActive()) 
	{//first mounting all hdd partitions if ide interface is activ or mount mmc if any device activated
		if (!mountAll())
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<": error while mounting partitions!"<<endl;
			err[ERR_MK_MOUNTS] = err[ERR_MOUNT_ALL];
			return false;
		}
	}
	else 
	{//or mounting all devices if nothing activated
		if (!unmountAll())
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<": error while unmounting partitions!"<<endl;
			err[ERR_MK_MOUNTS] = err[ERR_UNMOUNT_ALL];
			return false;
		}
	}
	
	return true;
}

// collects spported and available filsystem modules, writes to vector v_fs_modules
void CDriveSetup::loadFsModulList()
{
	//possible filesystems
	string mod[] 		= {"ext2", "ext3", "reiserfs", "xfs", "vfat"};

	uint dir_count = sizeof(moduldir) / sizeof(moduldir[0]);

	DIR *mdir[dir_count];

	//cleanup  collection
	v_fs_modules.clear();

	for (uint i = 0; i < dir_count; i++)
	{
		mdir[i] = opendir(moduldir[i].c_str());
		if (!mdir[i])
			cerr<<"[drive setup] "<<__FUNCTION__ <<": can't open directory "<<moduldir[i]<< " "<< strerror(errno)<<endl;
		else
			closedir(mdir[i]);
		
		for (uint ii = 0; ii < sizeof(mod) / sizeof(mod[0]); ii++)
		{
			//generating possible modules
			string path_var = moduldir[i] + "/" + mod[ii] + M_TYPE;
			string path_root = moduldir[i] + "/" + mod[ii] + "/" + mod[ii] + M_TYPE;
					
			if (access(path_var.c_str(), R_OK)==0 || access(path_root.c_str(), R_OK)==0)
			{
				//add found module
				v_fs_modules.push_back(mod[ii]);
			}
		}
	}

	//sort modules and remove possible double entries
	sort(v_fs_modules.begin(), v_fs_modules.end());
	v_fs_modules.resize(unique(v_fs_modules.begin(), v_fs_modules.end()) - v_fs_modules.begin());

	//set status for available fsdriver
 	have_fsdrivers = v_fs_modules.empty() ? false : true;
	
	// last fs must be swap
	if (!haveSwap())
		v_fs_modules.push_back("swap");
}

// collects spported and available mmc modules, writes to vector v_mmc_modules, return true on success
void CDriveSetup::loadMmcModulList()
{
	uint dir_count = sizeof(moduldir) / sizeof(moduldir[0]);

	DIR *mdir[dir_count];

	//cleanup  collections
	v_mmc_modules.clear();
	v_mmc_modules_opts.clear();

	for (uint i = 0; i < dir_count; i++)
	{
		mdir[i] = opendir(moduldir[i].c_str());
		if (!mdir[i]) 
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<": can't open directory "<<moduldir[i]<< " "<< strerror(errno)<<endl;
		}
		closedir(mdir[i]);


		// scan module dir 
		for (uint ii = 0; ii < MAXCOUNT_MMC_MODULES; ii++)
		{
			//generating possible modules
			string path_var = moduldir[i] + "/" + mmc_modules[ii] + M_TYPE;
			string path_root = moduldir[i] + "/" + mmc_modules[ii] + "/" + mmc_modules[ii] + M_TYPE;

			if (access(path_var.c_str(), R_OK)==0 || access(path_root.c_str(), R_OK)==0)
			{
				//add found module
				v_mmc_modules.push_back(mmc_modules[ii]);
				//set state found mmc module to true 
				have_mmc_modul[ii]=true;

				//add matching modul parameter
				if (mmc_modules[ii] ==  M_MMC)
					v_mmc_modules_opts.push_back(d_settings.drive_mmc_modul_parameter[MMC]);
				if (mmc_modules[ii] ==  M_MMC2)
					v_mmc_modules_opts.push_back(d_settings.drive_mmc_modul_parameter[MMC2]);
				if (mmc_modules[ii] ==  M_MMCCOMBO)
					v_mmc_modules_opts.push_back(d_settings.drive_mmc_modul_parameter[MMCCOMBO]);
			}
		}		
	}

	//sort modules and remove possible double entries
	sort(v_mmc_modules.begin(), v_mmc_modules.end());
	v_mmc_modules.resize(unique(v_mmc_modules.begin(), v_mmc_modules.end()) - v_mmc_modules.begin()); 
}

// returns name of current used mmc modul
string CDriveSetup::getUsedMmcModulName()
{
	unsigned int i = 0;

	while (i < v_mmc_modules.size())
	{
		if (isModulLoaded(v_mmc_modules[i]))
			return v_mmc_modules[i];
		i++;
	}

	return g_Locale->getText(LOCALE_OPTIONS_OFF);
}

void CDriveSetup::loadHddCount()
{
	hdd_count = 0; // reset

	int i = 0;
	while (i < MAXCOUNT_DRIVE)
	{
		device_isActive[i /*MASTER||SLAVE*/]	= foundHdd(drives[i].proc_device /*IDE0HDA||IDE0HDB*/);
		if (device_isActive[i])
			hdd_count++;
		i++;
	}
// 	printf("[drive setup] found harddiscs: %d\n", hdd_count);
}


void CDriveSetup::loadHddModels()
{
	v_model_name.clear();
	int i = 0;
	while (i < MAXCOUNT_DRIVE)
	{
		v_model_name.push_back(getModelName(drives[i].proc_device /*IDE0HDA||IDE0HDB*/));
// 		printf("[drive setup] detected hdd model: %d. %s\n", i+1, v_model_name[i].c_str());
		i++;
	}
	
}


typedef struct fstypes_t
{
	const long fs_type_const;
	const char *fs_name;
} fstypes_struct_t;

#define FS_TYPES_COUNT 45
const fstypes_struct_t fstypes[FS_TYPES_COUNT] =
{
	{0x073717368,	"squashfs" /*SQUASHFS_SUPER_MAGIC*/	},
	{0xadf5,	"ADFS_SUPER_MAGIC"			},
	{0xADFF,	"AFFS_SUPER_MAGIC"			},
	{0x42465331,	"BEFS_SUPER_MAGIC"			},
	{0x1BADFACE,	"BFS_MAGIC"				},
	{0xFF534D42,	"cifs"/*"CIFS_MAGIC_NUMBER"*/		},
	{0x73757245,	"CODA_SUPER_MAGIC"			},
	{0x012FF7B7,	"COH_SUPER_MAGIC"			},
	{0x28cd3d45,	"cramfs"/*"CRAMFS_MAGIC"*/		},
	{0x1373,	"devfs"/*"DEVFS_SUPER_MAGIC"*/		},
	{0x00414A53,	"EFS_SUPER_MAGIC"			},
	{0x137D,	"EXT_SUPER_MAGIC"			},
	{0xEF51,	"ext2"/*"EXT2_OLD_SUPER_MAGIC"*/	},
	{0xEF53,	"ext2"/*"EXT2_SUPER_MAGIC"*/		},
	{0xEF53,	"ext3"/*"EXT3_SUPER_MAGIC"*/		},
	{0x4244,	"HFS_SUPER_MAGIC"			},
	{0xF995E849,	"HPFS_SUPER_MAGIC"			},
	{0x958458f6,	"HUGETLBFS_MAGIC"			},
	{0x9660,	"ISOFS_SUPER_MAGIC"			},
	{0x72b6,	"jffs2"/*"JFFS2_SUPER_MAGIC"*/		},
	{0x3153464a,	"JFS_SUPER_MAGIC"			},
	{0x137F,	"MINIX_SUPER_MAGIC"			},
	{0x138F,	"MINIX_SUPER_MAGIC2"			},
	{0x2468,	"MINIX2_SUPER_MAGIC"			},
	{0x2478,	"MINIX2_SUPER_MAGIC2"			},
	{0x4d44,	"vfat"/*"MSDOS_SUPER_MAGIC"*/		},
	{0x564c,	"NCP_SUPER_MAGIC"			},
	{0x6969,	"nfs"/*"NFS_SUPER_MAGIC"*/		},
	{0x5346544e,	"NTFS_SB_MAGIC"				},
	{0x9fa1,	"OPENPROM_SUPER_MAGIC"			},
	{0x9fa0,	"procfs"/*"PROC_SUPER_MAGIC"*/		},
	{0x002f,	"QNX4_SUPER_MAGIC"			},
	{0x52654973,	"reiserfs"/*"REISERFS_SUPER_MAGIC"*/	},
	{0x7275,	"ROMFS_MAGIC"				},
	{0x517B,	"smbfs"/*"SMB_SUPER_MAGIC"*/		},
	{0x012FF7B6,	"SYSV2_SUPER_MAGIC"			},
	{0x012FF7B5,	"SYSV4_SUPER_MAGIC"			},
	{0x01021994,	"tmpfs"/*"TMPFS_MAGIC"*/		},
	{0x15013346,	"UDF_SUPER_MAGIC"			},
	{0x00011954,	"UFS_MAGIC"				},
	{0x9fa2,	"USBDEVICE_SUPER_MAGIC"			},
	{0xa501FCF5,	"VXFS_SUPER_MAGIC"			},
	{0x012FF7B4,	"XENIX_SUPER_MAGIC"			},
	{0x58465342,	"xfs"/*"XFS_SUPER_MAGIC"*/		},
	{0x012FD16D,	"_XIAFS_SUPER_MAGIC" 			}
};

/* returns fs type */
const char *CDriveSetup::getFsTypeStr(long &fs_type_const)
{
	for (unsigned int i = 0; i < FS_TYPES_COUNT; i++)
	{
		if (fs_type_const == fstypes[i].fs_type_const)
			return fstypes[i].fs_name;
	}
	
	return "unknown filesystem";
}


// returns device infos as long long, depends from parameter device_info 
long long CDriveSetup::getDeviceInfo(const char *mountpoint, const int& device_info)
{
	struct statfs s;
	long long blocks_used, blocks_percent_used=0;
	
	// exit if no available
	if(access(mountpoint, R_OK) !=0) 
		return 0;

	if (statfs(mountpoint, &s) != 0)
	{
		perror(mountpoint);
		return 0;
	}

	if (s.f_blocks > 0) 
	{
		blocks_used = s.f_blocks - s.f_bfree;
		blocks_percent_used = (long long)(blocks_used * 100.0 / (blocks_used + s.f_bavail) + 0.5);		
	}
		
	switch (device_info)
	{
		case KB_BLOCKS :
			return (long long) (s.f_blocks * (s.f_bsize / 1024.0));
			break;
		case KB_USED :
			return (long long) ((s.f_blocks - s.f_bfree) * (s.f_bsize / 1024.0));
			break;
		case KB_AVAILABLE :
			return (long long) (s.f_bavail * (s.f_bsize / 1024.0));
			break;
		case PERCENT_USED :
			return blocks_percent_used;
			break;
		case PERCENT_FREE :
			return 100 - blocks_percent_used;
			break;
		case FILESYSTEM :
			return s.f_type;
			break;
		case FREE_HOURS :
			return (long long) (s.f_bavail * (s.f_bsize / 1024.0))/1024/33/60;
			break;
		default : return 0;
	}

}

// returns info of swaps about partitions, filesystem and options from proc/swaps, e.g: getswapInfo(HDA1, MOUNTPOINT)
string CDriveSetup::getSwapInfo(const string& partname, const int& swap_info_num)
{
	string res = "";

	if(access(partname.c_str(), R_OK) !=0) 
	{// exit if no available
// 		cerr<<"[drive setup] "<<partname<<" not found..."<< endl;
		return res;
	}

	if(access(PROC_SWAPS, R_OK) !=0) 
	{ // exit if no available
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error, can't found "<<PROC_SWAPS<<endl;
		return res;
	}

	ifstream in (PROC_SWAPS, ios::in);

	if (!in) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while open "<<PROC_SWAPS<<" "<< strerror(errno)<<endl;
		return res;
	}

	char line[256];
	int column_num = swap_info_num;

	while (in.getline (line, 256))
	{
		string swaps_line = (string)line, str_res;
		string::size_type loc = swaps_line.find( partname, 0 );

		if ( loc != string::npos ) 
		{
			stringstream stream(swaps_line);

			for(int l = 0; l <= 5; l++)
			{
				stream >> str_res;
				if (l==column_num) 
				{
					res = str_res;
					break;
				}
					
			}
		}
	}
	in.close();
	return res;
}

// returns info of mounts about mountpoint, filesystem and options from mtab, e.g: getMountInfo(HDA1, MOUNTPOINT) returns the mountpoint 
string CDriveSetup::getMountInfo(const string& partname, const int& mtab_info_num)
{
	string ret = "";

	if(access(partname.c_str(), R_OK) !=0) 
	{// exit if no available
// 		cerr<<"[drive setup] "<<partname<<" not found..."<< endl;
		return ret;
	}

	if(access(PROC_MOUNTS, R_OK) !=0) 
	{ // exit if no available
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error, can't found "<<PROC_MOUNTS<<endl;
		return ret;
	}

	ret = getFileEntryString(PROC_MOUNTS, partname, mtab_info_num);

	return ret;
}

// helper: return a selectable tab entry from file 
string CDriveSetup::getFileEntryString(const char* filename, const std::string& filter_entry, const int& column_num)
{
	string ret = "";
	char line[256];
	ifstream in (filename, ios::in);

	if (!in) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while open "<<filename<<" "<< strerror(errno)<<endl;
		return ret;
	}

	while (in.getline (line, 256))
	{
		string tab_line = (string)line, str_res;
		string::size_type loc = tab_line.find( filter_entry, 0 );

		if ( loc != string::npos ) 
		{
			stringstream stream(tab_line);

			for(int i = 0; i <= 10; i++)
			{
				stream >> str_res;
				if (i==column_num) 
				{
					ret = str_res;
					in.close();
					return ret;
				}

			}
		}
	}
	in.close();
	return ret;
}


// returns mount stat of device, isMountedPartition("HDA1") = true=mounted, false=not mounted
bool CDriveSetup::isMountedPartition(const string& partname)
{
	string partx = partname;

	if(access(partx.c_str(), R_OK) !=0 || access(PROC_MOUNTS, R_OK) !=0) 
	{// exit if no available
		return false;
	}

	if (!getFileEntryString(PROC_MOUNTS, partname, 1).empty())
		return true;
	else
		return false;

}

// returns mode of swap partitions, true=is swap, false=no swap, usage: bool part_is_swap = isSwapPartition("/dev/hda1") 
bool CDriveSetup::isSwapPartition(const string& partname)
{
	string partx = partname;

	if(access(partx.c_str(), R_OK) !=0 || access(PROC_SWAPS, R_OK) !=0) 
	{// exit if no available
		return false;
	}

	if (!getFileEntryString(PROC_SWAPS, partname, 1).empty())
		return true;
	else
		return false;
}

// returns true if already exists an active swap partition
bool CDriveSetup::haveSwap()
{
	string swap_entry = getFileEntryString(PROC_SWAPS, "partition", 0);

	if (!swap_entry.empty())
		return true;
	else
		return false;
}

// generates several DATA from device
void CDriveSetup::generateAllUsableDataOfDevice(const int& device_num)
{
	unsigned long long total_used_cyl= 0;

 	if (loadFdiskPartTable(device_num)) 
	{
		unsigned long long cyl_size 	= getFileEntryLong(PART_TABLE, "Units", 8);
		unsigned long long fullcyl	= getFileEntryLong(PART_TABLE, "sectors/track", 4);
		string partname;
		count_Partitions = 0;

		for (int i = 0; i < MAXCOUNT_PARTS; i++)
		{
			partname = partitions[device_num][i];

			data_partition[device_num][i].start_cyl = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_START_CYL);
			data_partition[device_num][i].end_cyl 	= getFileEntryLong(PART_TABLE, partname, FDISK_INFO_END_CYL);
			data_partition[device_num][i].used_size = data_partition[device_num][i].start_cyl > 0 ? (data_partition[device_num][i].end_cyl - data_partition[device_num][i].start_cyl + 1)*cyl_size : 0;

			total_used_cyl += data_partition[device_num][i].start_cyl > 0 ? data_partition[device_num][i].end_cyl - data_partition[device_num][i].start_cyl + 1 : 0;

			if (data_partition[device_num][i].start_cyl > 0)
				count_Partitions++;
		}	

		// FREE Cylinders for mkpartition
		for (int i = 0; i < MAXCOUNT_PARTS; i++)
		{
			unsigned long long next_startcyl = 0, free_cyl = 0;
			// search for next start_cyl
			for (int ii = i; ii < MAXCOUNT_PARTS - 1; ii++)
			{
				if (data_partition[device_num][ii+1].start_cyl > 0)
				{
					next_startcyl = data_partition[device_num][ii+1].start_cyl;
					break;
				}
			}

			if (i == 0)                      // PARTITION 1
			{
				if (data_partition[device_num][i].start_cyl == 0 && next_startcyl > 0)
					free_cyl = next_startcyl - 1;
			}
			else if (i > 0 && i < 3)         // PARTITION 2 + 3
			{
				if (data_partition[device_num][i].start_cyl == 0 && next_startcyl > 0)
					free_cyl =  next_startcyl- 1 - data_partition[device_num][i-1].end_cyl;
			}

			if (data_partition[device_num][i].start_cyl == 0 && next_startcyl == 0)
			{
				free_cyl = fullcyl - total_used_cyl;
			}

			data_partition[device_num][i].free_size = free_cyl * cyl_size;

		}
	}
}

// returns free size for new partitions from device in Bytes
unsigned long long CDriveSetup::getUnpartedDeviceSize(const int& device_num)
{
	unsigned long long start_cyl=0, end_cyl=0, used_cyl=0, rest_cyl=0, rest_size=0;

 	if (loadFdiskPartTable(device_num)) 
	{
		unsigned long long cyl_size 	= getFileEntryLong(PART_TABLE, "Units", 8);
		unsigned long long fullcyl 	= getFileEntryLong(PART_TABLE, "sectors/track", 4);
		string partname;

		for (int i = 0; i < MAXCOUNT_PARTS; i++)
		{
			partname 	= partitions[device_num][i];
			start_cyl 	= getFileEntryLong(PART_TABLE, partname, FDISK_INFO_START_CYL);
			end_cyl 	= getFileEntryLong(PART_TABLE, partname, FDISK_INFO_END_CYL);
			used_cyl 	+= start_cyl > 0 ? end_cyl - start_cyl + 1 : 0;
		}
		
		rest_cyl 	= fullcyl-used_cyl;
		rest_size 	= rest_cyl*cyl_size;
	}

	return rest_size;
}

// calc cylinders from megabytes
unsigned long long CDriveSetup::calcCyl(const int& device_num /*MASTER || SLAVE*/, const unsigned long long& bytes)
{

	unsigned long long cyl_max 	= device_cylcount[device_num];
	unsigned long long cyl_size	= device_cyl_size[device_num];
	unsigned long long size		= bytes;
	unsigned long long cyl_used	= size / cyl_size;

	// do not allow more then available cylinders and set cylinders to max value if bytes == 0
	if (size == 0)
	{
 		cout<<"[drive setup] "<<__FUNCTION__ <<": set cylinders to max = "<<cyl_max<<endl;
		return cyl_max;
	}
	else
		return cyl_used;
}

// create temp partable of current device from fdisk
bool CDriveSetup::loadFdiskPartTable(const int& device_num /*MASTER||SLAVE*/, bool use_extra /*using extra functionality table*/ )
{
	string device = drives[device_num].device; 	/*HDA||HDB*/

	if(access(device.c_str(), R_OK) !=0) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<device<<" not available... "<<endl;
		return false;
	}

	ofstream temp_file( TEMP_SCRIPT );
	if (!temp_file) 
	{ // Error while open
		cerr <<"[drive setup] "<<__FUNCTION__ <<": error while creating temporary part table "<< TEMP_SCRIPT <<" "<< strerror(errno)<<endl;
		return false;
	}

	temp_file <<DISCTOOL<<" "<<device<<" << EOF"<<endl;

	if (use_extra) 
	{
		temp_file <<"x"<<endl;
	}

	temp_file <<"p"<<endl;
	temp_file <<"q"<<endl;
	temp_file <<"EOF"<<endl;
	temp_file.close();

	if (chmod(TEMP_SCRIPT, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) !=0 ) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while setting permissions for "<< TEMP_SCRIPT <<" "<< strerror(errno)<<endl;
		return false;
	}

	string 	cmd = TEMP_SCRIPT;
		cmd += " > ";
		cmd += PART_TABLE;
		cmd += " ";
		cmd += DEVNULL;

	if (CNeutrinoApp::getInstance()->execute_sys_command(cmd.c_str())!=0) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while executing "<< TEMP_SCRIPT <<endl;
		return false;
	}

	// remove temp file
 	remove(TEMP_SCRIPT);

	return true;
}

// returns size of a partition from PROC_PARTITIONS as bytes,
// device_num is e.g MASTER or SLAVE, part_number means not the real number 1...n, use 0...3
// on empty part_number, get size from device eG. hda, hdb...
unsigned long long CDriveSetup::getPartSize(const int& device_num /*MASTER||SLAVE*/, const int& part_number)
{
	unsigned long long res = 0;

	string partname = (part_number !=-1) ? partitions[device_num][part_number] : drives[device_num].device;

	//normalize part name, remove "/dev/"
	partname.replace(partname.find("/dev/"), 5 ,"");

	if(access(PROC_PARTITIONS, R_OK) !=0) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": "<< PROC_PARTITIONS <<" not available..."<<endl;
		return 0;
	}

	res = getFileEntryLong(PROC_PARTITIONS, partname, 2)*1024 /*bytes*/;

	return res;
}

// returns informations about partitions from DISCTOOL
// device_num is e.g MASTER or SLAVE, part_number means not the real number 1...n, use 0...n, info_t_num is
// PARTINFO_TYPE_NUM, this is the required return value
// bool load_new_table is default set to true, set to false if no new fdisk table needed, table will generate for one time if it's not available
unsigned long long CDriveSetup::getPartData(const int& device_num /*MASTER||SLAVE*/, const int& part_number, const int& info_t_num /*START_CYL||END_CYL...*/, bool refresh_table)
{
	unsigned long long res = 0;

	// create temp fdisk partable of current device, fdisk table musst be minimum available one time 
	if ((access(PART_TABLE, R_OK) !=0) || (refresh_table)) 
	{
		if (!loadFdiskPartTable(device_num))
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<PART_TABLE<<" not loaded..."<<endl;
			return 0;
		}
	}

	// get partition name (dev/hda1...4)
	string partname = partitions[device_num][part_number];

	if(access(partname.c_str(), R_OK) !=0) 
	{
		//cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<partname<<" not found..."<<endl;
		return 0;
	}


	switch (info_t_num) 
	{
		case START_CYL:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_START_CYL);
			break;
		case END_CYL:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_END_CYL);
			break;
		case SIZE_BLOCKS:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_SIZE_BLOCKS);
			break;
		case ID:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_ID);
			break;
		case SIZE_CYL:
			res = device_cyl_size[device_num]; // bytes
			break;
		case COUNT_CYL:
			res = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_END_CYL) - getFileEntryLong(PART_TABLE, partname, FDISK_INFO_START_CYL) + 1;
			break;
		case PART_SIZE: // bytes
			unsigned long long count_cyl = getFileEntryLong(PART_TABLE, partname, FDISK_INFO_END_CYL) - getFileEntryLong(PART_TABLE, partname, FDISK_INFO_START_CYL) + 1;
			res = device_cyl_size[device_num] * count_cyl;
			break;
	}

 	if (refresh_table) // do not kill table, if we need it ! use parameter 4 = refresh_table for refreshing
		remove(PART_TABLE);

	return res;
}

// helper: return a selectable tab entry from file as unsigned long long
unsigned long long CDriveSetup::getFileEntryLong(const char* filename, const string& filter_entry, const int& column_num)
{
	string str_res = getFileEntryString(filename, filter_entry, column_num);

	if (str_res.empty())
		return 0;

	unsigned long long ret;
	stringstream Str;
	Str << str_res;
	Str >> ret;
	return ret;
}

// prepares or deletes a partition, device_num is e.g MASTER or SLAVE, action is EDIT_PARTITION_MODE_NUM, part_number means not the real number 1...n, use 0...n
bool CDriveSetup::mkPartition(const int& device_num /*MASTER||SLAVE*/, const action_int_t& action, const int& part_number, const unsigned long long& start_cyl, const unsigned long long& size)
{
	string device = drives[device_num].device; 	/*HDA||HDB||MMC*/
	bool ret = true;
	err[ERR_MK_PARTITION] = "";

	// get partition name (dev/hda1...4)
	string partname = partitions[device_num][part_number];

	string 	s_out = DISCTOOL;
		s_out += " "; 
		s_out += device +  " <<EOF\n";

	unsigned int part_n = part_number+1; // real part number is needed

	switch (action)
	{
		case ADD:
			{
			unsigned long long cyl_max = device_cylcount[device_num]; 		//requesting max cylinders of device
			unsigned long long cyl = calcCyl(device_num, size);	//calc cylinders from user definied size
			unsigned long long end_cyl;

			if (cyl == cyl_max)
				end_cyl = 0;  // 0 means only enter/default end_cyl
			else
				end_cyl = start_cyl + cyl;

			// search for next start_cyl
			for (int i = part_n; i < MAXCOUNT_PARTS - 1; i++)  // 0-3
			{
				if (data_partition[device_num][i+1].start_cyl > 0)
				{
					if (end_cyl >= data_partition[device_num][i+1].start_cyl)  // existing next partition, so set end_cyl < start_cyl of next partition
					{                                                // use default end_cyl
						end_cyl = 0;
						break;
					}
					break;
				}
			}

			s_out += "n\n";
			s_out += "p\n";

			/* if we have 3 detected Partitions, fdisk set last Partition automaticly to "X",
			   so set only the Partition if we have none, 1 or 2 detected Partitions */
			if (count_Partitions < 3)
				s_out += iToString(part_n) + "\n";
			s_out += iToString(start_cyl) + "\n";
			if (end_cyl == 0) //use default endzylinder instead of MAX, this is safer when insert a partition
				s_out += "\n";
			else
				s_out += iToString(end_cyl) + "\n";
			if ((string)d_settings.drive_partition_fstype[device_num][part_number] == "swap") //setting system id
			{
				s_out += "t\n";

				/* if we create and change the 1. Partition, fdisk need no indication of the Partition
				   we will change, because it is only one; counter of Partitions == 0,
				   do not write output to scriptfile */
				if (count_Partitions > 0)
					s_out += iToString(part_n) + "\n";
				s_out += "82\n";
			}
			break;
			}
		case DELETE:
			if (!unmountPartition(device_num, part_number))
			{
				err[ERR_MK_PARTITION] = err[ERR_UNMOUNT_PARTITION]+ "\n";
				err[ERR_MK_PARTITION] += g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_DELETE_FAILED);
				return false;
			}
			s_out += "d\n";

			/* if we delete the last Partition, fdisk need no indication of Partition
			   we will delete, because it is logical; counter of Partitions == 1,
			   do not write output to scriptfile */
			if (count_Partitions > 1)
				s_out += iToString(part_n) + "\n";
			break;
		case DELETE_CLEAN:
			if (!unmountPartition(device_num, part_number))
			{
				err[ERR_MK_PARTITION] = err[ERR_UNMOUNT_PARTITION]+ "\n";
				err[ERR_MK_PARTITION] += g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_DELETE_FAILED);
				return false;
			}
			s_out += "d\n";

			/* if we delete the last Partition, fdisk need no indication of Partition
			   we will delete, because it is logical; counter of Partitions == 1,
			   do not write output to scriptfile */
			if (count_Partitions > 1)
				s_out += iToString(part_n) + "\n";
			//reset settings
			strcpy(d_settings.drive_partition_fstype[device_num][part_number],"");
			d_settings.drive_partition_mountpoint[device_num][part_number] = "";
			strcpy(d_settings.drive_partition_size[device_num][part_number],"");
			break;
	}

	s_out += "w\n";
	s_out += "EOF\n";

	ofstream out( PREPARE_SCRIPT_FILE );
	if (!out) 
	{ // Error while open
		cerr <<"[drive setup] "<<__FUNCTION__ <<": error while preparing "<< PREPARE_SCRIPT_FILE<<" "<< strerror(errno)<<endl;
		err[ERR_MK_PARTITION] = "Error while preparing ";
		err[ERR_MK_PARTITION] +=  PREPARE_SCRIPT_FILE;
		return false;
	}
	out <<s_out<<endl;
	out.close();

	// PREPARE_SCRIPT_FILE must be executable
	if (chmod(PREPARE_SCRIPT_FILE, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) !=0 ) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while setting permissions for "<<PREPARE_SCRIPT_FILE<<" "<< strerror(errno)<<endl;
		err[ERR_MK_PARTITION] = "Error while setting permissions for ";
		err[ERR_MK_PARTITION] +=  PREPARE_SCRIPT_FILE;
		return false;
	}

	if (unmountPartition(device_num, part_number)) // unmount first !!
	{ 
		if (CNeutrinoApp::getInstance()->execute_sys_command(PREPARE_SCRIPT_FILE)!=0) 
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<": error while executing "<<PREPARE_SCRIPT_FILE<<endl;
			err[ERR_MK_PARTITION] = "Error while executing ";
			err[ERR_MK_PARTITION] +=  PREPARE_SCRIPT_FILE;
			ret = false;
		}

		if ((action==DELETE) || (action==DELETE_CLEAN))
		{
			if (isActivePartition(partname)) 
			{ // partition was deleted but part table is not current, reboot is requiered
				bool reboot = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_HDD_EDIT_PARTITION, LOCALE_DRIVE_SETUP_MSG_REBOOT_REQUIERED, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_INFO, width) == CMessageBox::mbrYes);
				if (reboot) 
				{	
					CNeutrinoApp::getInstance()->exec(NULL, "reboot");
				}
				ret = true; //return success
			}
		}
	}
	else 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while unmounting " <<partname<<endl;
		err[ERR_MK_PARTITION] = err[ERR_UNMOUNT_PARTITION];
		ret = false;
	}

	remove(PREPARE_SCRIPT_FILE);
	return ret;
}

// gets the path of needed files from /var/etc (main_file from /var) or /etc (default_file) depends of writable filesystem
string CDriveSetup::getFilePath(const char* main_file, const char* default_file)
{
	long int fsnum = getDeviceInfo(ETC_DIR, FILESYSTEM);
	if ((fsnum != 0x28cd3d45 /*cramfs*/) && (fsnum != 0x073717368 /*squashfs*/)) 
	{
		remove(main_file);
		return default_file; // we have a writeable dir
	}
	else
		return main_file;
}

// gets the path of initfile for loading modules and other hdd stuff
string CDriveSetup::getInitIdeFilePath()
{
	return getFilePath(INIT_IDE_VAR_SCRIPT_PATH, INIT_IDE_SCRIPT_PATH);
}

// gets the path of initfile for mounts
string CDriveSetup::getInitMountFilePath()
{
	return getFilePath(INIT_MOUNT_VAR_SCRIPT_FILE, INIT_MOUNT_SCRIPT_FILE);
}

// gets the path of fstab file
string CDriveSetup::getFstabFilePath()
{
	return getFilePath(FSTAB_VAR, FSTAB);
}

#ifdef ENABLE_NFSSERVER
// gets the path of exports file
string CDriveSetup::getExportsFilePath()
{
	return getFilePath(EXPORTS_VAR, EXPORTS);
}
#endif

#ifdef ENABLE_SAMBASERVER
// gets the path of initfile for mounts
string CDriveSetup::getInitSmbFilePath()
{
	return getFilePath(INIT_SAMBA_VAR_SCRIPT_FILE, INIT_SAMBA_SCRIPT_FILE);
}

// gets a writable path of smb.conf
string CDriveSetup::getSmbConfFilePath()
{
	return getFilePath(SMBCONF_VAR, SMBCONF);
}
#endif

// formats a partition, 1st parameter "device_num" is MASTER, SLAVE..., 3rd parameter "filesystem" means a name of filesystem as string eg, ext3...
bool CDriveSetup::mkFs(const int& device_num /*MASTER||SLAVE*/, const int& part_number,  const std::string& fs_name)
{
	// get partition name (dev/hda1...4)
	string partname = partitions[device_num][part_number];
	err[ERR_MK_FS] = "";
	//TODO show correct progress status, it's not real at time ;-( 
	showStatus(40, "creating Filesystem, please wait...", 100);

	if (isActivePartition(partname))
	{	
		if (fs_name.empty())
		{
			err[ERR_MK_FS] = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_FAILED_NO_FS_DEFINIED);
			return false;
		}
	}

	string mkfs_cmd;

	// ensure: load fs drivers
	if (!initFsDrivers(false)) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": formating partition failed..."<<endl;
		err[ERR_MK_FS] = "mkfs "+ partname + " failed\n" + err[ERR_INIT_FSDRIVERS];
		return false;
	}

	for (uint i = 0; i < MAXCOUNT_FSTYPES; i++)
	{
		if (fs_name==fstype[i].fsname) 
 		{
			string opts = char(32) + (string)d_settings.drive_fs_format_option[i]/*fstype[i].mkfs_options*/ + char(32);

			if (fs_name == "swap")
				mkfs_cmd = MKSWAP + opts;
			else
 				mkfs_cmd = MKFSPREFIX + fstype[i].fsname + opts;
		}	
	}

	bool is_active = isActivePartition(partname);
	
	if (is_active) 
	{
		string cmdformat;
	
		if (fs_name == "swap") // make swap
		{
			cmdformat  = mkfs_cmd;
			cmdformat += partname;
			cmdformat += DEVNULL;

			if (CNeutrinoApp::getInstance()->execute_sys_command(cmdformat.c_str())!=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": mkswap at "<<partname<< " failed..."<<endl;
				err[ERR_MK_FS] = "mkswap for "+ partname + " failed";
				return false;
			}
		}
		else // make filesystem
		{
			cmdformat  = mkfs_cmd + char(32) + partname;;
			//cmdformat += DEVNULL;

			if (CNeutrinoApp::getInstance()->execute_sys_command(cmdformat.c_str())!=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": make filesystem "<<fs_name<< " for "<<partname<< " failed..."<<endl;
				err[ERR_MK_FS] = "mkfs for "+ partname + " failed";
				return false;
			}
		}
	}
	else 
	{
		// partition is not active
		cerr<<"[drive setup] "<<__FUNCTION__ <<": make filesystem failed "<<partname<< " is not active!..."<<endl;
		err[ERR_MK_FS] = "Can't make filesytem at "+ partname;
		return false;
	}

	return true;
}

// check fs of a partition, 1st parameter "device_num" is MASTER, SLAVE..., 3rd parameter "filesystem" means a name of filesystem as string eg, ext3...
bool CDriveSetup::chkFs(const int& device_num, const int& part_number,  const std::string& fs_name)
{
	bool ret = true;
	err[ERR_CHKFS] = "";
	//TODO show correct progress or message
	showStatus(50, "creating Filesystem, please wait...", 100);
	CProgressBar pb(false);

	// get partition name (dev/hda1...4)
	string partname = partitions[device_num][part_number];


	if (!unmountPartition(device_num, part_number)) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": umounting of: "<<partname<< " failed"<<endl;
		err[ERR_CHKFS] = err[ERR_UNMOUNT_PARTITION];
		return false;
	}
	
	string chkfs_cmd;

	for (uint i = 0; i < MAXCOUNT_FSTYPES; i++)
	{
		if (fs_name==fstype[i].fsname) 
 		{
			string opts = char(32) + fstype[i].fsck_options + char(32);

			if (fs_name == "swap")
				return true;
			else
 				chkfs_cmd = CKFSPREFIX + fstype[i].fsname + opts;
		}	
	}

	fb_pixel_t * pixbuf = new fb_pixel_t[pb_w * pb_h];
	if (pixbuf != NULL)
		frameBuffer->SaveScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);

	showStatus(70, "checking Filesystem...", 100);
	
	bool is_active = isActivePartition(partname);
	string 	cmd_check;
	
	if (is_active) 
	{
		// check filesystem
		{
			cmd_check  = chkfs_cmd + char(32) + partname;

			if (CNeutrinoApp::getInstance()->execute_sys_command(cmd_check.c_str())!=0) 
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<": checked filesystem with: "<<cmd_check<< "...with errors"<<endl;
				err[ERR_CHKFS] = "Checked filesystem with errors!";
			}
		}
	}
	else 
	{
		// partition is not active
		cerr<<"[drive setup] "<<__FUNCTION__ <<": checking filesystem with: "<<cmd_check<<" failed,\n"<< partname<<" is not active!"<<endl;
		err[ERR_CHKFS] = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_CHECK_FAILED);
		ret = false;
	}

	if (pixbuf != NULL) 
	{
		frameBuffer->RestoreScreen(pb_x, pb_y, pb_w, pb_h, pixbuf);
		delete[] pixbuf;
	}

	return ret;
}

#ifdef ENABLE_NFSSERVER
// generate exports file, returns true on success
bool CDriveSetup::mkExports()
{
	err[ERR_MK_EXPORTS] = "";
	// set exports path
	string exports = getExportsFilePath();
	string timestamp = getTimeStamp();
	vector<string> v_export_entries;
	string head = "# " + exports + " generated from neutrino ide/mmc/hdd drive-setup\n# " +  getDriveSetupVersion() + " " +  timestamp + "\n";

	// collecting export entries
	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
		{
			string partname = partitions[i][ii];
			string export_entry;
			//collects all mountpoints but not swap
			if (d_settings.drive_partition_nfs[i][ii] && !isSwapPartition(partname)) 
			{
				string mp = getMountInfo(partname, MOUNTPOINT);
				export_entry = mp;
				export_entry += " ";
				export_entry += d_settings.drive_partition_nfs_host_ip[i][ii];
				export_entry += "(rw,sync,no_subtree_check)";
				v_export_entries.push_back(export_entry);
			}
		}
	}

	// write exports
	if (!v_export_entries.empty())
	{
		ofstream str_exports(exports.c_str());
		if (!str_exports) 
		{ // Error while open
			cerr << "[drive setup] "<<__FUNCTION__ <<": write error "<<exports<<", please check permissions..." << strerror(errno)<<endl;
			err[ERR_MK_EXPORTS] += g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_MAKE_EXPORTS);
			return false;
		}
		else 
		{
			str_exports << head <<endl;

			for (unsigned int i = 0; i < v_export_entries.size(); i++) 
			{
				str_exports << v_export_entries[i] <<endl;
			}
		}
		str_exports.close();
		cout<<"[drive setup] "<<__FUNCTION__ <<": writing "<<exports<< "...ok"<<endl;
	}

	else
	{
		if (access(exports.c_str(), R_OK) == 0)
		{
			if (unlink(exports.c_str()) != 0)
				cerr << "[drive setup] "<<__FUNCTION__ <<": delete "<<exports<<" ..." << strerror(errno)<<endl;
		}	
	}


	return true;
}
#endif	

#ifdef ENABLE_SAMBASERVER
// generate samba config file, returns true on success
bool CDriveSetup::mkSmbConf()
{
	err[ERR_MK_SMBCONF] = "";

	// get a writeable path to smb.conf
	string smbconf = getSmbConfFilePath();

	// next: removing old samba configfile
	if ( access(smbconf.c_str(), R_OK) ==0 )
	{
		int res = remove(smbconf.c_str());
		if (res !=0)
		{	string  err_msg = "Error while deleting\n";
				err_msg += smbconf + "\n";
				err_msg += strerror(res);
			cerr << "[drive setup] "<<__FUNCTION__ <<": "<<err_msg<<endl;
			err[ERR_MK_SMBCONF] = err_msg;
			return false; 
		}
	}

	// check if samba feature is enabled and exit on OFF and return true, otherwise goto next
	if (g_settings.smb_setup_samba_on_off == CSambaSetup::OFF)
		return true;

	// genearting usefull informations for global samba configurations file 
	static CImageInfo imaginfo;

	// head entries 
  	string smb_head = "; " + smbconf + " generated by neutrino ide/mmc/hdd drive-setup, do not edit!\n; " +  getDriveSetupVersion() + " " +  getTimeStamp();
	
	// all conf entries
	string smb_entries;

	// globals
	// note! settings for network-interface and server string will be generated automaticly, workgroup is changable from user in samba menue 
	string  smb_auto = "interfaces = ";
		smb_auto += getInterface() + "\n";
		smb_auto += "workgroup = ";
		smb_auto +=  g_settings.smb_setup_samba_workgroup + "\n";
		smb_auto += "server string = Samba@";
		smb_auto += imaginfo.getImageInfo(IMAGENAME);

	// names and default values of global settings, 
	string smb_globals[] = {  smb_head,
 				 "[global]",
				  smb_auto,
				 "netbios name = dbox", //note! most of these following settings are static and not changable by user
				 "security = share",
				 "load printers = no",
				 "guest ok = yes",
				 "guest account = root",
				 "encrypt passwords = yes",
				 "os level = 0",
				 "log level = 0",
				 "browseable = yes",
				 "preserve case = yes",
				 "short preserve case = yes",
				 "character set = iso8859-1"};

 	// collecting share entries
	string smb_shares;

 	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
 	{
 		for (uint ii = 0; ii < MAXCOUNT_PARTS; ii++) 
 		{
			if (d_settings.drive_partition_samba[i][ii]) 
 			{
				smb_shares += "[" + d_settings.drive_partition_samba_share_name[i][ii] +"]\n";	//share name
				smb_shares += "comment = ";
				smb_shares += d_settings.drive_partition_samba_share_comment[i][ii] + "\n";//comment
  				smb_shares += "path = "; 
				smb_shares += d_settings.drive_partition_mountpoint[i][ii] + "\n"; //path
 				smb_shares += "read only = ";
				smb_shares += d_settings.drive_partition_samba_ro[i][ii] ? "yes\n" : "no\n"; //read only
				smb_shares += "public = ";
			 	smb_shares += d_settings.drive_partition_samba_public[i][ii]  ? "yes\n" : "no\n"; //public
  			}
 		}
 	}

	// merge global and share entries
	// check for definied shares
	if (smb_shares.empty())
	{
		err[ERR_MK_SMBCONF] += g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_NO_SMB_SHARE_DEFINIED);;
		return false;
	}
	else
	{
		for (uint j = 0; j < (sizeof(smb_globals) / sizeof(smb_globals[0])); j++)
		{
			smb_entries += smb_globals[j] + "\n";;
		}
		smb_entries += smb_shares;
	}

 	// writing smb.conf
	// NOTE: do only write smb.conf file to a writable directotry /etc or for squashfs to /var/etc!
	// if /etc/smb.conf file is not writeable, it must be a link to /var/etc/smb.conf! 
	ofstream str_smb(smbconf.c_str());
	if (!str_smb) 
	{ // Error while open
		cerr << "[drive setup] "<<__FUNCTION__ <<": write error "<<smbconf<<", please check permissions..." << strerror(errno)<<endl;
		err[ERR_MK_SMBCONF] += g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_MAKE_SMBCONF);
		return false;
	}
	else 
		str_smb << smb_entries <<endl;

	str_smb.close();

	cout<<"[drive setup] "<<__FUNCTION__ <<": writing "<<smbconf<< "...ok"<<endl;
 
	return true;
}

//unlink samba server initfiles 
bool CDriveSetup::unlinkSmbInitLinks()
{
	err[ERR_UNLINK_SMBINITLINKS] = "Error while unlink ";
	bool ret = true;

	string if_path = getInitSmbFilePath(); //init file path

	string symlinks[] = {	getInitSmbFilePath().insert(if_path.rfind("/")+1, "S"), 
				getInitSmbFilePath().insert(if_path.rfind("/")+1, "K")};

	// unlinking old symlinks
	for (uint i=0; i<(sizeof(symlinks) / sizeof(symlinks[0])) ; ++i)
	{
		if ( access(symlinks[i].c_str(), W_OK) ==0 )
		{
			if (unlink(symlinks[i].c_str()) !=0)
			{
				err[ERR_UNLINK_SMBINITLINKS] += symlinks[i] + "\n" + strerror(errno) + "\n";
				cerr<<"[drive setup] "<<__FUNCTION__ <<":  "<<err[ERR_UNLINK_SMBINITLINKS]<<endl;
				ret = false;
			}
			else
				cout<<"[drive setup] unlink of "<<symlinks[i]<< " ...ok"<<endl;
		}
	}
	
	return ret;
}

//linking sambaserver initfiles
bool CDriveSetup::linkSmbInitFiles()
{
	err[ERR_LINK_SMBINITFILES] = "Error while linking samba initfiles\n";
	bool ret = true;

	// exit if samba is disabled
	if (g_settings.smb_setup_samba_on_off == CSambaSetup::OFF)
		return true; 

	string if_path = getInitSmbFilePath(); //init file path

	if ( access(if_path.c_str(), R_OK) !=0 )
	{
		err[ERR_LINK_SMBINITFILES] += if_path + " " + "not found" + "\n";
		cerr<<"[drive setup] "<<__FUNCTION__ <<":  "<<err[ERR_LINK_SMBINITFILES]<<endl;
		return false;
	}
		 

	string symlinks[] = {	getInitSmbFilePath().insert(if_path.rfind("/")+1, "S"), 
				getInitSmbFilePath().insert(if_path.rfind("/")+1, "K")};	

	
	for (uint i=0; i<(sizeof(symlinks) / sizeof(symlinks[0])) ; ++i)
	{
		// create symmlink
		if ( access(symlinks[i].c_str(), R_OK) !=0 )
		{
			if (symlink(if_path.c_str(),symlinks[i].c_str()) !=0)
			{
				err[ERR_LINK_SMBINITFILES] += symlinks[i] + "\n" + strerror(errno) + "\n";
				cerr<<"[drive setup] "<<__FUNCTION__ <<":  "<<err[ERR_LINK_SMBINITFILES]<<endl;
				ret = false;
			}
			else
				cout<<"[drive setup] linking "<<if_path<<"-->"<<symlinks[i]<< " ...ok"<<endl;
		}
	}
	
	return ret;
}

//writes sambaserver initfile
bool CDriveSetup::mkSambaInitFile()
{
	err[ERR_MK_SMBINITFILE] = "";
	bool ret = true;
	string if_path = getInitSmbFilePath(); //init file path
	string timestamp = getTimeStamp();
	string head = "# " + if_path + " generated from neutrino ide/mmc/hdd drive-setup\n# " +  getDriveSetupVersion() + " " +  timestamp + "\n";
	string samba_conf_file = SMBCONF;

	// remove old samba startscript
	if ( access(if_path.c_str(), R_OK) ==0 )
	{
		int res = remove(if_path.c_str());
		if (res !=0)
		{	string  err_msg = "Error while deleting\n";
				err_msg += if_path + "\n";
				err_msg += strerror(res);
			cerr << "[drive setup] "<<__FUNCTION__ <<": "<<err_msg<<endl;
			err[ERR_MK_SMBINITFILE] = err_msg;
			return false; 
		}
	}

	// exit if samba is disabled
	if (g_settings.smb_setup_samba_on_off != CSambaSetup::ON)
		return true;

	ofstream out;
	out.open(if_path.c_str());

	if (!out) 
	{	 // Error while open
		cerr << "[samba setup] "<<__FUNCTION__ <<": error "<<if_path<<"..."<< strerror(errno)<<endl;
		err[ERR_MK_SMBINITFILE] += "Error while creating " + if_path + "\n" + strerror(errno);
		return false;
	}
	out << "#!/bin/sh"<<endl;
	out << head <<endl;
	out << "case $1 in"<<endl;
	out << "	start)"<<endl;
	out << "		if [ -e "<<SAMBA_MARKER<<" -a -e "<<samba_conf_file<<" ]; then"<<endl;
	out << " 			"<<NMBD<<" -D"<<endl;
	out << " 			"<<SMBD<<" -D -a -s "<<samba_conf_file<<endl;
	out << " 		fi"<<endl;
	out << "		;;"<<endl;
	out << " 	stop)"<<endl;
	out << " 		killall "<<SMBD<<endl;
	out << " 		killall "<<NMBD<<endl;
	out << " 		;;"<<endl;
	out << "esac"<<endl;

	out << "exit 0"<<endl;
	out.close();

	// INIT_IDE_SCRIPT_PATH must be executable
	int chmod_ret = chmod(if_path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

	if ( chmod_ret !=0 )
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": "<<if_path<<" "<<strerror(errno)<<endl;
		err[ERR_MK_SMBINITFILE] += "Error while setting permissions for " + if_path + "\n" + strerror(errno);
		ret = false;
	}

	return ret;
}
#endif

// mounts all available partitions for all devices 
bool CDriveSetup::mountAll(const bool force_mount)
{
	bool ret = true;
	string err_msg;
	err[ERR_MOUNT_ALL] = "";

	for (unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		if (!mountDevice(i, force_mount))
		{
			err_msg += "\n";
			err_msg += err[ERR_MOUNT_DEVICE];
			ret = false;
		}
	}			

	if (!ret)
		err[ERR_MOUNT_ALL] = err_msg;

	return ret;
}

// mounts all available partitions on device
bool CDriveSetup::mountDevice(const int& device_num, const bool force_mount)
{
	bool ret = true;
	int i = device_num;
	string err_msg;
	err[ERR_MOUNT_DEVICE] = "";

	for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
	{
		string partname = partitions[i][ii];
		if (d_settings.drive_partition_activ[i][ii] || force_mount) 
		{// mount only if option is set to activ
			if (isActivePartition(partname))
			{
				if (!mountPartition(i, ii, d_settings.drive_partition_fstype[i][ii], d_settings.drive_partition_mountpoint[i][ii]))
				{
					err_msg += g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_MOUNT_DEVICE);
					err_msg += "\n";
					err_msg += g_Locale->getText(mn_data[i].entry_locale);
					err_msg += + " / Partition: "; 
					err_msg += iToString(ii+1);
					err_msg += "\n"; 
					err_msg += err[ERR_MOUNT_PARTITION];
					
					ret = false;
				}
			}
		}
	}			
	
	if (!ret)
		err[ERR_MOUNT_DEVICE] = err_msg;

	return ret;
}


// mounting partitions, returns true on success, use force_mount if it's usefull to mount also a disabled partition 
bool CDriveSetup::mountPartition(const int& device_num /*MASTER||SLAVE*/, const int& part_number,  const std::string& fs_name, const std::string& mountpoint, const bool force_mount)
{
	string mp = mountpoint;
	err[ERR_MOUNT_PARTITION] = "";

	// get partition name (dev/hda1...4)
	string partname = partitions[device_num][part_number];

	// prepare user script
	char user_script[64];
	snprintf(user_script, 64, "%s/after_mount_%d_%d.sh", CONFIGDIR, device_num, part_number);
	user_script[63] = '\0'; /* ensure termination... */

	// return true and exit if settings force_mount = false
	if (!force_mount)
	{
		if (!d_settings.drive_partition_activ[device_num][part_number]) 
			return true;
	}

	// exit if no available
	if((access(partname.c_str(), R_OK) !=0)) 	
	{
		cout<<"[drive setup] "<<__FUNCTION__ <<":  "<<partname<< " partition not available, nothing to do...ok"<< endl;
 		return true;
	}

	// do mount or swapon
	if (isActivePartition(partname))
	{
		//exit if it's already mounted or swap
		if (isSwapPartition(partname) || isMountedPartition(partname))
 			return true;

		//swapon	
		//swapon first with normal settings
		if (fs_name == "swap" || mp == "none")
		{
			if (swapon(partname.c_str(), 0/*SWAPFLAGS=0*/) != 0)
			{ 
				cerr<<"[drive setup] "<<__FUNCTION__ <<":  swapon: "<<strerror(errno)<< " " << partname<<endl;
				err[ERR_MOUNT_PARTITION] = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_MOUNT_SWAP);
				strcpy(d_settings.drive_partition_fstype[device_num][part_number], "");
				d_settings.drive_partition_mountpoint[device_num][part_number] = "";
				return false;
			}
			else
			{	
				char size_opt[10];
				//use real size of partition for swap size, not setting /*d_settings.drive_partition_size[device_num][part_number]*/
				snprintf(size_opt, 10, "size=%sM", iToString(getPartSize(device_num,part_number)/1024/1024).c_str()); 
				if (mount("tmpfs", "/tmp" , NULL, MS_MGC_VAL | MS_REMOUNT, size_opt) !=0 )
				{
					cerr<<"[drive setup] "<<__FUNCTION__ <<":  mount: "<<strerror(errno)<<endl;
					err[ERR_MOUNT_PARTITION] += "\nError while remounting /tmp!";
				}

				strcpy(d_settings.drive_partition_fstype[device_num][part_number], "swap");
				d_settings.drive_partition_mountpoint[device_num][part_number] = "none";

				//executing user script if available after swapon
				if (my_system(user_script) != 0)
					perror(user_script);

				return true;
			}
		}

		//force swapon without settings
		if (fs_name.empty() || mp.empty())
		{
			if (swapon(partname.c_str(), 0/*SWAPFLAGS=0*/) == 0)
			{
				strcpy(d_settings.drive_partition_fstype[device_num][part_number], "swap");
				d_settings.drive_partition_mountpoint[device_num][part_number] = "none";

				//executing user script if available after swapon
				if (my_system(user_script) != 0)
					perror(user_script);

				return true;
			}
		}
		
		//swap is not mounted yet, then we must mount with mountpoints
		//creating default or possible mountpoints for empty settings
		if (mp.empty())
			mp = getMountPoint(device_num, part_number);
			
		//check mountpoint, if invalid or no mountpoint definied then generate an error message
		DIR   *mpCheck;
		mpCheck = opendir(mp.c_str());
		
		if (mpCheck == NULL) //exit on invalid mountpoint
		{
			cerr<<"[drive setup] "<<__FUNCTION__ <<":  invalid mountpoint" << mp<<endl;
			err[ERR_MOUNT_PARTITION] = mp + " " + g_Locale->getText(LOCALE_DRIVE_SETUP_PARTITION_MOUNT_NO_MOUNTPOINT);
			return false;
		}
		else
		{
			closedir( mpCheck );
			if (d_settings.drive_warn_jffs)
			{
				//checking mountpoint of selected mountpoint, it's dangerous if we using jffs2!
				long l_fs = getDeviceInfo(mp.c_str(), FILESYSTEM);
				if (l_fs == 0x72b6 /*jffs2*/ && access(mp.c_str(), W_OK) ==0)
				{
					//create warn message
					string s_fs = getFsTypeStr(l_fs);
					char warn_msg[255];
					snprintf(warn_msg, 255, g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_MOUNT_WARNING), mp.c_str(), s_fs.c_str());
					if ((ShowMsgUTF(LOCALE_MESSAGEBOX_WARNING, warn_msg, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_ERROR, width, -1))) // UTF-8
					{	
						d_settings.drive_partition_mountpoint[device_num][part_number] = mp;
						strcpy(d_settings.drive_partition_fstype[device_num][part_number],  s_fs.c_str());
						return true;
					}
					//ask user for mount warnings
					d_settings.drive_warn_jffs = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_PARTITION_MOUNT_NOW, LOCALE_DRIVE_SETUP_MSG_WARN_AGAIN, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_QUESTION, width) == CMessageBox::mbrNo) ? true:false; 
				}
			}	
		}

		//check filesystem
		if(fs_name.empty() && !force_mount)
		{
			err[ERR_MOUNT_PARTITION] =  g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_PARTITION_CREATE_FAILED_NO_FS_DEFINIED);
			return false;
		}

		 //load first the fs modul
		if (fs_name != "swap" && !fs_name.empty())
			initModul(fs_name, false);

		//mounting
		//TODO mount options 
		//mount first with user settings, if it fails... 
		if (mount(partname.c_str(),  mp.c_str() , fs_name.c_str(), 16 , NULL)!=0) 
		{
			int i = v_fs_modules.size()-1;
			bool ret = false;

			//...then mount with other available filesystems, exit on success and correcting user settings
			while (i != -1)
			{
				if (mount(partname.c_str(),  mp.c_str() , v_fs_modules[i].c_str(), 16 , NULL)==0)
				{
					//save fs and mountpoint!
					strcpy(d_settings.drive_partition_fstype[device_num][part_number], v_fs_modules[i].c_str());
					d_settings.drive_partition_mountpoint[device_num][part_number] = mp;
					ret = true;
					break;
				}
				i--;
			}
			
			//if we have no success, generate an error message and return false.
			if (!ret)
			{
				cerr<<"[drive setup] "<<__FUNCTION__ <<":  error["<<errno<< "] while mount: " << partname<<" "<<strerror(errno)<<endl;
				err[ERR_MOUNT_PARTITION] = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CANNOT_MOUNT_PARTITION);
				return false;
			}				
		}
 	}
	
	//executing user script if available after mounting
	if (my_system(user_script) != 0)
		perror(user_script);

	return true;
}

//returns a free mountpoint 
string CDriveSetup::getMountPoint(const int& device_num, const int& part_number)
{
	string ret;
	int dev = device_num;
	int p_num = part_number > 0 ? part_number-1 : part_number;
	
	string partname = partitions[dev][p_num];
	string mnt_dir = "/mnt";

	string mnt_name[MAXCOUNT_DRIVE] = { mnt_dir + "/hdd1", mnt_dir + "/hdd2", mnt_dir + "/mmc"};

	if (isSwapPartition(partname) || p_num == 0)
		ret = mnt_name[dev];
	else
		ret = mnt_name[dev] + "_" + iToString(p_num+1);

	DIR   *mp;
	mp = opendir(ret.c_str());

	if (mp)
	{
		closedir(mp);
		return ret;
	}
	else
	{
		if (getDeviceInfo(mnt_dir.c_str(), FILESYSTEM) != 0x72b6 /*jffs2*/)
			mnt_dir = "/var" + ret;
		else
			mnt_dir = ret;

		mkdir(mnt_dir.c_str(), 0555);
		ret = mnt_dir;
	}

 	return ret;
}

// returns status of mmc, returns true if is active
bool CDriveSetup::isMmcActive()
{
	if ((isModulLoaded(M_MMC)) || (isModulLoaded(M_MMC2)) || (isModulLoaded(M_MMCCOMBO)))
		return true;
	else
		return false;
}

//return true if mmc is enabled
bool CDriveSetup::isMmcEnabled()
{
	if ((string)d_settings.drive_mmc_module_name != g_Locale->getText(LOCALE_OPTIONS_OFF))
		return true;
	else
		return false;
}

// returns status of ide interface, returns true if is active
bool CDriveSetup::isIdeInterfaceActive()
{
	if ((isModulLoaded(DBOXIDE)) && (isModulLoaded(IDE_CORE)) && (isModulLoaded(IDE_DETECT)) && (isModulLoaded(IDE_DISK)))
		return true;
	else
		return false;
}


// returns current time string
string CDriveSetup::getTimeStamp()
{
	char ret[22];
	time_t now = time(0);
	struct tm lt;
	localtime_r(&now, &lt);
	strftime(ret, sizeof(ret), "%d.%m.%Y - %H:%M:%S", &lt);

	return (string)ret;
}

// returns a revision string
string CDriveSetup::getDriveSetupVersion()
{
	static CImageInfo imageinfo;
	return imageinfo.getModulVersion("","$Revision: 1.94 $");
}

// returns text for initfile headers
string CDriveSetup::getInitFileHeader(string& filename)
{
	string timestamp = getTimeStamp();
	// set  head lines for init file
	string	s_init = "#!/bin/sh\n";
		s_init += "echo ";
		s_init += char(34);
		s_init += filename + " generated from neutrino ide/mmc/hdd drive-setup\n";
		s_init += timestamp + "\n";
		s_init += getDriveSetupVersion();
		s_init += " do not edit!";
		s_init += char(34);

	return s_init;
}


// returns commands for mount init file
string CDriveSetup::getInitFileMountEntries()
{
	string mount_entries;
	string swapon_entries;
	string umount_entries;
	string swapoff_entries;

	for(int i = 0; i < MAXCOUNT_DRIVE; i++)
	{
		for(int ii = 0; ii < MAXCOUNT_PARTS; ii++)
		{
			string partname = partitions[i][ii];
			string mp = getMountInfo(partname, MOUNTPOINT);
			if (isMountedPartition(partname)) 
			{		
				string fs_opt = "-t " + (string)d_settings.drive_partition_fstype[i][ii] + " ";
				if (!d_settings.drive_use_fstab)
					mount_entries += "\t\t" MOUNT + fs_opt + partname + " " + mp +"\n";

				umount_entries += "\t\t" UMOUNT + mp + "\n";
			}
			else if (isSwapPartition(partname))
			{
				if (!d_settings.drive_use_fstab)
				{
					string swap_size = iToString(getPartSize(i,ii)/1024/1024); //use real size of partition for swap size /*d_settings.drive_partition_size[i][ii]*/
					swapon_entries += "\t\t" SWAPON + partname + " && " + MOUNT + "-n -t tmpfs tmpfs /tmp -o size=" + swap_size  + "M,remount\n";
					swapoff_entries += "\t\t" SWAPOFF + partname + "\n";
				}
			}
		}
	}

	if (d_settings.drive_use_fstab)
	{
		mount_entries = "\t\t"; 
		mount_entries += MOUNT;
		mount_entries += "-a\n";

		swapon_entries = "\t\t";
		swapon_entries += SWAPON;
		swapon_entries += "-a\n";

		swapoff_entries = "\t\t";
		swapoff_entries += SWAPOFF;
		swapoff_entries += "-a\n";		
	}

	string 	m_txt =  "case $1 in\n";
		m_txt += "\tstart)\n";
		m_txt += mount_entries;
		m_txt += swapon_entries;
		m_txt += "\t\t;;\n";
		m_txt += "\tstop)\n";
		m_txt += umount_entries;
		m_txt += swapoff_entries;
		m_txt += "\t\t;;\n";
		m_txt += "esac\n";
		m_txt += "exit 0";

	return m_txt;
}


// returns commands for modul init file
string CDriveSetup::getInitFileModulEntries(bool with_unload_entries)
{
	string load_entries;
	string unload_entries = "\t\t";

	// add commands for loading the filesystem modules
	for (unsigned int i=0; i<(v_init_fs_L_cmds.size()) ; ++i) 
	{
		load_entries += v_init_fs_L_cmds[i];
		load_entries += "\n\t\t";
		
	}

	// add init commands to enable the ide interface
	if (d_settings.drive_activate_ide != IDE_OFF)
	{
		for (unsigned int i=0; i<(v_init_ide_L_cmds.size()) ; ++i) 
		{
			load_entries += v_init_ide_L_cmds[i];
			load_entries += "\n\t\t";
		}
	}

	//add commands to activate mmc
	load_entries += s_init_mmc_cmd;
	load_entries += "\n\t\t";

	// add hdparm commands for writecache and spindown
	for (unsigned int i=0; i<(v_hdparm_cmds.size()) ; ++i) 
	{
		load_entries += v_hdparm_cmds[i];
		load_entries += "\n\t\t";
	}

	//generate unload commands only if with_unload_entries is set to true
	if (with_unload_entries)
	{
		//add unload commands to disable the ide interface
		for (unsigned int i=IDE_MODULES_COUNT; i>0 ; i--) 
		{
			unload_entries += UNLOAD + ide_modules[i-1].modul;
			unload_entries += "\n\t\t";
		}
	
		//add unload mmc command
		unload_entries += UNLOAD + getUsedMmcModulName();
		unload_entries += "\n\t\t";
	
		//add unload commands to unload fs modules
		for (unsigned int i=0; i<(v_fs_modules.size()) ; ++i) 
		{
			if (v_fs_modules[i] != "swap") 
			{
				unload_entries += UNLOAD + v_fs_modules[i];
				unload_entries += "\n\t\t";
			}
		}
	
	
		unload_entries += UNLOAD ;
		unload_entries += "jbd\n\t\t";
		unload_entries += UNLOAD;
		unload_entries += "vfat\n";
	}


	string 	e_txt =  "case $1 in\n";
		e_txt += "\tstart)\n\t\t";
		e_txt += load_entries;
		e_txt += ";;\n";
		e_txt += "\tstop)\n";
		e_txt += unload_entries; //optional
		e_txt += ";;\n";
		e_txt += "esac\n";
		e_txt += "exit 0";

	return e_txt;

}

#define INIT_SCRIPTS_COUNT 2

// linking initfiles and returns true on success
bool CDriveSetup::linkInitFiles()
{
	err[ERR_LINK_INITFILES] = "Error while linking initfiles:\n";

	string init_dir;
	string init_hdd_file = getInitIdeFilePath();
	string init_mount_file = getInitMountFilePath();

	long int fsnum = getDeviceInfo(ETC_DIR, FILESYSTEM);
	if ((fsnum != 0x28cd3d45 /*cramfs*/) && (fsnum != 0x073717368 /*squashfs*/)) 
		init_dir = INIT_D_DIR; // /etc/init.d is writeable, use this!
	else 
		init_dir = INIT_D_VAR_DIR; // /etc/init.d is not writeable, use /var/etc/init.d!

	string scripts[INIT_SCRIPTS_COUNT] = {	init_hdd_file, 
						init_mount_file}; 

	string symlinks[INIT_SCRIPTS_COUNT][INIT_SCRIPTS_COUNT] = {{init_dir + "/S" + INIT_IDE_SCRIPT_NAME, init_dir + "/K" + INIT_IDE_SCRIPT_NAME},
								  {init_dir + "/S" + INIT_MOUNT_SCRIPT_NAME, init_dir + "/K" + INIT_MOUNT_SCRIPT_NAME}}; 

	for (unsigned int i=0; i<(INIT_SCRIPTS_COUNT) ; ++i)
	{
		for (unsigned int ii=0; ii<(INIT_SCRIPTS_COUNT) ; ++ii)
		{
			if ( access(symlinks[i][ii].c_str(), R_OK) ==0 )
			{
				if (unlink(symlinks[i][ii].c_str()) !=0)
				{
					err[ERR_LINK_INITFILES] += "Error while unlink\n" + symlinks[i][ii] + "\n" + strerror(errno);
					cerr<<"[drive setup] "<<__FUNCTION__ <<":  "<<err[ERR_LINK_INITFILES]<<endl;
				}
			}
			if (symlink(scripts[i].c_str(),symlinks[i][ii].c_str()) !=0)
			{
				err[ERR_LINK_INITFILES] += "Error while link\n" + symlinks[i][ii] + "\n" + strerror(errno);
				cerr<<"[drive setup] "<<__FUNCTION__ <<":  "<<err[ERR_LINK_INITFILES]<<endl;
				return false;
			}
				cout<<"[drive setup] linking "<<scripts[i]<<"-->"<<symlinks[i][ii]<< " ...ok"<<endl;
		}
	}
	
	return true;
}

//returns true if any partition is available at device
bool CDriveSetup::haveActiveParts(const int& device_num)
{
	int i = device_num;

	for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
	{
		string partname = partitions[i][ii];
		if (isActivePartition(partname)) 
		{
			return true;
		}				
	}			
	
	return false;
}

//returns true if any partition is mounted at device
bool CDriveSetup::haveMounts(const int& device_num, mount_stat_uint_t mount_stat)
{
	int i = device_num;
	bool is_mounted[MAXCOUNT_PARTS], is_swap[MAXCOUNT_PARTS];
#ifdef ENABLE_SAMBASERVER
	bool is_shared[MAXCOUNT_PARTS];
#endif

	for (unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
	{
		is_mounted[ii] 	= isMountedPartition(partitions[i][ii]);
		is_swap[ii] 	= isSwapPartition(partitions[i][ii]);
#ifdef ENABLE_SAMBASERVER
		is_shared[ii] 	= d_settings.drive_partition_samba[i][ii];
#endif

		switch (mount_stat)
		{
			#ifdef ENABLE_SAMBASERVER
			case MOUNT_STAT_MOUNT_AND_SHARE: // return true if the partition is mounted and a samba share
				if (!is_swap[ii])
				{
					if (is_shared[ii] && is_mounted[ii])
						return true;
				}
				break;
			#endif
			case MOUNT_STAT_MOUNT_ONLY:
				if (is_mounted[ii] && !is_swap[ii])
					return true;
				break;
			default: /*MOUNT_STAT_MOUNT_AND_SWAP:*/
				if (is_mounted[ii] || is_swap[ii])
					return true;
				break;
		}
	}

	return false;
}

#ifdef ENABLE_SAMBASERVER
// get settings mode for samba servermode depends of available shares 
void CDriveSetup::setSambaMode()
{
	// set samba on/off mode to off...
	g_settings.smb_setup_samba_on_off = CSambaSetup::OFF;	

	// ...then set samba on/off mode to off, if no share was definied, otherwise set to on and exit loop
	for (uint i = 0; i <  MAXCOUNT_DRIVE; i++)
	{
		for (uint j = 0; j <  MAXCOUNT_PARTS; j++)
		{
			if (d_settings.drive_partition_samba[i][j])
			{
				g_settings.smb_setup_samba_on_off = CSambaSetup::ON;
				return;
			}
		}	
	}
}


// returns true if is any sambashare available
bool CDriveSetup::haveMountedSmbShares()
{
	for (uint i = 0; i <  MAXCOUNT_DRIVE; i++)
	{
		if (haveMounts(i, MOUNT_STAT_MOUNT_AND_SHARE))
			return true;
	}
	
	return false;
}
#endif /*ENABLE_SAMBASERVER*/


//helper, converts int to string
string CDriveSetup::iToString(int int_val)
{
    int i = int_val;
    ostringstream i_str;
    i_str << i;
    string i_string(i_str.str());
    return i_string;
}

//show helptext
/**NOTE: Please don't remove this lines in your public distribution!
that's important, for an effective feedback and bug fixes,
otherwise it could be detrimental for your coming distributions*/
void CDriveSetup::showHelp()
{
	Helpbox helpbox;
	string lines[] = {g_Locale->getText(LOCALE_SETTINGS_HELP_GENERAL),
			  "http://wiki.tuxbox.org/Drive-Setup",
			  g_Locale->getText(LOCALE_SETTINGS_HELP_BUGS),
			  "http://forum.tuxbox.org/bugs"};

	for (uint i = 0; i < (sizeof(lines) / sizeof(lines[0])); i++)
	{
		helpbox.addLine(lines[i]);
	}

	helpbox.show(LOCALE_SETTINGS_HELP);
}


// load settings from configfile
void CDriveSetup::loadDriveSettings()
{
	bool have_no_conf = false;
	
	//reset handeled settings
	v_int_settings.clear();
	v_string_settings.clear();
	
	if(!configfile.loadConfig(DRV_CONFIGFILE)) 
	{
		have_no_conf = true;
	}
	
	cout<<"[drive setup] "<<__FUNCTION__ <<": load settings from "<<DRV_CONFIGFILE<<endl;
	
	//ide mode
	d_settings.drive_activate_ide = configfile.getInt32("drive_activate_ide", IDE_OFF);
	handleSetting(&d_settings.drive_activate_ide);

	//mmc modul
	strcpy(d_settings.drive_mmc_module_name, configfile.getString("drive_mmc_module_name", "").c_str());
	old_drive_mmc_module_name = static_cast <string> (d_settings.drive_mmc_module_name);
	
	// mmc modul load parameter
	for(unsigned int i = 0; i < MAXCOUNT_MMC_MODULES; i++)
	{	
		sprintf(c_opt[OPT_MMC_PARAMETER], "drive_mmc_%d_modul_parameter", i);
		d_settings.drive_mmc_modul_parameter[i] = (string)configfile.getString(c_opt[OPT_MMC_PARAMETER], "");
		handleSetting(&d_settings.drive_mmc_modul_parameter[i]);
	}

	//advanced fstab
	d_settings.drive_use_fstab = configfile.getInt32("drive_use_fstab", YES);
	handleSetting(&d_settings.drive_use_fstab);
	d_settings.drive_use_fstab_auto_fs = configfile.getInt32("drive_use_fstab_auto_fs", NO);
	handleSetting(&d_settings.drive_use_fstab_auto_fs);

	//advanced insmod/modprobe options
	d_settings.drive_advanced_modul_command_load_options = configfile.getString("drive_advanced_modul_command_load_options", "");
	handleSetting(&d_settings.drive_advanced_modul_command_load_options);

	//advanced modul dir
	d_settings.drive_modul_dir = configfile.getString("drive_modul_dir", VAR_MOUDULDIR);
 	handleSetting(&d_settings.drive_modul_dir);

	//warning for mount in jffs2
	d_settings.drive_warn_jffs = configfile.getInt32("drive_warn_jffs", YES);
	handleSetting(&d_settings.drive_warn_jffs);
	
	//fs format options
	for(unsigned int i = 0; i < MAXCOUNT_FSTYPES; i++) 
	{
		sprintf(c_opt[OPT_FS_FORMAT_OPTION], "drive_fs_%s_format_option", fstype[i].fsname.c_str());
		strcpy(d_settings.drive_fs_format_option[i], configfile.getString(c_opt[OPT_FS_FORMAT_OPTION],fstype[i].mkfs_options.c_str()).c_str());
 		old_drive_fs_format_option[i] = static_cast <string> (d_settings.drive_fs_format_option[i]);
	}
	
	for(unsigned int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		//spindown
		sprintf(c_opt[OPT_SPINDOWN], "drive_%d_spindown", i);
		strcpy(d_settings.drive_spindown[i], configfile.getString(c_opt[OPT_SPINDOWN],"300").c_str());
		old_drive_spindown[i] = static_cast <string> (d_settings.drive_spindown[i]);

		//write_cache
		sprintf(c_opt[OPT_WRITECACHE], "drive_%d_write_cache", i);
		d_settings.drive_write_cache[i] = configfile.getInt32(c_opt[OPT_WRITECACHE], OFF);
		handleSetting(&d_settings.drive_write_cache[i]);

		for(unsigned int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
		{
			//partition_size
			sprintf(c_opt[OPT_PARTSIZE], "drive_%d_partition_%d_size", i, ii);
			strcpy(d_settings.drive_partition_size[i][ii], configfile.getString(c_opt[OPT_PARTSIZE], "0").c_str()); //not handled

			//partition_fstype
			sprintf(c_opt[OPT_FSTYPE], "drive_%d_partition_%d_fstype", i, ii);
			strcpy(d_settings.drive_partition_fstype[i][ii], configfile.getString(c_opt[OPT_FSTYPE], "").c_str()); //not handled

			//partition_mountpoint
			sprintf(c_opt[OPT_MOUNTPOINT], "drive_%d_partition_%d_mountpoint", i, ii);
			d_settings.drive_partition_mountpoint[i][ii] = (string)configfile.getString(c_opt[OPT_MOUNTPOINT], "");
			handleSetting(&d_settings.drive_partition_mountpoint[i][ii]);

			//partition_activ
			sprintf(c_opt[OPT_ACTIV_PARTITION], "drive_%d_partition_%d_activ", i, ii);
			d_settings.drive_partition_activ[i][ii] = configfile.getBool(c_opt[OPT_ACTIV_PARTITION], YES);
			handleSetting(&d_settings.drive_partition_activ[i][ii]);

#ifdef ENABLE_NFSSERVER
			//partition_nfs
			sprintf(c_opt[OPT_SHARE_FOR_NFS], "drive_%d_partition_%d_nfs", i, ii);
			d_settings.drive_partition_nfs[i][ii] = configfile.getBool(c_opt[OPT_SHARE_FOR_NFS], OFF);
			handleSetting(&d_settings.drive_partition_nfs[i][ii]);

			//partition_nfs_host_ip
			sprintf(c_opt[OPT_SHARE_NFS_CLIENT_IP], "drive_%d_partition_%d_nfs_host_ip", i, ii);
			d_settings.drive_partition_nfs_host_ip[i][ii] = (string)configfile.getString(c_opt[OPT_SHARE_NFS_CLIENT_IP], "");
			handleSetting(&d_settings.drive_partition_nfs_host_ip[i][ii] );
#endif /*ENABLE_NFSSERVER*/

#ifdef ENABLE_SAMBASERVER
			//partition_samba
			sprintf(c_opt[OPT_SHARE_FOR_SAMBA], "drive_%d_partition_%d_samba", i, ii);
			d_settings.drive_partition_samba[i][ii] = configfile.getBool(c_opt[OPT_SHARE_FOR_SAMBA], OFF);
			handleSetting(&d_settings.drive_partition_samba[i][ii]);

			//partition_samba_read only
			sprintf(c_opt[OPT_SHARE_SAMBA_RO], "drive_%d_partition_%d_samba_ro", i, ii);
			d_settings.drive_partition_samba_ro[i][ii] = configfile.getBool(c_opt[OPT_SHARE_SAMBA_RO], OFF);
			handleSetting(&d_settings.drive_partition_samba_ro[i][ii]);

			//partition_samba_public
			sprintf(c_opt[OPT_SHARE_SAMBA_PUBLIC], "drive_%d_partition_%d_samba_public", i, ii);
			d_settings.drive_partition_samba_public[i][ii] = configfile.getBool(c_opt[OPT_SHARE_SAMBA_PUBLIC], ON);
			handleSetting(&d_settings.drive_partition_samba_public[i][ii]);

			//partition_samba_share_name
			sprintf(c_opt[OPT_SHARE_SAMBA_NAME], "drive_%d_partition_%d_samba_share_name", i, ii);
			char def_name[10]; /* i and ii are 0 to 3, so this should be enough... */
			snprintf(def_name, 10, "Share_%d_%d", i+1, ii+1);
			d_settings.drive_partition_samba_share_name[i][ii] = (string)configfile.getString(c_opt[OPT_SHARE_SAMBA_NAME], def_name );
			handleSetting(&d_settings.drive_partition_samba_share_name[i][ii] );

			//samba_share_comment
			sprintf(c_opt[OPT_SHARE_SAMBA_COMMENT], "drive_%d_partition_%d_samba_share_comment", i, ii);
			d_settings.drive_partition_samba_share_comment[i][ii] = (string)configfile.getString(c_opt[OPT_SHARE_SAMBA_COMMENT], "" );
			handleSetting(&d_settings.drive_partition_samba_share_comment[i][ii] );
#endif /*ENABLE_SAMBASERVER*/
		}
	}

#ifdef ENABLE_SAMBASERVER
	handleSetting(&g_settings.smb_setup_samba_on_off);
	handleSetting(&g_settings.smb_setup_samba_workgroup);
#endif /*ENABLE_SAMBASERVER*/

	if (have_no_conf)
	{
		if (writeDriveSettings())
			cout<<"[drive setup] "<<__FUNCTION__ <<": found no "<<DRV_CONFIGFILE<< " defaults used..."<<endl;
	}
}

// saving settings
bool CDriveSetup::writeDriveSettings()
{
	err[ERR_WRITE_SETTINGS] = "";

	// drivesetup
	configfile.setInt32	( "drive_activate_ide", d_settings.drive_activate_ide);
	configfile.setString	( "drive_mmc_module_name", d_settings.drive_mmc_module_name );
	configfile.setInt32	( "drive_use_fstab", d_settings.drive_use_fstab );
	configfile.setInt32	( "drive_use_fstab_auto_fs", d_settings.drive_use_fstab_auto_fs );
	configfile.setString	( "drive_advanced_modul_command_load_options", d_settings.drive_advanced_modul_command_load_options);
	configfile.setString	( "drive_modul_dir", d_settings.drive_modul_dir);
	configfile.setInt32	( "drive_warn_jffs", d_settings.drive_warn_jffs );

	//mmc modul load options
	for(unsigned int i = 0; i < MAXCOUNT_MMC_MODULES; i++)
	{	
		//mmc_modul_parameter
		sprintf(c_opt[OPT_MMC_PARAMETER], "drive_mmc_%d_modul_parameter", i);
		configfile.setString( c_opt[OPT_MMC_PARAMETER], d_settings.drive_mmc_modul_parameter[i]);
	}
	
	//fs format options
	for(unsigned int i = 0; i < MAXCOUNT_FSTYPES; i++)
	{
		sprintf(c_opt[OPT_FS_FORMAT_OPTION], "drive_fs_%s_format_option", fstype[i].fsname.c_str());
		configfile.setString( c_opt[OPT_FS_FORMAT_OPTION], d_settings.drive_fs_format_option[i]);
	}

	for(int i = 0; i < MAXCOUNT_DRIVE; i++) 
	{
		//spindown
		sprintf(c_opt[OPT_SPINDOWN], "drive_%d_spindown", i);
		configfile.setString( c_opt[OPT_SPINDOWN], d_settings.drive_spindown[i/*MASTER||SLAVE*/] );

		//write_cache
		sprintf(c_opt[OPT_WRITECACHE], "drive_%d_write_cache", i);
		configfile.setInt32( c_opt[OPT_WRITECACHE], d_settings.drive_write_cache[i/*MASTER||SLAVE*/]);

		for(int ii = 0; ii < MAXCOUNT_PARTS; ii++) 
		{
			//partition_size
			sprintf(c_opt[OPT_PARTSIZE], "drive_%d_partition_%d_size", i, ii);
			configfile.setString( c_opt[OPT_PARTSIZE], d_settings.drive_partition_size[i/*MASTER||SLAVE*/][ii] );

			//partition_fstype
			sprintf(c_opt[OPT_FSTYPE], "drive_%d_partition_%d_fstype", i, ii);
			configfile.setString( c_opt[OPT_FSTYPE], d_settings.drive_partition_fstype[i/*MASTER||SLAVE*/][ii] );

			//partition_mountpoint
			sprintf(c_opt[OPT_MOUNTPOINT], "drive_%d_partition_%d_mountpoint", i, ii);
			configfile.setString( c_opt[OPT_MOUNTPOINT], d_settings.drive_partition_mountpoint[i/*MASTER||SLAVE*/][ii]);

			//partition_activ
			sprintf(c_opt[OPT_ACTIV_PARTITION], "drive_%d_partition_%d_activ", i, ii);
			configfile.setBool(c_opt[OPT_ACTIV_PARTITION], d_settings.drive_partition_activ[i/*MASTER||SLAVE*/][ii]);
#ifdef ENABLE_NFSSERVER
			//partition_nfs
			sprintf(c_opt[OPT_SHARE_FOR_NFS], "drive_%d_partition_%d_nfs", i, ii);
			configfile.setBool(c_opt[OPT_SHARE_FOR_NFS], d_settings.drive_partition_nfs[i/*MASTER||SLAVE*/][ii]);

			//partition_nfs_host_ip
			sprintf(c_opt[OPT_SHARE_NFS_CLIENT_IP], "drive_%d_partition_%d_nfs_host_ip", i, ii);
			configfile.setString( c_opt[OPT_SHARE_NFS_CLIENT_IP], d_settings.drive_partition_nfs_host_ip[i/*MASTER||SLAVE*/][ii]);
#endif /*ENABLE_NFSSERVER*/

#ifdef ENABLE_SAMBASERVER
			//partition_samba
			sprintf(c_opt[OPT_SHARE_FOR_SAMBA], "drive_%d_partition_%d_samba", i, ii);
			configfile.setBool(c_opt[OPT_SHARE_FOR_SAMBA], d_settings.drive_partition_samba[i/*MASTER||SLAVE*/][ii]);

			//partition_samba_ro
			sprintf(c_opt[OPT_SHARE_SAMBA_RO], "drive_%d_partition_%d_samba_ro", i, ii);
			configfile.setBool(c_opt[OPT_SHARE_SAMBA_RO], d_settings.drive_partition_samba_ro[i/*MASTER||SLAVE*/][ii]);

			//partition_samba_public
			sprintf(c_opt[OPT_SHARE_SAMBA_PUBLIC], "drive_%d_partition_%d_samba_public", i, ii);
			configfile.setBool(c_opt[OPT_SHARE_SAMBA_PUBLIC], d_settings.drive_partition_samba_public[i/*MASTER||SLAVE*/][ii]);

			//partition_samba_share_name
			sprintf(c_opt[OPT_SHARE_SAMBA_NAME], "drive_%d_partition_%d_samba_share_name", i, ii);
			configfile.setString(c_opt[OPT_SHARE_SAMBA_NAME], d_settings.drive_partition_samba_share_name[i/*MASTER||SLAVE*/][ii]);

			//partition_samba_share_comment
			sprintf(c_opt[OPT_SHARE_SAMBA_COMMENT], "drive_%d_partition_%d_samba_share_comment", i, ii);
			configfile.setString(c_opt[OPT_SHARE_SAMBA_COMMENT], d_settings.drive_partition_samba_share_comment[i/*MASTER||SLAVE*/][ii]);
#endif /*ENABLE_SAMBASERVER*/
		}
	}
	
	if (!configfile.saveConfig(DRV_CONFIGFILE)) 
	{
		cerr<<"[drive setup] "<<__FUNCTION__ <<": error while writing "<<DRV_CONFIGFILE<<endl;
		err[ERR_WRITE_SETTINGS] = g_Locale->getText(LOCALE_DRIVE_SETUP_MSG_ERROR_SAVE_CONFIGFILE_FAILED);
		return false;
	}

	//refresh lists to get modules with options
	loadMmcModulList();

	return true;
}

bool CDriveSetup::Reset()
{
	bool ret = true;
	err[ERR_RESET] = "";

	bool do_reset = (ShowLocalizedMessage(LOCALE_DRIVE_SETUP_RESET, LOCALE_DRIVE_SETUP_MSG_RESET_NOW, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NEUTRINO_ICON_INFO, width, 5) == CMessageBox::mbrYes);
		
		string init_ide_path = getInitIdeFilePath();
		string init_mount_path = getInitMountFilePath();
	#ifdef ENABLE_SAMBASERVER 
		string init_smb_path = getInitSmbFilePath();
	#endif

		if (do_reset)
		{		
			string init_files[] = {	init_ide_path,
						getInitIdeFilePath().insert(init_ide_path.rfind("/")+1, "S"),
						getInitIdeFilePath().insert(init_ide_path.rfind("/")+1, "K"),
						init_mount_path, 
						getInitMountFilePath().insert(init_mount_path.rfind("/")+1, "S"),
						getInitMountFilePath().insert(init_mount_path.rfind("/")+1, "K"),
						DRV_CONFIGFILE, 
						getFstabFilePath()
					#ifdef ENABLE_NFSSERVER 
						,getExportsFilePath()
					#endif
					#ifdef ENABLE_SAMBASERVER
						,init_smb_path
						,getInitSmbFilePath().insert(init_smb_path.rfind("/")+1, "S")
						,getInitSmbFilePath().insert(init_smb_path.rfind("/")+1, "K")
						,getSmbConfFilePath()
						,SAMBA_MARKER
					#endif /*ENABLE_SAMBASERVER*/
						};
			
			//removing init and configfiles
			for (unsigned int i = 0; i < (sizeof(init_files) / sizeof(init_files[0])); i++)
			{
				if(access(init_files[i].c_str(), R_OK) ==0)
				{
					if (remove(init_files[i].c_str()) !=0)
					{
						cerr << "[drive_setup] "<<__FUNCTION__ <<":  error while removing: "<<init_files[i] <<" "<< strerror(errno)<<endl;
						err[ERR_RESET] += "Can't remove " + init_files[i];
						err[ERR_RESET] += "\n";
						err[ERR_RESET] += strerror(errno);
						err[ERR_RESET] += "\n";
						
						ret = false;
					}
				}
			}
			
			bool (CDriveSetup::*pMember[])(void) = {	&CDriveSetup::unmountAll,
									&CDriveSetup::unloadFsDrivers,
									&CDriveSetup::unloadMmcDrivers,
									&CDriveSetup::unloadIdeDrivers};
	
			for (unsigned int i = 0; i < (sizeof(pMember) / sizeof(pMember[0])); i++)
			{ 
				if (!(*this.*pMember[i])())
					ret = false;
			}

			configfile.clear();
		#ifdef ENABLE_SAMBASERVER
			//reset samba options
			g_settings.smb_setup_samba_on_off = CSambaSetup::OFF;
			g_settings.smb_setup_samba_workgroup = "WORKGROUP";
		#endif
		}

		if (!ret)
		{
			err[ERR_RESET] += err[ERR_UNMOUNT_ALL];
			err[ERR_RESET] += "\n";
			err[ERR_RESET] += err[ERR_UNLOAD_FSDRIVERS];
			err[ERR_RESET] += "\n";
			err[ERR_RESET] += err[ERR_UNLOAD_MMC_DRIVERS];
			err[ERR_RESET] += "\n";
			err[ERR_RESET] += err[ERR_UNLOAD_IDEDRIVERS];
		}

	return ret;
}

//returns global error message
string CDriveSetup::getErrMsg()
{
	string ret = "";

	for (uint i = 0; i < ERROR_DESCRIPTIONS_NUM_COUNT; i++)
	{ 
		if (!err[i].empty())
			ret += err[i] + "\n";
	}

	return ret;
}


//handle/collects old string settings
void  CDriveSetup::handleSetting(string *setting)
{	
	settings_string_t val	= {*setting, setting};
	v_string_settings.push_back(val);
}

//handle/collects old int settings
void  CDriveSetup::handleSetting(int *setting)
{	
	settings_int_t val	= {*setting, setting};
	v_int_settings.push_back(val);
}

//restore old settings
void CDriveSetup::restoreSettings()
{
	//restore integer settings with current settings
	for (uint i = 0; i < v_int_settings.size(); i++)
		*v_int_settings[i].p_val = v_int_settings[i].old_val;

	//restore string settings with current settings
	for (uint i = 0; i < v_string_settings.size(); i++)
		*v_string_settings[i].p_val = v_string_settings[i].old_val;
	
	//restore char settings
	
	//Note: the order of next lines must be the same like in handleCharSettings() !! 
	//Yes, it's better to do this like with strings and integers overloaded in handleSetting(), but this hasn't working nice, please fix it, if you can ;-)
	//restore mmc modul name
	strcpy(d_settings.drive_mmc_module_name, old_drive_mmc_module_name.c_str()); 

	//restore spindown
	for(uint i = 0; i < MAXCOUNT_DRIVE; i++)
		strcpy(d_settings.drive_spindown[i], old_drive_spindown[i].c_str());
	
	//restore fs format option
	for(uint i = 0; i < MAXCOUNT_FSTYPES; i++)
		strcpy(d_settings.drive_fs_format_option[i], old_drive_fs_format_option[i].c_str());
	
}

//check for changed mounts,
//if we have unmounted or mounted devices that doesn't match with current settings (activ), then returns true
bool  CDriveSetup::haveChangedMounts()
{
	bool is_swap[MAXCOUNT_DRIVE][MAXCOUNT_PARTS];

	for(uint i = 0; i < MAXCOUNT_DRIVE; i++)
	{
		for(uint ii = 0; ii < MAXCOUNT_PARTS; ii++) 
		{
			if (isActivePartition(partitions[i][ii]))
			{
				is_swap[i][ii] = isSwapPartition(partitions[i][ii]);
	
				if (!is_swap[i][ii])
				{
					if (isMountedPartition(partitions[i][ii]) != d_settings.drive_partition_activ[i][ii])
						return true;
				}
				else
				{
					if (is_swap[i][ii] != d_settings.drive_partition_activ[i][ii])
	 					return true;
				}	
			}
		}
	}	
	return false;
}

//check for setup changes
bool  CDriveSetup::haveChangedSettings()
{
	//compare old integer settings with current settings
	for (uint i = 0; i < v_int_settings.size(); i++)
		if (v_int_settings[i].old_val != *v_int_settings[i].p_val)
			return true;

	//compare old string settings with current settings
	for (uint i = 0; i < v_string_settings.size(); i++)
		if (v_string_settings[i].old_val != *v_string_settings[i].p_val)
			return true;
	
	//compare old char settings with current settings
	//Note: it's not necessary to observe partsize and fstype, this values come from system 
	
	//Note: the order of next lines must be the same like in handleCharSettings() !! 
	//Yes, it's better to do this like with strings and integers overloaded in handleSetting(), but this hasn't working nice, please fix it, if you can ;-)
	//mmc modul name
	if (old_drive_mmc_module_name != static_cast <string> (d_settings.drive_mmc_module_name))
		return true;
	
	//spindown
	for(uint i = 0; i < MAXCOUNT_DRIVE; i++)
	{
		if (old_drive_spindown[i] != static_cast <string> (d_settings.drive_spindown[i]))
			return true;
	}

	//checking fstab
	if (d_settings.drive_use_fstab)
	{
		if(access(getFstabFilePath().c_str(), R_OK) !=0)
			return true;		
	}

	cout<<"[drive setup] no settings changed!"<<endl;
 	return false;
}


