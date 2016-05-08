/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 Stefan Seyfried

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <driver/vcrcontrol.h>

#include <gui/movieinfo.h>

#include <driver/encoding.h>
#include <driver/stream2file.h>

#include <system/helper.h>

#include <gui/widget/messagebox.h>

#ifdef ENABLE_LIRC
#include <irsend/irsend.h>
#endif

#include <global.h>
#include <neutrino.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <errno.h>

#include <daemonc/remotecontrol.h>
#include <zapit/client/zapittools.h>

extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_init();
extern "C" int  tuxtxt_start(int tpid);
extern "C" int  tuxtxt_stop();
extern "C" void tuxtxt_close();
#endif
extern "C" {
#include <driver/genpsi.h>
}

#define SA struct sockaddr
#define SAI struct sockaddr_in

/* list all allowed chars in filenames */
#define FILENAME_ALLOWED_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-"

static CVCRControl vcrControl;

CVCRControl * CVCRControl::getInstance()
{
	return &vcrControl;
}

//-------------------------------------------------------------------------
CVCRControl::CVCRControl()
{
	Device = NULL;
}

//-------------------------------------------------------------------------
CVCRControl::~CVCRControl()
{
	unregisterDevice();
}

//-------------------------------------------------------------------------
void CVCRControl::unregisterDevice()
{
	if (Device)
	{
		delete Device;
		Device = NULL;
	}
}

//-------------------------------------------------------------------------
void CVCRControl::registerDevice(CDevice * const device)
{
	unregisterDevice();
	
	Device = device;
}

//-------------------------------------------------------------------------
bool CVCRControl::Record(const CTimerd::RecordingInfo * const eventinfo)
{
	int mode = g_Zapit->isChannelTVChannel(eventinfo->channel_id) ? NeutrinoMessages::mode_tv : NeutrinoMessages::mode_radio;

	return Device->Record(eventinfo->channel_id, mode, eventinfo->epgID, eventinfo->epgTitle, eventinfo->apids, eventinfo->epg_starttime, eventinfo->eventRepeat);
}

//-------------------------------------------------------------------------
void CVCRControl::CDevice::getAPIDs(const unsigned char ap, APIDList & apid_list)
{
//	(strstr(g_RemoteControl->current_PIDs.APIDs[i].desc, "(AC3)") == NULL))
	unsigned char apids=ap;
	if (apids == TIMERD_APIDS_CONF)
		apids = g_settings.recording_audio_pids_default;
	apid_list.clear();
	CZapitClient::responseGetPIDs allpids;
	g_Zapit->getPIDS(allpids);
	// assume smallest apid ist std apid
	if (apids & TIMERD_APIDS_STD)
	{
		uint apid_min=UINT_MAX;
		uint apid_min_idx=0;
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
			{
				apid_min = allpids.APIDs[i].pid;
				apid_min_idx = i;
			}
		}
		if (apid_min != UINT_MAX)
		{
			APIDDesc a = {apid_min, apid_min_idx, false};
			apid_list.push_back(a);
		}		
	}
	if (apids & TIMERD_APIDS_ALT)
	{
		uint apid_min=UINT_MAX;
		uint apid_min_idx=0;
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
			{
				apid_min = allpids.APIDs[i].pid;
				apid_min_idx = i;
			}
		}
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].pid != apid_min && !allpids.APIDs[i].is_ac3)
			{
				APIDDesc a = {allpids.APIDs[i].pid, i, false};
				apid_list.push_back(a);
			}
		}		
	}
	if (apids & TIMERD_APIDS_AC3)
	{
		bool ac3_found=false;
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].is_ac3)
			{
				APIDDesc a = {allpids.APIDs[i].pid, i, true};
				apid_list.push_back(a);
				ac3_found=true;
			}
		}
		// add non ac3 apid if ac3 not found
		if (!(apids & TIMERD_APIDS_STD) && !ac3_found)
		{
			uint apid_min=UINT_MAX;
			uint apid_min_idx=0;
			for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
			{
				if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
				{
					apid_min = allpids.APIDs[i].pid;
					apid_min_idx = i;
				}
			}
			if (apid_min != UINT_MAX)
			{
				APIDDesc a = {apid_min, apid_min_idx, false};
				apid_list.push_back(a);
			}		
		}
	}
	// no apid selected use standard
	if (apid_list.empty() && !allpids.APIDs.empty())
	{	
		uint apid_min=UINT_MAX;
		uint apid_min_idx=0;
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
			{
				apid_min = allpids.APIDs[i].pid;
				apid_min_idx = i;
			}
		}
		if (apid_min != UINT_MAX)
		{
			APIDDesc a = {apid_min, apid_min_idx, false};
			apid_list.push_back(a);
		}		
		for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); ++it)
			printf("Record APID 0x%X %d\n",it->apid, it->ac3);

	}
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Stop()
{
	deviceState = CMD_VCR_STOP;

	if(last_mode != NeutrinoMessages::mode_scart)
	{
		g_RCInput->postMsg( NeutrinoMessages::VCR_OFF, 0 );
		g_RCInput->postMsg( NeutrinoMessages::CHANGEMODE , last_mode);
	}
#ifdef ENABLE_LIRC
	CIRSend irs("stop");
	return irs.Send();
#else
	return true;
#endif
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid,
				     const std::string& /*epgTitle*/, unsigned char apids, const time_t /*epg_time*/,
				     const CTimerd::CTimerEventRepeat /*eventRepeat*/)
{
	printf("Record channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %llx, apids 0x%X mode \n",
	       channel_id,
	       epgid,
	       apids);
	// leave menu (if in any)
	g_RCInput->postMsg( CRCInput::RC_timeout, 0 );
	
	last_mode = CNeutrinoApp::getInstance()->getMode();
	if(mode != last_mode)
	{
		CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , mode | NeutrinoMessages::norezap );
	}
	
	if(channel_id != 0)		// wenn ein channel angegeben ist
	{
		if(g_Zapit->getCurrentServiceID() != channel_id)	// und momentan noch nicht getuned ist
		{
			g_Zapit->zapTo_serviceID(channel_id);		// dann umschalten
		}
	}
	if(! (apids & TIMERD_APIDS_STD)) // nicht std apid
	{
		APIDList apid_list;
		getAPIDs(apids,apid_list);
		if(!apid_list.empty())
		{
			if(!apid_list.begin()->ac3)
				g_Zapit->setAudioChannel(apid_list.begin()->index);
			else
				g_Zapit->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !
		}
		else
			g_Zapit->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !
	}
	else
		g_Zapit->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !

	if(SwitchToScart)
	{
		// Auf Scart schalten
		CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::VCR_ON, 0 );
		// Das ganze nochmal in die queue, da obiges RC_timeout erst in der naechsten ev. loop ausgef�hrt wird
		// und dann das menu widget das display falsch r�cksetzt
		g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
	}

	deviceState = CMD_VCR_RECORD;
