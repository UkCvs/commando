/*
	$Id: infoviewer.cpp,v 1.316 2012/06/30 10:54:18 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Bugfixes/cleanups/dreambox port (C) 2007-2013 Stefan Seyfried
	(C) 2008 Novell, Inc. Author: Stefan Seyfried

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

#include <gui/infoviewer.h>
#include <gui/widget/icons.h>
#include <gui/widget/buttons.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/progressbar.h>


#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */
#include <zapit/client/zapittools.h>
#include <iostream>
#include <global.h>
#include <neutrino.h>

#include <algorithm>
#include <string>
#include <system/settings.h>
#include <system/helper.h>

#include <time.h>
#include <sys/param.h>
#include <unistd.h>

/* for showInfoFile... */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define COL_INFOBAR_BUTTONS_BACKGROUND (COL_INFOBAR_SHADOW_PLUS_1)

#define ICON_LARGE_WIDTH 26
#define ICON_SMALL_WIDTH 16
#define ICON_HEIGHT 16
#define ICON_OFFSET (2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 6)
#define BOTTOM_BAR_OFFSET 0
#define BOTTOM_BAR_FONT_OFFSET 5
#define borderwidth 4

#define RED_BAR 70
#define YELLOW_BAR 80
#define GREEN_BAR 100

// in us
#define FADE_TIME 40000

#define LCD_UPDATE_TIME_TV_MODE (60 * 1000 * 1000)

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_start(int tpid);
extern "C" int  tuxtxt_stop();
#endif

/* hack: remember the last shown event IDs to reduce flickering */
event_id_t CInfoViewer::last_curr_id = 0, CInfoViewer::last_next_id = 0;

extern CZapitClient::SatelliteList satList;

static bool sortByDateTime (const CChannelEvent& a, const CChannelEvent& b)
{
	return a.startTime < b.startTime;
}

CInfoViewer::CInfoViewer()
{
	frameBuffer      = CFrameBuffer::getInstance();

	BoxStartX        = BoxStartY = BoxEndX = BoxEndY = 0;
#ifdef ENABLE_RADIOTEXT
	rt_x             = rt_y      = rt_w    = rt_h    = 0;
#endif
	recordModeActive = false;
	is_visible       = false;
	showButtonBar    = false;
	gotTime          = g_Sectionsd->getIsTimeSet();
	CA_Status        = false;
	virtual_zap_mode = false;
	lcdUpdateTimer   = 0;
	oldinfo.current_uniqueKey = 0;
	oldinfo.next_uniqueKey = 0;
}

void CInfoViewer::start()
{
	InfoHeightY = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getHeight()*9/8 +
		2*g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight() +
		25;

	ChanWidth = 4 * g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getMaxDigitWidth() + 10;
	ChanHeight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getHeight()*9/8;

	ProgressBarHeight = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight() - 4;

	aspectRatio = g_Controld->getAspectRatio();
	
	time_height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getHeight()+5;
	time_left_width = 2 * g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getMaxDigitWidth();
	time_dot_width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->getRenderWidth(":");
	time_width = time_left_width* 2+ time_dot_width;

	if (lcdUpdateTimer == 0)
		lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_TV_MODE, false, true );
}

void CInfoViewer::showSatfind()
{
 	if (!g_settings.infobar_sat_display || !is_visible || !showButtonBar)
		return;

	CProgressBar pbsig(true, -1, -1, RED_BAR, GREEN_BAR, YELLOW_BAR, false);
	CProgressBar pbsnr(true, -1, -1, RED_BAR, GREEN_BAR, YELLOW_BAR, false);

	CZapitClient::responseFESignal s;
	g_Zapit->getFESignal(s);

	signal.sig = s.sig & 0xFFFF;
	signal.snr = s.snr & 0xFFFF;
	signal.ber = (s.ber < 0x3FFFF) ? s.ber : 0x3FFFF;

	char freq[20];
	char pos[6];
	std::string percent;
	int percent_width;
	int sig;
	int snr;
	int ber;

	if (g_info.delivery_system == DVB_S)
		sig = signal.sig * 100 / 65535;
	else
	{
		if (signal.sig >= 65535)
			sig = 0;
		else if (signal.sig > 28671)
			// UK signal 0x6FFF
			sig = 100;
		else
			sig = signal.sig * 100 / 28671;
	}

	snr = signal.snr * 100 / 65535;
	ber = signal.ber / 2621;

	if ((signal.ber > 0) && (signal.ber < 2621))
		ber = 1;

	// only update if required
	if ((lastsig != sig) || (lastsnr != snr) || (lastber != ber))
	{
		lastsig = sig;
		lastsnr = snr;
		lastber = ber;

		CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo();
		if (g_info.delivery_system == DVB_S)
			sprintf (freq, "%d.%03d MHz (%c)", si.tsfrequency / 1000, si.tsfrequency % 1000, (si.polarisation == HORIZONTAL) ? 'h' : 'v');
		else
			sprintf (freq, "%d.%06d MHz", si.tsfrequency / 1000000, si.tsfrequency % 1000000);

		frameBuffer->paintBoxRel(ChanInfoX, BoxEndY, BoxEndX-ChanInfoX, 30, COL_INFOBAR_PLUS_0);

		percent = "sig " + to_string(sig) + "%";
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX+ 10, BoxEndY+ 25, BoxEndX- ChanInfoX- 10, percent, COL_INFOBAR_PLUS_0);
		percent_width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(percent);
		pbsig.paintProgressBar(ChanInfoX+ 10+ percent_width+ 5, BoxEndY+ 7, 60, 15, sig, 100, 0, 0, COL_INFOBAR_PLUS_0, 0, "", COL_INFOBAR);

		percent = "snr " + to_string(snr) + "%";
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX+ 140, BoxEndY+ 25, BoxEndX- ChanInfoX- 140, percent, COL_INFOBAR_PLUS_0);
		percent_width = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(percent);
		pbsnr.paintProgressBar(ChanInfoX+ 140+ percent_width+ 5, BoxEndY+ 7, 60, 15, snr, 100, 0, 0, COL_INFOBAR_PLUS_0, 0, "", COL_INFOBAR);

		percent = "ber " + to_string(ber); // no unit
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX+ 270, BoxEndY+ 25, BoxEndX- ChanInfoX- 270, percent, COL_INFOBAR_PLUS_0);

		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(ChanInfoX+ 345, BoxEndY+ 25, BoxEndX- ChanInfoX- 345, freq, COL_INFOBAR_PLUS_0);

		if (satpos != 0 && (g_info.delivery_system == DVB_S))
		{
			sprintf (pos, "%d.%d%c", satpos < 0 ? -satpos / 10 : satpos / 10, satpos < 0 ? -satpos % 10 : satpos % 10, satpos < 0 ? 'W' : 'E');
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxEndX- 55, BoxEndY+ 25, 55, pos, COL_INFOBAR_PLUS_0);
		}
	}
}

void CInfoViewer::paintTime( bool show_dot, bool firstPaint )
{
	if (!gotTime)
		gotTime = g_Sectionsd->getIsTimeSet();

	if ( gotTime )
	{
	    ChanNameY = BoxStartY + (ChanHeight>>1)   + SHADOW_OFFSET; //oberkante schatten?

		char timestr[10];
		struct timeval tm;
		struct tm lt;

		gettimeofday(&tm, NULL);
		localtime_r(&tm.tv_sec, &lt);
		strftime(timestr, sizeof(timestr), "%H:%M", &lt);

		if ( ( !firstPaint ) && ( strcmp( timestr, old_timestr ) == 0 ) )
		{
			if ( show_dot ) // top dot
				frameBuffer->paintBoxRel(BoxEndX- time_width+ time_left_width- 10, ChanNameY, time_dot_width, time_height/2+2, COL_INFOBAR_PLUS_0);
			else
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(BoxEndX- time_width+ time_left_width- 10, ChanNameY+ time_height, time_dot_width, ":", COL_INFOBAR);
			strcpy( old_timestr, timestr );
		}
		else
		{
			strcpy( old_timestr, timestr );

			if ( !firstPaint ) // background
			{
				// must also be painted with rounded corner on top right, if infobar have also a rounded corner on top right
				frameBuffer->paintBoxRel(BoxEndX- time_width- 10, ChanNameY, time_width+ 10, time_height, COL_INFOBAR_PLUS_0, RADIUS_LARGE, CORNER_TOP_RIGHT);
			}

			timestr[2]= 0;
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(BoxEndX- time_width- 10, ChanNameY+ time_height, time_left_width, timestr, COL_INFOBAR);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(BoxEndX- time_left_width- 10, ChanNameY+ time_height, time_left_width, &timestr[3], COL_INFOBAR);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(BoxEndX- time_width+ time_left_width- 10, ChanNameY+ time_height, time_dot_width, ":", COL_INFOBAR);
			if ( show_dot )
				frameBuffer->paintBoxRel(BoxEndX- time_left_width- time_dot_width- 10, ChanNameY, time_dot_width, time_height/2+2, COL_INFOBAR_PLUS_0);
		}
	}
}

void CInfoViewer::showRecordIcon(const bool show)
{
	if(recordModeActive)
	{
		int rec_icon_x = BoxStartX + ChanWidth + 20;
		int rec_icon_y = BoxStartY + 10;
		if(show)
		{
			frameBuffer->paintIcon(NEUTRINO_ICON_REC, rec_icon_x, rec_icon_y);
		}
		else
		{
			frameBuffer->paintBackgroundBoxRel(rec_icon_x, rec_icon_y, 20, 20);
		}
	}
}

