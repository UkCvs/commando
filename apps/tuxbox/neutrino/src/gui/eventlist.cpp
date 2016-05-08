/*
	$Id: eventlist.cpp,v 1.154 2012/11/03 07:03:59 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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

#include <gui/eventlist.h>
#include <gui/epgplus.h>
#include <gui/timerlist.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/mountchooser.h>
#include <gui/widget/dirchooser.h>
#include <system/helper.h>

#include <global.h>
#include <neutrino.h>

#include "widget/hintbox.h"
#include "gui/bouquetlist.h"
#include <gui/widget/stringinput.h>
extern CBouquetList *bouquetList;

#include <zapit/client/zapitclient.h> /* CZapitClient::Utf8_to_Latin1 */
#include <driver/screen_max.h>

#include <zapit/client/zapittools.h>

#include <algorithm>
#include "gui/keyhelper.h"

// sort operators
#if 0
bool sortById (const CChannelEvent& a, const CChannelEvent& b)
{
	return a.eventID < b.eventID ;
}
#endif
static bool sortByDescription (const CChannelEvent& a, const CChannelEvent& b)
{
	int i = strcasecmp(a.description.c_str(), b.description.c_str());
	if (i == 0)
		return a.startTime < b.startTime;
	else
		return (i < 0);
}
static bool sortByDateTime (const CChannelEvent& a, const CChannelEvent& b)
{
	return a.startTime < b.startTime;
}

// unique operators
#if 0
bool uniqueByIdAndDateTime (const CChannelEvent& a, const CChannelEvent& b)
{
	return (a.eventID == b.eventID && a.startTime == b.startTime);
}
#endif

EventList::EventList()
{
	frameBuffer = CFrameBuffer::getInstance();
	selected = 0;
	current_event = -1;
	
	m_search_list = SEARCH_LIST_NONE;
	m_search_epg_item = SEARCH_LIST_NONE;
	m_search_epg_item = SEARCH_EPG_TITLE;
	m_search_channel_id = 1;
	m_search_bouquet_id= 1;

	liststart = 0;
	sort_mode = 0;

	init();
}

EventList::~EventList()
{
}

void EventList::init()
{
	width  = w_max (590, 20);
	height = h_max (480, 20);

	int iconw, iconh;
	iheight  = std::max(16, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight()) + 2;
	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_HELP, &iconw, &iconh);
	theight  = std::max(iconh, g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE]->getHeight());
	fheight1 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight();
	fheight2 = std::max(g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getHeight(), g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME]->getHeight());
	fheight = fheight1 + fheight2 + 2;
	fwidth1 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME]->getRenderWidth("DDD, 00:00,  ");
	fwidth2 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getRenderWidth("[999 min] ");

	listmaxshow = (height-theight-iheight-0)/fheight;
	height = theight+iheight+0+listmaxshow*fheight; // recalc height
	x = getScreenStartX (width);
	y = getScreenStartY (height);
}

void EventList::UpdateTimerList(void)
{
	Timer.getTimerList (timerlist);
	Timer.getRecordingSafety(timerPre,timerPost);
}

// Function: isTimer
// search for timer conflicts for given time and epgID.
// return:  low nibble: EventList::TIMER
//          high nibble: 1: conflict with other timer 
//          *timerID if timer event was found
// Note: Due to performance reason, the return values are coded in high and low nibble.
//unsigned char EventList::isTimer(time_t starttime,time_t endtime ,event_id_t epgid,int* timerID = NULL)
unsigned char EventList::isTimer(time_t starttime, time_t duration, t_channel_id channelID, int* timerID = NULL)
{
	unsigned char result = 0;
	//printf("* %d-%d, pre%d,post%d\n",starttime,duration,timerPre,timerPost);
	for(unsigned int i= 0; i < timerlist.size(); i++)
	{
		//printf("  %d-%d\n",timerlist[i].alarmTime,timerlist[i].stopTime);
		//if(timerlist[i].epgID == epgid)
		if( timerlist[i].epg_starttime == starttime && 
				timerlist[i].channel_id == channelID)
		{
			if(timerlist[i].eventType == CTimerd::TIMER_RECORD)
				result |= EventList::TIMER_RECORD;
			else if(timerlist[i].eventType == CTimerd::TIMER_ZAPTO)
				result |= EventList::TIMER_ZAPTO;
				
			if(timerID != NULL)
				*timerID = timerlist[i].eventID;
		}
		else if(timerlist[i].stopTime  > starttime-timerPre &&
			   timerlist[i].alarmTime < starttime+duration+timerPost)
		{
			// set conflict flag
			result |= (EventList::CONFLICT<<4);
		}
	}
	return result;
}


