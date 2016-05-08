/*
 * $Id: input.c,v 1.6 2012/06/16 14:27:28 rhabarber1848 Exp $
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

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include "input.h"
#include "text.h"
#include "io.h"
#include "gfx.h"
#include "color.h"
#include "inputd.h"
#ifdef HAVE_DBOX_HARDWARE
#include <dbox/fb.h>
#endif

#define NCF_FILE 	"/var/tuxbox/config/neutrino.conf"
#define ECF_FILE	"/var/tuxbox/config/engima/config"
#define BUFSIZE 	1024
#define I_VERSION	1.40

#define FONT "/share/fonts/pakenham.ttf"

void TrimString(char *strg);

char *buffer=NULL;

// Misc
char NOMEM[]="input <Out of memory>\n";
char TMP_FILE[]="/tmp/input.tmp";
unsigned char *lfb = 0, *lbb = 0, *obb = 0;
unsigned char nstr[512]="";
unsigned char *trstr;
unsigned char rc,sc[8]={'a','o','u','A','O','U','z','d'}, tc[8]={'ä','ö','ü','Ä','Ö','Ü','ß','°'};
int radius;

static void quit_signal(int sig)
{
	put_instance(get_instance()-1);
	printf("input Version %.2f killed\n",I_VERSION);
	exit(1);
}

int Read_Neutrino_Cfg(char *entry)
{
FILE *nfh;
char tstr [512], *cfptr=NULL;
int rv=-1,styp=0;

	if( ( ((nfh=fopen(NCF_FILE,"r"))!=NULL) && (styp=1) ) || ( ((nfh=fopen(ECF_FILE,"r"))!=NULL) && (styp=2) ) )
	{
		tstr[0]=0;

		while((!feof(nfh)) && ((strstr(tstr,entry)==NULL) || ((cfptr=strchr(tstr,'='))==NULL)))
		{
			fgets(tstr,500,nfh);
		}
		if(!feof(nfh) && cfptr)
		{
			++cfptr;
			if(styp==1)
			{
				if(sscanf(cfptr,"%d",&rv)!=1)
				{
					if(strstr(cfptr,"true")!=NULL)
					{
						rv=1;
					}
					else
					{
						if(strstr(cfptr,"false")!=NULL)
						{
							rv=0;
						}
						else
						{
							rv=-1;
						}
					}
				}
			}
			if(styp==2)
			{
				if(sscanf(cfptr,"%x",&rv)!=1)
				{
					if(strstr(cfptr,"true")!=NULL)
					{
						rv=1;
					}
					else
					{
						if(strstr(cfptr,"false")!=NULL)
						{
							rv=0;
						}
						else
						{
							rv=-1;
						}
					}
				}
			}
		}
		fclose(nfh);
	}
	return rv;
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

int Transform_Msg(char *msg)
{
int found=0,i;
char *sptr=msg, *tptr=msg;

	while(*sptr)
	{
		if(*sptr!='~')
		{
			*tptr=*sptr;
		}
		else
		{
			rc=*(sptr+1);
			found=0;
			for(i=0; i<sizeof(sc) && !found; i++)
			{
				if(rc==sc[i])
				{
					rc=tc[i];
					found=1;
				}
			}
			if(found)
			{
				*tptr=rc;
				++sptr;
			}
			else
			{
				*tptr=*sptr;
			}
		}
		++sptr;
		++tptr;
	}
	*tptr=0;
	return strlen(msg);
}

void ShowUsage(void)
{
	printf("\ninput Version %.2f Syntax:\n", I_VERSION);
	printf("    input l=\"layout\" [Options]\n\n");
	printf("    layout                : format-string\n");
	printf("                            #=numeric @=alphanumeric\n");
	printf("Options:\n");
	printf("    t=\"Window-Title\"      : specify title of window [default \"Eingabe\"]\n");
	printf("    d=\"Defaults\"          : default values\n");
	printf("    k=1/0                 : show the keyboard layout [default 0]\n");
	printf("    f=1/0                 : show frame around edit fields [default 1]\n");
	printf("    m=1/0                 : mask numeric inputs (for PIN entrys) [default 0]\n");
	printf("    h=1/0                 : return on help key (for PIN changes) [default 0]\n");
	printf("    c=n                   : colums per line, n=1..25 [default 25]\n");
	printf("    o=n                   : menu timeout (0=no timeout) [default 0]\n");

}
/******************************************************************************
 * input Main
 ******************************************************************************/

