/*
 * $Id: globals.h,v 1.2 2010/03/07 15:22:04 rhabarber1848 Exp $
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

#ifndef __GLOBALS_H__
#define __GLOBALS_H__


#define P_VERSION "0.78"

#define ADS_FILE 	"/tmp/blockads.ads"
#define ZAP_FILE	"/tmp/blockads.zap"
#define LST_FILE	"/tmp/blockads.lst"
#define FLG_FILE	"/tmp/blockads.flg"
#define MSG_FILE	"/tmp/blockads.msg"
#define STS_FILE	"/tmp/blockads.sts"
#define CFG_FILE	"/var/tuxbox/config/blockads.conf"
#define NCF_FILE 	"/var/tuxbox/config/neutrino.conf"
#define NUM_CHANNELS 12


extern int rezap, inet, zapalways, volume, mute, debounce;
extern int wtime[10];

int Read_Neutrino_Cfg(char *entry);
int ReadConf();
void Msg_Popup(char *msg);
void Trim_String(char *buffer);
int Translate_Channel(char *source, char *target);
int Get_ChannelNumber(char *chan);
char *Get_ChannelName(int chan);
void Do_Rezap(char *chan, int vol, int mut);
void Do_Zap(char *chan);

int Open_Socket(void);
void Close_Socket(void);
int Check_Socket(int channel, int *state);
int Check_Channel(int channel, int *state);

#endif