void EventList::readEvents(const t_channel_id channel_id)
{
	evtlist = g_Sectionsd->getEventsServiceKey(channel_id);
	time_t azeit=time(NULL);
	CChannelEventList::iterator e;

	if ( !evtlist.empty() ) {
		CEPGData epgData;
		// todo: what if there are more than one events in the Portal
		if (g_Sectionsd->getActualEPGServiceKey(channel_id, &epgData ))
		{
//			epgData.eventID;
//			epgData.epg_times.startzeit;
			CSectionsdClient::LinkageDescriptorList	linkedServices;
			if ( g_Sectionsd->getLinkageDescriptorsUniqueKey( epgData.eventID, linkedServices ) )
			{
				if ( linkedServices.size()> 1 )
				{
					CChannelEventList evtlist2; // stores the temporary eventlist of the subchannel channelid
					t_channel_id channel_id2;
#if 0
					for (e=evtlist.begin(); e!=evtlist.end(); ++e )
					{
						if ( e->startTime > azeit ) {
							break;
						}
					}
					// next line is to have a valid e
					if (evtlist.end() == e) --e;
#endif
					for (unsigned int i=0; i<linkedServices.size(); i++)
					{
						channel_id2 = CREATE_CHANNEL_ID_FROM_SERVICE_ORIGINALNETWORK_TRANSPORTSTREAM_ID(
								linkedServices[i].serviceId,
								linkedServices[i].originalNetworkId,
								linkedServices[i].transportStreamId);

						// do not add parent events
						if (channel_id != channel_id2) {
							evtlist2 = g_Sectionsd->getEventsServiceKey(channel_id2);

							for (unsigned int loop=0 ; loop<evtlist2.size(); loop++ )
							{
								// check if event is in the range of the portal parent event
#if 0
								if ( (evtlist2[loop].startTime >= azeit) /*&&
								     (evtlist2[loop].startTime < e->startTime + (int)e->duration)*/ )
#endif
								{
									evtlist.push_back(evtlist2[loop]);
								}
							}
						}
					}
				}
			}
		}

		// Houdini added for Private Premiere EPG, start sorted by start date/time
		sort_mode = 0;
		sort(evtlist.begin(),evtlist.end(),sortByDateTime);
	}

	current_event = -1;
	for ( e=evtlist.begin(); e!=evtlist.end(); ++e )
	{
		if ( e->startTime > azeit ) {
			break;
		}
		current_event++;
	}

	if ( evtlist.empty() )
	{
		CChannelEvent evt;

		evt.description = ZapitTools::UTF8_to_Latin1(g_Locale->getText(LOCALE_EPGLIST_NOEVENTS));
#warning FIXME: evtlist should be utf8-encoded
		evt.eventID = 0;
		evt.startTime = 0;
		evt.duration = 0;
		evtlist.push_back(evt);

	}
	if (current_event > -1)
	{
		selected = current_event;
		if (evtlist[current_event].startTime + (long)evtlist[current_event].duration < azeit)
			current_event = -1;
	}
	else
		selected = 0;

	return;
}


int EventList::exec(const t_channel_id channel_id, const std::string& channelname, const CChannelEventList &followlist) // UTF-8
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	if(m_search_list == SEARCH_LIST_NONE) // init globals once only
	{
		m_search_epg_item = SEARCH_EPG_TITLE;
		m_search_keyword = "";
		m_search_autokeyword = "";
		m_search_list = SEARCH_LIST_CHANNEL;
		m_search_channel_id = channel_id;
		m_search_bouquet_id= bouquetList->getActiveBouquetNumber();
		//m_search_source_text = "";
	}
	m_showSearchResults = false;

	name = channelname;
	sort_mode=0;
	paintHead();
	if (!followlist.empty())
	{
		std::insert_iterator<CChannelEventList> ii(evtlist, evtlist.begin());
		copy(followlist.begin(), followlist.end(), ii);
		showfollow = true;
	}
	else
	{
		readEvents(channel_id);
		showfollow = false;
	}
	UpdateTimerList();
	paint();
	showFunctionBar(true);

	int oldselected = selected;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

		if (msg_repeatok == CRCInput::RC_up || msg_repeatok == g_settings.key_channelList_pageup)
		{
			int step = (msg_repeatok == g_settings.key_channelList_pageup) ? listmaxshow : 1;  // browse or step 1
			int new_selected = selected - step;
			if (new_selected < 0)
				new_selected = evtlist.size() - 1;
			updateSelection(new_selected);
		}
		else if (msg_repeatok == CRCInput::RC_down || msg_repeatok == g_settings.key_channelList_pagedown)
		{
			unsigned int step = (msg_repeatok == g_settings.key_channelList_pagedown) ? listmaxshow : 1;  // browse or step 1
			unsigned int new_selected = selected + step;
			unsigned int e_size = evtlist.size();
			if (new_selected >= e_size)
			{
				if ((e_size / listmaxshow + 1) * listmaxshow == e_size + listmaxshow) // last page has full entries
					new_selected = 0;
				else
					new_selected = ((step == listmaxshow) && (new_selected < ((e_size / listmaxshow + 1) * listmaxshow))) ? (e_size - 1) : 0;
			}
			updateSelection(new_selected);
		}
		else if (msg == g_settings.key_channelList_sort)
		{
			if (showfollow)
				continue;

			event_id_t current_event_id = (current_event > -1) ? evtlist[current_event].eventID : 0;
			time_t current_event_start_time = (current_event > -1) ? evtlist[current_event].startTime : 0;
			event_id_t selected_id = evtlist[selected].eventID;
			time_t selected_start_time = evtlist[selected].startTime;
			if(sort_mode==0)
			{
				sort_mode++;
				sort(evtlist.begin(),evtlist.end(),sortByDescription);
			}
			else
			{
				sort_mode=0;
				sort(evtlist.begin(),evtlist.end(),sortByDateTime);
			}
			// find current
			if (current_event > -1)
			{
				for (current_event = 0; current_event < (int)evtlist.size(); current_event++)
				{
					if (evtlist[current_event].eventID == current_event_id &&
					    evtlist[current_event].startTime == current_event_start_time)
						break;
				}
			}
			// find selected
			for ( selected=0 ; selected < evtlist.size(); selected++ )
			{
				if (evtlist[selected].eventID == selected_id &&
				    evtlist[selected].startTime == selected_start_time)
					break;
			}
			oldselected=selected;
			if(selected<=listmaxshow)
				liststart=0;
			else
				liststart=(selected/listmaxshow)*listmaxshow;
			paint();
		}

