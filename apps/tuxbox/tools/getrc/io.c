/*
 * $Id: io.c,v 1.1 2009/12/31 14:52:03 rhabarber1848 Exp $
 *
 * getrc - d-box2 linux project
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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/un.h>
#include "io.h"

#define RC_DEVICE	"/dev/input/event0"

static struct input_event ev;
static unsigned short rccode;
static int rc;
static __u16 rc_last_key = KEY_RESERVED;

int InitRC(void)
{
	rc = open(RC_DEVICE, O_RDONLY);
	fcntl(rc, F_SETFL, fcntl(rc, F_GETFL) | O_EXCL | O_NONBLOCK);
	return 1;
}

int CloseRC(void)
{
	close(rc);
	return 1;
}

int RCKeyPressed(void)
{
int rval=0;

	if(read(rc, &ev, sizeof(ev)) == sizeof(ev))
	{
		rval=ev.value;
	}
	return rval;
}

int Translate(int gkey)
{
	int rv='X';
	
	switch(gkey)
	{
		case KEY_UP:		rv = RC_UP;
			break;

		case KEY_DOWN:		rv = RC_DOWN;
			break;

		case KEY_LEFT:		rv = RC_LEFT;
			break;

		case KEY_RIGHT:		rv = RC_RIGHT;
			break;

		case KEY_OK:		rv = RC_OK;
			break;

		case KEY_0:			rv = RC_0;
			break;

		case KEY_1:			rv = RC_1;
			break;

		case KEY_2:			rv = RC_2;
			break;

		case KEY_3:			rv = RC_3;
			break;

		case KEY_4:			rv = RC_4;
			break;

		case KEY_5:			rv = RC_5;
			break;

		case KEY_6:			rv = RC_6;
			break;

		case KEY_7:			rv = RC_7;
			break;

		case KEY_8:			rv = RC_8;
			break;

		case KEY_9:			rv = RC_9;
			break;

		case KEY_RED:		rv = RC_RED;
			break;

		case KEY_GREEN:		rv = RC_GREEN;
			break;

		case KEY_YELLOW:	rv = RC_YELLOW;
			break;

		case KEY_BLUE:		rv = RC_BLUE;
			break;

		case KEY_VOLUMEUP:	rv = RC_PLUS;
			break;

		case KEY_VOLUMEDOWN:	rv = RC_MINUS;
			break;

		case KEY_MUTE:		rv = RC_MUTE;
			break;

		case KEY_HELP:		rv = RC_HELP;
			break;

		case KEY_SETUP:		rv = RC_DBOX;
			break;

		case KEY_HOME:		rv = RC_HOME;
			break;

		case KEY_POWER:		rv = RC_STANDBY;
			break;
	}
	return rv;
}	
int GetRCCode(char *key, int timeout)
{
int tmo=timeout;
	
	timeout>>=1;
	
	if(key)
	{
		while(Translate(ev.code)!=(int)(*key))
		{
			RCKeyPressed();
			usleep(10000L);
			if(tmo &&((timeout-=10)<=0))
			{
				return 'X';
			}
		}
	}
	else
	{
		while(!RCKeyPressed())
		{
			usleep(10000L);
			if(tmo &&((timeout-=10)<=0))
			{
				return 'X';
			}
		}
	}

	do
	{
		rc_last_key = ev.code;
	}
	while(RCKeyPressed());

	rccode=Translate(rc_last_key);	

	return rccode;
}

