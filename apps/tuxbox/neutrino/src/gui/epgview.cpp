/*
	$Id: epgview.cpp,v 1.163 2012/11/03 07:03:59 rhabarber1848 Exp $

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

#include <algorithm>

#include <gui/epgview.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/buttons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/mountchooser.h>
#include <gui/widget/dirchooser.h>
#include <gui/widget/progressbar.h>
#include <gui/timerlist.h>
#include <system/helper.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>
#include <driver/screen_max.h>
#define ICON_LARGE_WIDTH 26
#define ICON_HEIGHT 16
#define BUTTONBAR_FONT_OFFSET 5

int findItem(std::string strItem, std::vector<std::string> & vecItems) {
	for (std::vector<std::string>::size_type nCnt = 0; nCnt < vecItems.size(); nCnt++) {
		std::string strThisItem = vecItems[nCnt];
		if (strItem == strThisItem) {
			return nCnt;
		}
	}
	return -1;
}

// 21.07.2005 - rainerk
// Merge multiple extended event strings into one description and localize the label
// Examples:
//   Actor1-ActorX      -> Darsteller 1, 2, 3
//   Year of production -> Produktionsjahr
//   Director           -> Regisseur
//   Guests             -> Gaeste
void reformatExtendedEvents(std::string strItem, std::string strLabel, bool bUseRange, CEPGData & epgdata) {
	std::vector<std::string> & vecDescriptions = epgdata.itemDescriptions;
	std::vector<std::string> & vecItems = epgdata.items;
	// Merge multiple events into 1 (Actor1-)
	if (bUseRange) {
		bool bHasItems = false;
		char index[3];
		// Maximum of 10 items should suffice
		for (int nItem = 1; nItem < 11; nItem++) {
			sprintf(index, "%d", nItem);
			// Look for matching items
			int nPos = findItem(std::string(strItem) + std::string(index), vecDescriptions);
			if (-1 != nPos) {
				std::string item = std::string(vecItems[nPos]);
				vecDescriptions.erase(vecDescriptions.begin() + nPos);
				vecItems.erase(vecItems.begin() + nPos);
				if (false == bHasItems) {
					// First item added, so create new label item
					vecDescriptions.push_back(strLabel);
					vecItems.push_back(item + ", ");
					bHasItems = true;
				} else {
					vecItems.back().append(item).append(", ");
				}
			}
		}
		// Remove superfluous ", "
		if (bHasItems) {
			vecItems.back().resize(vecItems.back().length() - 2);
		}
	} else {	// Single string event (e.g. Director)
		// Look for matching items
		int nPos = findItem(strItem, vecDescriptions);
		if (-1 != nPos) {
			vecDescriptions[nPos] = strLabel;
		}
	}
}

CEpgData::CEpgData()
{
	bigFonts = false;
	frameBuffer = CFrameBuffer::getInstance();
}

void CEpgData::start()
{
	/* This defines the size of the EPG window. We leave 35 pixels left and right,
	 * 25 pixels top and bottom. It adjusts itself to the "visible screen" settings
	 */
	ox = w_max (720, 70);
	oy = h_max (576, 50);

	frameBuffer->getIconSize(NEUTRINO_ICON_BUTTON_LEFT, &boticonwidth, &boticonheight);
	topheight     = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->getHeight();
	topboxheight  = topheight + 6;
	botheight     = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight();
	botboxheight  = std::max(botheight, boticonheight) + 6;
	buttonheight  = std::max(16, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight()) + BUTTONBAR_FONT_OFFSET;

	sx = getScreenStartX (ox);
	sy = getScreenStartY (oy);
	oy -= buttonheight;
	/* this is the text box height - and the height of the scroll bar */
	sb = oy - topboxheight - botboxheight;
	medlineheight = std::max(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight(), g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getHeight());
	medlinecount  = sb / medlineheight;

	toph = topboxheight;
}

void CEpgData::addTextToArray(const std::string & text, int flag) // UTF-8
{
	//printf("line: >%s<\n", text.c_str() );
	if (text==" ")
	{
		emptyLineCount ++;
		if(emptyLineCount<2)
		{
			epgText.push_back(epg_pair(text, flag));
		}
	}
	else
	{
		emptyLineCount = 0;
		epgText.push_back(epg_pair(text, flag));
	}
}