//  -- I commented out the following part (code is working)
//  -- reason: this is a little bit confusing, because e.g. you can enter the function
//  -- with RED, but pressing RED doesn't leave - it triggers a record timer instead
//  -- I think it's sufficient, to press RIGHT or HELP to get movie details and then
//  -- press "auto record" or "auto switch"  (rasc 2003-06-28)
//  --- hm, no need to comment out that part, leave the decision to the user
//  --- either set addrecord timer key to "no key" and leave eventlist with red (default now),
//  --- or set addrecord timer key to "red key" (zwen 2003-07-29)

		else if (msg == g_settings.key_channelList_addrecord)
		{
			if (g_settings.recording_type != CNeutrinoApp::RECORDING_OFF && evtlist[0].eventID != 0)
			{
				int timerID;
				//unsigned char is_timer = isTimer(evtlist[selected].startTime,evtlist[selected].startTime + evtlist[selected].duration,evtlist[selected].eventID,&timerID);
				unsigned char is_timer = isTimer(evtlist[selected].startTime, evtlist[selected].duration, evtlist[selected].get_channel_id(), &timerID);
				if(Timer.isTimerdAvailable() && !(is_timer & EventList::TIMER_RECORD))
				{
					std::string recDir = g_settings.recording_dir[0];
					if (g_settings.recording_choose_direct_rec_dir && g_settings.recording_type == RECORDING_FILE)
					{
						CRecDirChooser recDirs(LOCALE_TIMERLIST_RECORDING_DIR, NEUTRINO_ICON_TIMER, NULL, &recDir);
						hide();
						recDirs.exec(NULL,"");
						paintHead();
						paint();
						showFunctionBar(true);
						recDir = recDirs.get_selected_dir();
					}
					
					if (recDir.empty() && (RECORDING_FILE == g_settings.recording_type))
					{
						printf("set zapto timer failed, no record directory...\n");
						ShowLocalizedMessage(LOCALE_TIMER_EVENTRECORD_TITLE, LOCALE_EPGLIST_ERROR_NO_RECORDDIR_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_ERROR);
					}

					if (!recDir.empty() || (RECORDING_FILE != g_settings.recording_type))
					{
//						if (Timer.addRecordTimerEvent(channel_id,
						if (Timer.addRecordTimerEvent(evtlist[selected].get_channel_id(),
										     evtlist[selected].startTime,
										     evtlist[selected].startTime + evtlist[selected].duration,
										     evtlist[selected].eventID, evtlist[selected].startTime,
										     evtlist[selected].startTime - (ANNOUNCETIME + 120),
										     TIMERD_APIDS_CONF, true, recDir,false) == -1)
						{
							if(askUserOnTimerConflict(evtlist[selected].startTime - (ANNOUNCETIME + 120),
										  evtlist[selected].startTime + evtlist[selected].duration))
							{
//								Timer.addRecordTimerEvent(channel_id,
								Timer.addRecordTimerEvent(evtlist[selected].get_channel_id(),
									 evtlist[selected].startTime,
									 evtlist[selected].startTime + evtlist[selected].duration,
									 evtlist[selected].eventID, evtlist[selected].startTime,
									 evtlist[selected].startTime - (ANNOUNCETIME + 120),
									 TIMERD_APIDS_CONF, true, recDir,true);
								//ShowLocalizedMessage(LOCALE_TIMER_EVENTRECORD_TITLE, LOCALE_TIMER_EVENTRECORD_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);
								// delete zapto timer if any
								if(is_timer & EventList::TIMER_ZAPTO)
								{
									printf("remove zapto timer\n");
									Timer.removeTimerEvent(timerID);
								}
							}
						} 
						else 
						{
							// delete zapto timer if any
							if(is_timer & EventList::TIMER_ZAPTO)
							{
								printf("remove zapto timer\n");
								Timer.removeTimerEvent(timerID);
							}
						}
						UpdateTimerList();
						paintItem(selected - liststart);
						showFunctionBar(true);
					}
				}
				else if (Timer.isTimerdAvailable() )
				{
					// Timer already available in Timerlist, remove now
					printf("remove record timer\n");
					Timer.removeTimerEvent(timerID);
					UpdateTimerList();
					paintItem(selected - liststart);
					showFunctionBar(true);
				}
				else
					printf("timerd not available\n");
			}
		}
		else if (msg == g_settings.key_channelList_addremind)
		{
			if (evtlist[0].eventID != 0)
			{
				int timerID;
//				unsigned char is_timer = isTimer(evtlist[selected].startTime,evtlist[selected].startTime + evtlist[selected].duration,evtlist[selected].eventID,&timerID);
				unsigned char is_timer = isTimer(evtlist[selected].startTime,evtlist[selected].duration,evtlist[selected].get_channel_id(),&timerID);
				if(Timer.isTimerdAvailable() && !(is_timer & EventList::TIMER_ZAPTO))
				{
					// first delete zapto timer if any
					if(is_timer & EventList::TIMER_RECORD)
					{
						printf("remove record timer\n");
						Timer.removeTimerEvent(timerID);
					}
//					Timer.addZaptoTimerEvent(channel_id,
					Timer.addZaptoTimerEvent(evtlist[selected].get_channel_id(),
									evtlist[selected].startTime,
									evtlist[selected].startTime - ANNOUNCETIME, 0,
									evtlist[selected].eventID, evtlist[selected].startTime, 0, true);
					UpdateTimerList();
					paintItem(selected - liststart);
					showFunctionBar(true);
					//ShowLocalizedMessage(LOCALE_TIMER_EVENTTIMED_TITLE, LOCALE_TIMER_EVENTTIMED_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);
				}
				else if(Timer.isTimerdAvailable())
				{
					// Timer already available in Timerlist, remove now
					printf("remove zapto timer\n");
					Timer.removeTimerEvent(timerID);
					UpdateTimerList();
					paintItem(selected - liststart);
					showFunctionBar(true);
				}
				else
					printf("timerd not available\n");
			}
		}

		else if (msg == g_settings.key_channelList_reload)
		{
			if (showfollow || m_showSearchResults)
				continue;

			hide();
			paintHead();
			readEvents(channel_id);
			paint();
			showFunctionBar(true);
		}

		else if (msg == CRCInput::RC_timeout || msg == g_settings.key_channelList_cancel)
		{
			if (m_showSearchResults)
			{
				m_showSearchResults = false;
				hide();
				name = channelname;
				paintHead();
				readEvents(channel_id);
				paint();
				showFunctionBar(true);
			}
			else
			{
				selected = oldselected;
				loop = false;
			}
		}

		else if ( msg==CRCInput::RC_left || msg==CRCInput::RC_red)
		{
			loop= false;
		}
		else if (msg == CRCInput::RC_0) {
			hide();

			CTimerList *Timerlist = new CTimerList;
			Timerlist->exec(NULL, "");
			delete Timerlist;
			timerlist.clear();
			g_Timerd->getTimerList (timerlist);

			paintHead();
			paint();
			showFunctionBar(true);
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);
		}