#ifdef ENABLE_LIRC
	// Send IR
	CIRSend irs("record");
	return irs.Send();
#else
	return true;
#endif
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Pause()
{
#ifdef ENABLE_LIRC
	CIRSend irs("pause");
	return irs.Send();
#else
	return true;
#endif
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Resume()
{
#ifdef ENABLE_LIRC
	CIRSend irs("resume");
	return irs.Send();
#else
	return true;
#endif
}

//-------------------------------------------------------------------------
void CVCRControl::CFileAndServerDevice::RestoreNeutrino(void)
{
	if (!g_Zapit->isPlayBackActive() && 
	    CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_standby)
		g_Zapit->startPlayBack();
	g_Zapit->setRecordMode( false );

	// sectionsd starten, wenn er gestoppt oder neu gestartet wurde
	if (StopSectionsd == CNeutrinoApp::SECTIONSD_STOP)
	{
		g_Sectionsd->setPauseScanning(false);
	}
	else if (StopSectionsd == CNeutrinoApp::SECTIONSD_RESTART)
	{
		g_Sectionsd->setPauseScanning(false);
		g_Sectionsd->setServiceChanged(g_RemoteControl->current_channel_id, false);
	}

	// alten mode wiederherstellen (ausser wenn zwischenzeitlich auf oder aus standby oder SCART geschaltet wurde)
	if(CNeutrinoApp::getInstance()->getMode() != last_mode && 
	   CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_scart &&
	   last_mode != NeutrinoMessages::mode_scart &&
	   CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_standby &&
	   last_mode != NeutrinoMessages::mode_standby)
		g_RCInput->postMsg( NeutrinoMessages::CHANGEMODE , last_mode);

#ifndef TUXTXT_CFG_STANDALONE
	if(g_settings.tuxtxt_cache)
	{
		int vtpid=g_RemoteControl->current_PIDs.PIDs.vtxtpid;
		tuxtxt_init();
		if(vtpid)
			tuxtxt_start(vtpid);
	}
#endif
#ifdef ENABLE_RADIOTEXT
	if (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio &&
	    g_settings.radiotext_enable && g_Radiotext == NULL)
	{
		g_Radiotext = new CRadioText;
		g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
	}
#endif
}

void CVCRControl::CFileAndServerDevice::CutBackNeutrino(const t_channel_id channel_id, const int mode)
{
	if (channel_id != 0) // wenn ein channel angegeben ist
	{
		last_mode = CNeutrinoApp::getInstance()->getMode();
		if (mode != last_mode && (mode != CNeutrinoApp::getInstance()->getLastMode() ||
		    last_mode != NeutrinoMessages::mode_standby && last_mode != NeutrinoMessages::mode_scart))
		{
			CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , mode | NeutrinoMessages::norezap );
			// Wenn wir im Standby waren, dann brauchen wir f�rs streamen nicht aufwachen...
			if(last_mode == NeutrinoMessages::mode_standby)
				CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_standby);
			else if(last_mode == NeutrinoMessages::mode_scart) // possibly switch back to SCART mode
				CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::VCR_ON, 0 );
		}
		if(g_Zapit->getCurrentServiceID() != channel_id)	// und momentan noch nicht getuned ist
		{
			g_Zapit->zapTo_serviceID(channel_id);		// dann umschalten
		}
	}