/*
		 ___BoxStartX
		|-ChanWidth-|
		|           |  _recording icon                 _progress bar
    BoxStartY---+-----------+ |                               |
	|	|           | *  infobar.txt text            #######____
	|	|           |-------------------------------------------+--ChanNameY
	|	|           | Channelname                               |
    ChanHeight--+-----------+                                           |
		   |                                                    |
		   |01:23     Current Event                             |
		   |02:34     Next Event                                |
		   |                                                    |
    BoxEndY--------+----------------------------------------------------+
		                                                        |
		                                                BoxEndX-/
*/
void CInfoViewer::paintBackground(int col_NumBox)
{
	int c_shadow_width = (RADIUS_LARGE * 2) + 1;
	int ChanInfoY = BoxStartY + ChanHeight+ SHADOW_OFFSET;
	int BoxEndInfoY = BoxEndY;
	if (showButtonBar) // should we just always kill the button bar area, too?
		BoxEndInfoY += InfoHeightY_Info;
	// kill left side
	frameBuffer->paintBackgroundBox(BoxStartX,
					BoxStartY + ChanHeight - 6,
					BoxStartX + (ChanWidth/3),
					BoxStartY + ChanHeight + InfoHeightY_Info + 10 + 6);
	// kill progressbar + info-line
	frameBuffer->paintBackgroundBox(BoxStartX + ChanWidth + 40, // 40 for the recording icon!
					BoxStartY, BoxEndX, BoxStartY+ ChanHeight);

	// shadow...
	frameBuffer->paintBoxRel(BoxEndX - c_shadow_width,
				 ChanNameY + SHADOW_OFFSET,
				 SHADOW_OFFSET + c_shadow_width,
				 BoxEndInfoY - ChanNameY,
				 COL_INFOBAR_SHADOW_PLUS_0, RADIUS_LARGE, CORNER_RIGHT);
	frameBuffer->paintBoxRel(ChanInfoX + SHADOW_OFFSET,
				 BoxEndInfoY - c_shadow_width,
				 BoxEndX - ChanInfoX - SHADOW_OFFSET - c_shadow_width,
				 SHADOW_OFFSET + c_shadow_width,
				 COL_INFOBAR_SHADOW_PLUS_0, RADIUS_LARGE, CORNER_BOTTOM_LEFT);

	// background for channel name, epg data
	frameBuffer->paintBoxRel(ChanNameX - SHADOW_OFFSET,
				 ChanNameY,
				 BoxEndX - ChanNameX + SHADOW_OFFSET,
				 BoxEndY - ChanNameY,
				 COL_INFOBAR_PLUS_0, RADIUS_LARGE, CORNER_TOP_RIGHT | (showButtonBar ? 0 : CORNER_BOTTOM_RIGHT));

	// number box
	frameBuffer->paintBoxRel(BoxStartX + SHADOW_OFFSET,
				 BoxStartY + SHADOW_OFFSET,
				 ChanWidth,
				 ChanHeight,
				 COL_INFOBAR_SHADOW_PLUS_0, RADIUS_MID);
	frameBuffer->paintBoxRel(BoxStartX,
				 BoxStartY,
				 ChanWidth,
				 ChanHeight,
				 col_NumBox, RADIUS_MID);
	// paint background left side
	frameBuffer->paintBoxRel(ChanInfoX,
				 ChanInfoY,
				 ChanNameX - ChanInfoX,
				 BoxEndY - ChanInfoY,
				 COL_INFOBAR_PLUS_0, RADIUS_LARGE, showButtonBar ? 0 : CORNER_BOTTOM_LEFT);
}

void CInfoViewer::showMovieTitle(const int _playstate, const std::string &title, const std::string &sub_title,
				 const int percent, const time_t time_elapsed, const time_t time_remaining,
				 const int ac3state, const int vtxtpid, const int subpid,
				 const bool show_button_green, const int playmode)
{
					/* playmode is optional and only need for mp1 to show
					 correct caption for red and green button
					*/
	playstate = _playstate;
	const int mode = playmode;
	InfoHeightY_Info = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight() + BOTTOM_BAR_FONT_OFFSET;
	showButtonBar = true;
	bool fadeIn = (g_info.box_Type != CControld::TUXBOX_MAKER_NOKIA) && // dreambox and eNX only 
		g_settings.widget_fade && (!is_visible);

	is_visible = true;
	BoxStartX = g_settings.screen_StartX + 20;
	BoxEndX   = g_settings.screen_EndX - 20;
	BoxStartY = g_settings.screen_EndY - 20 - InfoHeightY - InfoHeightY_Info;
	BoxEndY   = g_settings.screen_EndY - 20 - InfoHeightY_Info;

	if (!gotTime)
		gotTime = g_Sectionsd->getIsTimeSet();

	if (fadeIn)
	{
		frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(100));
		frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(100));
		frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(100));
		frameBuffer->paletteSet();
#ifdef HAVE_DREAMBOX_HARDWARE
		usleep(100000);	// otherwise, the fade-in-effect is flashing on the dreambox :-(
#endif
#ifdef HAVE_TRIPLEDRAGON
		usleep(20000);	// this sucks bigtime!
#endif
	}

	ChanInfoX = BoxStartX + (ChanWidth / 3);
	ChanNameX = BoxStartX + ChanWidth;
	ChanNameY = BoxStartY + (ChanHeight / 2) + SHADOW_OFFSET;

	paintBackground(COL_INFOBAR_PLUS_0);

	//paint play state icon
	const char *icon;
	if (playstate == 4) // CMoviePlayerGui::PAUSE
		icon = NEUTRINO_ICON_PAUSE;
	else 
		icon = NEUTRINO_ICON_PLAY;
	/* calculate play state icon position, useful for using customized icons with other sizes */
	int icon_w = 0, icon_h = 0;
	frameBuffer->getIconSize(icon, &icon_w, &icon_h);
	if (icon_w > ChanWidth)
		icon_w = ChanWidth;
	if (icon_h > ChanHeight)
		icon_h = ChanHeight;
	int icon_x = BoxStartX + ChanWidth / 2 - icon_w / 2;
	int icon_y = BoxStartY + ChanHeight / 2 - icon_h / 2;
	frameBuffer->paintIcon(icon, icon_x, icon_y, 1, icon_w, icon_h);

	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(
				ChanNameX + 10,
				ChanNameY + time_height,
				BoxEndX - (ChanNameX + 20) - time_width - 15,
				g_Locale->getText(LOCALE_MOVIEPLAYER_HEAD), COL_INFOBAR, 0, true);

	paintTime(false, true);
	showInfoFile();

	sec_timer_id = g_RCInput->addTimer(1000000, false); //need for blinking dot in time

	frameBuffer->paintBoxRel(ChanInfoX, BoxEndY + BOTTOM_BAR_OFFSET,
				 BoxEndX - ChanInfoX, InfoHeightY_Info - BOTTOM_BAR_OFFSET,
				 COL_INFOBAR_BUTTONS_BACKGROUND, RADIUS_LARGE, CORNER_BOTTOM);

	showButton(SNeutrinoSettings::BUTTON_RED, true, mode);     // button red     // start plugin
	showButton(SNeutrinoSettings::BUTTON_YELLOW, true);  // button yellow  // playstatus
	showButton(SNeutrinoSettings::BUTTON_BLUE, true);    // buttons blue   // bookmarks

	if (show_button_green)
		showButton(SNeutrinoSettings::BUTTON_GREEN, true, mode);  // button green

	aspectRatio = g_Controld->getAspectRatio();
	showIcon_16_9();
	showIcon_Audio(ac3state);
	showIcon_VTXT(vtxtpid);
	showIcon_SubT(subpid);

	std::string runningRest = to_string((time_elapsed + 30) / 60) + " / " + to_string((time_remaining + 30) / 60) + " min";
	display_Info(title.c_str(), sub_title.c_str(), true, false, (percent * 112) / 100, NULL, runningRest.c_str());

	infobarLoop(false, fadeIn);
}

void CInfoViewer::showTitle(const int ChanNum, const std::string & Channel, const t_satellite_position satellitePosition, const t_channel_id new_channel_id, const bool calledFromNumZap, int epgpos)
{
	/* reset the "last shown eventid" markers */
	InfoHeightY_Info = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight() + BOTTOM_BAR_FONT_OFFSET + (g_settings.infobar_sat_display ? 30 : 0);

	lastsig = lastsnr = lastber = -1;
	last_curr_id = last_next_id = 0;
	showButtonBar = !calledFromNumZap;
	ChannelName = Channel;
	bool new_chan = false;
	
	bool fadeIn = (g_info.box_Type != CControld::TUXBOX_MAKER_NOKIA) && // dreambox and eNX only 
		g_settings.widget_fade &&
		(!is_visible) &&
		showButtonBar;

	is_visible = true;
#ifdef ENABLE_RADIOTEXT
	if (g_settings.radiotext_enable && g_Radiotext) {
		g_Radiotext->RT_MsgShow = true;
	}
#endif
	
	BoxStartX = g_settings.screen_StartX+ 20;
	BoxEndX   = g_settings.screen_EndX- 20;
	BoxStartY = g_settings.screen_EndY - 20 - InfoHeightY - InfoHeightY_Info;
	BoxEndY   = g_settings.screen_EndY - 20 - InfoHeightY_Info;

	if ( !gotTime )
		gotTime = g_Sectionsd->getIsTimeSet();

	if ( fadeIn )
	{
		frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(100));
		frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(100));
		frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(100));
		frameBuffer->paletteSet();
#ifdef HAVE_DREAMBOX_HARDWARE
		usleep(100000);	// otherwise, the fade-in-effect is flashing on the dreambox :-(
#endif
#ifdef HAVE_TRIPLEDRAGON
		usleep(20000);	// this sucks bigtime!
#endif
	}

	int col_NumBoxText = COL_INFOBAR;
	int col_NumBox = COL_INFOBAR_PLUS_0;
	if (virtual_zap_mode)
	{
		if (g_RemoteControl->current_channel_id != new_channel_id)
		{
			col_NumBoxText = COL_MENUHEAD;
			col_NumBox = COL_MENUHEAD_PLUS_0;
		}
		if ((channel_id != new_channel_id) || (evtlist.empty()))
		{
			evtlist = g_Sectionsd->getEventsServiceKey(new_channel_id);
			if (!evtlist.empty())
				sort(evtlist.begin(),evtlist.end(), sortByDateTime);
			new_chan = true;
		}
	}

	// get channel-id
	// ...subchannel is selected
	if (! calledFromNumZap && !(g_RemoteControl->subChannels.empty()) && (g_RemoteControl->selected_subchannel > 0)) 
	{
		channel_id = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].getChannelID();
		ChannelName += ": " + ZapitTools::Latin1_to_UTF8(g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].subservice_name.c_str());
	}
	else // ...channel is selected
	{
		channel_id = new_channel_id;
	}

	//infobox
	ChanInfoX = BoxStartX + (ChanWidth / 3);
	ChanNameX = BoxStartX + ChanWidth;
	ChanNameY = BoxStartY + (ChanHeight>>1)   + SHADOW_OFFSET; //oberkante schatten?
	int ChanNumYPos = BoxStartY + ChanHeight;

	paintBackground(col_NumBox);
	// sat display
	if (g_settings.infobar_sat_display)
	{
		satpos = satellitePosition;
/* satname not used in numberbox, use satpos bottom of infobar instead
		for (CZapitClient::SatelliteList::const_iterator satList_it = satList.begin(); satList_it != satList.end(); satList_it++)
			if (satList_it->satPosition == satellitePosition)
			{
				int satNameWidth = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(satList_it->satName);
				if (satNameWidth > ChanWidth)
					satNameWidth = ChanWidth;

				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(BoxStartX + ((ChanWidth - satNameWidth)>>1), BoxStartY + 22, ChanWidth, satList_it->satName, col_NumBoxText);
				ChanNumYPos += 10;

				break;
			}
*/
	}
	
