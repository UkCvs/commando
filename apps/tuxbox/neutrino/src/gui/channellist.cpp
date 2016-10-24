/*
	Neutrino-GUI  -   DBoxII-Project

	$Id: channellist.cpp,v 1.236 2012/11/01 19:35:25 rhabarber1848 Exp $
	
	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2007-2009 Stefan Seyfried

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

#include <gui/channellist.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/screen_max.h>
#include <driver/rcinput.h>

#include <gui/color.h>
#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/progressbar.h>

#include <system/settings.h>
#include <system/lastchannel.h>


#include <gui/bouquetlist.h>
#include <daemonc/remotecontrol.h>
#include <zapit/client/zapittools.h>

#include <algorithm>

extern CBouquetList * bouquetList;       /* neutrino.cpp */
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */
extern CZapitClient::SatelliteList satList;

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_stop();
#endif

#define ConnectLineBox_Width	12

CPIG * CChannelList::pig = NULL;

CChannelList::CChannel::CChannel(const int _key, const int _number, const std::string& _name, const t_satellite_position _satellitePosition, const t_channel_id ids)
{
	key                 = _key;
	number              = _number;
	name                = _name;
	satellitePosition   = _satellitePosition;
	channel_id          = ids;
	bAlwaysLocked       = false;
	last_unlocked_EPGid = 0;
}


CChannelList::CChannelList(const char * const Name, bool hMode, bool UsedInBouquet)
{
	frameBuffer = CFrameBuffer::getInstance();
	if (pig == NULL)
		pig = new CPIG(0);
	x = y = 0;
	info_height = 0;
	name = Name;
	selected = 0;
	liststart = 0;
	tuned=0xfffffff;
	zapProtection = NULL;
	this->historyMode = hMode;
	usedInBouquet = UsedInBouquet;
	eventFont = SNeutrinoSettings::FONT_TYPE_CHANNELLIST_EVENT;
}

CChannelList::~CChannelList()
{
	if (!usedInBouquet)
	{
		for (std::vector<CChannel *>::iterator it = chanlist.begin(); it != chanlist.end(); ++it)
			delete (*it);
	}
}

int CChannelList::exec()
{
	int nNewChannel = show();

	if ( nNewChannel > -1)
	{
		zapTo(nNewChannel);
		return menu_return::RETURN_REPAINT;
	}
	else if ( nNewChannel == -1)
	{
		// -1 bedeutet nur REPAINT
		return menu_return::RETURN_REPAINT;
	}
	else
	{
		// -2 bedeutet EXIT_ALL
		return menu_return::RETURN_EXIT_ALL;
	}
}

void CChannelList::calcSize()
{
	full_width = w_max(720, 2 * ConnectLineBox_Width);
	if (g_settings.channellist_additional != ADDITIONAL_OFF)
#if defined BOXMODEL_DM500 || defined HAVE_IPBOX_HARDWARE
		// the dm500 seems to like only half / quarter resolution...
		width = full_width - 180 -10;
#else
		width = full_width - 206 -10;
#endif
	else
		width = full_width;

	footerHeight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight()+6; //initial height value for buttonbar
	theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight = std::max(g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight(),
			std::max(g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getHeight(),
			std::max(g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getHeight(),
			1))); /* min 1 to avoid crash on invalid font */

	info_height = 3*fheight + 10;
	height = h_max(576, info_height);

	listmaxshow = (height - theight - footerHeight -0)/fheight;
	height = theight + footerHeight + listmaxshow * fheight;

	x = getScreenStartX(full_width - ConnectLineBox_Width);
	y = getScreenStartY(height + info_height);

	infozone_width = full_width - width;
	pig_width = infozone_width;
	if (g_settings.channellist_additional == ADDITIONAL_MTV) // with miniTV
#if defined BOXMODEL_DM500 || defined HAVE_IPBOX_HARDWARE
		pig_height = (pig_width * 144) / 180;
#else
		pig_height = (pig_width * 190) / 206;
#endif
	else
		pig_height = 0;
	infozone_height = height - theight - pig_height - footerHeight;
}

void CChannelList::updateEvents(void)
{
	time_t atime = time(NULL);
	int cnt=0;

	if (displayNext) {
		if (listmaxshow) {
			for (uint count=0; (count<listmaxshow) && (liststart+count<chanlist.size()); count++){
				// search only for channels whose current event is over
				if (1 || /*chanlist[liststart+count]->nextEvent.text.empty() ||*/
					((long)(chanlist[liststart+count]->nextEvent.startTime) < atime))
				{
					CChannelEventList events = g_Sectionsd->getEventsServiceKey(chanlist[liststart+count]->channel_id);
					chanlist[liststart+count]->nextEvent.startTime = (long)0x7fffffff;
					for ( CChannelEventList::iterator e= events.begin(); e != events.end(); ++e ) {
						if (((long)(e->startTime) > atime) && 
							((e->startTime) < (long)(chanlist[liststart+count]->nextEvent.startTime)))
						{
							chanlist[liststart+count]->nextEvent= *e;
						}
					}
				}
			}
		}
	} else {
		t_channel_id *p_requested_channels = NULL;
		int size_requested_channels = 0;

		if (listmaxshow) {
			size_requested_channels = listmaxshow*sizeof(t_channel_id);
			p_requested_channels 	= (t_channel_id*)malloc(size_requested_channels);
			if (p_requested_channels != NULL) {
				for (uint count=0; (count<listmaxshow) && (liststart+count<chanlist.size()); count++){
					// search only for channels whose current event is over
					if (chanlist[liststart+count]->currentEvent.text.empty() ||
						((long)(chanlist[liststart+count]->currentEvent.startTime + chanlist[liststart+count]->currentEvent.duration) < atime))
					{
						chanlist[liststart+count]->currentEvent = CChannelEvent(); // clear old event
						p_requested_channels[cnt++] = chanlist[liststart+count]->channel_id;
					}
				}
			}
		}
		size_requested_channels = cnt * sizeof(t_channel_id); // update to real size

		if (size_requested_channels) {
			/* request tv channel list if current mode is not radio mode */
			/* request only the events of the channel of the list */
			CChannelEventList events = g_Sectionsd->getChannelEvents((CNeutrinoApp::getInstance()->getMode()) != NeutrinoMessages::mode_radio, p_requested_channels, size_requested_channels);

			for ( CChannelEventList::iterator e= events.begin(); e != events.end(); ++e ) {
				for (uint count=0; (count<listmaxshow) && (liststart+count<chanlist.size()); count++){
					if (chanlist[liststart+count]->channel_id == e->get_channel_id())
					{
						chanlist[liststart+count]->currentEvent= *e;
						break;
					}
				}
			}
		}
		if (p_requested_channels != NULL) free(p_requested_channels);
	}
}


void CChannelList::addChannel(int key, int number, const std::string& _name, const t_satellite_position satellitePosition, t_channel_id ids)
{
	chanlist.push_back(new CChannel(key, number, _name, satellitePosition, ids));
}

void CChannelList::addChannel(CChannelList::CChannel* chan)
{
	if (chan != NULL)
		chanlist.push_back(chan);
}

CChannelList::CChannel* CChannelList::getChannel( int number)
{
	for (uint i=0; i< chanlist.size();i++)
	{
		if (chanlist[i]->number == number)
			return chanlist[i];
	}
	return(NULL);
}

int CChannelList::getKey(int id)
{
	if (id > -1 && id < (int)chanlist.size())
		return chanlist[id]->key;
	return 0;
}

