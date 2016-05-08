/*
 * $Id: globals.c,v 1.1 2010/03/03 20:47:03 rhabarber1848 Exp $
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
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
//#include <linux/delay.h>
//#include <asm/delay.h>
#include "globals.h"
#include "http.h"

static char channels[NUM_CHANNELS][10]={"DSF","VOX","RTL","Pro7","ARD","9Live","Super RTL","Sat.1","EuroSport","NTV","Kabel 1","RTL II"};

int rezap=180, inet=-1, zapalways=0, volume=0, mute=0, debounce=15;
int socket_nummer;
int laenge;
int ergebnis;
int anzahl;
int i,nf=0;
struct sockaddr_in adresse;
char empfangene_zeichen[550];
fd_set readset, writeset;
struct timeval to;
int timo=100000;
unsigned short int portnummer = 5450;
char ip_adresse[] = "217.160.185.104";
int cnum=-1;
int wtime[10]={1,2,3,4,5,6,7,8,9,10};

int Get_ChannelNumber(char *chan)
{
int rv=-1, i;

	for(i=0; rv==-1 && i<NUM_CHANNELS; i++)
	{
		if(strcmp(chan,channels[i])==0)
		{
			rv=i;
		}
	}
	return rv;
}

char *Get_ChannelName(int chan)
{
	if((chan>=0) && (chan <NUM_CHANNELS))
	{
		return channels[chan];
	}

	return "";
}

int Open_Socket(void)
{
 	socket_nummer = socket(AF_INET, SOCK_STREAM, 0);
 
 	adresse.sin_family = AF_INET;
 	adresse.sin_addr.s_addr = inet_addr(ip_adresse);
 	adresse.sin_port = htons(portnummer);
 	laenge = sizeof(adresse);

 	to.tv_sec = timo / 100000;
 	to.tv_usec = ( timo - ( to.tv_sec * 100000 ) ) * 10;

 
 	return connect(socket_nummer, (struct sockaddr *)&adresse, laenge);
}

void Close_Socket(void)
{
 	close(socket_nummer);  
}

int Check_Socket(int channel, int *state)
{
//int i;

	FD_ZERO( &readset );
  	FD_ZERO( &writeset );
	FD_SET( socket_nummer, &readset );

	if(( nf = select( socket_nummer + 1, &readset, &writeset, NULL, &to )) <= 0 )
  	{
		return nf;
  	}

   	anzahl = read(socket_nummer, empfangene_zeichen,sizeof(empfangene_zeichen));

   	empfangene_zeichen[anzahl]= '\0';
    
   	if(anzahl==512)
   	{
/*   		for(i=0; i<NUM_CHANNELS; i++)
   		{
   			printf("%12s %d\n",channels[i],((empfangene_zeichen[i/8] & (1<< (i%8)))==0));
   		}
   		printf("\n");
*/   		
		*state=((empfangene_zeichen[channel/8] & (1<< (channel%8)))==0);
		return 1;
	}
	else
	{
		Close_Socket();
		return Open_Socket();
	}
	
	return -1;
}	
 
int Check_Channel(int channel, int *state)
{
int i=*state;

	*state=((empfangene_zeichen[channel/8] & (1<< (channel%8)))==0);

	return (i!=*state);
}	

void Trim_String(char *buffer)
{
char *cptr;

	cptr=buffer+strlen(buffer)-1;
	while((cptr>=buffer) && (*cptr<' '))
	{
		*cptr=0;
		--cptr;
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
		}
		fclose(nfh);
	}
	return rv;
}

int ReadConf()
{
	int i,j;
	FILE *fd_conf;
	char line_buffer[512],tstr[20];
	char *cptr;

	//open config
	if(!(fd_conf = fopen(CFG_FILE, "r")))
	{
		printf("Blockads <unable to open Config-File>\n");
		return 0;
	}

	while(fgets(line_buffer, sizeof(line_buffer), fd_conf))
	{
		Trim_String(line_buffer);
		if((line_buffer[0]) && (line_buffer[0]!='#') && (line_buffer[0]>' ') && ((cptr=strchr(line_buffer,'='))!=NULL))
		{
			for(i=0; i<10; i++)
			{
				sprintf(tstr,"Time%1d=",(i<9)?(i+1):0);
				if(strstr(line_buffer,tstr) != NULL)
				{
					if(sscanf(cptr+1,"%d",&j)==1)
					{
						if(j>0 && j<100)
						{
							wtime[i]=j;
						}
					}
				}
			}
			if(strstr(line_buffer,"RezapTime") != NULL)
			{
				sscanf(cptr+1,"%d",&rezap);
			}
			if(strstr(line_buffer,"ZapAlways") != NULL)
			{
				sscanf(cptr+1,"%d",&zapalways);
			}
			if(strstr(line_buffer,"KeepVolume") != NULL)
			{
				sscanf(cptr+1,"%d",&volume);
			}
			if(strstr(line_buffer,"Debounce") != NULL)
			{
				sscanf(cptr+1,"%d",&debounce);
			}
			if(strstr(line_buffer,"Internet") != NULL)
			{
				if(strstr(cptr+1,"DSL")!=NULL)
				{
					inet=0;
				}
				if(strstr(cptr+1,"ISDN")!=NULL)
				{
					inet=1;
				}
				if(strstr(cptr+1,"ANALOG")!=NULL)
				{
					inet=2;
				}
			}
		}
	}
	return 1;
}