void CEpgData::processTextToArray(std::string text, int flag) // UTF-8
{
	Font* fnt_epg = g_Font[(flag == EPG_INFO1) ? SNeutrinoSettings::FONT_TYPE_EPG_INFO1 : SNeutrinoSettings::FONT_TYPE_EPG_INFO2];
	std::string	aktLine = "";
	std::string	aktWord = "";
	int	aktWidth = 0, aktWordWidth = 0;
	text += ' ';
	char* text_= (char*) text.c_str();

	while(*text_!=0)
	{
		if ( (*text_==' ') || (*text_=='\n') || (*text_=='-') || (*text_=='.') )
		{
			// Houdini: if there is a newline (especially in the Premiere Portal EPGs) do not forget to add aktWord to aktLine 
			// after width check, if width check failes do newline, add aktWord to next line 
			// and "reinsert" i.e. reloop for the \n
			if(*text_!='\n')
				aktWord += *text_;

			// check the wordwidth - add to this line if size ok
			aktWordWidth = fnt_epg->getRenderWidth(aktWord, true);
			if((aktWordWidth+aktWidth)<=(ox- 15- 15))
			{//space ok, add
				aktWidth += aktWordWidth;
				aktLine += aktWord;
			
				if(*text_=='\n')
				{	//enter-handler
					addTextToArray( aktLine, flag);
					aktLine = "";
					aktWidth= 0;
				}	
				aktWord = "";
			}
			else
			{//new line needed
				addTextToArray( aktLine, flag);
				aktLine = aktWord;
				aktWidth = aktWordWidth;
				aktWord = "";
				// Houdini: in this case where we skipped \n and space is too low, exec newline and rescan \n 
				// otherwise next word comes direct after aktLine
				if(*text_=='\n')
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
	addTextToArray( aktLine + aktWord, flag );
}

void CEpgData::showText( int startPos, int ypos )
{
	// recalculate
	medlineheight = std::max(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight(), g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getHeight());
	medlinecount = sb / medlineheight;

	int textSize = epgText.size();
	int y=ypos;
	const char tok = ' ';
	int offset = 0, count = 0;
	int max_mon_w = 0, max_wday_w = 0;
	int digi = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getRenderWidth("29..");
	for(int i = 0; i < 12;i++){
		max_mon_w = std::max(max_mon_w, g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getRenderWidth(std::string(g_Locale->getText(CLocaleManager::getMonth(i))) + ".", true)); // UTF-8
		if(i > 6)
		      continue;
		max_wday_w = std::max(max_wday_w, g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getRenderWidth(std::string(g_Locale->getText(CLocaleManager::getWeekday(i))) + ".", true)); // UTF-8
	}
	frameBuffer->paintBoxRel(sx, y, ox- 15, sb, COL_MENUCONTENT_PLUS_0); // background of the text box
	for(int i = startPos; i < textSize && i < startPos + medlinecount; i++, y += medlineheight)
	{
		if(epgText[i].second == SCREENING_AFTER || epgText[i].second == SCREENING_BEFORE){
			std::string::size_type pos1 = epgText[i].first.find_first_not_of(tok, 0);
			std::string::size_type pos2 = epgText[i].first.find_first_of(tok, pos1);
			while( pos2 != string::npos || pos1 != string::npos ){
				switch(count){
					case 1:
					offset += max_wday_w;
					break;
					case 3:
					offset += max_mon_w;
					break;
					default:
					offset += digi;
					break;
				}
				g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->RenderString(sx+10+offset, y+medlineheight, ox- 15- 15- offset,
						epgText[i].first.substr(pos1, pos2 - pos1),
						(epgText[i].second == SCREENING_BEFORE) ? COL_MENUCONTENTINACTIVE : COL_MENUCONTENT,
						0, true); // UTF-8
				count++;
				pos1 = epgText[i].first.find_first_not_of(tok, pos2);
				pos2 = epgText[i].first.find_first_of(tok, pos1);
			}
			offset = 0;
			count = 0;
		}
		else{
			int font_type = (epgText[i].second == EPG_INFO1) ? SNeutrinoSettings::FONT_TYPE_EPG_INFO1 : SNeutrinoSettings::FONT_TYPE_EPG_INFO2;
			g_Font[font_type]->RenderString(sx+10, y+medlineheight, ox- 15- 15, epgText[i].first, COL_MENUCONTENT, 0, true); // UTF-8
		}
	}

	int sbc = ((textSize - 1)/ medlinecount) + 1;
	int sbs = (startPos + 1)/ medlinecount;
	frameBuffer->paintBoxRel(sx+ ox- 15, ypos, 15, sb,  COL_MENUCONTENT_PLUS_1); // scrollbar bg
	frameBuffer->paintBoxRel(sx+ ox- 13, ypos+ 2+ sbs*(sb-4)/sbc , 11, (sb-4)/sbc,  COL_MENUCONTENT_PLUS_3, RADIUS_SMALL); // scrollbar
}

#define GENRE_MOVIE_COUNT 9
const neutrino_locale_t genre_movie[GENRE_MOVIE_COUNT] =
{
	LOCALE_GENRE_MOVIE_0,
	LOCALE_GENRE_MOVIE_1,
	LOCALE_GENRE_MOVIE_2,
	LOCALE_GENRE_MOVIE_3,
	LOCALE_GENRE_MOVIE_4,
	LOCALE_GENRE_MOVIE_5,
	LOCALE_GENRE_MOVIE_6,
	LOCALE_GENRE_MOVIE_7,
	LOCALE_GENRE_MOVIE_8
};
#define GENRE_NEWS_COUNT 5
const neutrino_locale_t genre_news[GENRE_NEWS_COUNT] =
{
	LOCALE_GENRE_NEWS_0,
	LOCALE_GENRE_NEWS_1,
	LOCALE_GENRE_NEWS_2,
	LOCALE_GENRE_NEWS_3,
	LOCALE_GENRE_NEWS_4
};
#define GENRE_SHOW_COUNT 4
const neutrino_locale_t genre_show[GENRE_SHOW_COUNT] =
{
	LOCALE_GENRE_SHOW_0,
	LOCALE_GENRE_SHOW_1,
	LOCALE_GENRE_SHOW_2,
	LOCALE_GENRE_SHOW_3
};
#define GENRE_SPORTS_COUNT 12
const neutrino_locale_t genre_sports[GENRE_SPORTS_COUNT] =
{
	LOCALE_GENRE_SPORTS_0,
	LOCALE_GENRE_SPORTS_1,
	LOCALE_GENRE_SPORTS_2,
	LOCALE_GENRE_SPORTS_3,
	LOCALE_GENRE_SPORTS_4,
	LOCALE_GENRE_SPORTS_5,
	LOCALE_GENRE_SPORTS_6,
	LOCALE_GENRE_SPORTS_7,
	LOCALE_GENRE_SPORTS_8,
	LOCALE_GENRE_SPORTS_9,
	LOCALE_GENRE_SPORTS_10,
	LOCALE_GENRE_SPORTS_11
};
#define GENRE_CHILDRENS_PROGRAMMES_COUNT 6
const neutrino_locale_t genre_childrens_programmes[GENRE_CHILDRENS_PROGRAMMES_COUNT] =
{
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_0,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_1,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_2,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_3,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_4,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_5
};
#define GENRE_MUSIC_DANCE_COUNT 7
const neutrino_locale_t genre_music_dance[GENRE_MUSIC_DANCE_COUNT] =
{
	LOCALE_GENRE_MUSIC_DANCE_0,
	LOCALE_GENRE_MUSIC_DANCE_1,
	LOCALE_GENRE_MUSIC_DANCE_2,
	LOCALE_GENRE_MUSIC_DANCE_3,
	LOCALE_GENRE_MUSIC_DANCE_4,
	LOCALE_GENRE_MUSIC_DANCE_5,
	LOCALE_GENRE_MUSIC_DANCE_6
};
#define GENRE_ARTS_COUNT 12
const neutrino_locale_t genre_arts_dance[GENRE_ARTS_COUNT] =
{
	LOCALE_GENRE_ARTS_0,
	LOCALE_GENRE_ARTS_1,
	LOCALE_GENRE_ARTS_2,
	LOCALE_GENRE_ARTS_3,
	LOCALE_GENRE_ARTS_4,
	LOCALE_GENRE_ARTS_5,
	LOCALE_GENRE_ARTS_6,
	LOCALE_GENRE_ARTS_7,
	LOCALE_GENRE_ARTS_8,
	LOCALE_GENRE_ARTS_9,
	LOCALE_GENRE_ARTS_10,
	LOCALE_GENRE_ARTS_11
};
#define GENRE_SOCIAL_POLITICAL_COUNT 4
const neutrino_locale_t genre_social_political[GENRE_SOCIAL_POLITICAL_COUNT] =
{
	LOCALE_GENRE_SOCIAL_POLITICAL_0,
	LOCALE_GENRE_SOCIAL_POLITICAL_1,
	LOCALE_GENRE_SOCIAL_POLITICAL_2,
	LOCALE_GENRE_SOCIAL_POLITICAL_3
};
#define GENRE_DOCUS_MAGAZINES_COUNT 8
const neutrino_locale_t genre_docus_magazines[GENRE_DOCUS_MAGAZINES_COUNT] =
{
	LOCALE_GENRE_DOCUS_MAGAZINES_0,
	LOCALE_GENRE_DOCUS_MAGAZINES_1,
	LOCALE_GENRE_DOCUS_MAGAZINES_2,
	LOCALE_GENRE_DOCUS_MAGAZINES_3,
	LOCALE_GENRE_DOCUS_MAGAZINES_4,
	LOCALE_GENRE_DOCUS_MAGAZINES_5,
	LOCALE_GENRE_DOCUS_MAGAZINES_6,
	LOCALE_GENRE_DOCUS_MAGAZINES_7
};
#define GENRE_TRAVEL_HOBBIES_COUNT 8
const neutrino_locale_t genre_travel_hobbies[GENRE_TRAVEL_HOBBIES_COUNT] =
{
	LOCALE_GENRE_TRAVEL_HOBBIES_0,
	LOCALE_GENRE_TRAVEL_HOBBIES_1,
	LOCALE_GENRE_TRAVEL_HOBBIES_2,
	LOCALE_GENRE_TRAVEL_HOBBIES_3,
	LOCALE_GENRE_TRAVEL_HOBBIES_4,
	LOCALE_GENRE_TRAVEL_HOBBIES_5,
	LOCALE_GENRE_TRAVEL_HOBBIES_6,
	LOCALE_GENRE_TRAVEL_HOBBIES_7
};
const unsigned char genre_sub_classes[10] =
{
	GENRE_MOVIE_COUNT,
	GENRE_NEWS_COUNT,
	GENRE_SHOW_COUNT,
	GENRE_SPORTS_COUNT,
	GENRE_CHILDRENS_PROGRAMMES_COUNT,
	GENRE_MUSIC_DANCE_COUNT,
	GENRE_ARTS_COUNT,
	GENRE_SOCIAL_POLITICAL_COUNT,
	GENRE_DOCUS_MAGAZINES_COUNT,
	GENRE_TRAVEL_HOBBIES_COUNT
};
const neutrino_locale_t * genre_sub_classes_list[10] =
{
	genre_movie,
	genre_news,
	genre_show,
	genre_sports,
	genre_childrens_programmes,
	genre_music_dance,
	genre_arts_dance,
	genre_social_political,
	genre_docus_magazines,
	genre_travel_hobbies
};

bool CEpgData::hasFollowScreenings(const t_channel_id /*channel_id*/, const std::string & title, const time_t startzeit)
{
	followlist.clear();
	for (CChannelEventList::iterator e = evtlist.begin(); e != evtlist.end(); ++e )
	{
		if (e->eventID != 0 && e->description == title && e->startTime != startzeit)
			followlist.push_back(*e);
	}
	return !followlist.empty();
}

const char * GetGenre(const unsigned char contentClassification) // UTF-8
{
	neutrino_locale_t res;

	unsigned char i = (contentClassification & 0x0F0);

	if ((i >= 0x010) && (i < 0x0B0))
	{
		i >>= 4;
		i--;
		res = genre_sub_classes_list[i][((contentClassification & 0x0F) < genre_sub_classes[i]) ? (contentClassification & 0x0F) : 0];
	}
	else
		res = LOCALE_GENRE_UNKNOWN;

	return g_Locale->getText(res);
}

static bool sortByDateTime (const CChannelEvent& a, const CChannelEvent& b)
{
	return a.startTime< b.startTime;
}

bool CEpgData::isCurrentEPG(const t_channel_id channel_id)
{
	t_channel_id live_channel_id = g_Zapit->getCurrentServiceID();
	if(( epg_done != -1 ) && live_channel_id == channel_id){
		return true;
	}
	return false;
}

int CEpgData::show(const t_channel_id channel_id, unsigned long long a_id, time_t* a_startzeit, bool doLoop, bool callFromfollowlist )
{
	int res = menu_return::RETURN_REPAINT;
	static unsigned long long id;
	static time_t startzeit;
	call_fromfollowlist = callFromfollowlist;
	if(a_startzeit)
		startzeit=*a_startzeit;
	id=a_id;

	GetEPGData(channel_id, id, &startzeit );
	if (doLoop)
	{
		if (!bigFonts && g_settings.bigFonts) {
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getSize() * BIG_FONT_FAKTOR / 10);
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getSize() * BIG_FONT_FAKTOR / 10);
		}
		bigFonts = g_settings.bigFonts;
		evtlist = g_Sectionsd->getEventsServiceKey(channel_id);
		// Houdini added for Private Premiere EPG start sorted by start date/time
		sort(evtlist.begin(),evtlist.end(),sortByDateTime);
	}

	if (epgData.title.empty()) /* no epg info found */
	{
		ShowLocalizedHint(LOCALE_MESSAGEBOX_INFO, LOCALE_EPGVIEWER_NOTFOUND);
		hide();
		return res;
	}

	printf("showing epgid: 0x%04llx (%lld)\n", epgData.eventID & 0xFFFFULL, epgData.eventID & 0xFFFFULL);

	std::string text1 = epgData.title;
	std::string text2 = "";
	if (g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->getRenderWidth(text1) > ox - 15) // 15 for the scroll bar...
	{
		int pos;
		do
		{
			pos = text1.find_last_of("[ .]+");
			if ( pos!=-1 )
				text1 = text1.substr( 0, pos );
		} while ( ( pos != -1 ) && (g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->getRenderWidth(text1) > ox - 15));
		text2 = epgData.title.substr(text1.length()+ 1, uint(-1) );
	}

	if (!text2.empty())
		toph = 2* topboxheight;
	else
		toph = topboxheight;

	sb = oy - toph - botboxheight;

	// 21.07.2005 - rainerk
	// Only show info1 if it's not included in info2!
	std::string strEpisode = "";	// Episode title in case info1 gets stripped
	if (!epgData.info1.empty()) {
		bool bHide = false;
		if (false == epgData.info2.empty()) {
			// Look for the first . in info1, usually this is the title of an episode.
			std::string::size_type nPosDot = epgData.info1.find('.');
			if (std::string::npos != nPosDot) {
				nPosDot += 2; // Skip dot and first blank
/*	Houdini: changed for safty reason (crashes with some events at WDR regional)
			if (nPosDot < epgData.info2.length()) {   // Make sure we don't overrun the buffer
*/
				if (nPosDot < epgData.info2.length() && nPosDot < epgData.info1.length()) {   // Make sure we don't overrun the buffer

					if (0 == epgData.info2.find(epgData.info1.substr(nPosDot, epgData.info1.length() - nPosDot))) {
						strEpisode = epgData.info1.substr(0, nPosDot) + "\n";
						bHide = true;
					}
				}
			}
			// Compare strings normally if not positively found to be equal before
			if (false == bHide && 0 == epgData.info2.find(epgData.info1)) {
				bHide = true;
			}
		}
		if (false == bHide) {
			processTextToArray(Latin1_to_UTF8(epgData.info1), EPG_INFO1);
		}
	}

	//scan epg-data - sort to list
	if (epgData.info2.empty() && epgText.empty())
		processTextToArray(g_Locale->getText(LOCALE_EPGVIEWER_NODETAILED)); // UTF-8
	else
		processTextToArray(Latin1_to_UTF8(strEpisode + epgData.info2));

	// Add a blank line
	processTextToArray("");

	// 21.07.2005 - rainerk
	// Show extended information
	if (!epgData.itemDescriptions.empty()) {
		char line[256];
		std::vector<std::string>::iterator description;
		std::vector<std::string>::iterator item;
		for (description = epgData.itemDescriptions.begin(), item = epgData.items.begin(); description != epgData.itemDescriptions.end(); ++description, ++item) {
			sprintf(line, "%s: %s",
					(isUTF8(*description) ? (*description) : Latin1_to_UTF8(*description)).c_str(),
					Latin1_to_UTF8(*item).c_str());
			processTextToArray(line);
		}
	}

	// Show FSK information
	if (epgData.fsk > 0)
	{
		char fskInfo[4];
		sprintf(fskInfo, "%d", epgData.fsk);
		processTextToArray(std::string(g_Locale->getText(LOCALE_EPGVIEWER_AGE_RATING)) + ": " + fskInfo); // UTF-8
	}

	// Show length information
	char lengthInfo[11];
	sprintf(lengthInfo, "%d", epgData.epg_times.dauer / 60);
	processTextToArray(std::string(g_Locale->getText(LOCALE_EPGVIEWER_LENGTH)) + ": " + lengthInfo); // UTF-8

	// Show audio information
	std::string audioInfo = "";
	CSectionsdClient::ComponentTagList tags;
	bool hasComponentTags = g_Sectionsd->getComponentTagsUniqueKey(epgData.eventID, tags);
	if (hasComponentTags)
	{
		for (unsigned int i = 0; i < tags.size(); i++)
			if (tags[i].streamContent == 2 && !tags[i].component.empty())
				audioInfo += tags[i].component + ", ";

		if (!audioInfo.empty())
		{
			audioInfo.erase(audioInfo.size()-2);
			processTextToArray(std::string(g_Locale->getText(LOCALE_EPGVIEWER_AUDIO)) + ": " + Latin1_to_UTF8(audioInfo)); // UTF-8
		}
	}

	// Show genre information
	if (!epgData.contentClassification.empty())
		processTextToArray(std::string(g_Locale->getText(LOCALE_EPGVIEWER_GENRE)) + ": " + GetGenre(epgData.contentClassification[0])); // UTF-8
//	processTextToArray( epgData.userClassification.c_str() );


	// -- display more screenings on the same channel
	// -- 2002-05-03 rasc
	if (hasFollowScreenings(channel_id, epgData.title, epgData.epg_times.startzeit)) {
		processTextToArray(""); // UTF-8
		processTextToArray(std::string(g_Locale->getText(LOCALE_EPGVIEWER_MORE_SCREENINGS)) + ':'); // UTF-8
		FollowScreenings(epgData.epg_times.startzeit);
	}

	//show the epg
	frameBuffer->paintBoxRel(sx, sy, ox, toph, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->RenderString(sx+10, sy + topheight+ 3, ox-15, text1, COL_MENUHEAD);
	if (!(text2.empty()))
		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->RenderString(sx+10, sy + 2* topheight+ 3, ox-15, text2, COL_MENUHEAD);

	//show date-time....
	frameBuffer->paintBoxRel(sx, sy+oy-botboxheight, ox, botboxheight, COL_MENUHEAD_PLUS_0);
	std::string fromto;
	int widthl,widthr;
	fromto = epg_start;
	fromto += " - ";
	fromto += epg_end;

	widthl = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(fromto);
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(sx+40,  sy+oy-3, widthl, fromto, COL_MENUHEAD);
	widthr = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(epg_date);
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(sx+ox-40-widthr,  sy+oy-3, widthr, epg_date, COL_MENUHEAD);

	int showPos = 0;
	textCount = epgText.size();
	showText(showPos, sy + toph);

	// show Timer Event Buttons
	bool wzap = isCurrentEPG(channel_id);
	frameBuffer->paintBoxRel(sx, sy + oy, ox, buttonheight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);
	showTimerEventBar (true, wzap);

	//show Content&Component for Dolby & 16:9
	if (hasComponentTags)
	{
		for (unsigned int i=0; i< tags.size(); i++)
		{
			if( tags[i].streamContent == 1 && (tags[i].componentType == 2 || tags[i].componentType == 3) )
			{	
				frameBuffer->paintIcon(NEUTRINO_ICON_16_9, ox + sx - (ICON_LARGE_WIDTH + 2 ) - (ICON_LARGE_WIDTH + 2) - 4, sy + oy + (buttonheight >> 1) - (ICON_HEIGHT >> 1));
			}
			else if( tags[i].streamContent == 2 && tags[i].componentType == 5 )
			{
				frameBuffer->paintIcon(NEUTRINO_ICON_DD, ox + sx - (ICON_LARGE_WIDTH + 2) - 4, sy + oy + (buttonheight >> 1) - (ICON_HEIGHT >> 1));
			}
		}
	}

	if ( epg_done!= -1 )	//show event progressbar
	{		
 		CProgressBar pb(true, -1, -1, 100, 0, 0, true); //only green color 
		pb.paintProgressBarDefault (sx + 10 + widthl + 10 + ((ox-104-widthr-widthl-10-10-20)>>1), sy+oy-botheight, 104, botheight-6, epg_done, 100);
	}

	GetPrevNextEPGData( epgData.eventID, &epgData.epg_times.startzeit );
	if (!call_fromfollowlist)
	{
		int iy = sy + oy - botboxheight/2 - boticonheight/2;
		if (prev_id != 0)
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_LEFT, sx + 6, iy);
		if (next_id != 0)
			frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RIGHT, sx + ox - boticonwidth - 6, iy);
	}

	if ( doLoop )
	{
		neutrino_msg_t      msg;
		neutrino_msg_data_t data;

		int scrollCount;

		bool loop = true;

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);

		while(loop)
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
			neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

			scrollCount = medlinecount;

			switch ( msg )
			{
				case CRCInput::RC_left:
					if ((prev_id != 0) && !call_fromfollowlist)
					{
						show(channel_id, prev_id, &prev_zeit, false);
						showPos=0;
					}
					break;

				case CRCInput::RC_right:
					if ((next_id != 0) && !call_fromfollowlist)
					{
						show(channel_id, next_id, &next_zeit, false);
						showPos=0;
					}
					break;

				case CRCInput::RC_down:
				case CRCInput::RC_down|CRCInput::RC_Repeat:
					if(showPos+scrollCount<textCount)
					{
						showPos += scrollCount;
						showText(showPos, sy + toph);
					}
					break;

				case CRCInput::RC_up:
				case CRCInput::RC_up|CRCInput::RC_Repeat:
					showPos -= scrollCount;
					if(showPos<0)
						showPos = 0;
					else
						showText(showPos, sy + toph);
					break;
				case CRCInput::RC_plus:
					if(isCurrentEPG(channel_id)){
						if(g_settings.wzap_time> 14)
							g_settings.wzap_time+=5;
						else
							g_settings.wzap_time++;
						if(g_settings.wzap_time>60)
							g_settings.wzap_time = 0;
						showTimerEventBar(true, true);
					}
					break;
				case CRCInput::RC_minus:
					if (isCurrentEPG(channel_id)) {
						if(g_settings.wzap_time> 19)
							g_settings.wzap_time-=5;
						else
							g_settings.wzap_time--;

						if(g_settings.wzap_time<0)
							g_settings.wzap_time = 60;
						showTimerEventBar(true, true);
					}
					break;

				// 31.05.2002 dirch		record timer
				case CRCInput::RC_red:
					if (g_settings.recording_type != CNeutrinoApp::RECORDING_OFF)
					{
						CTimerdClient timerdclient;
						if(timerdclient.isTimerdAvailable())
						{
							std::string recDir = g_settings.recording_dir[0];
							if (g_settings.recording_choose_direct_rec_dir && g_settings.recording_type == RECORDING_FILE)
							{
								CRecDirChooser recDirs(LOCALE_TIMERLIST_RECORDING_DIR, NEUTRINO_ICON_TIMER, NULL, &recDir);
								hide();
								recDirs.exec(NULL,"");
								if (!bigFonts && g_settings.bigFonts) {
									g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getSize() * BIG_FONT_FAKTOR / 10);
									g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getSize() * BIG_FONT_FAKTOR / 10);
								}
								bigFonts = g_settings.bigFonts;
								show(channel_id, id, &startzeit, false, call_fromfollowlist);
								showPos = 0;
								recDir = recDirs.get_selected_dir();
							}
							
							if (recDir.empty() && (RECORDING_FILE == g_settings.recording_type))
							{
								printf("set zapto timer failed, no record directory...\n");
								ShowLocalizedMessage(LOCALE_TIMER_EVENTRECORD_TITLE, LOCALE_EPGLIST_ERROR_NO_RECORDDIR_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_ERROR);
							}
								
							if (!recDir.empty() || (RECORDING_FILE != g_settings.recording_type))
							{
								if (timerdclient.addRecordTimerEvent(channel_id,
												     epgData.epg_times.startzeit,
												     epgData.epg_times.startzeit + epgData.epg_times.dauer,
												     epgData.eventID, epgData.epg_times.startzeit,
												     epgData.epg_times.startzeit - (ANNOUNCETIME + 120 ),
												     TIMERD_APIDS_CONF, true, recDir, false) == -1)
								{
									if(askUserOnTimerConflict(epgData.epg_times.startzeit - (ANNOUNCETIME + 120),
												  epgData.epg_times.startzeit + epgData.epg_times.dauer))
									{
										timerdclient.addRecordTimerEvent(channel_id,
														 epgData.epg_times.startzeit,
														 epgData.epg_times.startzeit + epgData.epg_times.dauer,
														 epgData.eventID, epgData.epg_times.startzeit,
														 epgData.epg_times.startzeit - (ANNOUNCETIME + 120 ),
														 TIMERD_APIDS_CONF, true, recDir, true);
										ShowLocalizedMessage(LOCALE_TIMER_EVENTRECORD_TITLE, LOCALE_TIMER_EVENTRECORD_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);
									}
								} else {
									ShowLocalizedMessage(LOCALE_TIMER_EVENTRECORD_TITLE, LOCALE_TIMER_EVENTRECORD_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);
								}
							}
						}
						else
							printf("timerd not available\n");
					}
					break;

				// 31.05.2002 dirch		zapto timer
				case CRCInput::RC_yellow:
				{
					if(g_Timerd->isTimerdAvailable())
					{
						if (!g_Timerd->adzap_eventID && g_settings.wzap_time && isCurrentEPG(channel_id)) {
							g_Timerd->addAdZaptoTimerEvent(channel_id,
											time (NULL) + (g_settings.wzap_time * 60));
							loop = false;
						} else {
							g_Timerd->addZaptoTimerEvent(channel_id,
											epgData.epg_times.startzeit,
											epgData.epg_times.startzeit - ANNOUNCETIME, 0,
											epgData.eventID, epgData.epg_times.startzeit, 0, true);
							ShowLocalizedMessage(LOCALE_TIMER_EVENTTIMED_TITLE, LOCALE_TIMER_EVENTTIMED_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);
						}
					}
					else
						printf("timerd not available\n");
					break;
				}

				// more screenings
				case CRCInput::RC_blue:
					if (!followlist.empty() && !call_fromfollowlist)
					{
						hide();
						time_t tmp_sZeit = epgData.epg_times.startzeit;
						unsigned long long tmp_eID = epgData.eventID;
						EventList* eventList = new EventList();
						res = eventList->exec(channel_id, g_Locale->getText(LOCALE_EPGVIEWER_MORE_SCREENINGS_SHORT), followlist); // UTF-8
						delete eventList;
						if (res == menu_return::RETURN_EXIT_ALL)
							loop = false;
						else
						{
							if (!bigFonts && g_settings.bigFonts) {
								g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getSize() * BIG_FONT_FAKTOR / 10);
								g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getSize() * BIG_FONT_FAKTOR / 10);
							}
							bigFonts = g_settings.bigFonts;
							show(channel_id, tmp_eID, &tmp_sZeit, false);
							showPos = 0;
						}
					}
					break;

				case CRCInput::RC_help:
					bigFonts = bigFonts ? false : true;
					if(bigFonts)
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getSize() * BIG_FONT_FAKTOR / 10);
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getSize() * BIG_FONT_FAKTOR / 10);
					}else
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getSize() * 10 / BIG_FONT_FAKTOR);
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getSize() * 10 / BIG_FONT_FAKTOR);
					}
					g_settings.bigFonts = bigFonts;
					show(channel_id, id, &startzeit, false, call_fromfollowlist);
					showPos=0;
					break;

				case CRCInput::RC_ok:
				case CRCInput::RC_timeout:
					loop = false;
					break;

				case CRCInput::RC_setup:
					loop = false;
					res = menu_return::RETURN_EXIT_ALL;
					break;

				default:
					// konfigurierbare Keys handlen...
					if (msg == g_settings.key_channelList_cancel)
						loop = false;
					else if (msg_repeatok == CRCInput::RC_plus || msg_repeatok == CRCInput::RC_minus)
						;
					else
					{
						if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
						{
							loop = false;
							res = menu_return::RETURN_EXIT_ALL;
						}
					}
			}
		}
		hide();
	}
	return res;
}

