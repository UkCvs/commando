/*
 * $Id: clock.c,v 1.7 2012/06/16 14:27:26 rhabarber1848 Exp $
 *
 * Clock / SSaver - d-box2 linux project
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
#include <time.h>
#include "clock.h"
#include "text.h"
#include "gfx.h"
#include "io.h"
#include "color.h"

extern int FSIZE_BIG;
extern int FSIZE_MED;
extern int FSIZE_SMALL;

unsigned char FONT[64]= "/share/fonts/pakenham.ttf";

#define MAXCOL 25
#define NCFG_FILE "/var/tuxbox/config/neutrino.conf"
#define CCFG_FILE "/var/tuxbox/config/clock.conf"
#define SCFG_FILE "/var/tuxbox/config/ssaver.conf"
#define MAIL_FILE "/tmp/tuxmail.new"
//#define CCFG_FILE "/tmp/clock.conf"
//#define SCFG_FILE "/tmp/ssaver.conf"

#define CL_VERSION  "0.15"

//
static int col[MAXCOL]={TRANSP,BLACK,WHITE,RED,GREEN,OLIVE,BLUE0,PURPLE,DGREEN,SILVER,GRAY,LRED,
					LGREEN,LYELLOW,BLUE1,PINK,BLUE2,CMCST,CMCS,CMCT,CMC,CMCIT,CMCI,CMHT,CMH};
unsigned char *lfb = 0, *lbb = 0;
char tstr[512];
double xpos = 540, ypos = 0;
int cCol = 0;
int ssaver = 0;
int show_date = 0, big = 0, show_sec = 1, blink = 1, fcol = 2, bcol = 1, slow = 1,  mail = 0;

int ExistFile(char *fname)
{
FILE *efh;

	if((efh=fopen(fname,"r"))==NULL)
	{
		return 0;
	}
	fclose(efh);
	return 1;
}

void TrimString(char *strg)
{
char *pt1=strg, *pt2=strg;

	while(*pt2 && *pt2<=' ')
	{
		++pt2;
	}
	if(pt1 != pt2)
	{
		do
		{
			*pt1=*pt2;
			++pt1;
			++pt2;
		}
		while(*pt2);
		*pt1=0;
	}
	while(strlen(strg) && strg[strlen(strg)-1]<=' ')
	{
		strg[strlen(strg)-1]=0;
	}
}

int ReadConf(char *fname)
{
	FILE *fd_conf;
	char *cptr;

	//open config

	if(!(fd_conf = fopen(fname, "r")))
	{
		return 0;
	}
	while(fgets(tstr, 511, fd_conf))
	{
		TrimString(tstr);
		if((tstr[0]) && (tstr[0]!='#') && (!isspace(tstr[0])) && ((cptr=strchr(tstr,'='))!=NULL))
		{
			if (!ssaver)
			{
				if(strstr(tstr,"X") == tstr)
				{
					sscanf(cptr+1,"%lf",&xpos);
				}
				if(strstr(tstr,"Y=") == tstr)
				{
					sscanf(cptr+1,"%lf",&ypos);
				}
				if(strstr(tstr,"MAIL=") == tstr)
				{
					sscanf(cptr+1,"%d",&mail);
				}
			}
			if(strstr(tstr,"DATE=") == tstr)
			{
				sscanf(cptr+1,"%d",&show_date);
			}
			if(strstr(tstr,"BIG=") == tstr)
			{
				sscanf(cptr+1,"%d",&big);
			}
			if(strstr(tstr,"SLOW=") == tstr)
			{
				sscanf(cptr+1,"%d",&slow);
			}
			if(strstr(tstr,"SEC=") == tstr)
			{
				sscanf(cptr+1,"%d",&show_sec);
			}
			if(strstr(tstr,"BLINK=") == tstr)
			{
				sscanf(cptr+1,"%d",&blink);
			}
			if(strstr(tstr,"FCOL=") == tstr)
			{
				sscanf(cptr+1,"%d",&fcol);
			}
			if(strstr(tstr,"BCOL=") == tstr)
			{
				sscanf(cptr+1,"%d",&bcol);
			}
		}
	}
	fclose(fd_conf);
	return 1;
}

int Read_Neutrino_Cfg(char *entry)
{
FILE *nfh;
char tstr [512], *cfptr=NULL;
int rv=-1;

	if((nfh=fopen(NCFG_FILE,"r"))!=NULL)
	{
		tstr[0]=0;

		while((!feof(nfh)) && ((strstr(tstr,entry)==NULL) || ((cfptr=strchr(tstr,'='))==NULL)))
		{
			fgets(tstr,500,nfh);
		}
		if(!feof(nfh) && cfptr)
		{
			++cfptr;
			if(sscanf(cfptr,"%d",&rv)!=1)
			{
				rv=-1;
			}
		}
		fclose(nfh);
	}
	return rv;
}

void Change_Col(int *col1, int *col2)
{
	do
	{
		*col1 = rand() %(MAXCOL - 1) + 1;
	}
	while (*col1 == *col2);
}

/******************************************************************************
 * Clock / SSaver Main
 ******************************************************************************/