void Msg_Popup(char *msg)
{
char mstr[512]="/control/message?popup=", *mptr=msg, *pptr;

	while(*mptr)
	{
		pptr=mstr+strlen(mstr);
		switch (*mptr)
		{
			case ' ': sprintf(pptr,"%%20"); break;
			case 10 : sprintf(pptr,"%%0A"); break;
			case 13 : break;
			case 'Ä': sprintf(pptr,"%%C3%%84"); break;
			case 'Ö': sprintf(pptr,"%%C3%%96"); break;
			case 'Ü': sprintf(pptr,"%%C3%%9C"); break;
			case 'ä': sprintf(pptr,"%%C3%%A4"); break;
			case 'ö': sprintf(pptr,"%%C3%%B6"); break;
			case 'ü': sprintf(pptr,"%%C3%%BC"); break;
			case 'ß': sprintf(pptr,"%%C3%%9F"); break;
			default : sprintf(pptr,"%c",*mptr); break;
		}
		++mptr;
	}
	HTTP_downloadFile("127.0.0.1",80, mstr, MSG_FILE, 0, 1);
}


int Translate_Channel(char *source, char *target)
{
int rv=-1,found=0;
FILE *tfh;
char trstr[512],*pt1=NULL,*pt2=NULL;

	*target=0;
	if((tfh=fopen(CFG_FILE,"r"))!=NULL)
	{	
		while(!found && (fgets(trstr, sizeof(trstr), tfh)))
		{
			Trim_String(trstr);
			if((strstr(trstr,"Programm=")==trstr) && ((pt1=strchr(trstr,'='))!=NULL) && ((pt2=strchr(pt1+1,','))!=NULL))
			{
				pt1++;
				*pt2=0;
				pt2++;
				if(strcmp(source,pt2)==0)
				{
					strcpy(target,pt1);
					found=1;
					rv=0;
				}
			}
		}
		fclose(tfh);
	}
	return rv;
}

void Do_Zap(char *chan)
{
char zstr[50];

	sprintf(zstr,"pzapit -n \"%s\"",chan);
	system(zstr);
}

void Do_Rezap(char *chan, int vol, int mut)
{
char zstr[150];
FILE *fh1;
int avol, amut;

	if(!HTTP_downloadFile("localhost",80,"/control/volume?status", ZAP_FILE, 0, 1))
	{
		if((fh1=fopen(ZAP_FILE,"r"))!=NULL)
		{
			while((fgets(zstr, sizeof(zstr), fh1)>0) && !strlen(zstr));
			if(strlen(zstr)>=1)
			{
				Trim_String(zstr);
				sscanf(zstr,"%d",&amut);
			}
			fclose(fh1);
		}
	}
	if(!HTTP_downloadFile("localhost",80,"/control/volume", ZAP_FILE, 0, 1))
	{
		if((fh1=fopen(ZAP_FILE,"r"))!=NULL)
		{
			while((fgets(zstr, sizeof(zstr), fh1)>0) && !strlen(zstr));
			if(strlen(zstr)>=1)
			{
				Trim_String(zstr);
				sscanf(zstr,"%d",&avol);
			}
			fclose(fh1);
		}
	}
	sprintf(zstr,"/control/zapto?%s",chan);
	HTTP_downloadFile("localhost", 80, zstr, MSG_FILE, 0, 1);
	if(volume)
	{
/*		sprintf(zstr,"pzapit -%s",(mut)?"mute":"unmute");
		system(zstr);
		sprintf(zstr,"pzapit -vol %d",vol);
		system(zstr);
*/
		if(mut!=amut)
		{
			sprintf(zstr,"/control/volume?%s",(mut)?"mute":"unmute");
			HTTP_downloadFile("localhost",80,zstr, "/dev/null/", 0, 1);
		}
		if(vol!=avol)
		{
			sprintf(zstr,"/control/volume?%d",vol);
			HTTP_downloadFile("localhost",80,zstr, "/dev/null/", 0, 1);
		}
	}
	remove(FLG_FILE);
}