#ifndef TUXTXT_CFG_STANDALONE
	if(g_settings.tuxtxt_cache)
	{
		tuxtxt_stop();
		tuxtxt_close();
	}
#endif
#if ENABLE_RADIOTEXT
	if (mode == NeutrinoMessages::mode_radio && g_settings.radiotext_enable && g_Radiotext != NULL)
	{
		delete g_Radiotext;
		g_Radiotext = NULL;
	}
#endif
	if(StopPlayBack && g_Zapit->isPlayBackActive())	// wenn playback gestoppt werden soll und noch l�uft
		g_Zapit->stopPlayBack();		// dann playback stoppen

	if (StopSectionsd == CNeutrinoApp::SECTIONSD_STOP)
	{
 		g_Sectionsd->setPauseScanning(true);	// sectionsd stoppen
	}
	else if (StopSectionsd == CNeutrinoApp::SECTIONSD_RESTART)
	{
		g_Sectionsd->Restart();			// sectionsd neu starten (pausiert automatisch)
		g_Sectionsd->RegisterNeutrino();
	}

	g_Zapit->setRecordMode( true );			// recordmode einschalten
}

std::string CVCRControl::CFileAndServerDevice::getMovieInfoString(const t_channel_id channel_id,
								  const event_id_t epgid, const time_t epg_time,
								  const std::string& epgTitle, unsigned char apids,
								  const CTimerd::CTimerEventRepeat eventRepeat,
								  const bool save_vtxt_pid, const bool save_sub_pids)
{
	std::string extMessage;
	CMovieInfo cMovieInfo;
	MI_MOVIE_INFO movieInfo;
	event_id_t epg_id = epgid;

	cMovieInfo.clearMovieInfo(&movieInfo);
	CZapitClient::responseGetPIDs pids;
	g_Zapit->getPIDS (pids);
	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo ();

	movieInfo.epgChannel = g_Zapit->getChannelName(channel_id);
	if (movieInfo.epgChannel.empty())
		movieInfo.epgChannel = "unknown";

	movieInfo.epgTitle = (epgTitle.empty()) ? "not available" : Latin1_to_UTF8(epgTitle);
	if (epg_id != 0)
	{
//#define SHORT_EPG
#ifdef SHORT_EPG
		CShortEPGData epgdata;
		if (g_Sectionsd->getEPGidShort(epg_id, &epgdata))
		{
#warning fixme sectionsd should deliver data in UTF-8 format
			movieInfo.epgTitle = Latin1_to_UTF8(epgdata.title);
			movieInfo.epgInfo1 = Latin1_to_UTF8(epgdata.info1);
			movieInfo.epgInfo2 = Latin1_to_UTF8(epgdata.info2);
		}
#else
		CEPGData epgdata;
		bool has_epgdata = g_Sectionsd->getEPGid(epg_id, epg_time, &epgdata);
		if (!has_epgdata)
		{
			has_epgdata = g_Sectionsd->getActualEPGServiceKey(channel_id, &epgdata);
			if (has_epgdata && !epgTitle.empty() && epgTitle != epgdata.title)
				has_epgdata = false;
			if (has_epgdata)
				epg_id = epgdata.eventID;
		}
		if (has_epgdata)
		{
#warning fixme sectionsd should deliver data in UTF-8 format
			movieInfo.epgTitle = Latin1_to_UTF8(epgdata.title);
			movieInfo.epgInfo1 = Latin1_to_UTF8(epgdata.info1);
			movieInfo.epgInfo2 = Latin1_to_UTF8(epgdata.info2);
			
			movieInfo.parentalLockAge = epgdata.fsk;
			if (!epgdata.contentClassification.empty())
				movieInfo.genreMajor = epgdata.contentClassification[0];
				
			movieInfo.length = epgdata.epg_times.dauer	/ 60;
				
			printf("fsk:%d, Genre:%d, Dauer: %d\r\n",movieInfo.parentalLockAge,movieInfo.genreMajor,movieInfo.length);	
		}
#endif
	}
	movieInfo.epgId = 		channel_id;
	movieInfo.epgEpgId =  	epg_id;
	movieInfo.epgMode = 	g_Zapit->getMode();
	movieInfo.epgVideoPid = si.vpid;

	EPG_AUDIO_PIDS audio_pids;
	// super hack :-), der einfachste weg an die apid descriptions ranzukommen
	g_RemoteControl->current_EPGid = epg_id;
	g_RemoteControl->current_PIDs = pids;
	g_RemoteControl->processAPIDnames();
	APIDList apid_list;
	getAPIDs(apids,apid_list);
	for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); ++it)
	{
		audio_pids.epgAudioPid = it->apid;
		audio_pids.epgAudioPidName = g_RemoteControl->current_PIDs.APIDs[it->index].desc;
		movieInfo.audioPids.push_back(audio_pids);
	}

	if (eventRepeat != CTimerd::TIMERREPEAT_ONCE)
		movieInfo.serieName = movieInfo.epgTitle;

	if (save_vtxt_pid)
		movieInfo.epgVTXPID = si.vtxtpid;

	if (save_sub_pids)
	{
		SUB_PIDS sub_pids;
		for (unsigned int i = 0; i < pids.SubPIDs.size(); i++)
		{
			sub_pids.subPid = pids.SubPIDs[i].pid;
			sub_pids.subPage = pids.SubPIDs[i].composition_page;
			sub_pids.subName = getISO639Description(pids.SubPIDs[i].desc);
			movieInfo.subPids.push_back(sub_pids);
		}
	}

	cMovieInfo.encodeMovieInfoXml(&extMessage,movieInfo);

	return extMessage;
}