#ifdef ENABLE_EPGPLUS
		else if (msg == CRCInput::RC_epg)
		{
			if (showfollow)
				continue;

			hide();
			CEPGplusHandler eplus;
			eplus.exec(NULL, "");
			loop = false;
		}
#endif
		else if (msg==CRCInput::RC_help || msg==CRCInput::RC_right || msg==CRCInput::RC_ok)
		{
			if (evtlist[selected].eventID != 0)
			{
				hide();

//				res = g_EpgData->show(channel_id, evtlist[selected].eventID, &evtlist[selected].startTime);
				res = g_EpgData->show(evtlist[selected].get_channel_id(), evtlist[selected].eventID, &evtlist[selected].startTime, true, showfollow);
				if ( res == menu_return::RETURN_EXIT_ALL )
				{
					loop = false;
				}
				else
				{
					g_RCInput->getMsg( &msg, &data, 0 );

					if ((msg & ~CRCInput::RC_Repeat) != CRCInput::RC_red &&
					     ( msg != CRCInput::RC_timeout ) )
					{
						// RC_red schlucken
						g_RCInput->postMsg( msg, data );
					}

					paintHead();
					UpdateTimerList();
					paint();
					showFunctionBar(true);
				}
			}
		}
		else if (msg == g_settings.key_channelList_search)
		{
			if (showfollow)
				continue;

			res = findEvents();
			if (res == menu_return::RETURN_EXIT_ALL)
			{
				loop = false;
			}
			else
			{
				paintHead();
				paint();
				showFunctionBar(true);
				timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);
			}
		}
		else if (msg == CRCInput::RC_setup)
		{
			loop = false;
			res = menu_return::RETURN_EXIT_ALL;
		}
		else
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
				res = menu_return::RETURN_EXIT_ALL;
			}
		}
	}

	hide();

	return res;
}

void EventList::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height);
}

void EventList::paintItem(unsigned int pos)
{
	uint8_t    color;
	fb_pixel_t bgcolor;
	int ypos = y+ theight+0 + pos*fheight;
	unsigned int curpos = liststart + pos;
	int c_rad_mid;

	if (curpos == selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
		c_rad_mid = RADIUS_MID;
	}
	else if ((int)curpos == current_event)
	{
		color   = COL_MENUCONTENT + 1;
		bgcolor = COL_MENUCONTENT_PLUS_1;
		c_rad_mid = RADIUS_MID;
	}
	else
	{
		color   = COL_MENUCONTENT;
		bgcolor = COL_MENUCONTENT_PLUS_0;
		c_rad_mid = 0;
	}

	frameBuffer->paintBoxRel(x, ypos, width- 15, fheight, bgcolor, c_rad_mid);

	if (curpos < evtlist.size())
	{
		std::string datetime1_str, datetime2_str, duration_str;

		if (evtlist[curpos].eventID != 0)
		{
			char tmpstr[256];
			struct tm tmStartZeit;
			localtime_r(&evtlist[curpos].startTime, &tmStartZeit);

			strftime(tmpstr, sizeof(tmpstr), ". %H:%M, ", &tmStartZeit );
			datetime1_str = (std::string)g_Locale->getText(CLocaleManager::getWeekday(&tmStartZeit)) + tmpstr;

			strftime(tmpstr, sizeof(tmpstr), " %d. ", &tmStartZeit );
			datetime2_str = (std::string)tmpstr + g_Locale->getText(CLocaleManager::getMonth(&tmStartZeit)) + '.';

			if (m_showSearchResults) // show the channel if we made a event search only (which could be made through all channels)
			{
				t_channel_id channel = evtlist[curpos].get_channel_id();
				datetime2_str += "           " + g_Zapit->getChannelName(channel);
			}

			duration_str = "[" + to_string(evtlist[curpos].duration / 60) + " min]";
		}

		// 1st line
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME]->RenderString(x+5,         ypos+ fheight2+3, fwidth1+5,            datetime1_str, color, 0, true); // UTF-8
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME]->RenderString(x+5+fwidth1, ypos+ fheight2+3, width-fwidth1-10- 20, datetime2_str, color, 0, true); // UTF-8

		int seit = (evtlist[curpos].startTime - time(NULL)) / 60;
		if ((seit > 0) && (seit < 100) && !duration_str.empty())
		{
			std::string beginnt = "in " + to_string(seit)  + " min";
			int w = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getRenderWidth(beginnt) + 10;

			g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString(x+width-fwidth2-5- 20- w, ypos+ fheight2+3, w, beginnt, color);
		}
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString(x+width-fwidth2-5- 20, ypos+ fheight2+3, fwidth2, duration_str, color, 0, true); // UTF-8
		// 2nd line
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->RenderString(x+ 20, ypos+ fheight, width- 25- 20, evtlist[curpos].description, color);
		
		unsigned char is_timer = isTimer(evtlist[curpos].startTime,evtlist[curpos].duration,evtlist[curpos].get_channel_id());
		ypos -= 3;

		if(is_timer == (EventList::CONFLICT<<4))
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_CONFLICT, x+203, ypos);
		}
		else if(is_timer == EventList::TIMER_RECORD)
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_RECORD, x+203, ypos);
		}
		else if(is_timer == (EventList::TIMER_RECORD | (EventList::CONFLICT<<4) ))
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_RECORD_CONFLICT, x+203, ypos);
		}
		else if(is_timer == EventList::TIMER_ZAPTO)
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_ZAPTO, x+203, ypos);
		}
		else if(is_timer == (EventList::TIMER_ZAPTO | (EventList::CONFLICT<<4) ))
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_ZAPTO_CONFLICT, x+203, ypos);
		}
		else if(is_timer == (EventList::TIMER_RECORD | EventList::TIMER_ZAPTO))
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_RECORD, x+203, ypos);
			frameBuffer->paintIcon(NEUTRINO_ICON_ZAPTO, x+223, ypos);
		}
		else if(is_timer == (EventList::TIMER_RECORD | EventList::TIMER_ZAPTO | (EventList::CONFLICT<<4) ))
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_RECORD_CONFLICT, x+203, ypos);
			frameBuffer->paintIcon(NEUTRINO_ICON_ZAPTO_CONFLICT, x+223, ypos);
		}
	}
}

