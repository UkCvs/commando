/*
 * $Id: io.h,v 1.1 2009/12/31 14:52:03 rhabarber1848 Exp $
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

#ifndef __IO_H__

#define __IO_H__

//rc codes

#define	RC_0		0x30
#define	RC_1		0x31
#define	RC_2		0x32
#define	RC_3		0x33
#define	RC_4		0x34
#define	RC_5		0x35
#define	RC_6		0x36
#define	RC_7		0x37
#define	RC_8		0x38
#define	RC_9		0x39
#define	RC_RIGHT	0x41
#define	RC_LEFT		0x42
#define	RC_UP		0x43
#define	RC_DOWN		0x44
#define	RC_OK		0x45
#define	RC_MUTE		0x46
#define	RC_STANDBY	0x47
#define	RC_GREEN	0x48
#define	RC_YELLOW	0x49
#define	RC_RED		0x4A
#define	RC_BLUE		0x4B
#define	RC_PLUS		0x4C
#define	RC_MINUS	0x4D
#define	RC_HELP		0x4E
#define	RC_DBOX		0x4F
#define	RC_HOME		0x50

int InitRC(void);
int CloseRC(void);
int RCKeyPressed(void);
int GetRCCode(char *key, int timeout);

#endif