std::string CVCRControl::CFileAndServerDevice::getCommandString(const CVCRCommand command, const t_channel_id channel_id,
								  const event_id_t epgid, const std::string& epgTitle, unsigned char apids,
								  const CTimerd::CTimerEventRepeat eventRepeat)
{
	char tmp[40];
	std::string apids_selected;
	const char * extCommand;
//		std::string extAudioPID= "error";
	std::string title, info1, info2;
	std::string extMessage = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n<neutrino commandversion=\"1\">\n\t<record command=\"";
	switch(command)
	{
	case CMD_VCR_RECORD:
		extCommand = "record";
		break;
	case CMD_VCR_STOP:
		extCommand = "stop";
		break;
	case CMD_VCR_PAUSE:
		extCommand = "pause";
		break;
	case CMD_VCR_RESUME:
		extCommand = "resume";
		break;
	case CMD_VCR_AVAILABLE:
		extCommand = "available";
		break;
	case CMD_VCR_UNKNOWN:
	default:
		extCommand = "unknown";
		printf("[CVCRControl] Unknown Command\n");
	}

	extMessage += extCommand;
	extMessage += 
		"\">\n"
		"\t\t<channelname>";
	
	CZapitClient::responseGetPIDs pids;
	g_Zapit->getPIDS (pids);
	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo ();

	APIDList apid_list;
	getAPIDs(apids,apid_list);
	apids_selected="";
	for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); ++it)
	{
		if(it != apid_list.begin())
			apids_selected += " ";
		sprintf(tmp, "%u", it->apid);
		apids_selected += tmp; 
	}
	
	std::string tmpstring = g_Zapit->getChannelName(channel_id);
	if (tmpstring.empty())
		extMessage += "unknown";
	else
		extMessage += ZapitTools::UTF8_to_UTF8XML(tmpstring.c_str());
	
	extMessage += "</channelname>\n\t\t<epgtitle>";
	