void EventList::paintHead()
{
	char l_name[100];
	snprintf(l_name, sizeof(l_name), g_Locale->getText(LOCALE_EPGLIST_HEAD), name.c_str()); // UTF-8

	frameBuffer->paintBoxRel(x, y, width, theight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);

	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE]->RenderString(x+10,y+theight+1, width-36, l_name, COL_MENUHEAD, 0, true); // UTF-8
}

void EventList::paint()
{
	int ypos = y+ theight;
	int sb = fheight* listmaxshow;
	int sbc= ((evtlist.size()- 1)/ listmaxshow)+ 1;
	int sbs= (selected/listmaxshow);

	liststart = (selected/listmaxshow)*listmaxshow;

	if (evtlist[0].eventID != 0)
	{
		int iconw = 0, iconh = 0;
		frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_HELP, &iconw, &iconh);
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x + width - (iconw + 6) , y + (theight >> 1) - (iconh >> 1));
	}

	// paint background, so no transparent corners beside selected item at first paint
	frameBuffer->paintBoxRel(x, ypos, width- 15, sb, COL_MENUCONTENT_PLUS_0);

	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);
	frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ sbs*(sb-4)/sbc , 11, (sb-4)/sbc, COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);
}

void EventList::updateSelection(unsigned int newpos)
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

		if ((g_settings.key_channelList_addremind != CRCInput::RC_nokey) ||
		   ((g_settings.recording_type != CNeutrinoApp::RECORDING_OFF) &&
		    (g_settings.key_channelList_addrecord != CRCInput::RC_nokey)))
		{
			showFunctionBar(true);
		}
	}
}

//
// -- Just display/hide function bar
// -- 2004-04-12 rasc
//
struct button_label EventListButtons[6] =
{
	{ "", LOCALE_GENERIC_EMPTY          },  // timerlist delete / record button
	{ "", LOCALE_EVENTFINDER_SEARCH     },  // search button
	{ "", LOCALE_GENERIC_EMPTY          },  // timerlist delete / channelswitch
	{ "", LOCALE_EVENTLISTBAR_EVENTSORT },  // sort button
	{ "", LOCALE_EVENTLISTBAR_RELOAD    },   // reload button
	{ "", LOCALE_TIMERLIST_NAME         }   // Timerlist button
};