int main (int argc, char **argv)
{
int tv,cols=25,debounce=25,tmo=0;
char ttl[]="Eingabe";
int dloop=1,keys=0,frame=1,mask=0,bhelp=0;
char *title=NULL, *format=NULL, *defstr=NULL, *aptr, *rptr; 
unsigned int alpha;

		if(argc==1)
		{
			ShowUsage();
			return 0;
		}

		dloop=0;
		for(tv=1; !dloop && tv<argc; tv++)
		{
			aptr=argv[tv];
			if((rptr=strchr(aptr,'='))!=NULL)
			{
				rptr++;
				if(strstr(aptr,"l=")!=NULL)
				{
					format=rptr;
					dloop=Transform_Msg(format)==0;
				}
				else
				{
					if(strstr(aptr,"t=")!=NULL)
					{
						title=rptr;
						dloop=Transform_Msg(title)==0;
					}
					else
					{
						if(strstr(aptr,"d=")!=NULL)
						{
							defstr=rptr;
							dloop=Transform_Msg(defstr)==0;
						}
						else
						{
							if(strstr(aptr,"m=")!=NULL)
							{
								if(sscanf(rptr,"%d",&mask)!=1)
								{
									dloop=1;
								}
							}
							else
							{
								if(strstr(aptr,"f=")!=NULL)
								{
									if(sscanf(rptr,"%d",&frame)!=1)
									{
										dloop=1;
									}
								}
								else
								{
									if(strstr(aptr,"k=")!=NULL)
									{
										if(sscanf(rptr,"%d",&keys)!=1)
										{
											dloop=1;
										}
									}
									else
									{
										if(strstr(aptr,"h=")!=NULL)
										{
											if(sscanf(rptr,"%d",&bhelp)!=1)
											{
												dloop=1;
											}
										}
										else
										{
											if(strstr(aptr,"c=")!=NULL)
											{
												if(sscanf(rptr,"%d",&cols)!=1)
												{
													dloop=1;
												}
											}
											else
											{
												if(strstr(aptr,"o=")!=NULL)
												{
													if(sscanf(rptr,"%d",&tmo)!=1)
													{
														dloop=1;
													}
												}
												else
												{
													dloop=2;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			switch (dloop)
			{
				case 1:
					printf("input <param error: %s>\n",aptr);
					return 0;
					break;
				
				case 2:
					printf("input <unknown command: %s>\n\n",aptr);
					ShowUsage();
					return 0;
					break;
			}
		}
		if(!format)
		{
			printf("input <missing format string>\n");
			return 0;
    	}
		if(!title)
		{
			title=ttl;
		}

		if((buffer=calloc(BUFSIZE+1, sizeof(char)))==NULL)
		{
			printf(NOMEM);
			return 0;
		}

		if(((sx=Read_Neutrino_Cfg("screen_StartX"))<0)&&((sx=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/left"))<0))
			sx=80;
		
		if(((ex=Read_Neutrino_Cfg("screen_EndX"))<0)&&((ex=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/right"))<0))
			ex=620;

		if(((sy=Read_Neutrino_Cfg("screen_StartY"))<0)&&((sy=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/top"))<0))
			sy=80;

		if(((ey=Read_Neutrino_Cfg("screen_EndY"))<0)&&((ey=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/bottom"))<0))
			ey=505;

		if(Read_Neutrino_Cfg("rounded_corners")>0)
			radius=9;
		else
			radius=0;

		fb = open(FB_DEVICE, O_RDWR);
#ifdef HAVE_DBOX_HARDWARE
		ioctl(fb, AVIA_GT_GV_GET_BLEV, &alpha);
#endif
		InitRC();

	//init framebuffer

		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("input <FBIOGET_FSCREENINFO failed>\n");
			return 0;
		}
		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("input <FBIOGET_VSCREENINFO failed>\n");
			return 0;
		}
		
		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("input <mapping of Framebuffer failed>\n");
			return 0;
		}
		
	//init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("input <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
			munmap(lfb, fix_screeninfo.smem_len);
			return 0;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("input <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return 0;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("input <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return 0;
		}

		if((error = FTC_Manager_LookupFace(manager, FONT, &face)))
		{
			printf("input <FTC_Manager_LookupFace failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return 0;
		}

		use_kerning = FT_HAS_KERNING(face);
		desc.face_id = FONT;
		desc.flags = FT_LOAD_MONOCHROME;
	//init backbuffer

		if(!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("input <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return 0;
		}

		if(!(obb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("input <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			free(lbb);
			munmap(lfb, fix_screeninfo.smem_len);
			return 0;
		}

		memcpy(lbb, lfb, var_screeninfo.xres*var_screeninfo.yres);
		memcpy(obb, lfb, var_screeninfo.xres*var_screeninfo.yres);

		startx = sx /*+ (((ex-sx) - 620)/2)*/;
		starty = sy /* + (((ey-sy) - 505)/2)*/;



	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);
	signal(SIGQUIT, quit_signal);

	//main loop
	put_instance(instance=get_instance()+1);
	printf("%s\n",inputd(format, title, defstr, keys, frame, mask, bhelp, cols, tmo, debounce));
	put_instance(get_instance()-1);

	memcpy(lfb, obb, var_screeninfo.xres*var_screeninfo.yres);

	free(buffer);

	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	free(lbb);
	free(obb);
#ifdef HAVE_DBOX_HARDWARE
	ioctl(fb, AVIA_GT_GV_SET_BLEV, alpha);
#endif
	munmap(lfb, fix_screeninfo.smem_len);

	close(fb);
	CloseRC();

	return 1;
}