static const std::string empty_string;

const std::string & CChannelList::getActiveChannelName(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->name;
	return empty_string;
}

t_satellite_position CChannelList::getActiveSatellitePosition(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->satellitePosition;
	return 0;
}

t_channel_id CChannelList::getActiveChannel_ChannelID(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->channel_id;
	return 0;
}

int CChannelList::getActiveChannelNumber(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->number;
	return 0;
}

void CChannelList::updateSelection(unsigned int newpos)
{
	if (selected != newpos)
	{
		unsigned int prev_selected = selected;
		unsigned int oldliststart = liststart;

		selected = newpos;
		liststart = (selected / listmaxshow) * listmaxshow;
		if (oldliststart != liststart)
			paint();
		else
		{
			paintItem(prev_selected - liststart);
			paintItem(selected - liststart);
		}
	}
}

int CChannelList::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	if (chanlist.empty())
	{
		//evtl. anzeige dass keine kanalliste....
		return res;
	}
	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, name.c_str());

	displayNext = false; // always start with current events
	displayList = true;  // always start with event list

	calcSize();
	paintHead();
	paintButtonBar();
//	updateEvents();
	paintItemDetailsBox();
	paint();
	paintDetails(selected);

	int oldselected = selected;
	int zapOnExit = false;
	bool bShowBouquetList = false;
	bool dont_hide = false;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

		if (msg == CRCInput::RC_timeout || msg == g_settings.key_channelList_cancel)
		{
			if (!usedInBouquet)
				selected = oldselected;
			loop=false;
		}
		else if (msg_repeatok == CRCInput::RC_up || msg_repeatok == g_settings.key_channelList_pageup)
		{
			int step = (msg_repeatok == g_settings.key_channelList_pageup) ? listmaxshow : 1;  // browse or step 1
			int new_selected = selected - step;
			if (new_selected < 0)
				new_selected = chanlist.size() - 1;
			updateSelection(new_selected);
		}
		else if (msg_repeatok == CRCInput::RC_down || msg_repeatok == g_settings.key_channelList_pagedown)
		{
			unsigned int step = (msg_repeatok == g_settings.key_channelList_pagedown) ? listmaxshow : 1;  // browse or step 1
			unsigned int new_selected = selected + step;
			unsigned int c_size = chanlist.size();
			if (new_selected >= c_size)
			{
				if ((c_size / listmaxshow + 1) * listmaxshow == c_size + listmaxshow) // last page has full entries
					new_selected = 0;
				else
					new_selected = ((step == listmaxshow) && (new_selected < ((c_size / listmaxshow + 1) * listmaxshow))) ? (c_size - 1) : 0;
			}
			updateSelection(new_selected);
		}
		else if ((msg_repeatok == g_settings.key_bouquet_up || msg_repeatok == g_settings.key_bouquet_down) && bouquetList != NULL)
		{
			if (!bouquetList->Bouquets.empty())
			{
				if (!usedInBouquet)
					selected = oldselected;
				int dir = (msg_repeatok == g_settings.key_bouquet_up) ? 1 : -1;
				int b_size = bouquetList->Bouquets.size();
				int nNext = (bouquetList->getActiveBouquetNumber() + b_size + dir) % b_size;
				bouquetList->activateBouquet(nNext);
				res = bouquetList->showChannelList();
				dont_hide = true;
				loop = false;
			}
		}
		else if ( msg == CRCInput::RC_ok )
		{
			zapOnExit = true;
			loop=false;
		}
		else if (this->historyMode && CRCInput::isNumeric(msg))
		{ //numeric zap
			selected = CRCInput::getNumericValue(msg);
			zapOnExit = true;
			loop = false;
		}
		else if( (msg==CRCInput::RC_green) ||
			 (msg==CRCInput::RC_setup) ||
			 (CRCInput::isNumeric(msg)) )
		{
			//pushback key if...
			if (!usedInBouquet)
				selected = oldselected;
			g_RCInput->postMsg( msg, data );
			loop=false;
		}
		else if ( msg == CRCInput::RC_red )   // changed HELP by RED (more straight forward) [rasc 28.06.2003]
		{
			hide();

			if ( g_EventList->exec(chanlist[selected]->channel_id, chanlist[selected]->name) == menu_return::RETURN_EXIT_ALL) // UTF-8
			{
				if (!usedInBouquet)
					selected = oldselected;
				res = -2;
				loop = false;
			}
			else
			{
				paintHead();
				paintButtonBar();
				paintItemDetailsBox();
				paint();
				paintDetails(selected);
			}
		}
		else if ( ( msg == CRCInput::RC_yellow ) &&
				  ( bouquetList != NULL ) )
		{
			if (!usedInBouquet)
				selected = oldselected;
			bShowBouquetList = true;
			loop=false;
		}
		else if ( msg == CRCInput::RC_blue )
		{
			if (g_settings.channellist_additional != ADDITIONAL_OFF) {
				displayList = !displayList;
				if (displayList)
					paint_events(selected);
				else
					showdescription(selected);
			}
			else {
				displayNext = !displayNext;
				paint();
				paintDetails(selected);
			}
			paintButtonBar();
		}
		else if ( msg == CRCInput::RC_help )
		{
			hide();
			CChannelEvent *p_event=NULL;
			if (displayNext)
			{
				p_event = &(chanlist[selected]->nextEvent);
			}

			int ret = menu_return::RETURN_REPAINT;
			if(p_event && p_event->eventID)
			{
				ret = g_EpgData->show(chanlist[selected]->channel_id, p_event->eventID, &(p_event->startTime));
			}
			else
			{
				ret = g_EpgData->show(chanlist[selected]->channel_id);
			}

			if (ret == menu_return::RETURN_EXIT_ALL)
			{
				if (!usedInBouquet)
					selected = oldselected;
				res = -2;
				loop = false;
			}
			else
			{
				paintHead();
				paintButtonBar();
				paintItemDetailsBox();
				paint();
				paintDetails(selected);
			}
		}
		else if (msg == (CRCInput::RC_up   | CRCInput::RC_Release) ||
				 msg == (CRCInput::RC_down | CRCInput::RC_Release) ||
				 msg == (g_settings.key_channelList_pageup   | CRCInput::RC_Release) ||
				 msg == (g_settings.key_channelList_pagedown | CRCInput::RC_Release) )
		{
			paintHead();
			paintDetails(selected);
		}
		else
		{
			if (msg == NeutrinoMessages::EVT_SERVICES_UPD)
			{
				/* need to make sure that channel list is closed,
				   because entries might not be valid anymore */
				if (!usedInBouquet)
					selected = oldselected;
				loop = false;
				res = -2;
			}
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				if (!usedInBouquet)
					selected = oldselected;
				loop = false;
				res = - 2;
			}
		}
	}

	if (!dont_hide)
		hide();

	if (bShowBouquetList)
	{
		if ( bouquetList->exec( true ) == menu_return::RETURN_EXIT_ALL )
			res = -2;
	}

	if (!dont_hide)
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);

	if(zapOnExit)
	{
		return(selected);
	}
	else
	{
		return(res);
	}

}

void CChannelList::hide()
{
	if (g_settings.channellist_additional == ADDITIONAL_MTV) // with miniTV
	{
		if (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_tv)
			pig->hide();
	}
	frameBuffer->paintBackgroundBoxRel(x, y, full_width, height+ info_height+ 5);
	clearItem2DetailsLine();
}