int main (int argc, char **argv)
{
	unsigned int margin_left_F, digit_width, margin_top_t, font_size, margin_top_box, margin_top_d, digits, secs_width, adj_height;
	int i = 0;
	int j = 0;
	int w = 0;
	int ms = 0;
	int mw = 0;
	int loop = 1;
	unsigned int newmail = 0;
	unsigned int mailgfx = 0;
	int xdir = 1, ydir = 1;
	double xstep = 1, ystep = 1; 
	double csx, cex, csy, cey;
	time_t atim;
	struct tm *ltim;
	char *aptr,*rptr;
	char dstr[2] = {0,0};
	FILE *tfh;

		printf("Clock / SSaver Version %s\n",CL_VERSION);

		for (i = 1; i < argc; i++)
		{
			if (!strncmp(argv[i], "-ss", 3))
			{
				ssaver = 1;
				continue;
			}
		}

		if (ssaver)
		{
			time(&atim);
			srand((unsigned int)atim);
			ReadConf(SCFG_FILE);
		}
		else
		{
			ReadConf(CCFG_FILE);
		}

		for (i = 1; i < argc; i++)
		{
			aptr=argv[i];
			if((rptr=strchr(aptr,'='))!=NULL)
			{
				rptr++;
				if (!ssaver)
				{
					if(strstr(aptr,"X=")!=NULL)
					{
						if(sscanf(rptr,"%d",&j)==1)
						{
							xpos=j;
						}
					}	
					if(strstr(aptr,"Y=")!=NULL)
					{
						if(sscanf(rptr,"%d",&j)==1)
						{
							ypos=j;
						}
					}
					if(strstr(aptr,"MAIL=")!=NULL)
					{
						if(sscanf(rptr,"%d",&j)==1)
						{
							mail=j;
						}
					}
				}
				if(strstr(aptr,"DATE=")!=NULL)
				{
					if(sscanf(rptr,"%d",&j)==1)
					{
						show_date=j;
					}
				}
				if(strstr(aptr,"BIG=")!=NULL)
				{
					if(sscanf(rptr,"%d",&j)==1)
					{
						big=(j)?1:0;
					}
				}
				if(strstr(aptr,"SEC=")!=NULL)
				{
					if(sscanf(rptr,"%d",&j)==1)
					{
						show_sec=j;
					}
				}
				if(strstr(aptr,"BLINK=")!=NULL)
				{
					if(sscanf(rptr,"%d",&j)==1)
					{
						blink=j;
					}
				}
				if(strstr(aptr,"SLOW=")!=NULL)
				{
					if(sscanf(rptr,"%d",&j)==1)
					{
						if(!j)
						{
							j=1;
						}
						slow=j;
					}
				}
				if(strstr(aptr,"FCOL=")!=NULL)
				{
					if(sscanf(rptr,"%d",&j)==1)
					{
						fcol=j;
					}
				}
				if(strstr(aptr,"BCOL=")!=NULL)
				{
					if(sscanf(rptr,"%d",&j)==1)
					{
						bcol=j;
					}
				}
			}
		}
		if((sx=Read_Neutrino_Cfg("screen_StartX"))<0)
			sx=80;
		
		if((ex=Read_Neutrino_Cfg("screen_EndX"))<0)
			ex=620;

		if((sy=Read_Neutrino_Cfg("screen_StartY"))<0)
			sy=80;

		if((ey=Read_Neutrino_Cfg("screen_EndY"))<0)
			ey=505;
		
		fb = open(FB_DEVICE, O_RDWR);

		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("Clock / SSaver <FBIOGET_FSCREENINFO failed>\n");
			return -1;
		}
		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("Clock / SSaver <FBIOGET_VSCREENINFO failed>\n");
			return -1;
		}
		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("Clock / SSaver <mapping of Framebuffer failed>\n");
			return -1;
		}

	//init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("Clock / SSaver <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("Clock / SSaver <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("Clock / SSaver <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_LookupFace(manager, FONT, &face)))
		{
			printf("Clock / SSaver <FTC_Manager_LookupFace failed with Errorcode 0x%.2X>\n", error);
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
			printf("Clock / SSaver <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if (!slow)
			slow=1;

		if (slow>10)
			slow=10;

		if (fcol > MAXCOL && !(ssaver == 1 && fcol == 99))
			fcol = 2;
		if (bcol > 3 && !(bcol == 10))
			bcol = 1;

		if (ssaver)
		{
			memset(lbb, col[bcol], var_screeninfo.xres*var_screeninfo.yres);
			memset(lfb, col[bcol], var_screeninfo.xres*var_screeninfo.yres);
		}
		else
			memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);

		if (big)			//grosse Schrift (time/date)
		{
			margin_left_F = 3;			// 3
			digit_width = 14;			// 14
			margin_top_t = 26;			// 26
			font_size = BIG;			// 40
			margin_top_box = 30;		// 30
			margin_top_d = 60;			// 60
		}
		else
		{
			margin_left_F = 7;			//7  Abstand links
			digit_width = 12;			//12 Ziffernblockbreite
			margin_top_t = 19;			//19 Abstand "TimeString"-Unterkante von oben
			font_size = MED;			//30 Schriftgroesse
			margin_top_box = 20;		//20 Abstand Renderbox von oben
			margin_top_d = 40;			//40 Abstand "DateString" von oben
		}
		digits = 0;
		secs_width = 0;
		startx = sx;
		starty = sy;
		mw = (big) ? 42 : 36;			//mailwidth
		adj_height = 1 * (!big && !show_date); //max steprange == 3, so we need always a top/bottom margin of >=3

		if (!show_sec && !show_date)
		{
			digits = 3;				//3 Platzhalter ':ss'
			secs_width = digits * digit_width;
		}

		if (ssaver)
		{
			xpos = rand() %480 + 10;
			ypos = rand() %460 + 10;	
			xdir *= (rand() &1) == 0 ? -1 : 1;
			ydir *= (rand() &1) == 0 ? -1 : 1;
			xstep/=(double)slow;
			ystep/=(double)slow;
			if (fcol == 99)
			{
				cCol = 1;
				Change_Col(&fcol, &bcol);
			}
			InitRC();
		}

	while (loop)
	{
		if (ssaver)
			usleep(15000L);
		else
		{
			usleep(150000L);
			newmail = 0;
			if(mail && ExistFile(MAIL_FILE))
			{
				if((tfh = fopen(MAIL_FILE,"r")) != NULL)
				{
					if(fgets(tstr, 511, tfh))
					{
						if(sscanf(tstr, "%d", &i))
						{
							newmail = i;
						}
					}
					fclose(tfh);
				}
			}
		}

		time(&atim);
		ltim=localtime(&atim);
		if (show_sec)
		{
			sprintf(tstr,"%02d:%02d:%02d", ltim->tm_hour, ltim->tm_min, ltim->tm_sec);
		}
		else
		{
			if (blink)
				sprintf(tstr,"   %02d%c%02d", ltim->tm_hour, (ltim->tm_sec & 1)? ' ' : ':', ltim->tm_min);
			else
				sprintf(tstr,"   %02d:%02d", ltim->tm_hour, ltim->tm_min);
		}

		if (!ssaver)
		{
			if (((int)xpos >= mw) || (!show_sec))
			{
				ms = (int)xpos + ((show_sec) ? 0 : mw) - mw;	//mail left
			}
			else
			{
				ms = (int)xpos + 100 + 20 * big;		//mail right
			}
			//paint Backgroundcolor to clear digit
			RenderBox(xpos+secs_width, ypos, xpos+secs_width+100+20*big, ypos+margin_top_box + adj_height, FILL, col[bcol]);
		}

		if (ssaver)
		{
			xpos += xstep * (double)xdir;
			ypos += ystep * (double)ydir;

			csx = xpos + secs_width;
			csy = ypos;
			cex = xpos + secs_width + 100 + 20 * big;
			cey = ypos + margin_top_t + 2 * (1 + big) + (margin_top_box * show_date) + adj_height;

			if ((int)csx < 0 || (sx + (int)cex) > ex)
			{
				if (cCol)
					Change_Col(&fcol, &bcol);
				xdir *= -1;
				xpos += xstep * (double)xdir;
				csx = xpos + secs_width;
				cex = xpos + secs_width + 100 + 20 * big;
				xstep = rand() &3;
				if (!xstep)
				{
					xstep = 1;
				}
				xstep /= (double)slow;
			}
			if ((int)csy < 0 || (sy + (int)cey) > ey)
			{
				if (cCol)
					Change_Col(&fcol, &bcol);
				ydir *= -1;
				ypos += ystep * (double)ydir;
				csy = ypos;
				cey = ypos + margin_top_t + 2 * (1 + big) + (margin_top_box * show_date) + adj_height;	
				ystep = rand() &3;
				if (!ystep)
				{
					ystep = 1;
				}
				ystep /= (double)slow;
			}
			RenderBox(csx, csy, cex, cey, FILL, col[bcol]);
		}

		for (i = digits; i < strlen(tstr); i++)
		{
			*dstr = tstr[i];
			RenderString(dstr, xpos - margin_left_F + (i * digit_width), ypos + margin_top_t, 30, CENTER, font_size, col[fcol]);
		}

		if (show_date)
		{
			sprintf(tstr, "%02d.%02d.%02d", ltim->tm_mday, ltim->tm_mon + 1, ltim->tm_year - 100);
			if (!ssaver)
			{
			//Backgroundbox color Date
			RenderBox(xpos, ypos + margin_top_box, xpos + 100 + 20 * big, ypos + margin_top_d, FILL, col[bcol]);
			}
			for(i = 0; i < strlen(tstr); i++)
			{
				*dstr = tstr[i];
				RenderString(dstr, xpos - margin_left_F + (i * digit_width), ypos + margin_top_d - 2 - (2 * big), 30, CENTER, font_size, col[fcol]);
			}
		}

		if (ssaver)
		{
			w = 100 + 20 * big + ((show_sec) ? 0 : - secs_width);
			for (i = 0; i <= ((show_date) ? 20 : 10) * (2 + big) + adj_height; i++)
			{
				j = (starty + (int)ypos + i) * var_screeninfo.xres + (int)xpos + ((show_sec) ? 0 : secs_width) + startx;
				if ((j + w) < var_screeninfo.xres * var_screeninfo.yres)
				{
					memcpy(lfb+j, lbb+j, w);
				}
			}
		}
		else
		{
			if (newmail > 0)
			{
				mailgfx = 1;

				//Background mail, left site from clock
				RenderBox(ms, ypos, ms+mw, ypos+margin_top_box + adj_height, FILL, col[bcol]); //bcol

				if(!(ltim->tm_sec & 1))
				{
					RenderBox (ms+8, ypos+5+(1+big), ms+mw-8, ypos+margin_top_box+adj_height-2-(3*big), GRID,	col[fcol]);
					DrawLine  (ms+8, ypos+5+(1+big), ms+mw-8, ypos+margin_top_box+adj_height-2-(3*big),			col[fcol]);
					DrawLine  (ms+8, ypos+margin_top_box+adj_height-2-(3*big), ms+mw-8, ypos+5+(1+big),			col[fcol]);
					DrawLine  (ms+(9+1*big), ypos+4+(1+big), ms+(mw/2), ypos+1,                  				col[fcol]);
					DrawLine  (ms+(9+1*big), ypos+5+(1+big), ms+(mw/2), ypos+2,                  				col[fcol]);
					DrawLine  (ms+(mw/2), ypos+1, ms+mw-(9+1*big), ypos+4+(1+big),               				col[fcol]);
					DrawLine  (ms+(mw/2), ypos+2, ms+mw-(9+1*big), ypos+5+(1+big),               				col[fcol]);
				}
				else
				{
					sprintf(tstr,"%d",newmail);
					RenderString(tstr, ms, ypos+margin_top_t, mw, CENTER, font_size, col[fcol]);
				}
			}
			else
			{
				if (mailgfx > 0)
					RenderBox(ms, ypos, ms+mw, ypos + margin_top_box + adj_height, FILL, (!show_date || show_sec) ? TRANSP : col[bcol]);
				else
					ms=(int)xpos;
			}

			w = 100 + 20 * big + ((mailgfx) ? ((show_sec) ? mw : 0) : - secs_width);
			for (i=0; i <= ((show_date) ? 20 : 10) * (2 + big) + adj_height; i++)
			{
				j = (starty + (int)ypos + i) * var_screeninfo.xres + ( ((ms < (int)xpos) ? ms : (int)xpos) + ((show_sec) ? 0 : ((mailgfx) ? 0 : secs_width)) ) + startx;			
				if ((j + w) < var_screeninfo.xres * var_screeninfo.yres)
				{
					memcpy(lfb+j, lbb+j, w);
				}
			}
			if (newmail == 0 && mailgfx > 0)
				mailgfx = 0;
		}

		if (++loop > 10)
		{
			if ( (ssaver && (RCKeyPressed() || ExistFile("/tmp/.ssaver_kill")))
				|| (!ssaver && ExistFile("/tmp/.clock_kill")) )
				loop = 0;
		}
	}


/****************************
 * close down Clock / SSaver
 ****************************/
	if (ssaver)
	{
		memset(lfb, 0, var_screeninfo.xres*var_screeninfo.yres);
		remove("/tmp/.ssaver_kill");
		CloseRC();
	}
	else
	{
		memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);
		remove("/tmp/.clock_kill");
		for (i=0; i <= ((show_date) ? 20 : 10) * (2 + big) + adj_height; i++)
		{
			j=(starty+(int)ypos+i)*var_screeninfo.xres+((ms<(int)xpos)?ms:(int)xpos)+((show_sec)?0:((mailgfx)?0:secs_width))+startx;
			if((j+100+20*big+((mail)?mw:0))<var_screeninfo.xres*var_screeninfo.yres)
			{
				memcpy(lfb+j, lbb+j, w);
			}
		}
	}

	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	free(lbb);
	munmap(lfb, fix_screeninfo.smem_len);

	close(fb);

	return 0;
}
