/*
 * $Id: http.c,v 1.1 2010/03/03 20:47:03 rhabarber1848 Exp $
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
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
//#include <linux/delay.h>
#include "blockads.h"

int HTTP_downloadFile(char *host, int port, char *page, char *downloadTarget, int tmo, int repeats)
{
	char tstr[256];
	
	sprintf(tstr,"wget -Y off -q -O %s http://%s:%d%s",downloadTarget,host,port,page);
	system(tstr);
	return 0;
}

/*
int HTTP_downloadFile(char *host, int port, char *page, char *downloadTarget, int tmo, int repeats)
{
 int socket_nummer;
 int laenge;
 int ergebnis,nf;
 int anzahl=0;
 struct hostent  *hp;
 fd_set readset, writeset;
 struct sockaddr_in adresse;
 char buf[65535],*wptr=NULL,*tptr=NULL;
 struct timeval to;
 int timo=100000;
 FILE *fh;

 to.tv_sec = timo / 100000;
 to.tv_usec = ( timo - ( to.tv_sec * 100000 ) ) * 10;

 if(( hp = gethostbyname( host )) != NULL )
 {
    memcpy( &adresse.sin_addr.s_addr, hp->h_addr_list[0], sizeof(adresse.sin_addr.s_addr));
 }
 else 
 {
 	perror("<HTTP> unknown Host\n");
    return -1;
 }

  sprintf(buf,"%03d.%03d.%03d.%03d",(unsigned char)hp->h_addr_list[0][0],(unsigned char)hp->h_addr_list[0][1],(unsigned char)hp->h_addr_list[0][2],(unsigned char)hp->h_addr_list[0][3]);
  *buf=0;
  
 socket_nummer = socket(AF_INET, SOCK_STREAM, 0);
 
 adresse.sin_family = AF_INET;
 adresse.sin_port = htons(port);
 laenge = sizeof(adresse);
 
 ergebnis = connect(socket_nummer, (struct sockaddr *)&adresse, laenge);
 
 if (ergebnis == -1)
   {
    perror(" Keine Verbindung erfolgt: ");
    return -1;
   }
 else

   {

	sprintf(buf,"GET %s HTTP/1.1\r\nHost: DBOX\r\n\r\n",page);

    anzahl = write(socket_nummer, buf, strlen(buf)+1);
 
 	*buf=0;
 	laenge=0;

 	nf=anzahl=1;
	while(nf>0 && anzahl>0)
	{
		FD_ZERO( &readset );
		FD_ZERO( &writeset );
  		FD_SET( socket_nummer, &readset );

  		if(( nf = select( socket_nummer + 1, &readset, &writeset, NULL, &to )) < 0 )
		{
			printf("Socket error\n");
			return -1;
		}
		if(nf>0)
		{
	      	anzahl = read(socket_nummer, buf+laenge, 65534-laenge);
    		if(anzahl>0)
    		{
    			laenge+=anzahl;
    		}
		}
    }
	anzahl=laenge;
    buf[anzahl]= '\0';
    
    if (strstr(buf,"200 OK") != NULL)
      {
      	if((fh=fopen(downloadTarget,"w"))!=NULL)
      	{
      		if((wptr=strstr(buf,"Content-Type: text/html"))!=NULL)
      		{
   				wptr=strstr(buf,"<html>");
   			}
   			else
   			{
	      		if((wptr=strstr(buf,"Content-Type: text/plain"))!=NULL)
     			{
      				if((wptr=strchr(wptr,10))!=NULL)
      				{
      					wptr++;
	      				if((wptr=strchr(wptr,10))!=NULL)
	      				{
    	  					wptr++;
    	  				}
    	  			}
    	  		}
    	  		else
    	  		{
   	  				wptr=buf;
    	  		}
    	  	}
   	  		if(wptr)
   	  		{
	      		if((tptr=strstr(buf,"Content-Length:"))!=NULL)
		      		{
		      			wptr=strchr(tptr,0x0A)+3;
		      		}
				fwrite(wptr, strlen(buf)-(wptr-buf), 1, fh);
   			}
			fclose(fh);
			if(!wptr)
			{
				remove(downloadTarget);
			}
      	 }
      }
    else
      {
       perror("\n Der Server hat die Datei nicht gesendet");
      }    
   }  
   
 close(socket_nummer);  

 return anzahl==0 || !wptr;
}
*/