bool CChannelList::showInfo(int pos, int epgpos)
{
	if((pos >= (signed int) chanlist.size()) || (pos<0))
	{
		return false;
	}

	CChannel* chan = chanlist[pos];
	g_InfoViewer->showTitle(pos+1, chan->name, chan->satellitePosition, chan->channel_id, true, epgpos); // UTF-8
	return true;
}

int CChannelList::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	if ( msg == NeutrinoMessages::EVT_PROGRAMLOCKSTATUS)
	{
		// 0x100 als FSK-Status zeigt an, dass (noch) kein EPG zu einem Kanal der NICHT angezeigt
		// werden sollte (vorgesperrt) da ist
		// oder das bouquet des Kanals ist vorgesperrt

		//printf("program-lock-status: %d\n", data);

		if ((g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_ONSIGNAL) || (g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_CHANGETOLOCKED))
		{
			if ( zapProtection != NULL )
				zapProtection->fsk = data;
			else
			{
				// require password if either
				// CHANGETOLOCK mode and channel/bouquet is pre locked (0x100)
				// ONSIGNAL mode and fsk(data) is beyond configured value
				// if programm has already been unlocked, dont require pin
				if ((data >= (neutrino_msg_data_t)g_settings.parentallock_lockage) &&
					 ((chanlist[selected]->last_unlocked_EPGid != g_RemoteControl->current_EPGid) || (g_RemoteControl->current_EPGid == 0)) &&
					 ((g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_CHANGETOLOCKED) || (data >= 0x100)))
				{
					g_RemoteControl->stopvideo();
					zapProtection = new CZapProtection( g_settings.parentallock_pincode, data );

					if ( zapProtection->check() )
					{
						g_RemoteControl->startvideo();

						// remember it for the next time
						chanlist[selected]->last_unlocked_EPGid= g_RemoteControl->current_EPGid;
					}
					delete zapProtection;
					zapProtection = NULL;
				}
				else
					g_RemoteControl->startvideo();
			}
		}
		else
			g_RemoteControl->startvideo();

		return messages_return::handled;
	}
    else
		return messages_return::unhandled;
}


//
// -- Zap to channel with channel_id
//
bool CChannelList::zapTo_ChannelID(const t_channel_id channel_id)
{
	for (unsigned int i=0; i<chanlist.size(); i++) {
		if (chanlist[i]->channel_id == channel_id) {
			zapTo (i);
			return true;
		}
	}

    return false;
}

bool CChannelList::adjustToChannelID(const t_channel_id channel_id)
{
	unsigned int i;

	for (i=0; i<chanlist.size(); i++) {
		if (chanlist[i]->channel_id == channel_id)
		{
			selected= i;
//			CChannel* chan = chanlist[selected];
			lastChList.store (selected, channel_id, false);

			tuned = i;
			if (bouquetList != NULL)
				bouquetList->adjustToChannel( getActiveChannelNumber());
			return true;
		}
	}
	return false;
}

void CChannelList::zapTo(int pos, bool forceStoreToLastChannels)
{
	if (chanlist.empty())
	{
		DisplayErrorMessage(g_Locale->getText(LOCALE_CHANNELLIST_NONEFOUND)); // UTF-8
		return;
	}
	if ( (pos >= (signed int) chanlist.size()) || (pos< 0) )
	{
		pos = 0;
	}

	selected= pos;
	CChannel* chan = chanlist[selected];
	lastChList.store (selected, chan->channel_id, forceStoreToLastChannels);

	if ( pos!=(int)tuned )
	{
#ifndef TUXTXT_CFG_STANDALONE
		if(g_settings.tuxtxt_cache && !CNeutrinoApp::getInstance ()->recordingstatus)
		{
			tuxtxt_stop();
		}
#endif

#ifdef ENABLE_RADIOTEXT
		if ((g_settings.radiotext_enable) && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio) && (g_Radiotext))
		{
			// stop radiotext PES decoding before zapping
			g_Radiotext->radiotext_stop();
		}
#endif

		tuned = pos;
		if (g_settings.lcd_setting[SNeutrinoSettings::LCD_EPGMODE] & CLCD::EPG_TITLE)
		{	/* microoptimization: only poll sectionsd if epg title display is configured
			   not sure if this is necessary, but the extra check won't hurt... */
			CSectionsdClient::CurrentNextInfo info;
			g_Sectionsd->getCurrentNextServiceKey(chan->channel_id, info);
			CLCD::getInstance()->setEPGTitle(info.current_name);
		}
		CLCD::getInstance()->showServicename(chan->name);
		g_RemoteControl->zapTo_ChannelID(chan->channel_id, chan->name, !chan->bAlwaysLocked); // UTF-8
	}
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	if (bouquetList != NULL)
		bouquetList->adjustToChannel( getActiveChannelNumber());

	/* zapTo can take some time.
	   To prevent unwanted "multizaps", clear the RC buffer" */
	g_RCInput->clearRCMsg();
}