//		CSectionsdClient::responseGetCurrentNextInfoChannelID current_next;
	title = (epgTitle.empty()) ? "not available" : Latin1_to_UTF8(epgTitle);
	if (epgid != 0)
	{
		CShortEPGData epgdata;
		if (g_Sectionsd->getEPGidShort(epgid, &epgdata))
		{
#warning fixme sectionsd should deliver data in UTF-8 format
			title = Latin1_to_UTF8(epgdata.title);
			info1 = Latin1_to_UTF8(epgdata.info1);
			info2 = Latin1_to_UTF8(epgdata.info2);
		}
	}
	extMessage += ZapitTools::UTF8_to_UTF8XML(title.c_str());
	
	extMessage += "</epgtitle>\n\t\t<id>";
	
	sprintf(tmp, PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS, channel_id);
	extMessage += tmp;
	
	extMessage += "</id>\n\t\t<info1>";
	extMessage += ZapitTools::UTF8_to_UTF8XML(info1.c_str());
	extMessage += "</info1>\n\t\t<info2>";
	extMessage += ZapitTools::UTF8_to_UTF8XML(info2.c_str());
	extMessage += "</info2>\n\t\t<epgid>";
	sprintf(tmp, "%llu", epgid);
	extMessage += tmp;
	extMessage += "</epgid>\n\t\t<mode>";
	sprintf(tmp, "%d", g_Zapit->getMode());
	extMessage += tmp;
	extMessage += "</mode>\n\t\t<videopid>";
	sprintf(tmp, "%u", si.vpid);
	extMessage += tmp;
	extMessage += "</videopid>\n\t\t<audiopids selected=\"";
	extMessage += apids_selected;
	extMessage += "\">\n";
	// super hack :-), der einfachste weg an die apid descriptions ranzukommen
	g_RemoteControl->current_EPGid = epgid;
	g_RemoteControl->current_PIDs = pids;
	g_RemoteControl->processAPIDnames();
	for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); ++it)
	{
		extMessage += "\t\t\t<audio pid=\"";
		sprintf(tmp, "%u", it->apid);
		extMessage += tmp;
		extMessage += "\" name=\"";
		extMessage += ZapitTools::UTF8_to_UTF8XML(g_RemoteControl->current_PIDs.APIDs[it->index].desc);
		extMessage += "\"/>\n";
	}
	extMessage += 
		"\t\t</audiopids>\n"
		"\t\t<vtxtpid>";
	sprintf(tmp, "%u", si.vtxtpid);
	extMessage += tmp;
	extMessage += "</vtxtpid>\n";
	tmpstring = "";
	for (unsigned int i = 0; i < pids.SubPIDs.size(); i++)
	{
		if (tmpstring.empty())
			tmpstring += "\t\t<subpids>\n";
		tmpstring += "\t\t\t<sub pid=\"";
		sprintf(tmp, "%u", pids.SubPIDs[i].pid);
		tmpstring += tmp;
		tmpstring += "\" page=\"";
		sprintf(tmp, "%u", pids.SubPIDs[i].composition_page);
		tmpstring += tmp;
		tmpstring += "\" name=\"";
		tmpstring += ZapitTools::UTF8_to_UTF8XML(getISO639Description(pids.SubPIDs[i].desc));
		tmpstring += "\"/>\n";
	}
	if (!tmpstring.empty())
		tmpstring += "\t\t</subpids>\n";
	extMessage += tmpstring;
	if (eventRepeat != CTimerd::TIMERREPEAT_ONCE)
	{
		extMessage += "\t\t<seriename>";
		extMessage += ZapitTools::UTF8_to_UTF8XML(title.c_str());
		extMessage += "</seriename>\n";
	}
	extMessage +=
		"\t</record>\n"
		"</neutrino>\n";

	return extMessage;
}








bool CVCRControl::CFileDevice::Stop()
{
	printf("Stop\n");

	bool return_value = (::stop_recording() == STREAM2FILE_OK);

#ifdef HAVE_DBOX_HARDWARE
	int actmode=g_Zapit->PlaybackState(); // get actual decoder mode
	if ((actmode == 1) && (!g_settings.misc_spts)) // actual mode is SPTS and settings require PES
		g_Zapit->PlaybackPES(); // restore PES mode
#endif

	RestoreNeutrino();

	deviceState = CMD_VCR_STOP;

	return return_value;
}