/* paint channel number, channelname or/and channellogo */
	sprintf((char*) strChanNum, "%d", ChanNum);

	int ChannelLogoMode = showChannelLogo(channel_id); // get logo mode, paint channel logo if adjusted

	if (ChannelLogoMode != LOGO_AS_CHANNELNUM) // no logo in numberbox
	{
		// show channel number in numberbox
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->RenderString(BoxStartX + ((ChanWidth - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER]->getRenderWidth(strChanNum))>>1), ChanNumYPos, ChanWidth, strChanNum, col_NumBoxText);
	}

	ChanNameW = BoxEndX- (ChanNameX+ 20)- time_width- 15; // set channel name width

	// ... with channel name 
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME]->RenderString(ChanNameX + 10, ChanNameY+ time_height, BoxEndX- (ChanNameX+ 20)- time_width- 15, ChannelName, COL_INFOBAR, 0, true); // UTF-8
/* paint channel number, channelname or/and channellogo */

	paintTime( false, true );
	showInfoFile();

	ButtonWidth = (BoxEndX- ChanInfoX- ICON_OFFSET)>> 2;

	if ( showButtonBar )
	{
		sec_timer_id = g_RCInput->addTimer(1000000, false);

		if ( BOTTOM_BAR_OFFSET> 0 )
			frameBuffer->paintBackgroundBox(ChanInfoX, BoxEndY, BoxEndX, BoxEndY+ BOTTOM_BAR_OFFSET);

		frameBuffer->paintBoxRel(ChanInfoX, BoxEndY + BOTTOM_BAR_OFFSET, BoxEndX - ChanInfoX, InfoHeightY_Info - BOTTOM_BAR_OFFSET, COL_INFOBAR_BUTTONS_BACKGROUND, RADIUS_LARGE, CORNER_BOTTOM);

		if (g_settings.infobar_sat_display)
			frameBuffer->paintBoxRel(ChanInfoX, BoxEndY, BoxEndX - ChanInfoX, 30, COL_INFOBAR_PLUS_0); // background for satfind

		showButton(SNeutrinoSettings::BUTTON_BLUE); // button blue // USERMENU
		showInfoIcons();
		showSatfind();
	}

	g_Sectionsd->getCurrentNextServiceKey(channel_id, info_CurrentNext);
	if (!calledFromNumZap)
		CLCD::getInstance()->setEPGTitle(info_CurrentNext.current_name);

	if (!evtlist.empty()) {
		if (new_chan) {
			for ( eli=evtlist.begin(); eli!=evtlist.end(); ++eli ) {
				if ((uint)eli->startTime >= info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer)
					break;
			}
			if (eli == evtlist.end()) // the end is not valid, so go back
				--eli;
		}

		/* epgpos != 0 => virtual zap mode */
		if (epgpos != 0) {
			info_CurrentNext.flags = 0;
			if ((epgpos > 0) && (eli != evtlist.end())) {
				++eli; // next epg
				if (eli == evtlist.end()) // the end is not valid, so go back
					--eli;
			}
			else if ((epgpos < 0) && (eli != evtlist.begin())) {
				--eli; // prev epg
			}
			info_CurrentNext.flags = CSectionsdClient::epgflags::has_current;
			info_CurrentNext.current_uniqueKey	= eli->eventID;
			info_CurrentNext.current_zeit.startzeit	= eli->startTime;
			info_CurrentNext.current_zeit.dauer	= eli->duration;
			if (eli->description.empty())
				info_CurrentNext.current_name	= ZapitTools::UTF8_to_Latin1(g_Locale->getText(LOCALE_INFOVIEWER_NOEPG));
			else
				info_CurrentNext.current_name	= eli->description;
			info_CurrentNext.current_fsk		= '\0';

			if (eli != evtlist.end()) {
				++eli;
				if (eli != evtlist.end()) {
					info_CurrentNext.flags 			= CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_next;
					info_CurrentNext.next_uniqueKey		= eli->eventID;
					info_CurrentNext.next_zeit.startzeit 	= eli->startTime;
					info_CurrentNext.next_zeit.dauer	= eli->duration;
					if (eli->description.empty())
						info_CurrentNext.next_name	= ZapitTools::UTF8_to_Latin1(g_Locale->getText(LOCALE_INFOVIEWER_NOEPG));
					else
						info_CurrentNext.next_name	= eli->description;
				}
				--eli;
			}
			if (info_CurrentNext.flags)
				info_CurrentNext.flags |= CSectionsdClient::epgflags::has_anything;
		}
	}

	if (!(info_CurrentNext.flags & (CSectionsdClient::epgflags::has_anything | CSectionsdClient::epgflags::not_broadcast)))
	{
		neutrino_locale_t loc;
		if (! gotTime)
			loc = LOCALE_INFOVIEWER_WAITTIME;
		else if (showButtonBar)
			loc = LOCALE_INFOVIEWER_EPGWAIT;
		else
			loc = LOCALE_INFOVIEWER_EPGNOTLOAD;
		display_Info(g_Locale->getText(loc), NULL);
	}
	else
	{
		show_Data();
	}

	showLcdPercentOver();

#if 0
	if ( ( g_RemoteControl->current_channel_id == channel_id) &&
		!( ( ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_next ) &&
			( info_CurrentNext.flags & ( CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_no_current ) ) ) ||
				( info_CurrentNext.flags & CSectionsdClient::epgflags::not_broadcast ) ) )
	{
		// EVENT anfordern!
		g_Sectionsd->setServiceChanged(channel_id, true );
	}
#endif

#ifdef ENABLE_RADIOTEXT
	if (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio)
	{
		if ((g_settings.radiotext_enable) && (!recordModeActive) && (!calledFromNumZap))
			showRadiotext();
		else
			showIcon_RadioText(false);
	}
#endif
	infobarLoop(calledFromNumZap, fadeIn);
}

void CInfoViewer::infobarLoop(bool calledFromNumZap, bool fadeIn)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int fadeValue;
	if (fadeIn)
		fadeValue = 100;
	else
		fadeValue= g_settings.infobar_alpha;
	bool fadeOut = false;

	CNeutrinoApp *neutrino = CNeutrinoApp::getInstance();

	if ( !calledFromNumZap )
	{
		int mode = neutrino->getMode();
		bool tsmode = (mode == NeutrinoMessages::mode_ts);
		bool show_dot= true;
		if ( fadeIn )
			fadeTimer = g_RCInput->addTimer( FADE_TIME, false );

		bool hideIt = true;
		unsigned long long timeoutEnd;
		switch (mode)
		{
			case NeutrinoMessages::mode_tv:
					timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR]);
					break;
			case NeutrinoMessages::mode_radio:
					timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR_RADIO]);
					break;
			case NeutrinoMessages::mode_ts:
					timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR_MOVIE]);
					break;
			default:
					timeoutEnd = CRCInput::calcTimeoutEnd(6); //default 6 seconds
					break;
		}

		int res = messages_return::none;

		while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
			neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;
			//printf(" g_RCInput->getMsgAbsoluteTimeout %x %x\n", msg, data);
#if 0
There is no need to poll for EPG when we are going to get events from sectionsd. Saves lots of useless
requests to sectionsd.
			if ( !( info_CurrentNext.flags & ( CSectionsdClient::epgflags::has_current ) ) )
			{
				if(difftime(time(&tb),ta) > 1.1)
				{
					time(&ta);
					info_CurrentNext = getEPG(channel_id);
					if ( ( info_CurrentNext.flags & ( CSectionsdClient::epgflags::has_current ) ) )
					{
						show_Data();
						showLcdPercentOver();
					}
				}
			}
#endif

			if ( msg == CRCInput::RC_help )
			{
				g_RCInput->postMsg( NeutrinoMessages::SHOW_EPG, 0 );
				res = messages_return::cancel_info;
			}
			else if ( ( msg == NeutrinoMessages::EVT_TIMER ) && ( data == fadeTimer ) )
			{
				if ( fadeOut )
				{
					fadeValue+= 15;

					if ( fadeValue>= 100 )
					{
						fadeValue= 100;
						g_RCInput->killTimer(fadeTimer);
						res = messages_return::cancel_info;
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(100) );
					}
					else
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(fadeValue) );
				}
				else
				{
					fadeValue-= 15;
					if ( fadeValue<= g_settings.infobar_alpha )
					{
						fadeValue= g_settings.infobar_alpha;
						g_RCInput->killTimer(fadeTimer);
						fadeTimer = 0;
//						fadeIn = false;
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(0) );
					}
					else
						frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(fadeValue) );
				}

				frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(fadeValue) );
				frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(fadeValue) );
				frameBuffer->paletteSet();
			}
			else if ((msg == CRCInput::RC_ok) ||
				 (msg == CRCInput::RC_home) ||
				 (msg == CRCInput::RC_timeout))
			{
				if ( fadeIn )
				{
					if (fadeTimer)
						g_RCInput->killTimer(fadeTimer);
					fadeIn = false;
					fadeOut = true;
					fadeTimer = g_RCInput->addTimer( FADE_TIME, false );
					timeoutEnd = CRCInput::calcTimeoutEnd( 1 );
				}
				else
				{
					if (!tsmode && (msg != CRCInput::RC_timeout) && (msg != CRCInput::RC_ok))
						g_RCInput->postMsg( msg, data );
					res = messages_return::cancel_info;
				}
			}
			else if (msg_repeatok == g_settings.key_quickzap_up ||
				 msg_repeatok == g_settings.key_quickzap_down ||
				/* msg == CRCInput::RC_0 || Not longer used here, because RC_0 is used
				   now for subchannel toggle, not for numeric Zap in director_mode */
				 msg == NeutrinoMessages::SHOW_INFOBAR)
			{
#ifdef ENABLE_RADIOTEXT
				if ((g_settings.radiotext_enable) && (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio))
					hideIt =  true;
				else
#endif
					hideIt = tsmode; // in movieplayer mode, hide infobar
				g_RCInput->postMsg( msg, data );
				res = messages_return::cancel_info;
			}
			else if ( msg == NeutrinoMessages::EVT_TIMESET )
			{
				/* handle timeset event in upper layer, ignore here */
				res = neutrino->handleMsg (msg, data);
			}
			else if ( ( msg == NeutrinoMessages::EVT_TIMER ) && ( data == sec_timer_id ) )
			{
				paintTime( show_dot, false );
				showRecordIcon(show_dot);
				show_dot = !show_dot;
				if (show_dot && !tsmode)
					showSatfind();
#ifdef ENABLE_RADIOTEXT
				if ((g_settings.radiotext_enable) && (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio))
					showRadiotext();
#endif
			}
			else if (!tsmode && g_settings.virtual_zap_mode &&
				 (msg == CRCInput::RC_right || msg == CRCInput::RC_left))
			{
				virtual_zap_mode = true;
				res = messages_return::cancel_all;
				hideIt = true;
			}
			else if (!(msg & CRCInput::RC_Release) && //ignore key release ...
				 msg != (CRCInput::RC_help | CRCInput::RC_Repeat)) //...and help key repeat
			{
				res = neutrino->handleMsg(msg, data);

				if ( res & messages_return::unhandled )
				{
					// raus hier und im Hauptfenster behandeln...
					g_RCInput->postMsg(  msg, data );
					res = messages_return::cancel_info;
				}
			}
		}

		if ( hideIt )
			killTitle();

		g_RCInput->killTimer(sec_timer_id);

		if ( fadeIn || fadeOut )
		{
			g_RCInput->killTimer(fadeTimer);
			frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
			frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
			frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(0) );
			frameBuffer->paletteSet();
		}
		if (virtual_zap_mode)
			CNeutrinoApp::getInstance()->channelList->virtual_zap_mode(msg == CRCInput::RC_right);
	}
}