void  EventList::showFunctionBar (bool show)
{
	int  bx,by;
	int  cellwidth;		// 5 cells
	int  space = 8;		// space between buttons
	int  iconw = 0, iconh = 0; //x+4;	// buttonwidht + space between icon and caption

	CKeyHelper keyhelper;
	neutrino_msg_t dummy = CRCInput::RC_nokey;
	const char * icon = NULL;
	std::string number_icons[5];
	std::string btncaption;	

	bx = x + 5;
	by = y + height - iheight;

	int ButtonWidth = (width - 10) / 5;

	// -- hide only?
	if (!show)
	{
		frameBuffer->paintBackgroundBoxRel(bx, by, width, iheight);
		return;
	}

	frameBuffer->paintBoxRel(x, by, width, iheight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);

	unsigned char is_timer = isTimer(evtlist[selected].startTime,evtlist[selected].duration,evtlist[selected].get_channel_id());
	
	// -- Button: Timer Record & Channelswitch
	if ((g_settings.recording_type != CNeutrinoApp::RECORDING_OFF) &&
		(g_settings.key_channelList_addrecord != CRCInput::RC_nokey))
	{
		if (CRCInput::isNumeric(g_settings.key_channelList_addrecord))
		{
			number_icons[0] = CRCInput::getKeyName(g_settings.key_channelList_addrecord);
			number_icons[0] += ".raw";
			icon = number_icons[0].c_str();
		}
		else
			keyhelper.get(&dummy, &icon, g_settings.key_channelList_addrecord);
		EventListButtons[0].button = icon;

		if(is_timer & EventList::TIMER_RECORD )
		{
			btncaption = g_Locale->getText(LOCALE_TIMERLIST_DELETE);
			EventListButtons[0].locale = LOCALE_TIMERLIST_DELETE;
		}
		else
		{
			btncaption = g_Locale->getText(LOCALE_EVENTLISTBAR_RECORDEVENT);
			EventListButtons[0].locale = LOCALE_EVENTLISTBAR_RECORDEVENT;
		}
		
		frameBuffer->getIconSize(icon, &iconw, &iconh);
		cellwidth = std::min(ButtonWidth, iconw + 4 + space + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(btncaption, true));

		// paint 1st button
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, bx, by, ButtonWidth, 1, &EventListButtons[0]);
		bx += cellwidth;
	}

	// Button: Event Search
	if (!showfollow && g_settings.key_channelList_search != CRCInput::RC_nokey)
	{
		if (CRCInput::isNumeric(g_settings.key_channelList_search))
		{
			number_icons[1] = CRCInput::getKeyName(g_settings.key_channelList_search);
			number_icons[1] += ".raw";
			icon = number_icons[1].c_str();
		}
		else
			keyhelper.get(&dummy, &icon, g_settings.key_channelList_search);
		EventListButtons[1].button = icon;
		
		btncaption = g_Locale->getText(LOCALE_EVENTFINDER_SEARCH);
		
		frameBuffer->getIconSize(icon, &iconw, &iconh);
		cellwidth = std::min(ButtonWidth, iconw + 4 + space + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(btncaption, true));
	
		// paint second button
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, bx, by, ButtonWidth, 1, &EventListButtons[1]);
		bx += cellwidth;
	}

	// Button: Timer Channelswitch
	if (g_settings.key_channelList_addremind != CRCInput::RC_nokey)
	{
		if (CRCInput::isNumeric(g_settings.key_channelList_addremind))
		{
			number_icons[2] = CRCInput::getKeyName(g_settings.key_channelList_addremind);
			number_icons[2] += ".raw";
			icon = number_icons[2].c_str();
		}
		else
			keyhelper.get(&dummy, &icon, g_settings.key_channelList_addremind);
		EventListButtons[2].button = icon;

		if(is_timer & EventList::TIMER_ZAPTO)
		{
			btncaption =  g_Locale->getText(LOCALE_TIMERLIST_DELETE);
			EventListButtons[2].locale = LOCALE_TIMERLIST_DELETE;
		}
		else
		{
			btncaption =  g_Locale->getText(LOCALE_EVENTLISTBAR_CHANNELSWITCH);
			EventListButtons[2].locale = LOCALE_EVENTLISTBAR_CHANNELSWITCH;
		}
		
		frameBuffer->getIconSize(icon, &iconw, &iconh);
		cellwidth = std::min(ButtonWidth, iconw + 4 + space + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(btncaption, true));

		// paint 3rd button
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, bx, by, ButtonWidth, 1, &EventListButtons[2]);
		bx += cellwidth;
	}

	// Button: Event Re-Sort
	if (!showfollow && g_settings.key_channelList_sort != CRCInput::RC_nokey)
	{
		if (CRCInput::isNumeric(g_settings.key_channelList_sort))
		{
			number_icons[3] = CRCInput::getKeyName(g_settings.key_channelList_sort);
			number_icons[3] += ".raw";
			icon = number_icons[3].c_str();
		}
		else
			keyhelper.get(&dummy, &icon, g_settings.key_channelList_sort);
		EventListButtons[3].button = icon;
		
		btncaption =  g_Locale->getText(LOCALE_EVENTLISTBAR_EVENTSORT);
		
		frameBuffer->getIconSize(icon, &iconw, &iconh);
		cellwidth = std::min(ButtonWidth, iconw + 4 + space + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(btncaption, true));
	
		// paint 4th button
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, bx, by, ButtonWidth, 1, &EventListButtons[3]);
		bx += cellwidth;
	}

	// Button: Event Reload/Refresh
	if (!showfollow && !m_showSearchResults && g_settings.key_channelList_reload != CRCInput::RC_nokey)
	{
		if (CRCInput::isNumeric(g_settings.key_channelList_reload))
		{
			number_icons[4] = CRCInput::getKeyName(g_settings.key_channelList_reload);
			number_icons[4] += ".raw";
			icon = number_icons[4].c_str();
		}
		else
			keyhelper.get(&dummy, &icon, g_settings.key_channelList_reload);
		EventListButtons[4].button = icon;

		btncaption =  g_Locale->getText(LOCALE_EVENTLISTBAR_RELOAD);
		cellwidth = std::min(ButtonWidth, iconw + 4 + space + g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(btncaption, true));

		// paint 5th button
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, bx, by, ButtonWidth, 1, &EventListButtons[4]);
		bx += cellwidth;
	}
	// Button 6 Timerlist - show always
	EventListButtons[5].locale = LOCALE_TIMERLIST_NAME;
	EventListButtons[5].button = NEUTRINO_ICON_BUTTON_0;
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, bx, by, ButtonWidth, 1, &EventListButtons[5]);

}