bool CVCRControl::CFileDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid,
					const std::string &epgTitle, unsigned char apids, const time_t epg_time,
					const CTimerd::CTimerEventRepeat eventRepeat)
{
	printf("Record channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %llx, apids 0x%X mode %d\n",
	       channel_id,
	       epgid,
	       apids,
	       mode);

	CutBackNeutrino(channel_id, mode);

#ifdef HAVE_DBOX_HARDWARE
	int repeatcount=0;
	int actmode=g_Zapit->PlaybackState(); // get actual decoder mode
	bool sptsmode=g_settings.misc_spts;   // take default from settings

	// aviaEXT is loaded, actual mode is not SPTS and switchoption is set , only in tvmode
	if (actmode == 0 && g_settings.recording_in_spts_mode && mode == NeutrinoMessages::mode_tv)
	{
		g_Zapit->PlaybackSPTS();
		while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 1)) {
			sleep(1); 
		}
		sptsmode = true;
	}
	else if (mode == NeutrinoMessages::mode_radio)
	{
		if (actmode == 1)
		{
			g_Zapit->PlaybackPES();
			while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 0)) {
				sleep(1); 
			}
		}
		sptsmode = false;
	}

	if (actmode == 1 && g_settings.recording_in_spts_mode && !sptsmode && mode == NeutrinoMessages::mode_tv) {
		sptsmode = true;
	}
#else
	bool sptsmode = g_settings.recording_in_spts_mode;
#endif
#define MAXPIDS		64
	unsigned short pids[MAXPIDS];
	unsigned int numpids;
	unsigned int pos;

	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo();
	if (GenPsi && sptsmode)
		reset_pids();
	if (si.vpid != 0)
	{
		pids[0] = si.vpid;
		numpids = 1;
		if (GenPsi && sptsmode)
			transfer_pids(si.vpid, EN_TYPE_VIDEO, 0);

		if (si.pcrpid != 0 && si.pcrpid != si.vpid)
		{
			pids[1] = si.pcrpid;
			numpids = 2;
			if (GenPsi && sptsmode)
				transfer_pids(si.pcrpid, EN_TYPE_PCR, 0);
		}
	}
	else
	{
		/* no video pid */
		numpids = 0;
	}
	APIDList apid_list;
	getAPIDs(apids,apid_list);
	for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); ++it)
	{
		pids[numpids++] = it->apid;
		if (GenPsi && sptsmode)
			transfer_pids(it->apid, EN_TYPE_AUDIO, it->ac3 ? 1 : 0);
	}
	if(!apid_list.empty())
		g_Zapit->setAudioChannel(apid_list.begin()->index);
	
	CZapitClient::responseGetPIDs allpids;
	g_Zapit->getPIDS(allpids);
	bool save_vtxt_pid = false;
	bool save_sub_pids = false;

	if ((StreamVTxtPid) && (si.vtxtpid != 0))
	{
		pids[numpids++] = si.vtxtpid;
		save_vtxt_pid = true;
	}

	if (StreamSubtitlePid && !allpids.SubPIDs.empty()) {
		// Add ttx-pid only once
		unsigned txtdone = 0;
		if (StreamVTxtPid) {
			txtdone = si.vtxtpid;
		}
		for (unsigned ii = 0 ; ii < allpids.SubPIDs.size() ; ++ii) {
			if (allpids.SubPIDs[ii].pid != txtdone) {
				pids[numpids++] = allpids.SubPIDs[ii].pid;
			}
			if (allpids.SubPIDs[ii].pid == si.vtxtpid) {
				txtdone = si.vtxtpid;
			}
		}
		save_vtxt_pid = (txtdone > 0);
		save_sub_pids = true;
	}
	
	char filename[512]; // UTF-8

	// Create filename for recording
	pos = Directory.size();
	strcpy(filename, Directory.c_str());

	if ((pos == 0) ||
	    (filename[pos - 1] != '/'))
	{
		filename[pos] = '/';
		pos++;
		filename[pos] = '\0';
	}

	// %C == channel, %T == title, %I == info1, %d == date, %t == time
	if (FilenameTemplate.empty())
		FilenameTemplate = "%C_%T_%d_%t";
	
	std::string expandedTemplate;
	if (CreateTemplateDirectories)
	{	
		expandedTemplate = FilenameTemplate;
	} else
	{
		expandedTemplate = std::string(basename(FilenameTemplate.c_str()));
	}
	char buf[256];
	buf[255] = '\0';

	appendChannelName(buf,255,channel_id);
	StrSearchReplace(expandedTemplate, "%C", buf);

	appendEPGTitle(buf, 255, epgid, epgTitle);
	StrSearchReplace(expandedTemplate, "%T", buf);

	appendEPGInfo(buf, 255, epgid);
	StrSearchReplace(expandedTemplate, "%I", buf);

	time_t t = time(NULL);
	struct tm tm;
	localtime_r(&t, &tm);
	strftime(buf,11,"%Y-%m-%d", &tm);
	StrSearchReplace(expandedTemplate, "%d", buf);
	
	strftime(buf,7,"%H%M%S", &tm);
	StrSearchReplace(expandedTemplate, "%t", buf);

	//printf("[CFileDevice] filename: %s, expandedTemplate: %s\n",filename,expandedTemplate.c_str());
	strncpy(&(filename[pos]),expandedTemplate.c_str(),511-pos);

	stream2file_error_msg_t error_msg;
	if (CreateTemplateDirectories && !createRecordingDir(filename))
	{
		error_msg = STREAM2FILE_INVALID_DIRECTORY;
	} else
	{
		error_msg = ::start_recording(filename,
					      getMovieInfoString(channel_id, epgid, epg_time, epgTitle, apids, eventRepeat, save_vtxt_pid, save_sub_pids).c_str(),
					      mode,
					      Use_O_Sync,
					      Use_Fdatasync,
					      ((unsigned long long)SplitSize) * 1048576ULL,
					      numpids,
					      pids,
					      sptsmode,
					      RingBuffers,
					      GenPsi,
					      NHD_TS);
	}
	CreateTemplateDirectories = true;
	if (error_msg == STREAM2FILE_OK)
	{
		deviceState = CMD_VCR_RECORD;
		return true;
	}
	else
	{
		RestoreNeutrino();

		printf("[CFileDevice] stream2file error code: %d\n", error_msg);
#warning FIXME: Use better error message
		DisplayErrorMessage(g_Locale->getText(
						      error_msg == STREAM2FILE_BUSY ? LOCALE_STREAMING_BUSY :
						      error_msg == STREAM2FILE_INVALID_DIRECTORY ? LOCALE_STREAMING_DIR_NOT_WRITABLE :
						      error_msg == STREAM2FILE_RECORDING_THREADS_FAILED ? LOCALE_STREAMING_OUT_OF_MEMORY :
						      LOCALE_STREAMINGSERVER_NOCONNECT
						      )); // UTF-8

		return false;
	}
}