void CEpgData::hide()
{
        // 2004-09-10 rasc  (bugfix, scale large font settings back to normal)
	if (bigFonts) {
		bigFonts = false;
		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getSize() * 10 / BIG_FONT_FAKTOR);
		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->setSize(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getSize() * 10 / BIG_FONT_FAKTOR);
	}

	frameBuffer->paintBackgroundBoxRel(sx, sy, ox, oy);
	showTimerEventBar(false);
}

void CEpgData::GetEPGData(const t_channel_id channel_id, unsigned long long id, time_t* startzeit )
{
	epgData.title.clear();
	epgText.clear();
	emptyLineCount = 0;

	bool res;

	if ( id!= 0 )
		res = g_Sectionsd->getEPGid( id, *startzeit, &epgData );
	else
		res = g_Sectionsd->getActualEPGServiceKey(channel_id, &epgData );

	if ( res )
	{
		// If we have items, merge and localize them (e.g. actor1, actor2, ... -> Actors)
		if (false == epgData.itemDescriptions.empty()) {
			reformatExtendedEvents("Year of production", g_Locale->getText(LOCALE_EPGEXTENDED_YEAR_OF_PRODUCTION), false, epgData);
			reformatExtendedEvents("Original title", g_Locale->getText(LOCALE_EPGEXTENDED_ORIGINAL_TITLE), false, epgData);
			reformatExtendedEvents("Director", g_Locale->getText(LOCALE_EPGEXTENDED_DIRECTOR), false, epgData);
			reformatExtendedEvents("Actor", g_Locale->getText(LOCALE_EPGEXTENDED_ACTORS), true, epgData);
			reformatExtendedEvents("Guests", g_Locale->getText(LOCALE_EPGEXTENDED_GUESTS), false, epgData);
			reformatExtendedEvents("Presenter", g_Locale->getText(LOCALE_EPGEXTENDED_PRESENTER), false, epgData);
		}

		struct tm pStartZeit;
		localtime_r(&epgData.epg_times.startzeit, &pStartZeit);
		char temp[11] = {0};
		strftime( temp, sizeof(temp), "%d.%m.%Y", &pStartZeit);
		epg_date = g_Locale->getText(CLocaleManager::getWeekday(&pStartZeit));
		epg_date += ". ";
		epg_date += temp;
		strftime( temp, sizeof(temp), "%H:%M", &pStartZeit);
		epg_start= temp;

		long int uiEndTime((epgData.epg_times).startzeit+ (epgData.epg_times).dauer);
		struct tm pEndeZeit;
		localtime_r((time_t*)&uiEndTime, &pEndeZeit);
		strftime( temp, sizeof(temp), "%H:%M", &pEndeZeit);
		epg_end= temp;

		epg_done= -1;
		if (( time(NULL)- (epgData.epg_times).startzeit )>= 0 )
		{
			unsigned nProcentagePassed=((time(NULL)-(epgData.epg_times).startzeit) * 100 / (epgData.epg_times).dauer);
			if (nProcentagePassed<= 100)
				epg_done= nProcentagePassed;
		}
	}
}