void CInfoViewer::showSubchan()
{
	CNeutrinoApp *neutrino = CNeutrinoApp::getInstance();

	std::string subChannelName; 	// holds the name of the subchannel/audio channel
	int subchannel=0;		// holds the channel index
	bool subChannelNameIsUTF = false;

	if (!(g_RemoteControl->subChannels.empty())) {
		// get info for nvod/subchannel
		subchannel = g_RemoteControl->selected_subchannel;
		if ( g_RemoteControl->selected_subchannel >= 0)
			subChannelName = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].subservice_name;
	}
	else if (g_RemoteControl->current_PIDs.APIDs.size()>1 && g_settings.audiochannel_up_down_enable)
	{
		// get info for audio channel
		subchannel = g_RemoteControl->current_PIDs.PIDs.selected_apid + 1;
		subChannelName = g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].desc;
		subChannelNameIsUTF = true;
	}

	if (!(subChannelName.empty()))
	{
		char text[100];
		if (subchannel == 0)
			sprintf( text, "%s - %s", "  ", subChannelName.c_str() );
		else
			sprintf( text, "%d - %s", subchannel, subChannelName.c_str() );

		int dx = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(text, subChannelNameIsUTF) + 20;
		int dy = 25;
		
		if( g_settings.infobar_subchan_disp_pos == SUBCHAN_DISP_POS_INFOBAR )
		{
			// show full infobar for subschannel 
			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR , 0 );
		}
		else
		{	
			if ( g_RemoteControl->director_mode )
			{
				int w= 20+ g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(g_Locale->getText(LOCALE_NVODSELECTOR_DIRECTORMODE), true) + 20; // UTF-8
				if ( w> dx )
					dx= w;
				dy= dy* 3;
			}
			else
				dy= dy +5;
	
			int x=0,y=0;
			if( g_settings.infobar_subchan_disp_pos == SUBCHAN_DISP_POS_TOP_RIGHT )
			{
				x = g_settings.screen_EndX - dx - 10;
				y = g_settings.screen_StartY + 10;
			}
			else if( g_settings.infobar_subchan_disp_pos == SUBCHAN_DISP_POS_TOP_LEFT )
			{
				x = g_settings.screen_StartX + 10;
				y = g_settings.screen_StartY + 10;
			}
			else if( g_settings.infobar_subchan_disp_pos == SUBCHAN_DISP_POS_BOTTOM_LEFT )
			{
				x = g_settings.screen_StartX + 10;
				y = g_settings.screen_EndY - dy - 10;
			}
			else if( g_settings.infobar_subchan_disp_pos == SUBCHAN_DISP_POS_BOTTOM_RIGHT )
			{
				x = g_settings.screen_EndX - dx - 10;
				y = g_settings.screen_EndY - dy - 10;
			}
	
			fb_pixel_t pixbuf[(dx+ 2* borderwidth) * (dy+ 2* borderwidth)];
			frameBuffer->SaveScreen(x- borderwidth, y- borderwidth, dx+ 2* borderwidth, dy+ 2* borderwidth, pixbuf);
	
			// clear border
			frameBuffer->paintBackgroundBoxRel(x- borderwidth, y- borderwidth, dx+ 2* borderwidth, borderwidth);
			frameBuffer->paintBackgroundBoxRel(x- borderwidth, y+ dy, dx+ 2* borderwidth, borderwidth);
			frameBuffer->paintBackgroundBoxRel(x- borderwidth, y, borderwidth, dy);
			frameBuffer->paintBackgroundBoxRel(x+ dx, y, borderwidth, dy);
			
			{			
			// show default small infobar for subchannel
			frameBuffer->paintBoxRel(x, y, dx, dy, COL_MENUCONTENT_PLUS_0, RADIUS_SMALL);

			// take the dimensions only from yellow icon
			int icon_w = 0, icon_h = 0;
			frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_YELLOW, &icon_w, &icon_h); 
			if (subchannel == 0)
				frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, x + 16 - (icon_w >> 1), y + 16 - (icon_h >> 1));
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(x+10, y+ 30, dx-20, text, COL_MENUCONTENT, 0, subChannelNameIsUTF); // UTF-8
			
			// show yellow and blue button
			// USERMENU
			const char* txt_a = NULL;
			const char* txt_b = NULL;	
			if (g_RemoteControl->director_mode)
			{
				txt_a = g_RemoteControl->subChannels[0].subservice_name.c_str();
				if (!g_settings.usermenu_text[SNeutrinoSettings::BUTTON_YELLOW].empty())
					txt_b = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_YELLOW].c_str();
				else
					txt_b = g_Locale->getText(LOCALE_NVODSELECTOR_DIRECTORMODE);
			}

			if (txt_a != NULL && txt_b != NULL)
			{
				frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, x+ 16- (icon_w >> 1), y+ dy- 33-(icon_h >> 1));
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ 30, y+ dy- 22, dx- 40, txt_a, COL_MENUCONTENT, 0, true); // UTF-8
				frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, x+ 16- (icon_w >> 1), y+ dy- 13- (icon_h >> 1));
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+ 30, y+ dy- 2, dx- 40, txt_b, COL_MENUCONTENT, 0, true); // UTF-8
			}
			
			unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( 2 );
			int res = messages_return::none;
	
			neutrino_msg_t      msg;
			neutrino_msg_data_t data;
	
			while (!(res & (messages_return::cancel_info | messages_return::cancel_all)))
			{
				g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

				if (msg == CRCInput::RC_timeout)
				{
					res = messages_return::cancel_info;
				}
				else
				{
					res = neutrino->handleMsg(msg, data);

					if (res & messages_return::unhandled)
					{
						// raus hier und im Hauptfenster behandeln...
						g_RCInput->postMsg(msg, data);
						res = messages_return::cancel_info;
					}
				}
			}

			frameBuffer->RestoreScreen(x- borderwidth, y- borderwidth, dx+ 2* borderwidth, dy+ 2* borderwidth, pixbuf);
			}
		}
	}
	else
	{
		g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
	}
}

struct button_label InfobarButtons[4] =
{
	{ NEUTRINO_ICON_BUTTON_RED    , LOCALE_GENERIC_EMPTY  },
	{ NEUTRINO_ICON_BUTTON_GREEN  , LOCALE_GENERIC_EMPTY  },
	{ NEUTRINO_ICON_BUTTON_YELLOW , LOCALE_GENERIC_EMPTY  },
	{ NEUTRINO_ICON_BUTTON_BLUE   , LOCALE_GENERIC_EMPTY  }
};

void CInfoViewer::showButton(const int button, const bool calledFromMPlayer, const int mode) const //GETAWAY
{
	const char* txt = NULL;
	int bx = BoxStartX + (ChanWidth / 3);
	int by = BoxEndY + (InfoHeightY_Info >> 3) + ((g_settings.infobar_sat_display && !calledFromMPlayer) ? 25 : 0);
	int startX = bx;
	bool paint = true;

	switch (button)
	{
		case SNeutrinoSettings::BUTTON_RED:  // show red button  // USERMENU
			if (calledFromMPlayer)
			{
				if (mode == VLC_MODE)
					InfobarButtons[SNeutrinoSettings::BUTTON_RED].locale = LOCALE_AUDIOPLAYER_STOP;
				else
					InfobarButtons[SNeutrinoSettings::BUTTON_RED].locale = LOCALE_MOVIEPLAYER_PLUGIN;
			}
			else
			{
				if (!g_settings.usermenu_text[SNeutrinoSettings::BUTTON_RED].empty()) {
					txt = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_RED].c_str();
				}
				else if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_anything) {
					InfobarButtons[SNeutrinoSettings::BUTTON_RED].locale = LOCALE_INFOVIEWER_EVENTLIST;
				}
				else
					return;
			}
			startX = bx + 6;
			break;

		case SNeutrinoSettings::BUTTON_GREEN:  // show green button  // USERMENU
			if(calledFromMPlayer)
			{
				if (mode == VLC_MODE)
					InfobarButtons[SNeutrinoSettings::BUTTON_GREEN].locale = LOCALE_MOVIEPLAYER_RESYNC;
				else
					InfobarButtons[SNeutrinoSettings::BUTTON_GREEN].locale = LOCALE_INFOVIEWER_LANGUAGES;
			}
			else
			{
				// green, in case of several APIDs
				// -- always show Audio Option, due to audio option restructuring (2005-08-31 rasc)
				uint count = g_RemoteControl->current_PIDs.APIDs.size();
				if (!g_settings.usermenu_text[SNeutrinoSettings::BUTTON_GREEN].empty())
					txt = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_GREEN].c_str();
				else if (g_settings.audio_left_right_selectable || count > 1 ||
				         !g_RemoteControl->current_PIDs.SubPIDs.empty())
					InfobarButtons[SNeutrinoSettings::BUTTON_GREEN].locale = LOCALE_INFOVIEWER_LANGUAGES;
				else
					paint = false;

				int ac3state;
				if ( ( g_RemoteControl->current_PIDs.PIDs.selected_apid < count ) &&
				     ( g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3 ) )
					ac3state = AC3_ACTIVE;
				else if ( g_RemoteControl->has_ac3 )
					ac3state = AC3_AVAILABLE;
				else
					ac3state = NO_AC3;		

				showIcon_Audio(ac3state);
			}
			startX = BoxEndX - ICON_OFFSET - 3 * ButtonWidth;
			break;

		case SNeutrinoSettings::BUTTON_YELLOW:  // yellow button for subservices / NVODs  // USERMENU // Movieplayer
			if (calledFromMPlayer)
			{
				if (playstate == 4) // CMoviePlayerGui::PAUSE
					InfobarButtons[SNeutrinoSettings::BUTTON_YELLOW].locale = LOCALE_AUDIOPLAYER_PLAY;
				else 
					InfobarButtons[SNeutrinoSettings::BUTTON_YELLOW].locale = LOCALE_AUDIOPLAYER_PAUSE;
			}
			else
			{
				if (!g_settings.usermenu_text[SNeutrinoSettings::BUTTON_YELLOW].empty()) {
					txt = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_YELLOW].c_str();
				}
				else if (!g_RemoteControl->subChannels.empty())	{
					InfobarButtons[SNeutrinoSettings::BUTTON_YELLOW].locale = g_RemoteControl->are_subchannels ? LOCALE_INFOVIEWER_SUBSERVICE : LOCALE_INFOVIEWER_SELECTTIME;
				}
				else
					return;
			}
			startX = BoxEndX - ICON_OFFSET - 2 * ButtonWidth;
			break;

		case SNeutrinoSettings::BUTTON_BLUE:  // blue button  // USERMENU  //Movieplayer
			if (calledFromMPlayer)
			{
				InfobarButtons[SNeutrinoSettings::BUTTON_BLUE].locale = LOCALE_MOVIEPLAYER_BOOKMARK;
			}
			else
			{
				if (g_RemoteControl->director_mode)
					InfobarButtons[SNeutrinoSettings::BUTTON_BLUE].locale = LOCALE_INFOVIEWER_SUBCHAN_PORTAL;
				else if (!g_settings.usermenu_text[SNeutrinoSettings::BUTTON_BLUE].empty())
					txt = g_settings.usermenu_text[SNeutrinoSettings::BUTTON_BLUE].c_str();
				else
					InfobarButtons[SNeutrinoSettings::BUTTON_BLUE].locale = LOCALE_INFOVIEWER_STREAMINFO;
			}
			startX = BoxEndX - ICON_OFFSET - ButtonWidth;
			break;

		default:
			break;
	}
		if (paint)
			::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL],	g_Locale,
				startX, by, ButtonWidth, 1, &InfobarButtons[button], 0, false, COL_INFOBAR_SHADOW_PLUS_1, txt);
}

