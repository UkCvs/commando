/*
 * $Id: logoset.c,v 1.8 2012/08/29 18:11:26 rhabarber1848 Exp $
 *
 * logomask - d-box2 linux project
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
#include <time.h>
#include "logomask.h"
#include "io.h"
#include "gfx.h"
#include "color.h"
#include "text.h"

extern int FSIZE_BIG;
extern int FSIZE_MED;
extern int FSIZE_SMALL;

#define NCF_FILE "/var/tuxbox/config/neutrino.conf"
#define CFG_FILE "/var/tuxbox/config/logomask.conf"
unsigned char FONT[64]= "/share/fonts/pakenham.ttf";
//#define CFG_FILE "/tmp/logomask.conf"


#define CL_VERSION  "0.30"
#define MAX_MASK 16


unsigned char *lfb = 0, *lbb = 0;
char tstr[4096];


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

int Read_Neutrino_Cfg(char *entry)
{
FILE *nfh;
char tstr [512], *cfptr=NULL;
int rv=-1;

	if((nfh=fopen(NCF_FILE,"r"))!=NULL)
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
//			printf("%s\n%s=%s -> %d\n",tstr,entry,cfptr,rv);
		}
		fclose(nfh);
	}
	return rv;
}

/******************************************************************************
 * logomask Main
 ******************************************************************************/