/************************************************************************************************/
int EventList::findEvents(void) 
/************************************************************************************************/
{
	int res = menu_return::RETURN_REPAINT;
	int event = 0;
	t_channel_id channel_id;  //g_Zapit->getCurrentServiceID()

	if((m_search_keyword.empty() || m_search_keyword == m_search_autokeyword) && evtlist[selected].eventID != 0)
	{
		m_search_keyword = evtlist[selected].description;
		m_search_autokeyword = m_search_keyword;
	}
	
	CEventFinderMenu menu(	&event,
				&m_search_epg_item,
				&m_search_keyword,
				&m_search_list,
				&m_search_channel_id,
				&m_search_bouquet_id
				);
	hide();
	res = menu.exec(NULL,"");
	
	if(event == 1)
	{
		m_showSearchResults = true;
		evtlist.clear();
		if(m_search_list == SEARCH_LIST_CHANNEL)
		{
			g_Sectionsd->getEventsServiceKeySearchAdd(evtlist, m_search_channel_id, m_search_epg_item, m_search_keyword);
		}
		else if(m_search_list == SEARCH_LIST_BOUQUET)
		{
			int channel_nr = bouquetList->Bouquets[m_search_bouquet_id]->channelList->getSize();
			for(int channel = 0; channel < channel_nr; channel++)
			{
				channel_id = (*(bouquetList->Bouquets[m_search_bouquet_id]->channelList))[channel]->channel_id; 
				g_Sectionsd->getEventsServiceKeySearchAdd(evtlist,channel_id,m_search_epg_item,m_search_keyword);
			}
		}
		else if(m_search_list == SEARCH_LIST_ALL)
		{
			CHintBox box(LOCALE_TIMING_EPG,g_Locale->getText(LOCALE_EVENTFINDER_SEARCHING));
			box.paint();
			int bouquet_nr = bouquetList->Bouquets.size();
			std::vector<t_channel_id> v;
			for(int bouquet = 0; bouquet < bouquet_nr; bouquet++)
			{
				int channel_nr = bouquetList->Bouquets[bouquet]->channelList->getSize();
				for(int channel = 0; channel < channel_nr; channel++)
				{
					channel_id = (*(bouquetList->Bouquets[bouquet]->channelList))[channel]->channel_id; 
					v.push_back(channel_id);
				}
			}
			// search in unique channel list => duplicate events impossible
			sort(v.begin(), v.end());
			std::vector<t_channel_id>::iterator last_it = unique(v.begin(), v.end());
			std::vector<t_channel_id>::iterator it;
			for (it = v.begin(); it != last_it; ++it)
				g_Sectionsd->getEventsServiceKeySearchAdd(evtlist, *it, m_search_epg_item, m_search_keyword);
			box.hide();
		}
		sort_mode = 0;
		sort(evtlist.begin(),evtlist.end(),sortByDateTime);
#if 0
		// remove duplicates
		evtlist.resize(unique(evtlist.begin(), evtlist.end(), uniqueByIdAndDateTime) - evtlist.begin());
#endif
		current_event = -1;
		time_t azeit=time(NULL);
		
		CChannelEventList::iterator e;
		for ( e=evtlist.begin(); e!=evtlist.end(); ++e )
		{
			if ( e->startTime > azeit ) {
				break;
			}
			current_event++;
		}
		if (evtlist.empty())
		{
			CChannelEvent evt;
			evt.description = ZapitTools::UTF8_to_Latin1(g_Locale->getText(LOCALE_EPGLIST_NOEVENTS));
#warning FIXME: evtlist should be utf8-encoded
			evt.eventID = 0;
			evt.startTime = 0;
			evt.duration = 0;
			evtlist.push_back(evt);
		}
		if (current_event > -1)
		{
			selected = current_event;
			if (evtlist[current_event].startTime + (long)evtlist[current_event].duration < azeit)
				current_event = -1;
		}
		else
			selected = 0;
		
		name = (std::string)g_Locale->getText(LOCALE_EVENTFINDER_SEARCH) + ": '" + ZapitTools::Latin1_to_UTF8(m_search_keyword.c_str()) + "'";
	}

	return(res);
}




//
//  -- EventList Menu Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-06 rasc)
//

int CEventListHandler::exec(CMenuTarget* parent, const std::string &/*actionkey*/)
{
	int           res = menu_return::RETURN_EXIT_ALL;
	EventList     *e;
	CChannelList  *channelList;


	if (parent) {
		parent->hide();
	}

	e = new EventList;

	channelList = CNeutrinoApp::getInstance()->channelList;
//	e->exec(channelList->getActiveChannel_ChannelID(), channelList->getActiveChannelName()); // UTF-8
	e->exec(g_Zapit->getCurrentServiceID(), channelList->getActiveChannelName()); // UTF-8
	delete e;

	return res;
}




/************************************************************************************************
*  class CEventFinderMenu
************************************************************************************************/
#define SEARCH_EPG_OPTION_COUNT 4
const CMenuOptionChooser::keyval SEARCH_EPG_OPTIONS[SEARCH_EPG_OPTION_COUNT] =
{
//	{ EventList::SEARCH_EPG_NONE, 	LOCALE_PICTUREVIEWER_RESIZE_NONE     },
	{ EventList::SEARCH_EPG_TITLE, 	LOCALE_FONTSIZE_EPG_TITLE    },
	{ EventList::SEARCH_EPG_INFO1, 	LOCALE_FONTSIZE_EPG_INFO1    },
	{ EventList::SEARCH_EPG_INFO2, 	LOCALE_FONTSIZE_EPG_INFO2    },
//	{ EventList::SEARCH_EPG_GENRE, 	LOCALE_MOVIEBROWSER_INFO_GENRE_MAJOR },
	{ EventList::SEARCH_EPG_ALL, 	LOCALE_EVENTFINDER_SEARCH_ALL_EPG }
};

#define SEARCH_LIST_OPTION_COUNT 3
const CMenuOptionChooser::keyval SEARCH_LIST_OPTIONS[SEARCH_LIST_OPTION_COUNT] =
{
//	{ EventList::SEARCH_LIST_NONE        , LOCALE_PICTUREVIEWER_RESIZE_NONE     },
	{ EventList::SEARCH_LIST_CHANNEL     , LOCALE_TIMERLIST_CHANNEL    },
	{ EventList::SEARCH_LIST_BOUQUET     , LOCALE_BOUQUETLIST_HEAD     },
	{ EventList::SEARCH_LIST_ALL         , LOCALE_CHANNELLIST_HEAD     }
};


/************************************************************************************************/
CEventFinderMenu::CEventFinderMenu(	int* 		event,
					int* 		search_epg_item,
					std::string* 	search_keyword,
					int* 		search_list,		
					t_channel_id*	search_channel_id,
					t_bouquet_id*	search_bouquet_id)
/************************************************************************************************/
{
	m_search_channelname_mf = NULL;

	m_event			= event;
	m_search_epg_item	= search_epg_item;
	m_search_keyword	= search_keyword;
	m_search_list		= search_list;
	m_search_channel_id	= search_channel_id;
	m_search_bouquet_id	= search_bouquet_id;
}