#ifdef ENABLE_RADIOTEXT
void CInfoViewer::showIcon_RadioText(bool rt_available) const
// painting the icon for radiotext mode
{
	if (showButtonBar)
	{
		int mode = CNeutrinoApp::getInstance()->getMode();
		std::string rt_icon = NEUTRINO_ICON_RADIOTEXT_OFF;
		if ((!virtual_zap_mode) && (!recordModeActive) && (mode == NeutrinoMessages::mode_radio))
		{
			if (g_settings.radiotext_enable){
					rt_icon = rt_available ? NEUTRINO_ICON_RADIOTEXT_GET : NEUTRINO_ICON_RADIOTEXT_WAIT;
				}
		}
		frameBuffer->paintIcon(rt_icon, BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6), BoxEndY + (g_settings.infobar_sat_display ? 15 : 0) + (InfoHeightY_Info - ICON_HEIGHT) / 2);
	}
}
#endif

void CInfoViewer::showIcon_16_9() const
{
	int mode = CNeutrinoApp::getInstance()->getMode();
	bool tsmode = (mode == NeutrinoMessages::mode_ts);

#ifdef ENABLE_RADIOTEXT
	if (mode != NeutrinoMessages::mode_radio)
#endif
	frameBuffer->paintIcon((aspectRatio != 0) ? NEUTRINO_ICON_16_9 : NEUTRINO_ICON_16_9_GREY,
				BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),
				BoxEndY + (g_settings.infobar_sat_display && !tsmode ? 15 : 0) + (InfoHeightY_Info - ICON_HEIGHT) / 2);
}

void CInfoViewer::showIcon_VTXT(const int VTxtPid) const
{
	int vtpid = (VTxtPid > -1) ? VTxtPid : g_RemoteControl->current_PIDs.PIDs.vtxtpid;

	frameBuffer->paintIcon((vtpid != 0) ? NEUTRINO_ICON_VTXT : NEUTRINO_ICON_VTXT_GREY,
				BoxEndX - (ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),
				BoxEndY + (g_settings.infobar_sat_display && VTxtPid < 0 ? 15 : 0) + (InfoHeightY_Info - ICON_HEIGHT) / 2);

#ifndef TUXTXT_CFG_STANDALONE
	if(g_settings.tuxtxt_cache && !CNeutrinoApp::getInstance ()->recordingstatus)
	{
		static int last_vtpid=0;
		if(vtpid !=last_vtpid)
		{
			tuxtxt_stop();
			if(vtpid)
				tuxtxt_start(vtpid);
			last_vtpid=vtpid;
		}
	}
#endif
}

void CInfoViewer::showIcon_SubT(const int SubPid) const
{
	int subpid = 0;

	for (unsigned i = 0 ;
		i < g_RemoteControl->current_PIDs.SubPIDs.size() ; i++) {
		if (g_RemoteControl->current_PIDs.SubPIDs[i].pid !=
			g_RemoteControl->current_PIDs.PIDs.vtxtpid) {
			subpid = g_RemoteControl->current_PIDs.SubPIDs[i].pid;
			break;
		}
	}

	if (SubPid > -1)
		subpid = SubPid;

	frameBuffer->paintIcon((subpid != 0) ? NEUTRINO_ICON_SUBT : NEUTRINO_ICON_SUBT_GREY,
				BoxEndX - (ICON_SMALL_WIDTH + 6),
				BoxEndY + (g_settings.infobar_sat_display && SubPid < 0 ? 15 : 0) + (InfoHeightY_Info - ICON_HEIGHT) / 2);
}

void CInfoViewer::showIcon_Audio(const int ac3state) const
{
	const char *dd_icon;
	switch (ac3state)
	{
		case AC3_ACTIVE:
			dd_icon = NEUTRINO_ICON_DD;
			break;
		case AC3_AVAILABLE:
			dd_icon = NEUTRINO_ICON_DD_AVAIL;
			break;
		case NO_AC3:
		default:
			dd_icon = NEUTRINO_ICON_DD_GREY;
			break;
	}

	bool tsmode = (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_ts);

	frameBuffer->paintIcon(dd_icon,
			       BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),
			       BoxEndY + (g_settings.infobar_sat_display && !tsmode ? 15 : 0) + (InfoHeightY_Info - ICON_HEIGHT) / 2);
}

void CInfoViewer::showInfoIcons()
{
	showButton(SNeutrinoSettings::BUTTON_YELLOW); // Button yellow // SubServices
	showIcon_16_9();
	showIcon_VTXT();
	showIcon_SubT();
	showButton(SNeutrinoSettings::BUTTON_GREEN); // Button green // Audio
	showIcon_CA_Status();
}

void CInfoViewer::showFailure()
{
	ShowLocalizedHint(LOCALE_MESSAGEBOX_ERROR, LOCALE_INFOVIEWER_NOTAVAILABLE, 430);
}

void CInfoViewer::showMotorMoving(int duration)
{
	char text[256];

	strcpy(text, g_Locale->getText(LOCALE_INFOVIEWER_MOTOR_MOVING));
	strcat(text, " (");
	strcat(text, to_string(duration).c_str());
	strcat(text, " s)");
	
	ShowHintUTF(LOCALE_MESSAGEBOX_INFO, text, g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(text, true) + 10, duration); // UTF-8
}

#ifdef ENABLE_RADIOTEXT
void CInfoViewer::killRadiotext()
{
	frameBuffer->paintBackgroundBox(rt_x, rt_y, rt_w, rt_h);
}

