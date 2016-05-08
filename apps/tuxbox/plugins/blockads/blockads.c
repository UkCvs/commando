/*
 * $Id: blockads.c,v 1.6 2012/06/16 14:27:26 rhabarber1848 Exp $
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

#include <string.h>
#include <stdio.h>
#include "blockads.h"
#include "gfx.h"
#include "io.h"
#include "text.h"
#include "http.h"
#include "globals.h"


struct fb_fix_screeninfo fix_screeninfo;
struct fb_var_screeninfo var_screeninfo;
int fb, rc;
int startx, starty, sx, ex, sy, ey;
int radius;
unsigned char msgchan[40],*lfb, *lbb;

static void ShowInfo(int index)
{
	int loop, tind;
	char tstr[512];
	int ixw=160, iyw=300, dy, my, offs=10, soffs=3;

	dy=iyw/12;
	my=2*dy;
	
	startx = sx + (((ex-sx) - ixw)/2);
	starty = sy + (((ey-sy) - iyw)/2);

	tind=index;
	
	//frame layout
	RenderBox(0, 0, ixw, iyw, radius, CMC);
//	RenderBox(0, 0, ixw, iyw, GRID, CMCS, radius);

	// titlebar
	RenderBox(2, 2, ixw-2, dy+2*soffs, radius, CMH);

	//selectbar
	RenderBox(2, offs+soffs+(index+1)*dy, ixw, offs+soffs+(index+2)*dy, radius, CMCS);

	RenderString("Rezap-Zeit", 0, dy, ixw, CENTER, MED, CMHT);

	for(loop = 0; loop < 10; ++loop)
	{
		sprintf(tstr,"%1d =",(loop<9)?(loop+1):0);
		RenderString(tstr, 10, offs+my, 40, CENTER, MED, (loop == tind)?CMCST:CMCT);
		sprintf(tstr,"%2d   min",wtime[loop]);
		RenderString(tstr, 40, offs+my, ixw-60, RIGHT, MED, (loop == tind)?CMCST:CMCT);
		my += dy;
	}

	//copy backbuffer to framebuffer
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
}

void Start_NoInet(char *chan)
{
int ztim=30,cindex=0,cloop=1,rcm, found=0;
char nstr[128],sstr[50],*nptr;
FILE *fh;

	cindex=rezap-1;
	if(cindex<0)
	{
		cindex=0;
	}
	if(cindex>9)
	{
		cindex=9;
	}
	
	ShowInfo(cindex);

	do{
		rcm=-1;
		switch((rcm = GetRCCode()))
		{
			case RC_1:
				cindex=0;
				cloop=0;
				break;
	
			case RC_2:
				cindex=1;
				cloop=0;
				break;

			case RC_3:
				cindex=2;
				cloop=0;
				break;

			case RC_4:
				cindex=3;
				cloop=0;
				break;

			case RC_5:
				cindex=4;
				cloop=0;
				break;

			case RC_6:
				cindex=5;
				cloop=0;
				break;

			case RC_7:
				cindex=6;
				cloop=0;
				break;

			case RC_8:
				cindex=7;
				cloop=0;
				break;

			case RC_9:
				cindex=8;
				cloop=0;
				break;

			case RC_0:
				cindex=9;
				cloop=0;
				break;

			case RC_UP:
			case RC_MINUS:	--cindex;
				break;
		
			case RC_DOWN:	
			case RC_PLUS:	++cindex;
				break;
			
			case RC_OK:	
				cloop=0;
				break;
						
			case RC_HOME: 
				cloop=-1; 
				break;

			case RC_BLUE:
				cindex=rezap-1;
				if(cindex<0)
				{
					cindex=0;
				}
				if(cindex>9)
				{
					cindex=9;
				}
				cloop=0;
			break;	

			case RC_MUTE:	break;
		
			case RC_STANDBY: break;
		
			default:	continue;
		}

		if (cindex>=10)
		{
			cindex=0;
		}
		if(cindex<0)
		{
			cindex=9;
		}

		ShowInfo(cindex);

	} while(cloop>0);

	if(cloop==0)
	{
		ztim=60*wtime[cindex];			
		if((fh=fopen(FLG_FILE,"w"))!=NULL)
		{
			fprintf(fh,"%s\n-1\n%d\n%s\n%d\n%d\n%d\n",chan,ztim,msgchan,volume,mute,debounce);
			fclose(fh);
			system("/bin/blockad &");
			if((fh=fopen(CFG_FILE, "r"))!=NULL)
			{
				sprintf(sstr,"ZapChan%1d=",cindex);
				while(!found && fgets(nstr, sizeof(nstr), fh))
				{
					if(strstr(nstr,sstr)==nstr)
					{
						Trim_String(nstr);
						nptr=strchr(nstr,'=');
						++nptr;
						if(strlen(nptr))
						{
							strncpy(sstr,nptr,sizeof(sstr));
							found=1;
						}
					}
				}
				fclose(fh);
			}
			if(found)
			{
				Do_Zap(sstr);
				sprintf(nstr,"Umgeschaltet auf %s.\nAutomatische Rückschaltung in %d:00 min.",sstr,ztim/60);
			}
			else
			{
				sprintf(nstr,"Sie können jetzt umschalten.\nAutomatische Rückschaltung in %d:00 min.",ztim/60);
			}
			Msg_Popup(nstr);
		}
	}
}

/******************************************************************************
 * Blockads Main
 ******************************************************************************/