void CVCRControl::CFileDevice::appendEPGInfo(char *buf, unsigned int size, const event_id_t epgid) {
	
	CShortEPGData epgdata;
	std::string epgInfo;
	if (size > 0)
		buf[0] = '\0';
	if (g_Sectionsd->getEPGidShort(epgid, &epgdata))
		epgInfo = epgdata.info1;
	else
		epgInfo = "";

	if (!epgInfo.empty())
	{
#warning fixme sectionsd should deliver data in UTF-8 format
//				strcpy(&(filename[pos]), Latin1_to_UTF8(epgdata.title).c_str());
// all characters with code >= 128 will be discarded anyway
		strncpy(buf, epgInfo.c_str(), size); // buf already terminated correctly in CFileDevice::Record(...)
		char * p_act = buf;
		do {
			p_act += strspn(p_act, FILENAME_ALLOWED_CHARS);
			if (*p_act) {
				*p_act++ = '_';
			}
		} while (*p_act);
	}
}
	


void CVCRControl::CFileDevice::appendEPGTitle(char *buf, unsigned int size, const event_id_t epgid, const std::string& epgTitleTimer) {
	
	CShortEPGData epgdata;
	std::string epgTitle;
	if (size > 0)
		buf[0] = '\0';
	if (g_Sectionsd->getEPGidShort(epgid, &epgdata))
		epgTitle = epgdata.title;
	else
		epgTitle = epgTitleTimer;

	if (!epgTitle.empty())
	{
#warning fixme sectionsd should deliver data in UTF-8 format
//				strcpy(&(filename[pos]), Latin1_to_UTF8(epgdata.title).c_str());
// all characters with code >= 128 will be discarded anyway
		strncpy(buf, epgTitle.c_str(), size); // buf already terminated correctly in CFileDevice::Record(...)
		char * p_act = buf;
		do {
			p_act += strspn(p_act, FILENAME_ALLOWED_CHARS);
			if (*p_act) {
				*p_act++ = '_';
			}
		} while (*p_act);
	}
}

void CVCRControl::CFileDevice::appendChannelName(char *buf, unsigned int size, const t_channel_id channel_id) {
	
	if (size > 0)
		buf[0] = '\0';
	std::string ext_channel_name = g_Zapit->getChannelName(channel_id);
	if (!ext_channel_name.empty())
	{
		strncpy(buf, UTF8_TO_FILESYSTEM_ENCODING(ext_channel_name.c_str()), size); // buf already terminated correctly in CFileDevice::Record(...)
		
		char * p_act = buf;
		do {
			p_act += strspn(p_act, FILENAME_ALLOWED_CHARS);
			if (*p_act)
			{
				*p_act++ = '_';
			}
		} while (*p_act);
	}
}