void CInfoViewer::showRadiotext()
{
	char stext[3][100];
	int yoff = 8, ii = 0;
	bool RTisIsUTF = false;

	if (g_Radiotext == NULL) return;
	showIcon_RadioText(g_Radiotext->haveRadiotext());

	// dimensions of radiotext window
	rt_dx = BoxEndX - BoxStartX;
	rt_dy = 25;
	rt_x = BoxStartX;
	rt_y = g_settings.screen_StartY + 10;
	rt_h = rt_y + 7 + rt_dy*(g_Radiotext->S_RtOsdRows+1)+SHADOW_OFFSET;
	rt_w = rt_x+rt_dx+SHADOW_OFFSET;

	if (g_Radiotext->S_RtOsd)
	{
		int lines = 0;
		for (int i = 0; i < g_Radiotext->S_RtOsdRows; i++) {
			if (g_Radiotext->RT_Text[i][0] != '\0') lines++;
		}
		if (lines == 0)
			frameBuffer->paintBackgroundBox(rt_x, rt_y, rt_w, rt_h);

		if (g_Radiotext->RT_MsgShow) {

			if (g_Radiotext->S_RtOsdTitle == 1) {

		// Title
		//	sprintf(stext[0], g_Radiotext->RT_PTY == 0 ? "%s - %s %s%s" : "%s - %s (%s)%s",
		//	g_Radiotext->RT_Titel, tr("Radiotext"), g_Radiotext->RT_PTY == 0 ? g_Radiotext->RDS_PTYN : g_Radiotext->ptynr2string(g_Radiotext->RT_PTY), g_Radiotext->RT_MsgShow ? ":" : tr("  [waiting ...]"));
				if ((lines) || (g_Radiotext->RT_PTY !=0)) {
					sprintf(stext[0], g_Radiotext->RT_PTY == 0 ? "%s %s%s" : "%s (%s)%s", tr("Radiotext"), g_Radiotext->RT_PTY == 0 ? g_Radiotext->RDS_PTYN : g_Radiotext->ptynr2string(g_Radiotext->RT_PTY), ":");
					
					// shadow
					frameBuffer->paintBoxRel(rt_x+SHADOW_OFFSET, rt_y+SHADOW_OFFSET, rt_dx, rt_dy, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_LARGE, CORNER_TOP);
					frameBuffer->paintBoxRel(rt_x, rt_y, rt_dx, rt_dy, COL_INFOBAR_PLUS_0, RADIUS_LARGE, CORNER_TOP);
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rt_x+10, rt_y+ 30, rt_dx-20, stext[0], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
				}
				yoff = 17;
				ii = 1;
#if 0
			// RDS- or Rass-Symbol, ARec-Symbol or Bitrate
			int inloff = (ftitel->Height() + 9 - 20) / 2;
			if (Rass_Flags[0][0]) {
				osd->DrawBitmap(Setup.OSDWidth-51, inloff, rass, bcolor, fcolor);
				if (ARec_Record)
					osd->DrawBitmap(Setup.OSDWidth-107, inloff, arec, bcolor, 0xFFFC1414);	// FG=Red
				else
					inloff = (ftitel->Height() + 9 - ftext->Height()) / 2;
				osd->DrawText(4, inloff, RadioAudio->bitrate, fcolor, clrTransparent, ftext, Setup.OSDWidth-59, ftext->Height(), taRight);
			}
			else {
				osd->DrawBitmap(Setup.OSDWidth-84, inloff, rds, bcolor, fcolor);
				if (ARec_Record)
					osd->DrawBitmap(Setup.OSDWidth-140, inloff, arec, bcolor, 0xFFFC1414);	// FG=Red
				else
					inloff = (ftitel->Height() + 9 - ftext->Height()) / 2;
				osd->DrawText(4, inloff, RadioAudio->bitrate, fcolor, clrTransparent, ftext, Setup.OSDWidth-92, ftext->Height(), taRight);
			}
#endif
			}
			// Body
			if (lines) {
				frameBuffer->paintBoxRel(rt_x+SHADOW_OFFSET, rt_y+rt_dy+SHADOW_OFFSET, rt_dx, 7+rt_dy* g_Radiotext->S_RtOsdRows, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_LARGE, CORNER_BOTTOM);
				frameBuffer->paintBoxRel(rt_x, rt_y+rt_dy, rt_dx, 7+rt_dy* g_Radiotext->S_RtOsdRows, COL_INFOBAR_PLUS_0, RADIUS_LARGE, CORNER_BOTTOM);

				// RT-Text roundloop
				int ind = (g_Radiotext->RT_Index == 0) ? g_Radiotext->S_RtOsdRows - 1 : g_Radiotext->RT_Index - 1;
				int rts_x = rt_x+10;
				int rts_y = rt_y+ 30;
				int rts_dx = rt_dx-20;
				if (g_Radiotext->S_RtOsdLoop == 1) { // latest bottom
					for (int i = ind+1; i < g_Radiotext->S_RtOsdRows; i++)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
					for (int i = 0; i <= ind; i++)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
				}
				else { // latest top
					for (int i = ind; i >= 0; i--)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
					for (int i = g_Radiotext->S_RtOsdRows-1; i > ind; i--)
						g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(rts_x, rts_y + (ii++)*rt_dy, rts_dx, g_Radiotext->RT_Text[i], COL_INFOBAR, 0, RTisIsUTF); // UTF-8
				}
			}
#if 0
			// + RT-Plus or PS-Text = 2 rows
			if ((S_RtOsdTags == 1 && RT_PlusShow) || S_RtOsdTags >= 2) {
				if (!RDS_PSShow || !strstr(RTP_Title, "---") || !strstr(RTP_Artist, "---")) {
					sprintf(stext[1], "> %s  %s", tr("Title  :"), RTP_Title);
					sprintf(stext[2], "> %s  %s", tr("Artist :"), RTP_Artist);
					osd->DrawText(4, 6+yoff+fheight*(ii++), stext[1], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
					osd->DrawText(4, 3+yoff+fheight*(ii++), stext[2], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
				}
				else {
					char *temp = "";
					int ind = (RDS_PSIndex == 0) ? 11 : RDS_PSIndex - 1;
					for (int i = ind+1; i < 12; i++)
						asprintf(&temp, "%s%s ", temp, RDS_PSText[i]);
					for (int i = 0; i <= ind; i++)
						asprintf(&temp, "%s%s ", temp, RDS_PSText[i]);
					snprintf(stext[1], 6*9, "%s", temp);
					snprintf(stext[2], 6*9, "%s", temp+(6*9));
					free(temp);
					osd->DrawText(6, 6+yoff+fheight*ii, "[", fcolor, clrTransparent, ftext, 12, ftext->Height());
					osd->DrawText(Setup.OSDWidth-12, 6+yoff+fheight*ii, "]", fcolor, clrTransparent, ftext, Setup.OSDWidth-6, ftext->Height());
					osd->DrawText(16, 6+yoff+fheight*(ii++), stext[1], fcolor, clrTransparent, ftext, Setup.OSDWidth-16, ftext->Height(), taCenter);
					osd->DrawText(6, 3+yoff+fheight*ii, "[", fcolor, clrTransparent, ftext, 12, ftext->Height());
					osd->DrawText(Setup.OSDWidth-12, 3+yoff+fheight*ii, "]", fcolor, clrTransparent, ftext, Setup.OSDWidth-6, ftext->Height());
					osd->DrawText(16, 3+yoff+fheight*(ii++), stext[2], fcolor, clrTransparent, ftext, Setup.OSDWidth-16, ftext->Height(), taCenter);
				}
			}
#endif
		}
#if 0
// framebuffer can only display raw images
		// show mpeg-still
		char *image;
		if (g_Radiotext->Rass_Archiv >= 0)
			asprintf(&image, "%s/Rass_%d.mpg", DataDir, g_Radiotext->Rass_Archiv);
		else
			asprintf(&image, "%s/Rass_show.mpg", DataDir);
		frameBuffer->useBackground(frameBuffer->loadBackground(image));// set useBackground true or false
		frameBuffer->paintBackground();
//		RadioAudio->SetBackgroundImage(image);
		free(image);
#endif
	}
	g_Radiotext->RT_MsgShow = false;

}
#endif /* ENABLE_RADIOTEXT */

int CInfoViewer::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	if ((msg == NeutrinoMessages::EVT_CURRENTNEXT_EPG) ||
	    (msg == NeutrinoMessages::EVT_NEXTPROGRAM    ))
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			getEPG(*(t_channel_id *)data, info_CurrentNext);
			CLCD::getInstance()->setEPGTitle(info_CurrentNext.current_name);
			if (is_visible && showButtonBar) // if we are called from numzap, showButtonBar is false
				show_Data( true );
			showLcdPercentOver();
		}
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_TIMER )
	{
		if ( data == fadeTimer )
		{
			// hierher kann das event nur dann kommen, wenn ein anderes Fenster im Vordergrund ist!
			g_RCInput->killTimer(fadeTimer);
			frameBuffer->setAlphaFade(COL_INFOBAR, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
			frameBuffer->setAlphaFade(COL_INFOBAR_SHADOW, 8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );
			frameBuffer->setAlphaFade(0, 16, convertSetupAlpha2Alpha(0) );
			frameBuffer->paletteSet();

			return messages_return::handled;
		}
		else if ( data == lcdUpdateTimer )
		{
			if ( is_visible )
				show_Data( true );
			showLcdPercentOver();
			return messages_return::handled;
		}
		else if ( data == sec_timer_id )
			return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_RECORDMODE )
	{
		recordModeActive = data;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_GOTAPIDS )
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			if (is_visible && showButtonBar)
			{
				showButton(SNeutrinoSettings::BUTTON_GREEN);  // Button Audio
			}
#ifdef ENABLE_RADIOTEXT
			if (g_settings.radiotext_enable && g_Radiotext && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio))
				g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
#endif
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_GOTPIDS )
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			if ( is_visible && showButtonBar ) {
				showIcon_VTXT();
				showIcon_SubT();
			}
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES )
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			if (is_visible && showButtonBar)
			{
				showButton(SNeutrinoSettings::BUTTON_YELLOW); // Button SubServices
			}
		}
	    return messages_return::handled;
	}
	else if ((msg == NeutrinoMessages::EVT_ZAP_COMPLETE) ||
		 (msg == NeutrinoMessages::EVT_ZAP_ISNVOD))
	{
		channel_id = (*(t_channel_id *)data);
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_ZAP_SUB_COMPLETE)
	{
		channel_id = (*(t_channel_id *)data);
		//if ((*(t_channel_id *)data) == channel_id)
		{
			if ( is_visible && showButtonBar &&  ( !g_RemoteControl->are_subchannels ) )
				show_Data( true );
		}
		showLcdPercentOver();
		eventname	= info_CurrentNext.current_name;
		CLCD::getInstance()->setEPGTitle(eventname);
		return messages_return::handled;
	}
	else if ((msg == NeutrinoMessages::EVT_ZAP_FAILED) ||
	         (msg == NeutrinoMessages::EVT_ZAP_SUB_FAILED))
	{
		if ((*(t_channel_id *)data) == channel_id)
		{
			// show failure..!
			CLCD::getInstance()->showServicename("(" + g_RemoteControl->getCurrentChannelName() + ')');
			printf("zap failed!\n");
			showFailure();
			CLCD::getInstance()->showPercentOver(255);
		}
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_MOTOR)
	{
		showMotorMoving(data);
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_MODECHANGED )
	{
		aspectRatio = data & 0xFF; // strip away VCR aspect ratio
		if ( is_visible && showButtonBar )
			showIcon_16_9();

		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_TIMESET )
	{
		gotTime = true;
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_CA_CLEAR )
	{
		Set_CA_Status(false);
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_CA_LOCK )
	{
		Set_CA_Status(true);
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_CA_FTA )
	{
		Set_CA_Status(false);
		return messages_return::handled;
	}

   return messages_return::unhandled;
}