void CEpgData::GetPrevNextEPGData( unsigned long long id, time_t* startzeit )
{
	prev_id= 0;
	next_id= 0;
	unsigned int i;

	for ( i= 0; i< evtlist.size(); i++ )
	{
		//printf("%d %llx/%llx - %x %x\n", i, evtlist[i].eventID, id, evtlist[i].startTime, *startzeit);
		if ( ( evtlist[i].eventID == id ) && ( evtlist[i].startTime == *startzeit ) )
		{
			if ( i > 0 )
			{
				prev_id= evtlist[i- 1].eventID;
				prev_zeit= evtlist[i- 1].startTime;
			}
 			if ( i < ( evtlist.size()- 1 ) )
			{
				next_id= evtlist[i+ 1].eventID;
				next_zeit= evtlist[i+ 1].startTime;
			}
			break;
		}
	}
}


//
// -- get following screenings of this program title
// -- yek! a better class design would be more helpfull
// -- BAD THING: Cross channel screenings will not be shown
// --            $$$TODO
// -- 2002-05-03 rasc
//

void CEpgData::FollowScreenings(const time_t startzeit)
{
	CChannelEventList::iterator e;
	struct  tm		tmStartZeit;
	std::string		screening_dates, screening_nodual;
	int 			flag = SCREENING_AFTER;
	char			tmpstr[256] = {0};

	screening_dates = screening_nodual = "";

	for (e = followlist.begin(); e != followlist.end(); ++e)
	{
		localtime_r(&e->startTime, &tmStartZeit);

		screening_dates = g_Locale->getText(CLocaleManager::getWeekday(&tmStartZeit));
		screening_dates += '.';

		strftime(tmpstr, sizeof(tmpstr), " %d.", &tmStartZeit );
		screening_dates += tmpstr;

		screening_dates += g_Locale->getText(CLocaleManager::getMonth(&tmStartZeit));

		strftime(tmpstr, sizeof(tmpstr), ". %H:%M ", &tmStartZeit );
		screening_dates += tmpstr;

		flag = (e->startTime <= startzeit) ? SCREENING_BEFORE : SCREENING_AFTER;

		if (screening_dates != screening_nodual){
			screening_nodual=screening_dates;
			processTextToArray(screening_dates, flag ); // UTF-8
		}
	}
}