bool CVCRControl::CFileDevice::createRecordingDir(const char *filename) 
{
	//printf("[CFileDevice] trying to create directory %s\n",filename);
	char *pos;
	unsigned int start = 0;
	while ((pos = (char*)strchr(&(filename[start]),'/')) != NULL) {
		if (pos == &filename[0])
		{
			start = 1;
			continue;
		}
		*pos = '\0';
		start = strlen(filename)+1;
		struct stat statInfo;
		if (stat(filename,&statInfo) == -1)
		{
			if (errno == ENOENT)
			{	
				if (mkdir(filename,0000) == 0)
				{
					mode_t mode = strtoul(g_settings.recording_dir_permissions,(char**)NULL,8);
					if (chmod(filename,mode) != 0)
					{
						perror("[CFileDevice] chmod:");
						*pos = '/';
						return false;
					}
				} else
				{
					perror("[CFileDevice] mkdir");
					*pos = '/';
					return false;
				}
				
			} else {
				perror("[CFileDevice] stat");
				*pos = '/';
				return false;
			}
		} else {
			if (!S_ISDIR(statInfo.st_mode)) {
				printf("[CFileDevice] cannot create directory %s\n",filename);
				*pos = '/';
				return false;
			}
		}		
		*pos = '/';
	}
	return true;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Stop()
{
	printf("Stop\n");

	bool return_value = sendCommand(CMD_VCR_STOP);

#ifdef HAVE_DBOX_HARDWARE
	int actmode=g_Zapit->PlaybackState(); // get actual decoder mode
	if ((actmode == 1) && (!g_settings.misc_spts)) // actual mode is SPTS and settings require PES
	{
		int repeatcount=0;
		g_Zapit->PlaybackPES(); // restore PES mode
		while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 0)) {
			sleep(1); 
		}
	}
#endif

	RestoreNeutrino();

	return return_value;
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid,
					const std::string &epgTitle, unsigned char apids, const time_t /*epg_time*/,
					const CTimerd::CTimerEventRepeat eventRepeat)
{
	printf("Record channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %s(%llx), apids 0x%X mode %d\n",
	       channel_id,
			 epgTitle.c_str(),
	       epgid,
	       apids,
	       mode);

	CutBackNeutrino(channel_id, mode);

#ifdef HAVE_DBOX_HARDWARE
	int repeatcount=0;
	int actmode=g_Zapit->PlaybackState() ; // get actual decoder mode

	// aviaEXT is loaded, actual mode is not SPTS and switchoption is set , only in tvmode
	if (actmode == 0 && g_settings.recording_in_spts_mode && mode == NeutrinoMessages::mode_tv)
	{
		g_Zapit->PlaybackSPTS();
		while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 1)) {
			sleep(1); 
		}
	}
	else if (mode == NeutrinoMessages::mode_radio && actmode == 1)
	{
			g_Zapit->PlaybackPES();
			while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 0)) {
				sleep(1); 
			}
	}
#endif

	if(!sendCommand(CMD_VCR_RECORD, channel_id, epgid, epgTitle, apids, eventRepeat))
	{
		RestoreNeutrino();

		DisplayErrorMessage(g_Locale->getText(LOCALE_STREAMINGSERVER_NOCONNECT));

		return false;
	}
	else
		return true;
}


//-------------------------------------------------------------------------
void CVCRControl::CServerDevice::serverDisconnect()
{
	close(sock_fd);
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::sendCommand(CVCRCommand command, const t_channel_id channel_id,
								  const event_id_t epgid, const std::string& epgTitle, unsigned char apids,
								  const CTimerd::CTimerEventRepeat eventRepeat)
{
	printf("Send command: %d channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %s(%llx)\n",
	       command,
	       channel_id,
			 epgTitle.c_str(),
	       epgid);
	if(serverConnect())
	{
		std::string extMessage = getCommandString(command, channel_id, epgid, epgTitle, apids, eventRepeat);

		printf("sending to vcr-client:\n\n%s\n", extMessage.c_str());
		write(sock_fd, extMessage.c_str() , extMessage.length() );

		serverDisconnect();

		deviceState = command;
		return true;
	}
	else
		return false;

}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::serverConnect()
{

	printf("connect to server: %s:%d\n",ServerAddress.c_str(),ServerPort);

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SAI servaddr;
	memset(&servaddr,0,sizeof(SAI));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(ServerPort);
	inet_pton(AF_INET, ServerAddress.c_str(), &servaddr.sin_addr);


	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("[cvcr] -  cannot connect to streamingserver\n");
		return false;
	}

	return true;
}
//-------------------------------------------------------------------------