void CInfoViewer::getEPG(const t_channel_id for_channel_id, CSectionsdClient::CurrentNextInfo &info)
{
	/* to clear the oldinfo for channels without epg, call getEPG() with for_channel_id = 0 */
	if (for_channel_id == 0)
	{
		oldinfo.current_uniqueKey = 0;
		return;
	}

	g_Sectionsd->getCurrentNextServiceKey(for_channel_id, info );

	if (info.current_uniqueKey != oldinfo.current_uniqueKey || info.next_uniqueKey != oldinfo.next_uniqueKey)
	{
		char *p = new char[sizeof(t_channel_id)];
		memcpy(p, &for_channel_id, sizeof(t_channel_id));
		neutrino_msg_t msg;
		if (info.flags & (CSectionsdClient::epgflags::has_current | CSectionsdClient::epgflags::has_next))
		{
			if (info.flags & CSectionsdClient::epgflags::has_current)
				msg = NeutrinoMessages::EVT_CURRENTEPG;
			else
				msg = NeutrinoMessages::EVT_NEXTEPG;
		}
		else
			msg = NeutrinoMessages::EVT_NOEPG_YET;
		g_RCInput->postMsg(msg, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory
		memcpy(&oldinfo, &info, sizeof(CSectionsdClient::CurrentNextInfo));
	}
}

void CInfoViewer::display_Info(const char *current, const char *next,
			       bool UTF8, bool starttimes, const int pb_pos,
			       const char *runningStart, const char *runningRest,
			       const char *nextStart, const char *nextDuration,
			       bool update_current, bool update_next)
{
	/* dimensions of the two-line current-next "box":
	   top of box    == ChanNameY + time_height (bottom of channel name)
	   bottom of box == BoxStartY + InfoHeightY
	   height of box == (BoxStartY + InfoHeightY) - (ChanNameY + time_height)
	   middle of box == top + height / 2
			 == ChanNameY + time_height + (BoxStartY + InfoHeightY - (ChanNameY + time_height))/2
			 == ChanNameY + time_height + (BoxStartY + InfoHeightY - ChanNameY - time_height)/2
			 == (BoxStartY + InfoHeightY + ChanNameY + time_height)/2
	   The bottom of current info and the top of next info is == middle of box.
	 */
	int height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight();
	int CurrInfoY = (BoxStartY + InfoHeightY + ChanNameY + time_height)/2;	// lower end of current info box
	int NextInfoY = CurrInfoY + height;	// lower end of next info box
	int xStart;
	int InfoX = ChanInfoX + 10;

	if (starttimes)
		xStart = BoxStartX + ChanWidth;
	else
		xStart = InfoX;

	if (pb_pos > -1)
	{
		int pb_p = pb_pos;
		int pb_w = 112;
		if (pb_p > pb_w)
			pb_p = pb_w;
/*
 		// EXAMPLE PROGRESS BARS

		// MULTICOLOURED progressbar 30% red, 100% green, 70% yellow
		CProgressBar pb(true, -1, -1, 30, GREEN_BAR, 70, true);

		// RED progressbar
		CProgressBar pb(true, -1, -1, 100, 0, 0, false);

		// GREEN progressbar
		CProgressBar pb(true, -1, -1, 100, 0, 0, true);

		// YELLOW progressbar
		CProgressBar pb(true, -1, -1, 0, 0, 100, true);

		// STANDARD progressbar
		CProgressBar pb(false);
*/
		CProgressBar pb(true, -1, -1, 100, 0, 0, true);
		pb.paintProgressBar(BoxEndX - pb_w - SHADOW_OFFSET, ChanNameY - (ProgressBarHeight + 10), pb_w, ProgressBarHeight, pb_p, pb_w,
				    0, 0, g_settings.progressbar_color ? COL_INFOBAR_SHADOW_PLUS_0 : COL_INFOBAR_PLUS_0, COL_INFOBAR_SHADOW_PLUS_0, "", COL_INFOBAR);
	}

	int currTimeW = 0;
	int nextTimeW = 0;
	if (runningRest != NULL)
		currTimeW = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(runningRest, UTF8);
	if (nextDuration != NULL)
		nextTimeW = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getRenderWidth(nextDuration, UTF8);
	int currTimeX = BoxEndX - currTimeW - 10;
	int nextTimeX = BoxEndX - nextTimeW - 10;
	static int oldCurrTimeX = currTimeX; // remember the last pos. of remaining time, in case we change from 20/100min to 21/99min

	if (current != NULL && update_current)
	{
		frameBuffer->paintBox(InfoX, CurrInfoY - height, currTimeX, CurrInfoY, COL_INFOBAR_PLUS_0);
		if (runningStart != NULL)
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(InfoX, CurrInfoY, 100, runningStart, COL_INFOBAR, 0, UTF8);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart, CurrInfoY, currTimeX - xStart - 5, current, COL_INFOBAR, 0, UTF8);
		oldCurrTimeX = currTimeX;
	}

	if (currTimeX < oldCurrTimeX)
		oldCurrTimeX = currTimeX;
	frameBuffer->paintBox(oldCurrTimeX, CurrInfoY-height, BoxEndX, CurrInfoY, COL_INFOBAR_PLUS_0);
	oldCurrTimeX = currTimeX;
	if (currTimeW != 0)
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(currTimeX, CurrInfoY, currTimeW, runningRest, COL_INFOBAR, 0, UTF8);

	if (next != NULL && update_next)
	{
		frameBuffer->paintBox(InfoX, NextInfoY-height, BoxEndX, NextInfoY, COL_INFOBAR_PLUS_0);
		if (nextStart != NULL)
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(InfoX, NextInfoY, 100, nextStart, COL_INFOBAR, 0, UTF8);
		if (starttimes)
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(xStart, NextInfoY, nextTimeX - xStart - 5, next, COL_INFOBAR, 0, UTF8);
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(xStart, NextInfoY, nextTimeX - xStart - 5, next, COL_INFOBAR, 0, UTF8);
		if (nextTimeW != 0)
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(nextTimeX, NextInfoY, nextTimeW, nextDuration, COL_INFOBAR, 0, UTF8);
	}
}

void CInfoViewer::show_Data(bool calledFromEvent)
{
	if (!is_visible) // no need to do anything else...
		return;

	CNeutrinoApp *neutrino = CNeutrinoApp::getInstance();
	if (neutrino->getMode() == NeutrinoMessages::mode_ts)
		return;	//do nothing in movieplayer mode

	char runningStart[10];
	char runningRest[20];
	int progressbarPos = -1;
	char nextStart[10];
	char nextDuration[10];
	bool is_nvod = false;

	if ((g_RemoteControl->current_channel_id == channel_id) &&
	    !g_RemoteControl->subChannels.empty() && !g_RemoteControl->are_subchannels)
	{
		is_nvod = true;
		info_CurrentNext.current_zeit.startzeit = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].startzeit;
		info_CurrentNext.current_zeit.dauer = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].dauer;
	}
#if 0
/* this triggers false positives on some channels.
 * TODO: test on real NVOD channels, if this was even necessary at all */
	else
	{
		if ((info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) &&
		    (info_CurrentNext.flags & CSectionsdClient::epgflags::has_next) &&
		    showButtonBar)
		{
			if ((uint)info_CurrentNext.next_zeit.startzeit < info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer)
			{
				is_nvod = true;
			}
		}
	}
#endif
	
	time_t jetzt=time(NULL);
	struct tm pStartZeit;
	if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
	{
		int seit = (abs(jetzt - info_CurrentNext.current_zeit.startzeit) + 30) / 60;
		int rest = (info_CurrentNext.current_zeit.dauer / 60) - seit;
		progressbarPos = 0;
		if (!gotTime)
			sprintf((char*)&runningRest, "%d min", info_CurrentNext.current_zeit.dauer / 60);
		else if (jetzt < info_CurrentNext.current_zeit.startzeit)
			sprintf((char*)&runningRest, "in %d min", seit);
		else
		{
			progressbarPos = (jetzt - info_CurrentNext.current_zeit.startzeit) * 112 / info_CurrentNext.current_zeit.dauer;
			if (rest >= 0)
				sprintf((char*)&runningRest, "%d / %d min", seit, rest);
			else 
				sprintf((char*)&runningRest, "%d +%d min", info_CurrentNext.current_zeit.dauer / 60, -rest);
		}
		localtime_r(&info_CurrentNext.current_zeit.startzeit, &pStartZeit);
		sprintf((char*)&runningStart, "%02d:%02d", pStartZeit.tm_hour, pStartZeit.tm_min);
	} else
		last_curr_id = 0;

	if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_next)
	{
		unsigned dauer = info_CurrentNext.next_zeit.dauer / 60;
		sprintf((char*)&nextDuration, "%d min", dauer);
		localtime_r(&info_CurrentNext.next_zeit.startzeit, &pStartZeit);
		sprintf((char*)&nextStart, "%02d:%02d", pStartZeit.tm_hour, pStartZeit.tm_min);
	} else
		last_next_id = 0;

	if (showButtonBar)
	{
		// show percent/event progressbar?
		if (!(info_CurrentNext.flags & CSectionsdClient::epgflags::has_current))
			progressbarPos = -1; // no!
		showButton(SNeutrinoSettings::BUTTON_RED);
	}

	if ((info_CurrentNext.flags & CSectionsdClient::epgflags::not_broadcast) ||
	    (calledFromEvent) && !(info_CurrentNext.flags & (CSectionsdClient::epgflags::has_next|CSectionsdClient::epgflags::has_current)))
	{
		// no EPG available
		display_Info(g_Locale->getText(gotTime ? LOCALE_INFOVIEWER_NOEPG : LOCALE_INFOVIEWER_WAITTIME), NULL);
		/* send message. Parental pin check gets triggered on EPG events... */
		char *p = new char[sizeof(t_channel_id)];
		memcpy(p, &channel_id, sizeof(t_channel_id));
		/* clear old info in getEPG */
		CSectionsdClient::CurrentNextInfo dummy;
		getEPG(0, dummy);
		g_RCInput->postMsg(NeutrinoMessages::EVT_NOEPG_YET, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory
		return;
	}

	// irgendein EPG gefunden
	const char *current   = NULL;
	const char *curr_time = NULL;
	const char *curr_rest = NULL;
	const char *next      = NULL;
	const char *next_time = NULL;
	const char *next_dur  = NULL;
	bool curr_upd = true;
	bool next_upd = true;

	if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
	{
		if (info_CurrentNext.current_uniqueKey != last_curr_id)
		{
			last_curr_id = info_CurrentNext.current_uniqueKey;
			curr_time = runningStart;
			current = info_CurrentNext.current_name.c_str();
		}
		else
			curr_upd = false;
		curr_rest = runningRest;
	}
	else
		current = g_Locale->getText(LOCALE_INFOVIEWER_NOCURRENT);

	if (info_CurrentNext.flags & CSectionsdClient::epgflags::has_next)
	{
		if (!(is_nvod && (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current))
		    && info_CurrentNext.next_uniqueKey != last_next_id)
		{	/* if current is shown, show next only if !nvod. Why? I don't know */
			//printf("SHOWDATA: last_next_id = 0x%016llx next_id = 0x%016llx\n", last_next_id, info_CurrentNext.next_uniqueKey);
			last_next_id = info_CurrentNext.next_uniqueKey;
			next_time = nextStart;
			next = info_CurrentNext.next_name.c_str();
			next_dur = nextDuration;
		}
		else
			next_upd = false;
	}
	display_Info(current, next, false, true, progressbarPos, curr_time, curr_rest, next_time, next_dur, curr_upd, next_upd);
}

void CInfoViewer::showInfoFile()
{
	char infotext[80];
	int fd, xStart, xEnd, height;
	ssize_t cnt;

	fd = open("/tmp/infobar.txt", O_RDONLY);

	if (fd < 0)
		return;

	cnt = read(fd, infotext, 79);
	if (cnt < 1) {
		fprintf(stderr, "CInfoViewer::showInfoFile: could not read from infobar.txt: %m");
		close(fd);
		return;
	}
	close(fd);
	if (infotext[cnt-1] == '\n')
		infotext[cnt-1] = '\0';
	else
		infotext[cnt] = '\0';

	xStart = BoxStartX + ChanWidth + 40;	// right of record icon
	xEnd   = BoxEndX - 125;			// left of progressbar
	height = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->getHeight() + 2;

	// shadow
	frameBuffer->paintBox(xStart + SHADOW_OFFSET, BoxStartY + SHADOW_OFFSET, xEnd + SHADOW_OFFSET, BoxStartY + height + SHADOW_OFFSET, COL_INFOBAR_SHADOW_PLUS_0, RADIUS_SMALL);

	// background
	frameBuffer->paintBox(xStart, BoxStartY, xEnd, BoxStartY + height, COL_INFOBAR_PLUS_0, RADIUS_SMALL);

	// text
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO]->RenderString(
		xStart + RADIUS_SMALL, BoxStartY + height, xEnd - xStart - RADIUS_SMALL*2, infotext, COL_INFOBAR, height);
}