/************************************************************************************************/
int CEventFinderMenu::exec(CMenuTarget* parent, const std::string &actionkey)
/************************************************************************************************/
{
	int res = menu_return::RETURN_REPAINT;
	
	if(parent != NULL)
		parent->hide();

	if(actionkey.empty())
	{
		res = showMenu();
	}
	else if(actionkey == "select_channel")
	{
		// get channel id / bouquet id
		if(*m_search_list == EventList::SEARCH_LIST_CHANNEL)
		{
			int nNewBouquet = bouquetList->show();
			//printf("new_bouquet_id %d\n",nNewBouquet);
			if (nNewBouquet > -1)
			{
				int nNewChannel = bouquetList->Bouquets[nNewBouquet]->channelList->show();
				//printf("nNewChannel %d\n",nNewChannel);
				if (nNewChannel > -1)
				{
					*m_search_bouquet_id = nNewBouquet;
					*m_search_channel_id = bouquetList->Bouquets[nNewBouquet]->channelList->getActiveChannel_ChannelID();
					m_search_channelname = g_Zapit->getChannelName(*m_search_channel_id);
				}
			}
		}
		else if(*m_search_list == EventList::SEARCH_LIST_BOUQUET)
		{
			int nNewBouquet = bouquetList->show();
			//printf("new_bouquet_id %d\n",nNewBouquet);
			if (nNewBouquet > -1)
			{
				*m_search_bouquet_id = nNewBouquet;
				m_search_channelname = bouquetList->Bouquets[nNewBouquet]->channelList->getName();
			}
		}
	}
	else if(actionkey == "start_search")
	{
		*m_event = true;
		res = menu_return::RETURN_EXIT;
	}
	
	return res;
}

/************************************************************************************************/
int CEventFinderMenu::showMenu(void)
/************************************************************************************************/
{
	int res = menu_return::RETURN_REPAINT;
	*m_event = false;
	
	if(*m_search_list == EventList::SEARCH_LIST_CHANNEL)
	{
		m_search_channelname = g_Zapit->getChannelName(*m_search_channel_id);
	}
	else if(*m_search_list == EventList::SEARCH_LIST_BOUQUET)
	{
		if (*m_search_bouquet_id >= bouquetList->Bouquets.size())
			*m_search_bouquet_id = bouquetList->getActiveBouquetNumber();
		if (!bouquetList->Bouquets.empty())
			m_search_channelname = bouquetList->Bouquets[*m_search_bouquet_id]->channelList->getName();
		else
			m_search_channelname = "";
	}
	else if(*m_search_list == EventList::SEARCH_LIST_ALL)
	{
		m_search_channelname = "";
	}
	
	CStringInputSMS stringInput(LOCALE_EVENTFINDER_KEYWORD, m_search_keyword, 20, false, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz\xE4\xF6\xFC\xDF""0123456789-_/()<>=.,:!?\\'\"& ");
	
	CMenuForwarder* mf0 = new CMenuForwarder(LOCALE_EVENTFINDER_KEYWORD, true, *m_search_keyword, &stringInput, NULL, CRCInput::RC_red);
	CMenuOptionChooser* mo0 = new CMenuOptionChooser(LOCALE_EVENTFINDER_SEARCH_WITHIN_LIST, m_search_list, SEARCH_LIST_OPTIONS, SEARCH_LIST_OPTION_COUNT, true, this, CRCInput::RC_1);
	m_search_channelname_mf = new CMenuForwarder("", *m_search_list != EventList::SEARCH_LIST_ALL, m_search_channelname, this, "select_channel", CRCInput::RC_2);
	CMenuOptionChooser* mo1 = new CMenuOptionChooser(LOCALE_EVENTFINDER_SEARCH_WITHIN_EPG, m_search_epg_item, SEARCH_EPG_OPTIONS, SEARCH_EPG_OPTION_COUNT, true, NULL, CRCInput::RC_3);
	CMenuForwarder* mf1 = new CMenuForwarder(LOCALE_EVENTFINDER_START_SEARCH, true, NULL, this, "start_search", CRCInput::RC_green);
	
	CMenuWidget searchMenu(LOCALE_EVENTFINDER_HEAD, NEUTRINO_ICON_FEATURES, 450);

	searchMenu.addIntroItems(NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, CMenuWidget::BTN_TYPE_CANCEL);
	//***************************************
	searchMenu.addItem(mf0);
	searchMenu.addItem(GenericMenuSeparatorLine);
	//***************************************
	searchMenu.addItem(mo0);
	searchMenu.addItem(m_search_channelname_mf);
	searchMenu.addItem(mo1);
	//***************************************
	searchMenu.addItem(GenericMenuSeparatorLine);
	searchMenu.addItem(mf1);
	
	res = searchMenu.exec(NULL,"");
	return(res);
}

/************************************************************************************************/
bool CEventFinderMenu::changeNotify(const neutrino_locale_t OptionName, void *)
/************************************************************************************************/
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_EVENTFINDER_SEARCH_WITHIN_LIST))
	{
		if (*m_search_list == EventList::SEARCH_LIST_CHANNEL)
		{
			m_search_channelname = g_Zapit->getChannelName(*m_search_channel_id);
			m_search_channelname_mf->setActive(true);
		}
		else if (*m_search_list == EventList::SEARCH_LIST_BOUQUET)
		{
			if (*m_search_bouquet_id >= bouquetList->Bouquets.size())
				*m_search_bouquet_id = bouquetList->getActiveBouquetNumber();
			if (!bouquetList->Bouquets.empty())
				m_search_channelname = bouquetList->Bouquets[*m_search_bouquet_id]->channelList->getName();
			else
				m_search_channelname = "";
			m_search_channelname_mf->setActive(true);
		}
		else if (*m_search_list == EventList::SEARCH_LIST_ALL)
		{
			m_search_channelname = "";
			m_search_channelname_mf->setActive(false);
		}
	}
	return false;
}
