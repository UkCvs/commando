/*
 * $Id: logomask.c,v 1.2 2012/08/29 18:26:31 rhabarber1848 Exp $
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
#include "gfx.h"
#include "color.h"

extern int FSIZE_BIG;
extern int FSIZE_MED;
extern int FSIZE_SMALL;

#define NCF_FILE "/var/tuxbox/config/neutrino.conf"
#define CFG_FILE "/var/tuxbox/config/logomask.conf"

#define CL_VERSION  "0.31"
#define MAX_MASK 16

unsigned char *lfb = 0, *lbb = 0;
char tstr[4096];
int xpos=0,ypos=0,sdat=0,big=0,secs=1;

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
	int i,j,m,cmc=BLACK,transp=TRANSP,found,loop=1,mask=0,test=0,pmode=0,lmode=0,mchanged=1,cchanged=2,mwait;
	unsigned char lastchan[20]="", actchan[20]=""/*,channel[128]=""*/;
	int xp[MAX_MASK][8],yp[MAX_MASK][8],xw[MAX_MASK][8],yw[MAX_MASK][8],valid[MAX_MASK],xxp,xxw,yyp,yyw,nmsk=0;
	FILE *fh;
	char *cpt1,*cpt2,mc[MAX_MASK];
	
		if(argc==2 && strstr(argv[1],"test")!=NULL)
		{
			test=1;
		}
		printf("logomask Version %s\n",CL_VERSION);
		if((mwait=Read_Neutrino_Cfg("timing.infobar"))<0)
			mwait=6;
		
		mwait-=1;	

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

	//init backbuffer

		if(!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("logomask <allocating of Backbuffer failed>\n");
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);
//		memset(mc, BLACK, sizeof(mc));
		startx = sx;
		starty = sy;


	// if problem with config file return from plugin

		while(loop)
		{
			sleep(1);
			system("saa -w > /tmp/logomaskset.stat");
			if((fh=fopen("/tmp/logomaskset.stat","r"))!=NULL)
			{
				if(fgets(tstr,500,fh))
				{
					TrimString(tstr);
					if(strlen(tstr))
					{
						lmode=pmode;
						if(sscanf(tstr+strlen(tstr)-1,"%d",&i)!=1)
						{
							pmode=0;
						}
						else
						{
							mchanged=(pmode!=i);
							pmode=i;
						}
					}
				}
				fclose(fh);
			}
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
					cchanged=(cchanged==2)?3:((strcmp(actchan,lastchan)?1:0));
					if(mchanged || cchanged)
					{
						found=0;
						if(cchanged)
						{
							if(cchanged==1)
							{
								sleep(mwait);
							}
							cchanged=1;
						}
						if(mask)
						{
							for(m=0; m<nmsk; m++)
							{
								if(valid[m])
								{
									xxp=xp[m][lmode];
									xxw=xw[m][lmode];				
									yyp=yp[m][lmode];
									yyw=yw[m][lmode];
									cmc=mc[m];
									RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, transp);
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
						}
						mask=0;
						
					if((fh=fopen(CFG_FILE,"r"))!=NULL)
					{
						strcpy(lastchan,actchan);
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
			}
			if(mask)
			{
				for(m=0; m<nmsk; m++)
				{
					if(valid[m])
					{
						xxp=xp[m][pmode];
						xxw=xw[m][pmode];				
						yyp=yp[m][pmode];
						yyw=yw[m][pmode];
						cmc=mc[m];
						RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, (test)?GRID:FILL, cmc);
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
			}
			if(++loop>5)
			{
				if(access("/tmp/.logomask_kill",0)!=-1)
				{
					loop=0;
				}
			}	
		}
	}

	for(m=0; m<nmsk; m++)
	{
		if(valid[m])
		{
			xxp=xp[m][pmode];
			xxw=xw[m][pmode];				
			yyp=yp[m][pmode];
			yyw=yw[m][pmode];
			cmc=mc[m];
			RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, transp);
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

	free(lbb);
	munmap(lfb, fix_screeninfo.smem_len);
	close(fb);
	remove("/tmp/.logomask_kill");
	remove("/tmp/logomask.*");
	return 0;
}
