/*
 * $Id: io.c,v 1.1 2009/12/31 14:52:03 rhabarber1848 Exp $
 *
 * input - d-box2 linux project
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
#include "inputd.h"

#ifdef HAVE_DBOX_HARDWARE

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

int GetRCCode(void)
{
	if(!RCKeyPressed() || (get_instance()>instance))
	{
		return -1;
	}

	do
	{
		rc_last_key = ev.code;
	}
	while(RCKeyPressed());
	
	switch(rc_last_key)
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

		case KEY_0:			rccode = RC_0;
			break;

		case KEY_1:			rccode = RC_1;
			break;

		case KEY_2:			rccode = RC_2;
			break;

		case KEY_3:			rccode = RC_3;
			break;

		case KEY_4:			rccode = RC_4;
			break;

		case KEY_5:			rccode = RC_5;
			break;

		case KEY_6:			rccode = RC_6;
			break;

		case KEY_7:			rccode = RC_7;
			break;

		case KEY_8:			rccode = RC_8;
			break;

		case KEY_9:			rccode = RC_9;
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

		default:			rccode = -1;
	}

	return rccode;
}

#else

#define RC_DEVICE	"/dev/dbox/rc0"

int key_count;
unsigned short lastkey;
static unsigned short rccode;
static int rc;


char buf[32];
int x=0;

int InitRC(void)
{
	rc = open(RC_DEVICE, O_RDONLY|O_NONBLOCK);
	// RC Buffer leeren
	read(rc,buf,32);
	return 1;
}

int CloseRC(void)
{
	close(rc);
	return 1;
}


int RCKeyPressed(void)
{
// Nix
}


int GetRCCode(void)
{

  x=read(rc, &rccode, 2);
  
  if (x < 2)
    return -1;
  
  if (lastkey == rccode)
      {
      key_count++;
      if (key_count < 3)
          return -1;
      } else
          key_count=0;

  lastkey=rccode;

  if((rccode & 0xFF00) == 0x5C00)
  {
      	switch(rccode)
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

		case KEY_0:			rccode = RC_0;
			break;

		case KEY_1:			rccode = RC_1;
			break;

		case KEY_2:			rccode = RC_2;
			break;

		case KEY_3:			rccode = RC_3;
			break;

		case KEY_4:			rccode = RC_4;
			break;

		case KEY_5:			rccode = RC_5;
			break;

		case KEY_6:			rccode = RC_6;
			break;

		case KEY_7:			rccode = RC_7;
			break;

		case KEY_8:			rccode = RC_8;
			break;

		case KEY_9:			rccode = RC_9;
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

		default:			rccode = -1;
	}

  }
  else 
       rccode = -1;

  return rccode;
}

#endif