int CChannelList::numericZap(neutrino_msg_t key)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	if (chanlist.empty()) {
		DisplayErrorMessage(g_Locale->getText(LOCALE_CHANNELLIST_NONEFOUND)); // UTF-8
		return res;
	}


	// -- quickzap "0" to last seen channel...
	// -- (remains for those who want to avoid the channel history menue)
	// -- (--> girl friend complained about the history menue, so be it...)
	// -- we should be able to configure this in the future, so "0"
	// -- will do quizap or history...
	if (key == g_settings.key_lastchannel) {
		int  ch;

		if( (ch=lastChList.getlast(1)) != -1)
		{
			if ((unsigned int)ch != tuned)
			{
				//printf("quicknumtune(0)\n");
				lastChList.clear_storedelay (); // ignore store delay
				zapTo(ch);		        // zap to last
			}
		}
		return res;
	}

	// -- zap history bouquet, similar to "0" quickzap,
	// -- but shows a menue of last channels
	if (key == g_settings.key_zaphistory) {

		if (this->lastChList.size() > 1) {
			CChannelList channelList(g_Locale->getText(LOCALE_CHANNELLIST_HISTORY), true);

			for ( unsigned int i = 1 ; i < this->lastChList.size() ; ++i) {
				int channelnr = this->lastChList.getlast(i);
				if (channelnr < int(this->chanlist.size())) {
					CChannel* channel = new CChannel(*this->chanlist[channelnr]);
					channelList.addChannel(channel);
        			}
			}

			if (!channelList.isEmpty()) {
				this->frameBuffer->paintBackground();
				int newChannel = channelList.show() ;

				if (newChannel > -1) {
					int lastChannel(this->lastChList.getlast(newChannel + 1));
					if (lastChannel > -1) this->zapTo(lastChannel, true);
				}
			}
		}
		return res;
	}

	int ox=300;
	int oy=200;
	int sx = 4 * g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getMaxDigitWidth() + 14;
	int sy = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight() + 6;
	char valstr[10];
	int chn = CRCInput::getNumericValue(key);
	int pos = 1;
	int lastchan= -1;
	bool doZap = true;
	bool showEPG = false;

	while(1)
	{
		if (lastchan != chn)
		{
			snprintf((char*) &valstr, sizeof(valstr), "%d", chn);
			while(strlen(valstr)<4)
				strcat(valstr,"\xB7");   //MIDDLE DOT 

			frameBuffer->paintBoxRel(ox, oy, sx, sy, COL_INFOBAR_PLUS_0, RADIUS_MID);

			for (int i=3; i>=0; i--)
			{
				valstr[i+ 1]= 0;
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->RenderString(ox+7+ i*((sx-14)>>2), oy+sy-3, sx, &valstr[i], COL_INFOBAR);
			}

			showInfo(chn - 1);
			lastchan= chn;
		}

		g_RCInput->getMsg( &msg, &data, g_settings.timing[SNeutrinoSettings::TIMING_NUMERICZAP] * 10 );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ( msg == CRCInput::RC_timeout )
		{
			if ( ( chn > (int)chanlist.size() ) || (chn == 0) )
				chn = tuned + 1;
			break;
		}
		else if (CRCInput::isNumeric(msg))
		{
			if (pos == 4)
			{
				chn = 0;
				pos = 1;
			}
			else
			{
				chn *= 10;
				pos++;
			}
			chn += CRCInput::getNumericValue(msg);
		}
		else if ( msg == CRCInput::RC_ok )
		{
			if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) )
			{
				chn = tuned + 1;
			}
			break;
		}
		else if (msg_repeatok == g_settings.key_quickzap_down)
		{
			if ( chn == 1 )
				chn = chanlist.size();
			else
			{
				chn--;

				if (chn > (int)chanlist.size())
					chn = (int)chanlist.size();
			}
		}
		else if (msg_repeatok == g_settings.key_quickzap_up)
		{
			chn++;

			if (chn > (int)chanlist.size())
				chn = 1;
		}
		else if ( ( msg == CRCInput::RC_home ) ||
				  ( msg == CRCInput::RC_left ) ||
				  ( msg == CRCInput::RC_right) )
		{
			// Abbruch ohne Channel zu wechseln
			doZap = false;
			break;
		}
		else if ( msg == CRCInput::RC_red )
		{
			// Rote Taste zeigt EPG fuer gewaehlten Kanal an
			if ( ( chn <= (signed int) chanlist.size() ) && ( chn != 0 ) )
			{
				doZap = false;
				showEPG = true;
				break;
			}
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
		{
			doZap = false;
			res = menu_return::RETURN_EXIT_ALL;
			break;
		}
	}

	frameBuffer->paintBackgroundBoxRel(ox, oy, sx, sy);

	chn--;
	if (chn<0)
		chn=0;
	if ( doZap )
	{
		zapTo( chn );
	}
	else
	{
		if (!g_RemoteControl->subChannels.empty() && g_RemoteControl->selected_subchannel > 0)
			g_InfoViewer->showTitle(getActiveChannelNumber(),
					getActiveChannelName(),
					getActiveSatellitePosition(),
					g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].getChannelID(),
					true);
		else
			showInfo(tuned);
		g_InfoViewer->killTitle();

		// Rote Taste zeigt EPG fuer gewaehlten Kanal an
		if ( showEPG )
			g_EventList->exec(chanlist[chn]->channel_id, chanlist[chn]->name);
	}

	return res;
}

void CChannelList::virtual_zap_mode(bool up)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	if (chanlist.empty()) {
		DisplayErrorMessage(g_Locale->getText(LOCALE_CHANNELLIST_NONEFOUND)); // UTF-8
		return;
	}

	int chn = getActiveChannelNumber() + (up ? 1 : -1);
	if (chn > (int)chanlist.size())
	  chn = 1;
	if (chn == 0)
	  chn = (int)chanlist.size();
	int lastchan= -1;
	bool doZap = true;
	bool showEPG = false;
	int epgpos = 0;

	while(1)
	{
		if ((lastchan != chn) || (epgpos != 0))
		{
			showInfo(chn- 1, epgpos);
			lastchan= chn;
		}
		epgpos = 0;
		g_RCInput->getMsg( &msg, &data, atoi(g_settings.timing_string[3])*10 ); // virtual zap timout = TIMING_INFOBAR
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;
		//printf("########### %u ### %u #### %u #######\n", msg, NeutrinoMessages::EVT_TIMER, CRCInput::RC_timeout);

		if ( msg == CRCInput::RC_ok )
		{
			if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) )
			{
				chn = tuned + 1;
			}
			break;
		}
		else if (msg_repeatok == CRCInput::RC_left)
		{
			if ( chn == 1 )
				chn = chanlist.size();
			else
			{
				chn--;

				if (chn > (int)chanlist.size())
					chn = (int)chanlist.size();
			}
		}
		else if (msg_repeatok == CRCInput::RC_right)
		{
			chn++;

			if (chn > (int)chanlist.size())
				chn = 1;
		}
		else if (msg_repeatok == CRCInput::RC_up)
		{
			epgpos = -1;
		}
		else if (msg_repeatok == CRCInput::RC_down)
		{
			epgpos = 1;
		}
		else if ( ( msg == CRCInput::RC_home ) || ( msg == CRCInput::RC_timeout ) )
		{
			// Abbruch ohne Channel zu wechseln
			doZap = false;
			break;
		}
		else if ( msg == CRCInput::RC_red )
		{
			// Rote Taste zeigt EPG fuer gewaehlten Kanal an
			if ( ( chn <= (signed int) chanlist.size() ) && ( chn != 0 ) )
			{
				doZap = false;
				showEPG = true;
				break;
			}
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
		{
			doZap = false;
			break;
		}
	}
	g_InfoViewer->clearVirtualZapMode();

	chn--;
	if (chn<0)
		chn=0;
	if ( doZap )
	{
		zapTo( chn );
	}
	else
	{
		if (!g_RemoteControl->subChannels.empty() && g_RemoteControl->selected_subchannel > 0)
			g_InfoViewer->showTitle(getActiveChannelNumber(),
					getActiveChannelName(),
					getActiveSatellitePosition(),
					g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].getChannelID(),
					true);
		else
			showInfo(tuned);
		g_InfoViewer->killTitle();

		// Rote Taste zeigt EPG fuer gewaehlten Kanal an
		if ( showEPG )
			g_EventList->exec(chanlist[chn]->channel_id, chanlist[chn]->name);
	}
}

void CChannelList::quickZap(neutrino_msg_t key)
{
        if (chanlist.empty())
        {
                //evtl. anzeige dass keine kanalliste....
                return;
        }

        if (key == g_settings.key_quickzap_down)
        {
                if(selected==0)
                        selected = chanlist.size()-1;
                else
                        selected--;
                //                              CChannel* chan = chanlist[selected];
        }
        else if (key == g_settings.key_quickzap_up)
        {
                selected = (selected+1)%chanlist.size();
                //                      CChannel* chan = chanlist[selected];
        }

        zapTo( selected );
}

int CChannelList::hasChannel(int nChannelNr)
{
	for (uint i=0;i<chanlist.size();i++)
	{
		if (getKey(i) == nChannelNr)
			return(i);
	}
	return(-1);
}

// for adjusting bouquet's channel list after numzap or quickzap
void CChannelList::setSelected( int nChannelNr)
{
	selected = nChannelNr;
}