void CInfoViewer::killTitle()
{
	if (is_visible )
	{
		is_visible = false;
		int top = ChanNameY - (ProgressBarHeight + 10);  // vertical pos of progressbar
		if (BoxStartY < top)
			top = BoxStartY;
		int bottom = BoxEndY + SHADOW_OFFSET;
		if (showButtonBar)
			bottom += InfoHeightY_Info;
		frameBuffer->paintBackgroundBox(BoxStartX, top, BoxEndX + SHADOW_OFFSET, bottom);
#ifdef ENABLE_RADIOTEXT
		if (g_settings.radiotext_enable && g_Radiotext) {
			g_Radiotext->S_RtOsd = g_Radiotext->haveRadiotext() ? 1 : 0;
			killRadiotext();
		}
#endif
	}
	showButtonBar = false;
}

void CInfoViewer::showIcon_CA_Status() const
{
	frameBuffer->paintIcon((CA_Status) ? NEUTRINO_ICON_CA : NEUTRINO_ICON_FTA,
				BoxEndX - (ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_LARGE_WIDTH + 2 + ICON_SMALL_WIDTH + 2 + ICON_SMALL_WIDTH + 6),
				BoxEndY + (g_settings.infobar_sat_display ? 15 : 0) + (InfoHeightY_Info - ICON_HEIGHT) / 2);
}

void CInfoViewer::Set_CA_Status(int Status)
{
	CA_Status = Status;
	if ( is_visible && showButtonBar )
		showIcon_CA_Status();
}

int CInfoViewer::showChannelLogo( const t_channel_id logo_channel_id  )
{
	char strChanId[16];
	sprintf((char*) strChanId, "%llx", logo_channel_id);
	std::string	mimetype = "raw",
			strLogoIDName = (std::string)strChanId + "." + mimetype,
			strLogoName = ChannelName + "." + mimetype,
			strAbsIconChIDPath = (std::string)g_settings.infobar_channel_logodir +"/"+ strLogoIDName,
			strAbsIconChNamePath = (std::string)g_settings.infobar_channel_logodir +"/"+ strLogoName,
			strAbsIconPath,
			strErrText= "[infoviewer] error while painting channel logo\n -> channel logo too large...use maximal %2dpx%2dpx or change display mode\n -> current logo size: %2dpx%2dpx\n -> current mode: %d\n";	

	int x_mid, y_mid, logo_w = 0, logo_h = 0; 
	int logo_x=0, logo_y=0;
	int res = NO_LOGO;
	int start_x = ChanNameX, chan_w = BoxEndX- (start_x+ 20)- time_width- 15;
	
	bool logo_available = false;
	
	if (g_settings.infobar_show_channellogo != NO_LOGO)
	{
		// check if logo is available
		if (access(strAbsIconChIDPath.c_str(), 0) != -1)
		{
			strAbsIconPath = strAbsIconChIDPath;
			logo_available = true;
		}
		else if (access(strAbsIconChNamePath.c_str(), 0) != -1)
		{
			strAbsIconPath = strAbsIconChNamePath; // strLogoName;
			logo_available = true;
		}
		
		if (logo_available)
		{
			// get logo sizes
			frameBuffer->getIconSize(strAbsIconPath.c_str(), &logo_w, &logo_h);

			if ((logo_w == 0) || (logo_h == 0)) // corrupt logo size?
			{
				printf("[infoviewer] channel logo: \n -> %s (%s) has no size\n -> please check logo file!\n",strAbsIconPath.c_str(), ChannelName.c_str());
			}
			else
			{	
				if (g_settings.infobar_show_channellogo == LOGO_AS_CHANNELNUM)
				{
					// calculate mid of numberbox
					int satNameHeight = 0; // no sat name display now, picon doesnt need to set an offset for y
					x_mid = BoxStartX+ChanWidth/2;
					y_mid = (BoxStartY+satNameHeight)+ChanHeight/2;
						
					// check logo dimensions
					if ((logo_w > ChanWidth) || (logo_h > ChanHeight))	
					{
						printf(strErrText.c_str(), ChanWidth, ChanHeight, logo_w, logo_h, LOGO_AS_CHANNELNUM);
					}
					else
					{
						// channel name with number
						ChannelName = (std::string)strChanNum + ". " + ChannelName;
						// get position of channel logo, must be centered in number box
						logo_x = x_mid - logo_w/2;
						logo_y = y_mid - logo_h/2;
						res = LOGO_AS_CHANNELNUM;
					}
				}
				else if (g_settings.infobar_show_channellogo == LOGO_AS_CHANNELNAME)
				{
					// check logo dimensions
					if ((logo_w > chan_w) || (logo_h > ChanHeight))
					{
						printf(strErrText.c_str(), chan_w, ChanHeight, logo_w, logo_h, LOGO_AS_CHANNELNAME);
					}
					else
					{
						// hide channel name
						ChannelName = "";
						// calculate logo position
						y_mid = (ChanNameY+time_height) - time_height/2;
						logo_x = start_x+10;
						logo_y = y_mid - logo_h/2;				
						res = LOGO_AS_CHANNELNAME;
					}
				}
				else if (g_settings.infobar_show_channellogo == LOGO_BESIDE_CHANNELNAME)
				{
					// check logo dimensions
					int Logo_max_width = chan_w - logo_w - 10;
					if ((logo_w > Logo_max_width) || (logo_h > ChanHeight))
					{
						printf(strErrText.c_str(), Logo_max_width, ChanHeight, logo_w, logo_h, LOGO_BESIDE_CHANNELNAME);
					}
					else
					{
						// calculate logo position
						y_mid = (ChanNameY+time_height) - time_height/2;
						logo_x = start_x+10;
						logo_y = y_mid - logo_h/2;
	
						// set channel name x pos
						ChanNameX =  start_x + logo_w + 10;
						res = LOGO_BESIDE_CHANNELNAME;
					}
				}			
			
				// paint logo background (shaded/framed)
				if ((g_settings.infobar_channellogo_background != NO_BACKGROUND) && (res != NO_LOGO))
				{	
					int frame_w = 2, logo_bg_x=0, logo_bg_y=0, logo_bg_w=0, logo_bg_h=0;
					
					if (g_settings.infobar_channellogo_background == LOGO_FRAMED)
					{
						//sh_offset = 2;
						logo_bg_x = logo_x-frame_w;
						logo_bg_y = logo_y-frame_w;
						logo_bg_w = logo_w+frame_w*2;
						logo_bg_h = logo_h+frame_w*2;
					}
					else if (g_settings.infobar_channellogo_background == LOGO_SHADED)
					{
						//sh_offset = 3;
						logo_bg_x = logo_x+SHADOW_OFFSET;
						logo_bg_y = logo_y+SHADOW_OFFSET;
						logo_bg_w = logo_w;
						logo_bg_h = logo_h;
					}
					frameBuffer->paintBoxRel(logo_bg_x, logo_bg_y, logo_bg_w, logo_bg_h, COL_INFOBAR_BUTTONS_BACKGROUND);
				}

				// paint the logo
				if (res != NO_LOGO)
				{
					if (!frameBuffer->paintIcon(strAbsIconPath, logo_x, logo_y)) 
						res = NO_LOGO; // paint logo was failed
				}
			}
		}
	}

	return res;
}

void CInfoViewer::showLcdPercentOver()
{
	if (g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME] != CLCD::STATUSLINE_VOLUME)
	{
		static long long old_interval = 0;
		int runningPercent=-1;
		time_t jetzt=time(NULL);
#if 0
No need to poll for EPG, we are getting events from sectionsd!
		if ( ! (info_CurrentNext.flags & CSectionsdClient::epgflags::has_current) ||
		     jetzt > (int)(info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer))
		{
			info_CurrentNext = getEPG(channel_id);
		}
#endif
		long long interval = 60000000; /* 60 seconds default update time */
		if ( info_CurrentNext.flags & CSectionsdClient::epgflags::has_current)
		{
			if (jetzt < info_CurrentNext.current_zeit.startzeit)
				runningPercent = 0;
			else if (jetzt > (int)(info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer))
				runningPercent = -2; /* overtime */
			else
			{
				runningPercent=MIN((jetzt-info_CurrentNext.current_zeit.startzeit) * 100 /
					            info_CurrentNext.current_zeit.dauer ,100);
				interval = info_CurrentNext.current_zeit.dauer * 1000LL * (1000/100); // update every percent
				if (is_visible && interval > 60000000)
					interval = 60000000;	// if infobar visible, update at least once per minute (radio mode)
				if (interval < 5000000)
					interval = 5000000;	// update only every 5 seconds
			}
		}
		if (interval != old_interval)
		{
			g_RCInput->killTimer(lcdUpdateTimer);
			lcdUpdateTimer = g_RCInput->addTimer(interval, false);
			//printf("lcdUpdateTimer: interval %lld old %lld\n",interval/1000000,old_interval/1000000);
			old_interval = interval;
		}
		CLCD::getInstance()->showPercentOver(runningPercent);
	}
}

void CInfoViewer::showEpgInfo()   //message on event change
{
	char nextStart[10];
	int mode = CNeutrinoApp::getInstance()->getMode();
	struct tm pnStartZeit;
	localtime_r(&info_CurrentNext.next_zeit.startzeit, &pnStartZeit);
	sprintf((char*)&nextStart, "%02d:%02d", pnStartZeit.tm_hour, pnStartZeit.tm_min);

	/* show epg info only if we in TV- or Radio mode and current event is not the same like before */
	if ((eventname != info_CurrentNext.current_name) && (mode != 0))
	{
		eventname = info_CurrentNext.current_name;
		if (g_settings.infobar_show == EPGINFO_SIMPLE_MESSAGE)
		{
			if (!eventname.empty())
 			{
				std::string event = eventname + "\n" + g_Locale->getText(LOCALE_INFOVIEWER_MESSAGE_TO) + nextStart;
				std::string event_message =  ZapitTools::Latin1_to_UTF8(event.c_str());
				ShowHintUTF(LOCALE_INFOVIEWER_MESSAGE_NOW, event_message.c_str(), 420 , 6, NEUTRINO_ICON_EPGINFO);
			}
		}
		else if (g_settings.infobar_show == EPGINFO_COMPLEX_MESSAGE)
		{
			// complex message, show infobar
			g_RCInput->postMsg(NeutrinoMessages::SHOW_INFOBAR , 0);
		}
	}
}

//
//  -- InfoViewer Menu Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-06 rasc)
// 

int CInfoViewerHandler::exec(CMenuTarget* parent, const std::string &)
{
	int           res = menu_return::RETURN_EXIT_ALL;
	CChannelList  *channelList;
	CInfoViewer   *i;


	if (parent) {
		parent->hide();
	}

	i = new CInfoViewer;

	channelList = CNeutrinoApp::getInstance()->channelList;
	i->start();
	i->showTitle(channelList->getActiveChannelNumber(), channelList->getActiveChannelName(), channelList->getActiveSatellitePosition(), channelList->getActiveChannel_ChannelID()); // UTF-8
	delete i;

	return res;
}