//
// -- Just display or hide TimerEventbar
// -- 2002-05-13 rasc
//
struct button_label epgviewButtons[3] =
{
	{ NEUTRINO_ICON_BUTTON_RED    , LOCALE_TIMERBAR_RECORDEVENT            },
	{ NEUTRINO_ICON_BUTTON_YELLOW , LOCALE_GENERIC_EMPTY                   }, // channelswitch / adZap
	{ NEUTRINO_ICON_BUTTON_BLUE   , LOCALE_EPGVIEWER_MORE_SCREENINGS_SHORT }
};

void CEpgData::showTimerEventBar(bool _show, bool webzap)
{
	int ButtonWidth = (ox - 70) / 3; // 3 cells + 16:9 & dd
	int by = sy + oy + 2;

	// hide only?
	if (! _show)
	{
		frameBuffer->paintBackgroundBoxRel(sx, sy + oy, ox, buttonheight);
		return;
	}

	frameBuffer->paintBoxRel(sx, sy + oy, ox-60, buttonheight, COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_LEFT);

	std::string tmp_but_name;
	const char *but_name = NULL;
	if (g_settings.wzap_time && webzap && !g_Timerd->adzap_eventID) {
		tmp_but_name = g_Locale->getText(LOCALE_ADZAP);
		tmp_but_name += " "+ to_string(g_settings.wzap_time) + " ";
		tmp_but_name += g_Locale->getText(LOCALE_WORD_MINUTES_SHORT);
		but_name = tmp_but_name.c_str();
	}
	else
		epgviewButtons[1].locale = LOCALE_TIMERBAR_CHANNELSWITCH;

	// Button: Timer Record & Channelswitch
	if (g_settings.recording_type != CNeutrinoApp::RECORDING_OFF)
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL],	g_Locale, sx + 6, by, ButtonWidth, 1, &epgviewButtons[0]);

	// Button: Timer Channelswitch / AdZap
	::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL],	g_Locale, sx + 6 + ButtonWidth-25, by, ButtonWidth, 1, &epgviewButtons[1], 0, false, COL_INFOBAR_SHADOW_PLUS_1, but_name);

	// Button: more screenings
	if (!followlist.empty() && !call_fromfollowlist)
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL],	g_Locale, sx + 6 + 2 * ButtonWidth, by, ButtonWidth, 1, &epgviewButtons[2], 2 * ButtonWidth - 2 * (ICON_LARGE_WIDTH + 2) - 4);
}





//
//  -- EPG Data Viewer Menu Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-06 rasc)
//

int CEPGDataHandler::exec(CMenuTarget* parent, const std::string & /*actionkey*/)
{
	int           res = menu_return::RETURN_EXIT_ALL;
	CChannelList  *channelList;
	CEpgData      *e;


	if (parent) {
		parent->hide();
	}

	e = new CEpgData;

	channelList = CNeutrinoApp::getInstance()->channelList;
	e->start();
	e->show( channelList->getActiveChannel_ChannelID() );
	delete e;

	return res;
}