void CChannelList::paintDetails(unsigned int index)
{
	CChannelEvent *p_event = NULL;
	if (displayNext) {
		p_event = &chanlist[index]->nextEvent;
	} else {
		p_event = &chanlist[index]->currentEvent;
	}

	// clear DetailsBox
	paintItemDetailsBox();

	if (!(p_event->description.empty())) {
		char cNoch[50] = {0}; // UTF-8
		char cSeit[50] = {0}; // UTF-8

		struct		tm pStartZeit;
		localtime_r(&p_event->startTime, &pStartZeit);
		unsigned    seit = ( time(NULL) - p_event->startTime + 30) / 60;

		if (displayNext) {
			snprintf(cNoch, sizeof(cNoch), "(%d min)", p_event->duration / 60);
			snprintf(cSeit, sizeof(cSeit), g_Locale->getText(LOCALE_CHANNELLIST_START), pStartZeit.tm_hour, pStartZeit.tm_min);
		} else {
			snprintf(cSeit, sizeof(cSeit), g_Locale->getText(LOCALE_CHANNELLIST_SINCE), pStartZeit.tm_hour, pStartZeit.tm_min);
			int noch = (p_event->duration / 60) - seit;
			if ((noch< 0) || (noch>=10000))
				noch= 0;
			snprintf(cNoch, sizeof(cNoch), "(%d / %d min)", seit, noch);
		}
		int seit_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getRenderWidth(cSeit, true); // UTF-8
		int noch_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(cNoch, true); // UTF-8

		std::string text1= p_event->description;
		std::string text2= p_event->text;

		int xstart = 10;
		if (g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(text1) > (full_width - 30 - seit_len) )
		{
			// zu breit, Umbruch versuchen...
			int pos;
			do
			{
				pos = text1.find_last_of("[ -.]+");
				if ( pos!=-1 )
					text1 = text1.substr( 0, pos );
			} while ( ( pos != -1 ) && (g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(text1) > (full_width - 30 - seit_len) ) );

			std::string text3 = ""; /* not perfect, but better than crashing... */
			if (p_event->description.length() > text1.length())
				text3 = p_event->description.substr(text1.length()+ 1);

			if (!text2.empty() && !text3.empty())
				text3 += " \xB7 "; //MIDDLE DOT

			xstart += g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(text3);
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 10, y+ height+ 5+ 2* fheight, full_width - 30- noch_len, text3, COL_MENUCONTENTDARK);
		}

		if (!(text2.empty()))
		{
			while ( text2.find_first_of("[ -.+*#?=!$%&/]+") == 0 )
				text2 = text2.substr( 1 );
			text2 = text2.substr( 0, text2.find('\n') );
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString(x+ xstart, y+ height+ 5+ 2* fheight, full_width- xstart- 20- noch_len, text2, COL_MENUCONTENTDARK);
		}

		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 10, y+ height+ 5+ fheight, full_width - 30 - seit_len, text1, COL_MENUCONTENTDARK);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString (x+ full_width- 10- seit_len, y+ height+ 5+    fheight   , seit_len, cSeit, COL_MENUCONTENTDARK, 0, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x+ full_width- 10- noch_len, y+ height+ 5+ 2* fheight- 2, noch_len, cNoch, COL_MENUCONTENTDARK, 0, true); // UTF-8
	}
	else {
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 10, y+ height+ 5+ fheight, full_width - 20, g_Locale->getText(LOCALE_EPGLIST_NOEVENTS), COL_MENUCONTENTDARK, 0, true); // UTF-8
	}
	if (g_settings.channellist_foot == FOOT_FREQ && g_Zapit->getCurrentServiceID() == getActiveChannel_ChannelID()) {
		TP_params 	TP;
		g_Zapit->get_current_TP(&TP);
		std::string desc = "";

		char buf[5] = {0};
		if (g_info.delivery_system == DVB_S)
			sprintf (buf, "%d(%c)", TP.feparams.frequency / 1000, (TP.polarization == HORIZONTAL) ? 'h' : 'v');
		else
			sprintf (buf, "%d",  TP.feparams.frequency / 1000000);
		desc = desc + buf + " ";

#ifdef HAVE_TRIPLEDRAGON
		sprintf(buf, "%d", TP.feparams.symbolrate / 1000);
#else
		sprintf(buf, "%d", TP.feparams.u.qam.symbol_rate / 1000);
#endif
		desc = desc + buf + " ";

#ifdef HAVE_TRIPLEDRAGON
		switch (TP.feparams.fec)
#else
		switch (TP.feparams.u.qam.fec_inner)
#endif
		{
			case FEC_NONE:	desc+= "NONE"; break;
			case FEC_1_2:	desc+= "1/2";  break;
			case FEC_2_3:	desc+= "2/3";  break;
			case FEC_3_4:	desc+= "3/4";  break;
			case FEC_5_6:	desc+= "5/6";  break;
			case FEC_7_8:   desc+= "7/8";  break;
#if HAVE_DVB_API_VERSION >= 3
			case FEC_4_5:	desc+= "4/5";  break;
			case FEC_6_7:	desc+= "6/7";  break;
			case FEC_8_9:	desc+= "8/9";  break;
#endif
			case FEC_AUTO:	desc+= "AUTO"; break;
				default:	desc+= "UNKNOWN"; break;
		}
		desc+= " DVB ";

#ifdef HAVE_TRIPLEDRAGON
		desc+= "QPSK";
#else
		switch (TP.feparams.u.qam.modulation)
		{
			case 0x00:	desc+= "QPSK"; break;
			case 0x01:	desc+= "QAM_16"; break;
			case 0x02:	desc+= "QAM_32"; break;
			case 0x03:	desc+= "QAM_64"; break;
			case 0x04:	desc+= "QAM_128"; break;
			case 0x05:	desc+= "QAM_256"; break;
				default:   desc+= "QAM_AUTO"; break;
		}
#endif

		if (g_info.delivery_system == DVB_S)
		{
			int satpos = getActiveSatellitePosition();
			char satpos_str[6];
			snprintf(satpos_str, 5, "%d.%d%c", satpos < 0 ? -satpos / 10 : satpos / 10, satpos < 0 ? -satpos % 10 : satpos % 10, satpos < 0 ? 'W' : 'E');
			satpos_str[5] = '\0';
			desc = desc + " (" + satpos_str + ")";
		}

		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 10, y+ height+ 5+ 3*fheight, full_width - 30, desc, COL_MENUCONTENTDARK, 0, true);
	}
	else if( !displayNext && g_settings.channellist_foot == FOOT_NEXT) {
		char buf[128] = {0};
		char cFrom[50] = {0}; // UTF-8
		CSectionsdClient::CurrentNextInfo CurrentNext;
		g_Sectionsd->getCurrentNextServiceKey(chanlist[index]->channel_id, CurrentNext);
		if (!(CurrentNext.next_name.empty())) {
			struct tm pStartZeit;
			localtime_r(&CurrentNext.next_zeit.startzeit, &pStartZeit);
			snprintf(cFrom, sizeof(cFrom), "%s %02d:%02d",g_Locale->getText(LOCALE_WORD_FROM),pStartZeit.tm_hour, pStartZeit.tm_min );
			snprintf(buf, sizeof(buf), "%s",  Latin1_to_UTF8(CurrentNext.next_name).c_str());
			int from_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getRenderWidth(cFrom, true); // UTF-8

			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+ 10, y+ height+ 5+ 3*fheight, full_width - 30 - from_len, buf, COL_MENUCONTENTDARK, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString(x+ full_width- 10- from_len, y+ height+ 5+ 3*fheight, from_len, cFrom, COL_MENUCONTENTDARK, 0, true); // UTF-8
		}
	}
	if (g_settings.channellist_additional != ADDITIONAL_OFF) {
		if (displayList)
			paint_events(selected);
		else
			showdescription(selected);
	}
}


//
// -- Decoreline to connect ChannelDisplayLine with ChannelDetail display
// -- 2002-03-17 rasc
//