int main (int argc, char **argv)
{
	int i,j,m,found=0,acmc=TRANSP,cmc=BLACK,mask=1,kmode=1,pmode=0, lc=-1, changed=0, todo=1, help=1, help_changed=0;
	unsigned char actchan[20]=""/*,channel[128]=""*/;
	FILE *fh,*fh2;
	char *cpt1,*cpt2,mc[MAX_MASK];;
	int tsx=startx+210, tsy, tdy=20, tsz=26, tcol=COL_MENUHEAD_TEXT;
	int xp[MAX_MASK][8],yp[MAX_MASK][8],xw[MAX_MASK][8],yw[MAX_MASK][8],valid[MAX_MASK],xxp,xxw,yyp,yyw,nmsk=0,amsk=0;
	double xs=1.0, ys=1.0;
	time_t t1,t2;

		for(j=0; j<MAX_MASK; j++)
		{
			valid[j]=0;
			for(i=0; i<8; i++)
			{
				xp[j][i]=340;
				xw[j][i]=40;
				yp[j][i]=250;
				yw[j][i]=20;
			}	
		}
		system("saa -w > /tmp/logomaskset.stat");
		if((fh=fopen("/tmp/logomaskset.stat","r"))!=NULL)
		{
			if(fgets(tstr,500,fh))
			{
				TrimString(tstr);
				if(strlen(tstr))
				{
					if(sscanf(tstr+strlen(tstr)-1,"%d",&pmode)!=1)
					{
						pmode=0;
					}
				}
			}
			fclose(fh);
		}
	
		system("touch /tmp/.logomask_kill");

		fb = open(FB_DEVICE, O_RDWR);

		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("logomask <FBIOGET_FSCREENINFO failed>\n");
			return -1;
		}
		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("logomask <FBIOGET_VSCREENINFO failed>\n");
			return -1;
		}
		
		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("logomask <mapping of Framebuffer failed>\n");
			return -1;
		}

	//init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("logomask <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("logomask <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("logomask <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_LookupFace(manager, FONT, &face)))
		{
			printf("logomask <FTC_Manager_LookupFace failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		use_kerning = FT_HAS_KERNING(face);

		desc.face_id = FONT;
		desc.flags = FT_LOAD_MONOCHROME;

		InitRC();

	//init backbuffer

		if(!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("logomask <allocating of Backbuffer failed>\n");
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);

		startx = sx;
		starty = sy;

		system("wget -q -Y off -O /tmp/logomask.chan http://localhost/control/zapto");
		if((fh=fopen("/tmp/logomask.chan","r"))!=NULL)
		{
			if(fgets(tstr, 4095, fh))
			{
				TrimString(tstr);
			}
			fclose(fh);
			if(strlen(tstr))
			{
				strcpy(actchan,tstr);
			}

			if((fh=fopen(CFG_FILE,"r"))!=NULL)
			{
				found=0;
				while(fgets(tstr, 4095, fh) && !found)
				{
					TrimString(tstr);
					if(strlen(tstr))
					{
						if(strstr(tstr,actchan)!=NULL)
						{
							mask=1;
							nmsk=0;
							cpt2=strstr(tstr,",MC");
							if((cpt1=strchr(tstr,','))!=NULL)
							{
								while(cpt1)
								{
									valid[nmsk]=0;
									if(cpt2 && sscanf(cpt2+1,"MC%d",&m)==1)
									{
										cmc=m;
										cpt2=strchr(cpt2+1,',');
									}
									else
									{
										cmc=BLACK;
									}
									for(i=0; i<8 && cpt1; i++)
									{
										cpt1++;
										if(sscanf(cpt1,"%d,%d,%d,%d",&xxp,&xxw,&yyp,&yyw)==4)
										{
											xp[nmsk][i]=xxp;
											xw[nmsk][i]=xxw;
											yp[nmsk][i]=yyp;
											yw[nmsk][i]=yyw;
											mc[nmsk]=cmc;
											found=1;
											valid[nmsk]=1;
										}
										for(j=0; j<4 && cpt1; j++)
										{
											cpt1=strchr(cpt1+1,',');
										}
									}
									if(valid[nmsk])
									{
										nmsk++;
									}
								}
							}
						}
					}
				}
				fclose(fh);
			}
		}

		if(!nmsk)
		{
			nmsk=1;
			valid[0]=1;
		}
		mask=nmsk;
		for(m=0; m<MAX_MASK; m++)
		{
			if(valid[m])
			{
				xxp=xp[m][pmode];
				xxw=xw[m][pmode];				
				yyp=yp[m][pmode];
				yyw=yw[m][pmode];
				cmc=mc[m];
				RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, cmc);
				if(m==amsk)
					RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, GRID, LBLUE);
				for(i=0;i<=yyw;i++)
				{
					j=(yyp+i)*var_screeninfo.xres+xxp;
					if((j+xxw)<var_screeninfo.xres*var_screeninfo.yres)
					{
						memcpy(lfb+j, lbb+j, xxw);
					}
				}
			}
		}
		time(&t1);
		while((rc!=RC_HOME) && (rc!=RC_OK))
		{
			rc=GetRCCode();
			if((rc!=-1) && (rc!=RC_HOME) && (rc!=RC_OK))
			{
				time(&t1);
				acmc=TRANSP;
				xxp=xp[amsk][pmode];
				xxw=xw[amsk][pmode];
				yyp=yp[amsk][pmode];
				yyw=yw[amsk][pmode];
				cmc=mc[amsk];
				switch(rc)
				{
					case RC_LEFT:
					if(lc==RC_LEFT)
					{
						xs+=0.3;
					}
					else
					{
						xs=1.0;
					}
					if(kmode)
					{
						if(xxp>0)
						{
							changed=1;
							xxp-=xs;
						}
					}
					else
					{
						if(xxw>6)
						{
							changed=1;
							xxw-=xs;
						}
					}
					break;
				
					case RC_RIGHT:
					if((xxp+xxw)<(var_screeninfo.xres-1))
					{
						changed=1;
						if(lc==RC_RIGHT)
						{
							xs+=0.3;
						}
						else
						{
							xs=1.0;
						}
						if(kmode)
						{
							xxp+=xs;
						}
						else
						{
							xxw+=xs;
						}
					}
					break;
				
					case RC_UP:
					if(lc==RC_UP)
					{
						ys+=0.2;
					}
					else
					{
						ys=1.0;
					}
					if(kmode)
					{
						if(yyp>0)
						{
							changed=1;
							yyp-=ys;
						}
					}
					else
					{
						if(yyw>6)
						{
							changed=1;
							yyw-=ys;
						}
					}
					break;
				
					case RC_DOWN:
					if((yyp+yyw)<(var_screeninfo.yres-1))
					{
						changed=1;
						if(lc==RC_DOWN)
						{
							ys+=0.2;
						}
						else
						{
							ys=1.0;
						}
						if(kmode)
						{
							yyp+=ys;
						}
						else
						{
							yyw+=ys;
						}
					}
					break;
				
					case RC_RED:
						changed=1;
						RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, TRANSP);
						for(i=0;i<=yyw;i++)
						{
							j=(yyp+i)*var_screeninfo.xres+xxp;
							if((j+xxw)<var_screeninfo.xres*var_screeninfo.yres)
							{
								memcpy(lfb+j, lbb+j, xxw);
							}
						}
						valid[amsk]=0;
						nmsk--;
						kmode=1;
						if(nmsk)
						{
							todo=2;
							amsk=-1;
							for(m=0; m<MAX_MASK && amsk<0; m++)
							{
								if(valid[m])
								{
									amsk=m;
									xxp=xp[amsk][pmode];
									xxw=xw[amsk][pmode];
									yyp=yp[amsk][pmode];
									yyw=yw[amsk][pmode];
									cmc=mc[amsk];
								}
							}	
						}
						else
						{
							todo=mask=0;
						}
					break;
				
					case RC_GREEN:
						if(nmsk<MAX_MASK)
						{
							todo=2;
							changed=1;
							kmode=1;
							amsk=-1;
							for(m=0; amsk<0 && m<MAX_MASK; m++)
							{
								if(!valid[m])
								{
									amsk=m;
									valid[amsk]=1;
									nmsk++;
									for(i=0; i<8; i++)
									{
										xp[amsk][i]=340;
										xw[amsk][i]=40;
										yp[amsk][i]=250;
										yw[amsk][i]=20;
										mc[amsk]=BLACK;
									}
									xxp=xp[amsk][pmode];
									xxw=xw[amsk][pmode];
									yyp=yp[amsk][pmode];
									yyw=yw[amsk][pmode];
									cmc=mc[amsk];
								}
							}
						}	
					break;
					
					case RC_PLUS:
						if(nmsk>1)
						{
							m=amsk+1;
							if(m>=MAX_MASK)
							{
								m=0;
							}
							while(!valid[m])
							{
								if(++m>=MAX_MASK)
								{
									m=0;
								}
							}
							RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, cmc);
							amsk=m;
							xxp=xp[amsk][pmode];
							xxw=xw[amsk][pmode];
							yyp=yp[amsk][pmode];
							yyw=yw[amsk][pmode];
							cmc=mc[amsk];
						}
					break;
				
					case RC_MINUS:
						if(nmsk>1)
						{
							m=amsk-1;
							if(m<0)
							{
								m=MAX_MASK-1;
							}
							while(!valid[m])
							{
								if(--m<0)
								{
									m=MAX_MASK;
								}
							}
							RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, cmc);
							amsk=m;
							xxp=xp[amsk][pmode];
							xxw=xw[amsk][pmode];
							yyp=yp[amsk][pmode];
							yyw=yw[amsk][pmode];
							cmc=mc[amsk];
						}
					break;

					case RC_YELLOW:
						kmode=0;
					break;
				
					case RC_BLUE:
						kmode=1;
					break;
					
					case RC_MUTE:
						if(nmsk)
						{
							if(++cmc>15)
								cmc=1;
							acmc=cmc;
							changed=1;
						}
					break;

					case RC_HELP:
						help_changed=1;
					break;
				}
				lc=rc;
				if(mask || todo==2)
				{
					RenderBox(xp[amsk][pmode], yp[amsk][pmode], xp[amsk][pmode]+xw[amsk][pmode], yp[amsk][pmode]+yw[amsk][pmode], FILL, acmc);
					for(i=0;i<=yw[amsk][pmode];i++)
					{
						j=(yp[amsk][pmode]+i)*var_screeninfo.xres+xp[amsk][pmode];
						if((j+xw[amsk][pmode])<var_screeninfo.xres*var_screeninfo.yres)
						{
							memcpy(lfb+j, lbb+j, xw[amsk][pmode]+1);
						}
					}
					xp[amsk][pmode]=xxp;
					xw[amsk][pmode]=xxw;
					yp[amsk][pmode]=yyp;
					yw[amsk][pmode]=yyw;
					mc[amsk]=cmc;
					for(m=0; mask && m<MAX_MASK; m++)
					{
						if(valid[m])
						{
							xxp=xp[m][pmode];
							xxw=xw[m][pmode];
							yyp=yp[m][pmode];
							yyw=yw[m][pmode];
							cmc=mc[m];
							RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, (m==amsk)?GRID:FILL, (m==amsk)?((kmode)?LBLUE:COL_MENUHEAD_TEXT):cmc);
							for(i=0;i<=yyw;i++)
							{
								j=(yyp+i)*var_screeninfo.xres+xxp;
								if((j+xxw)<var_screeninfo.xres*var_screeninfo.yres)
								{
									memcpy(lfb+j, lbb+j, xxw+1);
								}
							}
						}
					}
				}
			}
			time(&t2);
			if((t2-t1)>1)
			{
				xs=1.0;
				ys=1.0;
				if(help_changed)
				{
					help^=1;
				}
				if(help)
				{
					tsy=startx+200;
//					memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
					RenderBox(200,200,530,410,FILL,TRANSP);
					if(nmsk)
						RenderBox(xp[amsk][pmode], yp[amsk][pmode], xp[amsk][pmode]+xw[amsk][pmode], yp[amsk][pmode]+yw[amsk][pmode], GRID, (kmode)?LBLUE:COL_MENUHEAD_TEXT);
					RenderString("?         :  Hilfetext ein/ausschalten", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
					RenderString("Blau   :  Umschalten auf Positionseinstellung", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
					RenderString("Gelb  :  Umschalten auf Größeneinstellung", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
					RenderString("Grün  :  Maske hinzufügen", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
					RenderString("Rot    :  Maske löschen", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
					RenderString("Vol+   :  nächste Maske auswählen", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
					RenderString("Vol-   :  vorherige Maske auswählen", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
					RenderString("Mute  :  Maskenfarbe wählen", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
					RenderString("Home:  Abbrechen", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
					RenderString("OK     :  Speichern und Beenden", tsx, tsy+=tdy, 400, LEFT, tsz, tcol);
				}
				else
				{
					if(help_changed)
					{
//						memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
						RenderBox(200,200,530,410,FILL,TRANSP);
						if(nmsk)
							RenderBox(xp[amsk][pmode], yp[amsk][pmode], xp[amsk][pmode]+xw[amsk][pmode], yp[amsk][pmode]+yw[amsk][pmode], GRID, (kmode)?LBLUE:COL_MENUHEAD_TEXT);
					}
				}
				help_changed=0;
				memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
			}
		}
		if(rc==RC_HOME)
		{
			changed=0;
			todo=0;
		}
		if(rc==RC_OK && changed)
		{
			if((fh2=fopen("/tmp/logomask.conf","w"))!=NULL)
			{
				fh=fopen(CFG_FILE,"r");
				while(fh && fgets(tstr, 4095, fh))
				{
					TrimString(tstr);
					if(strlen(tstr))
					{
						if(strstr(tstr,actchan)==NULL)
						{
							fprintf(fh2,"%s\n",tstr);
						}
					}
				}
				if(fh)
				{
					fclose(fh);
				}
				if(todo)
				{
					fprintf(fh2,"%s",actchan);
					for(j=0; j<MAX_MASK; j++)
					{
						if(valid[j])
						{
							for(i=0; i<8; i++)
							{
								fprintf(fh2,",%d,%d,%d,%d",xp[j][i],xw[j][i],yp[j][i],yw[j][i]);
							}
						}
					}
					for(j=0; j<MAX_MASK; j++)
					{
						if(valid[j])
						{
							fprintf(fh2,",MC%d",mc[j]);
						}
					}
					fprintf(fh2,",\n");
				}
				fclose(fh2);
				remove(CFG_FILE);
				system("mv /tmp/logomask.conf /var/tuxbox/config/logomask.conf");
			}
		}		
		free(lbb);
		munmap(lfb, fix_screeninfo.smem_len);

		close(fb);
		CloseRC();
		remove("/tmp/.logomask_kill");
		remove("/tmp/logomaskset.*");
		system("/bin/logomask &");
		return 0;
}