int main (void)
{
int loop=0,found,werbung=0,tnet, tv, cnum=-1;
char tstr[512],line_buffer[512],zapchan[40],srcchan[40],*cpt1=NULL;
FILE *fh1,*fh2;

	printf("Blockads Version %s\n",P_VERSION);
	
	if((fh1=fopen(FLG_FILE,"r"))==NULL)
	{
//		system("ping -c 5 google.com &");
		fb = open(FB_DEVICE, O_RDWR);
		rc = open(RC_DEVICE, O_RDONLY);
		fcntl(rc, F_SETFL, (fcntl(rc, F_GETFL) | O_EXCL) & ~O_NONBLOCK);


		if((sx=Read_Neutrino_Cfg("screen_StartX"))<0)
			sx=80;
		
		if((ex=Read_Neutrino_Cfg("screen_EndX"))<0)
			ex=620;

		if((sy=Read_Neutrino_Cfg("screen_StartY"))<0)
			sy=80;

		if((ey=Read_Neutrino_Cfg("screen_EndY"))<0)
			ey=505;

		if(Read_Neutrino_Cfg("rounded_corners")>0)
			radius=9;
		else
			radius=0;

	//init framebuffer
		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("Blockads <FBIOGET_FSCREENINFO failed>\n");
			return -1;
		}
		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("Blockads <FBIOGET_VSCREENINFO failed>\n");
			return -1;
		}
		
		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("Blockads <mapping of Framebuffer failed>\n");
			return -1;
		}
	
	//init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("Tuxwetter <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("Tuxwetter <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("Tuxwetter <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_LookupFace(manager, FONT, &face)))
		{
			printf("Tuxwetter <FTC_Manager_LookupFace failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		use_kerning = FT_HAS_KERNING(face);

		desc.face_id = FONT;
		desc.flags = FT_LOAD_MONOCHROME;
	//init backbuffer
		if(!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("Blockads <allocating of Backbuffer failed>\n");
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
	
		if (!ReadConf())
		{
			printf("Blockads <Configuration failed>\n");
			return -1;
		}
		found=0;
		loop=0;
		if(!HTTP_downloadFile("localhost",80,"/control/channellist", LST_FILE, 0, 1))
		{
			if(!HTTP_downloadFile("localhost",80,"/control/zapto", ZAP_FILE, 0, 1))
			{
				if((fh1=fopen(ZAP_FILE,"r"))!=NULL)
				{
					while((fgets(line_buffer, sizeof(line_buffer), fh1)>0) && (strlen(line_buffer)<4));
					if(strlen(line_buffer)>1)
					{
						Trim_String(line_buffer);
						strcpy(zapchan,line_buffer);
						if((fh2=fopen(LST_FILE,"r"))!=NULL)
						{
							while(!found && (fgets(line_buffer, sizeof(line_buffer), fh2)))
							{
								if(strstr(line_buffer,zapchan)==line_buffer)
								{
									if((cpt1=strchr(line_buffer,' '))!=NULL)
									{
										cpt1++;
										Trim_String(cpt1);
										strcpy(msgchan,cpt1);
										loop=1;
										if(!Translate_Channel(cpt1, tstr))
										{
											cnum=Get_ChannelNumber(tstr);
											strcpy(srcchan,tstr);
											found=1;
										}
									}
								}
							}
							if(!found)
							{
								inet=-1;
							}
							fclose(fh2);
						}
					}
					fclose(fh1);
				}
			}
		}		
		if(!loop)
		{
			sprintf(tstr,"Fehler beim Auslesen der Kanalliste.\nPlugin konnte nicht gestartet werden.");
			Msg_Popup(tstr);
			return -1;
		}
		if(volume)
		{
			volume=0;
			if(!HTTP_downloadFile("localhost",80,"/control/volume?status", ZAP_FILE, 0, 1))
			{
				if((fh1=fopen(ZAP_FILE,"r"))!=NULL)
				{
					while((fgets(line_buffer, sizeof(line_buffer), fh1)>0) && !strlen(line_buffer));
					if(strlen(line_buffer)>=1)
					{
						Trim_String(line_buffer);
						sscanf(line_buffer,"%d",&mute);
					}
					fclose(fh1);
				}
			}
			if(!HTTP_downloadFile("localhost",80,"/control/volume", ZAP_FILE, 0, 1))
			{
				if((fh1=fopen(ZAP_FILE,"r"))!=NULL)
				{
					while((fgets(line_buffer, sizeof(line_buffer), fh1)>0) && !strlen(line_buffer));
					if(strlen(line_buffer)>=1)
					{
						Trim_String(line_buffer);
						sscanf(line_buffer,"%d",&volume);
					}
					fclose(fh1);
				}
			}
		}
		if(inet>=0)
		{
			tnet=-1;
			if(Open_Socket()!=-1)
			{
				tv=Check_Socket(cnum,&werbung);
				if(tv>0)
				{
					tnet=inet;
					if(!werbung)
					{
						if(!zapalways)
						{
							int adf=0;

							sprintf(tstr,"Für Kanal %s wird\nmomentan keine Werbung gemeldet.\nWerbung läuft im Augenblick auf:\n",msgchan);
							for(tv=0; tv<NUM_CHANNELS; tv++)
							{
								Check_Channel(tv,&found);
								if(found)
								{
									sprintf(tstr+strlen(tstr),"\n%s",Get_ChannelName(tv));
									adf=1;
								}
							}
							if(!adf)
							{
								sprintf(tstr+strlen(tstr),"\nkeinem anderen Sender");
							}
							Msg_Popup(tstr);
							loop=0;
						}
						else
						{
							tnet=-1;
						}
					}
					else
					{
						if((fh2=fopen(FLG_FILE,"w"))!=NULL)
						{
							int adf=0;
							
							fprintf(fh2,"%s\n%d\n%d\n%s\n%d\n%d\n%d\n",zapchan,cnum,rezap*60,msgchan,volume,mute,debounce);
							fclose(fh2);
							system("/bin/blockad &");
							sprintf(tstr,"Werbezapper fr Kanal %s aktiviert.\nSie können jetzt umschalten.\n\nWerbung läuft im Augenblick auch auf:\n",msgchan);
							for(tv=0; tv<NUM_CHANNELS; tv++)
							{
								Check_Channel(tv,&found);
								if(found && (tv != cnum))
								{
									sprintf(tstr+strlen(tstr),"\n%s",Get_ChannelName(tv));
									adf=1;
								}
							}
							if(!adf)
							{
								sprintf(tstr+strlen(tstr),"\nkeinem anderen Sender");
							}
							Msg_Popup(tstr);
						}
					}
				}
				Close_Socket();
			}
			inet=tnet;
		}
		if(inet==-1 && loop)
		{
			Start_NoInet(zapchan);
		}
	}
	else
	{
		Msg_Popup("Werbezapper deaktiviert");
		remove(FLG_FILE);
		sleep(2);
	}

	// clear Display
	memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
	munmap(lfb, fix_screeninfo.smem_len);
	close(fb);
	free(lbb);

	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);


	fcntl(rc, F_SETFL, O_NONBLOCK);
	
	close(rc);

	return 0;
}