void CChannelList::clearItem2DetailsLine()
{
	paintItem2DetailsLine(-1);
}

void CChannelList::paintItem2DetailsLine(int pos)
{
	int xpos  = x - ConnectLineBox_Width; // 12
	int ypos1 = y + theight+0 + pos*fheight;
	int ypos2 = y + height;
	int ypos1a = ypos1 + (fheight/2)-2;
	int ypos2a = ypos2 + (info_height/2)-2;
	fb_pixel_t col1 = COL_MENUCONTENT_PLUS_6;
	fb_pixel_t col2 = COL_MENUCONTENT_PLUS_1;

	// Clear
	frameBuffer->paintBackgroundBoxRel(xpos, y, ConnectLineBox_Width, height+info_height);

	// paint Line if detail info (and not valid list pos)
	if (pos > -1)
	{
		// 1. top line vert.
		frameBuffer->paintBoxRel(xpos+11, ypos1+4, 1,fheight-8, col2); // item marker gray vert.
		frameBuffer->paintBoxRel(xpos+8,  ypos1+4, 3,fheight-8, col1); // white vert.

		// 2. top line hor.
		frameBuffer->paintBoxRel(xpos, ypos1a  , 8, 3, col1); //top small hor. line
		frameBuffer->paintBoxRel(xpos, ypos1a+3, 8, 1, col2); // hor. gray top xxx

		// 3. long line
		frameBuffer->paintBoxRel(xpos,   ypos1a,   3,ypos2a-ypos1a,   col1); // white
		frameBuffer->paintBoxRel(xpos+3, ypos1a+4, 1,ypos2a-ypos1a-4, col2); //vert. gray

		// 4. bottom line vert.
		frameBuffer->paintBoxRel(xpos, ypos2a  , 8,3, col1); // white
		frameBuffer->paintBoxRel(xpos, ypos2a+3, 8,1, col2); // gray

		// 5. col thick line bottom
		int yoffs = g_settings.rounded_corners ? 10 : 0;
		frameBuffer->paintBoxRel(x-1, ypos2+yoffs, 1, info_height-2*yoffs, col2);
		frameBuffer->paintBoxRel(x-4, ypos2+yoffs, 3, info_height-2*yoffs, col1);
	}
}

void CChannelList::paintItemDetailsBox()
{
	int ypos2 = y + height;

	//info box frame
	frameBuffer->paintBoxFrame(x, ypos2, full_width, info_height, 2, COL_MENUCONTENT_PLUS_6, RADIUS_MID);
	frameBuffer->paintBoxRel(x + 2, y + height + 2, full_width - 4, info_height - 4, COL_MENUCONTENTDARK_PLUS_0, RADIUS_MID);
}

void CChannelList::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int c_rad_small;
	uint8_t    color;
	fb_pixel_t bgcolor;
	unsigned int curr = liststart + pos;
	bool curr_is_active = (getKey(curr) == CNeutrinoApp::getInstance()->channelList->getActiveChannelNumber());

	if (curr == selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		paintItem2DetailsLine(pos);
		c_rad_small = RADIUS_SMALL;
	}
	else if (curr_is_active)
	{
		color   = !displayNext ? COL_MENUCONTENT + 1 : COL_MENUCONTENTINACTIVE;
		bgcolor = !displayNext ? COL_MENUCONTENT_PLUS_1 : COL_MENUCONTENTINACTIVE_PLUS_0;
		c_rad_small = RADIUS_SMALL;
	}
	else
	{
		color   = !displayNext ? COL_MENUCONTENT : COL_MENUCONTENTINACTIVE;
		bgcolor = !displayNext ? COL_MENUCONTENT_PLUS_0 : COL_MENUCONTENTINACTIVE_PLUS_0;
		c_rad_small = 0;
	}
	frameBuffer->paintBoxRel(x, ypos, width- 12, fheight, bgcolor, c_rad_small);

	if (curr < chanlist.size())
	{
		char nameAndDescription[100];
		char tmp[10];
		CChannel* chan = chanlist[curr];
		int prg_offset=0;
		int title_offset=0;
		uint8_t tcolor=(curr == selected) ? color : COL_MENUCONTENT;
		int xtheight=fheight-2;
		
		if(g_settings.channellist_extended)
		{
			prg_offset = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("00:00");
			title_offset=6;
		}
		
		snprintf((char*) tmp, sizeof(tmp), "%d", this->historyMode ? pos : chan->number);

		CChannelEvent *p_event = NULL;
		if (displayNext) {
			p_event = &chan->nextEvent;		
		} else {
			p_event = &chan->currentEvent;
		}

		//number
		int numpos = x+5+numwidth- g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth(tmp);
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(numpos,ypos+fheight, numwidth+5, tmp, color, fheight);

		int l=0;
		if (this->historyMode)
			l = snprintf(nameAndDescription, sizeof(nameAndDescription), ": %d %s", chan->number, ZapitTools::UTF8_to_Latin1(chan->name.c_str()).c_str());
		else
			l = snprintf(nameAndDescription, sizeof(nameAndDescription), "%s", ZapitTools::UTF8_to_Latin1(chan->name.c_str()).c_str());

		CProgressBar pb(false); /* never colored */
		int pb_space = prg_offset - title_offset;
		int pb_max = pb_space - 4;
		
		if (!(p_event->description.empty()))
		{
			// add MIDDLE DOT separator between name and description
			//const char *sep= g_settings.channellist_epgtext_align_right ? "   " :  " \xB7 " ;
			//strncat(nameAndDescription, sep, sizeof(nameAndDescription) - (strlen(nameAndDescription) + 1));
			snprintf(nameAndDescription+l, sizeof(nameAndDescription)-l,g_settings.channellist_epgtext_align_right ? "   " : " \xB7 ");
			int ch_name_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(nameAndDescription);
			int ch_desc_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->getRenderWidth(p_event->description);

			if ((width - (int)numwidth - 20 - 12 - prg_offset - ch_name_len) < ch_desc_len)
				ch_desc_len = (width - numwidth - 20 - 12 - ch_name_len - prg_offset);
			if (ch_desc_len < 0)
				ch_desc_len = 0;

			if(g_settings.channellist_extended)
			{		
				if(displayNext)
				{
					struct		tm pStartZeit;
					localtime_r(&p_event->startTime, &pStartZeit);
			
					snprintf((char*) tmp, sizeof(tmp), "%02d:%02d", pStartZeit.tm_hour, pStartZeit.tm_min);
					g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->RenderString(x + 5 + numwidth + 6, ypos + xtheight, width - numwidth - 20 - 12 - prg_offset, tmp, tcolor, 0, true);
				}
				else
				{
					time_t jetzt=time(NULL);
					int runningPercent = 0;
							
					if (((jetzt - p_event->startTime + 30) / 60) < 0 )
					{
						runningPercent= 0;
					}
					else
					{
						runningPercent=(jetzt-p_event->startTime) * pb_max / p_event->duration;
						if (runningPercent > pb_max)	// this would lead to negative value in paintBoxRel
							runningPercent = pb_max;	// later on which can be fatal...
					}

					// progressbar colors
					fb_pixel_t pb_activeCol, pb_passiveCol;
					pb_activeCol = COL_MENUCONTENTINACTIVE;
					if (curr != selected)
						pb_passiveCol = COL_MENUCONTENT;
					else
						pb_passiveCol = COL_MENUCONTENTDARK;

					// progressbar
					pb.paintProgressBar(x+5+numwidth + title_offset, ypos + fheight/4, pb_space + 2, fheight/2, runningPercent, pb_max, 0, 0, pb_activeCol, pb_passiveCol);
				}
			}

			if (g_settings.channellist_epgtext_align_right){
				// align right
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString(x + width - 16 - ch_desc_len, ypos + fheight, ch_desc_len, p_event->description, color);
			}
			else{
				// align left
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR]->RenderString(x + numwidth + 10 + ch_name_len + 5 + prg_offset, ypos + fheight, ch_desc_len, p_event->description, color);
			}
		}
		else
		{
			// progressbar with diagonal zero line
			if(g_settings.channellist_extended)
			{
				fb_pixel_t pbz_activeCol, pbz_passiveCol;
				if (curr != selected){
					pbz_activeCol = (curr_is_active && !displayNext) ? COL_MENUCONTENT_PLUS_2 : COL_MENUCONTENT_PLUS_1;
					pbz_passiveCol = (curr_is_active && !displayNext) ? COL_MENUCONTENT_PLUS_1 : COL_MENUCONTENT_PLUS_0;
				}
				else {
					pbz_activeCol = COL_MENUCONTENTSELECTED_PLUS_2;
					pbz_passiveCol = COL_MENUCONTENTSELECTED_PLUS_0;
				}
				pb.paintProgressBar(x+5+numwidth + title_offset, ypos + fheight/4, pb_space + 2, fheight/2, 0, pb_max, pbz_activeCol, pbz_passiveCol, pbz_activeCol, 0, NULL, 0, NULL, true);
			}
		}
		// name
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + 5 + numwidth + 10 + prg_offset, ypos + fheight, width - numwidth - 20 - 12 - prg_offset, nameAndDescription, color);

		if (curr == selected) // at last LCD
		{
			CLCD::getInstance()->showMenuText(0, chan->name.c_str(), -1, true); // UTF-8
			CLCD::getInstance()->showMenuText(1, p_event->description.c_str());
		}
	}
}

