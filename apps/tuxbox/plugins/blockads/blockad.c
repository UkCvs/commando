/*
 * $Id: blockad.c,v 1.1 2010/03/03 20:47:03 rhabarber1848 Exp $
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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <time.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "globals.h"
#include "lcd.h"
#include "http.h"

/******************************************************************************
 * Blockad Main
 ******************************************************************************/

int main (void)
{
int loop=0, werbung=0, tnet=-1,blit=0,cnum,tv,wflag=1,runtime=0, mut=0, vol=0, tmin=15;
char tstr[40],zapchan[40],msgchan[40];
FILE *fh1;
time_t t1,t2,t3,t4;

	printf("Blockad Version %s\n",P_VERSION);

	if (!ReadConf())
	{
		printf("Blockad <Configuration failed>\n");
	}
	else
	{
		if((fh1=fopen(FLG_FILE,"r"))!=NULL)
		{
			if(fgets(zapchan, sizeof(zapchan), fh1))
			{
				Trim_String(zapchan);
				if(fgets(tstr, sizeof(tstr), fh1))
				{
					sscanf(tstr,"%d",&cnum);
					if(fgets(tstr, sizeof(tstr), fh1))
					{
						sscanf(tstr,"%d",&rezap);
						if(fgets(msgchan, sizeof(msgchan), fh1))
						{
							Trim_String(msgchan);
						}
						else
						{
							sprintf(msgchan,"Unbekannt");
						}
						if(fgets(tstr, sizeof(tstr), fh1))
						{
							if(sscanf(tstr,"%d",&vol)==1)
							{
								if(volume)
								{
									if(fgets(tstr, sizeof(tstr), fh1))
									{
										if(sscanf(tstr,"%d",&mut)!=1)
										{
											mut=0;
										}
									}
								}
							}
						}
						if(fgets(tstr, sizeof(tstr), fh1))
						{
							if(sscanf(tstr,"%d",&tmin)!=1)
							{
								tmin=15;
							}
						}
					}
				}
				else
				{
					rezap=420;
				}	
				loop=1;
				if(cnum<0)
				{
					inet=-1;
				}
			}
			fclose(fh1);
		}
		time(&t1);
		t4=t3=t1;
		LCD_Init();

		if(inet>=0)
		{
			if(Open_Socket()==-1)
			{
				inet=-1;
			}
		}
		while(loop)
		{
			time(&t2);
			if(inet>=0)
			{
				tnet=-1;
				if(inet>=0)
				{
					tv=Check_Socket(cnum,&werbung);
					if(!werbung)
					{
						if((t2-t4)>tmin)
						{
							Do_Rezap(zapchan,vol,mut);
							loop=0;
						}
					}
					else
					{
						t4=t2;
					}
					if(tv>=0)
					{
						tnet=inet;
					}
				}
				inet=tnet;
			}
			if(loop)
			{
				if((fh1=fopen(FLG_FILE,"r"))!=NULL)
				{
					fclose(fh1);
					if(inet>=0)
					{
						if(wflag)
						{
							fh1=fopen(STS_FILE,"w");
							fprintf(fh1,"%s\n",msgchan);
							fprintf(fh1,"Auto\n");
							fclose(fh1);
							wflag=0;
						}
						if(t3!=t2)
						{
							sprintf(tstr,"%s",(blit^=1)?"   ":" WZ");
							LCD_Read();
							LCD_draw_string(97, 13, tstr);
							LCD_update();
							t3=t2;
						}
						sleep(1);
					}
					else
					{
						runtime=t2-t1;
						if(runtime>rezap)
						{
							Do_Rezap(zapchan,vol,mut);
							loop=0;
						}
						else
						{
							if(t3!=t2)
							{
								runtime=rezap-runtime;
								sprintf(tstr,"%2d:%02d",runtime/60,runtime%60);
								fh1=fopen(STS_FILE,"w");
								fprintf(fh1,"%s\n",msgchan);
								fprintf(fh1,"%s\n",tstr);
								fclose(fh1);
								LCD_Read();
								LCD_draw_string(81, 13, tstr);
								LCD_update();
								sleep(1);
								t3=t2;
							}
						}
					}
				}
				else
				{
					loop=0;
				}
			}
		}
	}

	LCD_Close();
	if(inet>=0)
	{
		Close_Socket();
	}
	sprintf(tstr,"cd /tmp\nrm blockads.*");
	system(tstr);

	return 0;
}

