/*
 * $Id: io.c,v 1.2 2011/05/14 13:34:51 rhabarber1848 Exp $
 *
 * blockads - d-box2 linux project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
*/

#include <stdio.h>
#include "blockads.h"

#if HAVE_DVB_API_VERSION >= 3

int GetRCCode()
{
	static __u16 rc_last_key = KEY_RESERVED;
	//get code

	if(read(rc, &ev, sizeof(ev)) == sizeof(ev))
	{
		if(ev.value)
		{
			if(ev.code != rc_last_key)
			{
				rc_last_key = ev.code;
				switch(ev.code)
				{
					case KEY_UP:		rccode = RC_UP;
								break;

					case KEY_DOWN:		rccode = RC_DOWN;
								break;

					case KEY_LEFT:		rccode = RC_LEFT;
								break;

					case KEY_RIGHT:		rccode = RC_RIGHT;
								break;

					case KEY_OK:		rccode = RC_OK;
								break;

					case KEY_0:		rccode = RC_0;
								break;

					case KEY_1:		rccode = RC_1;
								break;

					case KEY_2:		rccode = RC_2;
								break;

					case KEY_3:		rccode = RC_3;
								break;

					case KEY_4:		rccode = RC_4;
								break;

					case KEY_5:		rccode = RC_5;
								break;

					case KEY_6:		rccode = RC_6;
								break;

					case KEY_7:		rccode = RC_7;
								break;

					case KEY_8:		rccode = RC_8;
								break;

					case KEY_9:		rccode = RC_9;
								break;

					case KEY_RED:		rccode = RC_RED;
								break;

					case KEY_GREEN:		rccode = RC_GREEN;
								break;

					case KEY_YELLOW:	rccode = RC_YELLOW;
								break;

					case KEY_BLUE:		rccode = RC_BLUE;
								break;

					case KEY_VOLUMEUP:	rccode = RC_PLUS;
								break;

					case KEY_VOLUMEDOWN:	rccode = RC_MINUS;
								break;

					case KEY_MUTE:		rccode = RC_MUTE;
								break;

					case KEY_HELP:		rccode = RC_HELP;
								break;

					case KEY_SETUP:		rccode = RC_DBOX;
								break;

					case KEY_HOME:		rccode = RC_HOME;
								break;

					case KEY_POWER:		rccode = RC_STANDBY;
								break;

					default:		rccode = -1;
				}
			}
			else
				rccode = -1;
		}
		else
		{
			rccode = -1;
			rc_last_key = KEY_RESERVED;
		}
	}

		return rccode;
}

#else


int GetRCCode()
{
	static unsigned short LastKey = -1;

	//get code

		read(rc, &rccode, sizeof(rccode));

		if(rccode != LastKey)
		{
			LastKey = rccode;

			//translation required?

				if((rccode & 0xFF00) == 0x5C00)
				{
					switch(rccode)
					{
						case RC1_UP:		rccode = RC_UP;
									break;

						case RC1_DOWN:		rccode = RC_DOWN;
									break;

						case RC1_LEFT:		rccode = RC_LEFT;
									break;

						case RC1_RIGHT:		rccode = RC_RIGHT;
									break;

						case RC1_OK:		rccode = RC_OK;
									break;

						case RC1_0:		rccode = RC_0;
									break;

						case RC1_1:		rccode = RC_1;
									break;

						case RC1_2:		rccode = RC_2;
									break;

						case RC1_3:		rccode = RC_3;
									break;

						case RC1_4:		rccode = RC_4;
									break;

						case RC1_5:		rccode = RC_5;
									break;

						case RC1_6:		rccode = RC_6;
									break;

						case RC1_7:		rccode = RC_7;
									break;

						case RC1_8:		rccode = RC_8;
									break;

						case RC1_9:		rccode = RC_9;
									break;

						case RC1_RED:		rccode = RC_RED;
									break;

						case RC1_GREEN:		rccode = RC_GREEN;
									break;

						case RC1_YELLOW:	rccode = RC_YELLOW;
									break;

						case RC1_BLUE:		rccode = RC_BLUE;
									break;

						case RC1_PLUS:		rccode = RC_PLUS;
									break;

						case RC1_MINUS:		rccode = RC_MINUS;
									break;

						case RC1_MUTE:		rccode = RC_MUTE;
									break;

						case RC1_HELP:		rccode = RC_HELP;
									break;

						case RC1_DBOX:		rccode = RC_DBOX;
									break;

						case RC1_HOME:		rccode = RC_HOME;
									break;

						case RC1_STANDBY:	rccode = RC_STANDBY;
					}
				}
				else rccode &= 0x003F;
		}
		else rccode = -1;

		return rccode;
}

#endif