void CChannelList::paintHead()
{
	int timestr_len = 0;
	int timestr_offset = 0;
	int provstr_len = 0;
	int provstr_offset = 0;
	char timestr[10];
	char provstr[20];
	time_t now = time(NULL);
	struct tm tm;
	localtime_r(&now, &tm);

	bool gotTime = g_Sectionsd->getIsTimeSet();

	if(gotTime){
		strftime(timestr, 10, "%H:%M", &tm);
		timestr_len = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(timestr, true); // UTF-8
		timestr_offset = timestr_len + 10;
	}
	
	if (g_info.delivery_system == DVB_S)
	{
		for (CZapitClient::SatelliteList::const_iterator satList_it = satList.begin(); satList_it != satList.end(); ++satList_it)
		{
			if (satList_it->satPosition == getActiveSatellitePosition())
			{
				snprintf(provstr, 19, "%s", satList_it->satName);
				provstr[19] = '\0';
				provstr_len = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getRenderWidth(provstr, true); // UTF-8
				provstr_offset = provstr_len + 10;
				break;
			}
		}
	}

	frameBuffer->paintBoxRel(x,y, full_width,theight+0, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + 10, y + theight + 2, full_width - 20 - timestr_offset - provstr_offset, name, COL_MENUHEAD, 0, true); // UTF-8


	if ((g_info.delivery_system == DVB_S) && (provstr_len > 0))
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x + full_width - 10 - timestr_offset - provstr_len, y + theight / 2 + fheight / 2 + 2, provstr_len + 1, provstr, COL_MENUHEAD, 0, true); // UTF-8
	}

	if (gotTime){
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + full_width - 10 - timestr_len, y + theight + 2, timestr_len + 1, timestr, COL_MENUHEAD, 0, true); // UTF-8
	}
}

struct button_label CChannelListButtons[] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_INFOVIEWER_EVENTLIST },
	{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_BOUQUETLIST_HEAD     },
	{ NEUTRINO_ICON_BUTTON_BLUE  , LOCALE_INFOVIEWER_NEXT      },
	{ NEUTRINO_ICON_BUTTON_HELP  , LOCALE_EPGMENU_EVENTINFO    }
};

void CChannelList::paintButtonBar()
{
	int ButtonWidth = (full_width - 20) / 4;

	//manage now/next button
	if (g_settings.channellist_additional != ADDITIONAL_OFF)
	{
		if (displayList)
			CChannelListButtons[2].locale = LOCALE_FONTSIZE_CHANNELLIST_DESCR;
		else
			CChannelListButtons[2].locale = LOCALE_INFOVIEWER_NEXT;
	}
	else
	{
		if (displayNext)
			CChannelListButtons[2].locale = LOCALE_INFOVIEWER_NOW;
		else
			CChannelListButtons[2].locale = LOCALE_INFOVIEWER_NEXT;
	}

	int y_foot = y + (height - footerHeight);
	frameBuffer->paintBoxRel(x, y_foot, full_width, footerHeight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y_foot+2, ButtonWidth, 4, CChannelListButtons);
}

void CChannelList::paint()
{
	liststart = (selected/listmaxshow)*listmaxshow;
	int lastnum =  chanlist[liststart]->number + listmaxshow;

	if(lastnum<10)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0");
	else if(lastnum<100)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("00");
	else if(lastnum<1000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("000");
	else if(lastnum<10000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("0000");
	else // if(lastnum<100000)
		numwidth = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER]->getRenderWidth("00000");

	updateEvents();

	// paint background for main box
	frameBuffer->paintBoxRel(x, y+theight, width, height-footerHeight-theight, COL_MENUCONTENT_PLUS_0);
	if (g_settings.channellist_additional != ADDITIONAL_OFF)
		// paint background for right box
		frameBuffer->paintBoxRel(x+width,y+theight,infozone_width,pig_height+infozone_height,COL_MENUCONTENT_PLUS_0);

	const int ypos = y+ theight;
	const int sb = fheight* listmaxshow;
	frameBuffer->paintBoxRel(x+ width- 12,ypos, 12, sb,  COL_MENUCONTENT_PLUS_1);

	const int sbc= ((chanlist.size()- 1)/ listmaxshow)+ 1;
	const int sbs= (selected/listmaxshow);

	frameBuffer->paintBoxRel(x+ width- 10, ypos+ 2+ sbs*(sb-4)/sbc, 8, (sb-4)/sbc, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);

	if (g_settings.channellist_additional == ADDITIONAL_MTV) // with miniTV
	{
		if (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_tv &&
		    pig->getStatus() == CPIG::HIDE)
		{
			// paint PIG
#if defined BOXMODEL_DM500 || defined HAVE_IPBOX_HARDWARE
			// the dm500 seems to like only half / quarter resolution...
			paint_pig(x+width+4, y+theight+4, 180, 144);
		}
		frameBuffer->paintBackgroundBoxRel(x+width+4, y+theight+4, 180, 144);
#else
			paint_pig(x+width+4, y+theight+4, 206, 190);
		}
		frameBuffer->paintBackgroundBoxRel(x+width+4, y+theight+4, 206, 190);
#endif
	}

	for(unsigned int count = 0; count < listmaxshow; count++)
	{
		paintItem(count);
	}
}

void CChannelList::paint_pig (int _x, int _y, int w, int h)
{
#if HAVE_DVB_API_VERSION < 3
	frameBuffer->paintBackgroundBoxRel(_x, _y, w, h);
#else
	frameBuffer->paintBoxRel(_x, _y, w, h, COL_MENUCONTENT_PLUS_0);
#endif
#if defined BOXMODEL_DM500 || defined HAVE_IPBOX_HARDWARE
	pig->show (_x-6, _y+3, w, h);
#else
	pig->show (_x, _y, w, h);
#endif
}

void CChannelList::paint_events(int index)
{
	ffheight = g_Font[eventFont]->getHeight();
	readEvents(chanlist[index]->channel_id);
	frameBuffer->paintBoxRel(x+ width,y+ theight+pig_height, infozone_width, infozone_height,COL_MENUCONTENT_PLUS_0);

	char startTime[10];
	int eventStartTimeWidth = g_Font[eventFont]->getRenderWidth("22:22") + 5; // use a fixed value
	int startTimeWidth = 0;
	CChannelEventList::iterator e;
	time_t azeit;
	time(&azeit);

	int i=1;
	for (e=evtlist.begin(); e!=evtlist.end(); ++e )
	{
		//Remove events in the past
		time_t dif = azeit - e->startTime;
		if ( (dif > 0) && (!(e->eventID == 0)))
		{
			do
			{
				//printf("%d seconds in the past - deleted %s\n", dif, e->description.c_str());
				e = evtlist.erase( e );
				if (e == evtlist.end())
					break;
				dif = azeit - e->startTime;
			}
			while ( dif > 0 );
		}
		if (e == evtlist.end())
			break;

		//Display the remaining events
		if ((y+ theight+ pig_height + i*ffheight) < (y+ theight+ pig_height + infozone_height))
		{
			if (e->eventID)
			{
				struct tm tmStartZeit;
				localtime_r(&e->startTime, &tmStartZeit);
				strftime(startTime, sizeof(startTime), "%H:%M", &tmStartZeit );
				//printf("%s %s\n", startTime, e->description.c_str());
				startTimeWidth = eventStartTimeWidth;
				g_Font[eventFont]->RenderString(x+ width+5, y+ theight+ pig_height + i*ffheight, startTimeWidth, startTime, /*(g_settings.colored_events_channellist == 2) ? COL_COLORED_EVENTS_CHANNELLIST :*/ COL_MENUCONTENTINACTIVE, 0, true);
				g_Font[eventFont]->RenderString(x+ width+5+startTimeWidth, y+ theight+ pig_height + i*ffheight, infozone_width - startTimeWidth - 10, Latin1_to_UTF8(e->description), COL_MENUCONTENT, 0, true);
			}
			else
				g_Font[eventFont]->RenderString(x+ width+5+startTimeWidth, y+ theight+ pig_height + i*ffheight, infozone_width - startTimeWidth - 10, e->description, COL_MENUCONTENT, 0, true); // UTF-8
		}
		else
		{
			break;
		}
		i++;
	}
}

static bool sortByDateTime (const CChannelEvent& a, const CChannelEvent& b)
{
	return a.startTime < b.startTime;
}

void CChannelList::readEvents(const t_channel_id channel_id)
{
	evtlist = g_Sectionsd->getEventsServiceKey(channel_id);

	if ( evtlist.empty() )
	{
		CChannelEvent evt;
		evt.description = g_Locale->getText(LOCALE_EPGLIST_NOEVENTS); // UTF-8
		evt.eventID = 0;
		evt.startTime = 0;
		evtlist.push_back(evt);
	}
	else
		sort(evtlist.begin(),evtlist.end(),sortByDateTime);

	return;
}

/*
CChannelList::CChannel* CChannelList::getChannelFromChannelID(const t_channel_id channel_id)
{
	for (std::vector<CChannel *>::iterator it = chanlist.begin(); it != chanlist.end(); it++)
	{
		if ((*it)->channel_id == channel_id)
			return (*it);
	}
	return NULL;
}
*/



// for EPG+  (2004-03-05 rasc, code sent by vivamiga)

//ExtrasMenu
void CChannelList::ReZap()
{
	CChannel* chan = chanlist[selected];
	g_RemoteControl->zapTo_ChannelID(chan->channel_id, chan->name, !chan->bAlwaysLocked);

}

bool CChannelList::isEmpty() const
{
	return this->chanlist.empty();
}

int CChannelList::getSize() const
{
	return this->chanlist.size();
}

int CChannelList::getSelectedChannelIndex() const
{
	return this->selected;
}

void CChannelList::showdescription(int index)
{
	ffheight = g_Font[eventFont]->getHeight();
	CChannelEvent *p_event = &chanlist[index]->currentEvent;
	g_Sectionsd->getEPGidShort(p_event->eventID, &epgData);

#warning fixme sectionsd should deliver data in UTF-8 format
	if ((epgData.info1.empty()) && (epgData.info2.empty()))
		processTextToArray(g_Locale->getText(LOCALE_EPGVIEWER_NODETAILED)); // UTF-8
	else if (!(epgData.info1.empty()) && (epgData.info2.empty()))
		processTextToArray(Latin1_to_UTF8(epgData.info1));
	else if ((epgData.info1.empty()) && !(epgData.info2.empty()))
		processTextToArray(Latin1_to_UTF8(epgData.info2));
	else
		processTextToArray(Latin1_to_UTF8(epgData.info1 + "\n" + epgData.info2));

	frameBuffer->paintBoxRel(x+ width,y+ theight+pig_height, infozone_width, infozone_height,COL_MENUCONTENT_PLUS_0);
	for (int i = 1; (i < (int)epgText.size()+1) && ((y+ theight+ pig_height + i*ffheight) < (y+ theight+ pig_height + infozone_height)); i++)
		g_Font[eventFont]->RenderString(x+ width+5, y+ theight+ pig_height + i*ffheight, infozone_width - 20, epgText[i-1], COL_MENUCONTENT, 0, true);

	epgData.info1.clear();
	epgData.info2.clear();
	epgText.clear();
}

void CChannelList::addTextToArray(const std::string & text) // UTF-8
{
	//printf("line: >%s<\n", text.c_str() );
	if (text==" ")
	{
		emptyLineCount ++;
		if (emptyLineCount<2)
		{
			epgText.push_back(text);
		}
	}
	else
	{
		emptyLineCount = 0;
		epgText.push_back(text);
	}
}

void CChannelList::processTextToArray(std::string text) // UTF-8
{
	std::string	aktLine = "";
	std::string	aktWord = "";
	int	aktWidth = 0, aktWordWidth = 0;
	text += ' ';
	char* text_= (char*) text.c_str();

	while (*text_!=0)
	{
		if ( (*text_==' ') || (*text_=='\n') || (*text_=='-') || (*text_=='.') )
		{
			if (*text_!='\n')
				aktWord += *text_;

			aktWordWidth = g_Font[eventFont]->getRenderWidth(aktWord, true);
			if ((aktWordWidth+aktWidth)<=(infozone_width - 20))
			{//space ok, add
				aktWidth += aktWordWidth;
				aktLine += aktWord;

				if (*text_=='\n')
				{	//enter-handler
					addTextToArray( aktLine );
					aktLine = "";
					aktWidth= 0;
				}
				aktWord = "";
			}
			else
			{//new line needed
				addTextToArray( aktLine );
				aktLine = aktWord;
				aktWidth = aktWordWidth;
				aktWord = "";
				if (*text_=='\n')
					continue;
			}
		}
		else
		{
			aktWord += *text_;
		}
		text_++;
	}
	//add the rest
	addTextToArray( aktLine + aktWord );
}
