/*
  $Id: systeminfo.cpp, v2.3 2008/09/20 19:25:21 mohousch Exp $
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <gui/widget/buttons.h>

#include <global.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <neutrino.h>
#include "systeminfo.h"

#include <driver/screen_max.h>
#include <driver/fontrenderer.h>


sfileline sinbuffer[(3*MAXLINES)];
sreadline sysbuffer[(2*MAXLINES)];

int slinecount,syscount;
bool refreshIt = true;

//Konstruktor
CBESysInfoWidget::CBESysInfoWidget(int m)
{
       frameBuffer = CFrameBuffer::getInstance();
       selected = 0;
       //width = 576;
       //height = 440;
	width  = w_max (580, 40);
	height = h_max (440, 20);

       
       ButtonHeight = 25;
       theight= g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
       fheight= g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->getHeight();
       listmaxshow = (height-theight-0)/fheight;
       height = theight+listmaxshow*fheight; // recalc height
       x = getScreenStartX (width);
       y = getScreenStartY (height);
       liststart = 0;
       state = beDefault;
       mode=m;
}

//zeichnet einen Listeneintrag
void CBESysInfoWidget::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight;
	int c_rad_small;
	uint8_t    color;
	fb_pixel_t bgcolor;
		if (liststart + pos == selected)
		{ 
			color   = COL_MENUCONTENTSELECTED;
			bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
			c_rad_small = RADIUS_SMALL;
		}
		else
		{
			color   = COL_MENUCONTENT;
			bgcolor = COL_MENUCONTENT_PLUS_0;
			c_rad_small = 0;
		}
				
       if ((liststart+pos==selected)&&(mode != 1))
       {
            color = COL_MENUCONTENTSELECTED;
	    c_rad_small = RADIUS_SMALL;
       }

       frameBuffer->paintBoxRel(x,ypos, width- 15, fheight, bgcolor, c_rad_small);

           if (liststart+pos<syscount)
           {
                   char tmpline75[75];

                   memcpy(tmpline75,  &sysbuffer[liststart+pos].line[0],  75);
                   tmpline75[75]='\0';

                   g_Font[SNeutrinoSettings::FONT_TYPE_CHANNELLIST]->RenderString(x+5,  ypos+ fheight, width-30, tmpline75, color);
           }
}

//zeichent die Liste
void CBESysInfoWidget::paint()
{
	printf("[neutrino-rebuild] system info\n");

       liststart = (selected/listmaxshow)*listmaxshow;

       for(unsigned int count=0; count<listmaxshow; count++)
       {
           paintItem(count);
       }

       int ypos = y + theight;
       int sb   = fheight* listmaxshow;
       frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

       int sbc = (syscount/listmaxshow) + 1;
       sbc = (syscount/listmaxshow) + 1;
       float sbh= (sb - 4)/ sbc;
       int sbs  = (selected/listmaxshow);
       frameBuffer->paintBoxRel(x+ width- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT_PLUS_3, RADIUS_SMALL);

}

//zeichnet Kopfzeile
void CBESysInfoWidget::paintHead()
{
       char buf[100];

       frameBuffer->paintBoxRel(x,y, width,theight+0, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_TOP);
       if(mode==1)sprintf((char *) buf, "%s", "System-Info:");
       if(mode==2)sprintf((char *) buf, "%s", "System-Messages:");
       if(mode==3)sprintf((char *) buf, "%s", "CPU/Filesystem-Info:");
       if(mode==4)sprintf((char *) buf, "%s", "Memory/Process-List:");
       if(mode==5)sprintf((char *) buf, "%s", "Camd-Info:");

       g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+theight+0, width, buf, COL_MENUHEAD);
}

//zeichnet Fusszeile
void CBESysInfoWidget::paintFoot()
{
       int ButtonWidth = (width-28) / 4;
       frameBuffer->paintBoxRel(x,y+height, width,ButtonHeight, COL_MENUHEAD_PLUS_0, RADIUS_MID, CORNER_BOTTOM);
       frameBuffer->paintHLine(x, x+width,  y, COL_INFOBAR_SHADOW_PLUS_0);

       if(mode!=1)
       {
               frameBuffer->paintIcon("rot.raw",    x+width- 4* ButtonWidth - 20, y+height+4);
               g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+width- 4* ButtonWidth, y+height+24 - 2, ButtonWidth- 26, "sysinfo", COL_INFOBAR);
       }
       if(mode!=2)
       {
               frameBuffer->paintIcon("gruen.raw",  x+width- 3* ButtonWidth - 30, y+height+4);
               g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+width- 3* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, "dmesg", COL_INFOBAR);
       }
       if(mode!=3)
       {
               frameBuffer->paintIcon("gelb.raw",   x+width- 2* ButtonWidth - 30, y+height+4);
               g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+width- 2* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, "cpu/file", COL_INFOBAR);
       }
       if(mode!=4)
       {
               frameBuffer->paintIcon("blau.raw",   x+width- 1* ButtonWidth - 30, y+height+4);
               g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+width- 1* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, "mem/ps", COL_INFOBAR);
       }
       else
       {
               if(refreshIt==true)
               {
                       frameBuffer->paintIcon("blau.raw",   x+width- 1* ButtonWidth - 30, y+height+4);
                       g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+width- 1* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, "stop refresh", COL_INFOBAR);
               }
               else
               {
                       frameBuffer->paintIcon("blau.raw",   x+width- 1* ButtonWidth - 30, y+height+4);
                       g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+width- 1* ButtonWidth - 10, y+height+24 - 2, ButtonWidth- 26, "start refresh", COL_INFOBAR);
               }

       }

}

//kill sysinfo
void CBESysInfoWidget::hide()
{
       frameBuffer->paintBackgroundBoxRel(x,y, width,height+ButtonHeight);
}

//main
int CBESysInfoWidget::exec(CMenuTarget* parent, const std::string & actionKey)
{

       int res = menu_return::RETURN_REPAINT;


       if(mode==1)
           sysinfo();
       else if(mode==2)
           dmesg();
       else if(mode==3)
           cpuinfo();
       else if(mode==4)
           top();
       else if(mode==5)
           caminfo();
       else
       {
           //ShowHint("Alert", "Error", "info.raw", 430);
           hide();
           return(-1);
       }

       if (parent)
       {
               parent->hide();
       }

       paintHead();
       paint();
       paintFoot();

       uint msg; uint data;
       int timercount = 0;
       unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd(5);

       while (msg != (uint) g_settings.key_channelList_cancel)
       {
               g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

               if (msg <= CRCInput::RC_MaxRC  ) timeoutEnd = g_RCInput->calcTimeoutEnd(5);
               if (msg == CRCInput::RC_timeout)
               {
                   if (mode == 1)
                   {
                       timercount = 0;
                       sysinfo();
                       selected = 0;
                       paintHead();
                       paint();
                       paintFoot();
                   }
                   if ((mode == 2)&&(++timercount>11))
                   {
                       timercount = 0;
                       dmesg();
                       paintHead();
                       paint();
                       paintFoot();
                   }
                   if ((mode == 4)&&(refreshIt==true))
                   {
                       timercount = 0;
                       top();
                       paintHead();
                       paint();
                       paintFoot();
                   }
                   if (mode == 5)
                   {
                       timercount = 0;
                       caminfo();
                       paintHead();
                       paint();
                       paintFoot();
                   }

                   timeoutEnd = g_RCInput->calcTimeoutEnd(5);
                   g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

               }
               if ((msg == CRCInput::RC_up || msg == CRCInput::RC_left)&&(mode != 1))
               {
                   int step = 0;
                   int prev_selected = selected;

                   step = (msg == CRCInput::RC_left) ? listmaxshow : 1;
                   selected -= step;
                   if((prev_selected-step) < 0) selected = syscount-1;
                   if(state==beDefault)
                   {
                       paintItem(prev_selected - liststart);
                       unsigned int oldliststart = liststart;
                       liststart = (selected/listmaxshow)*listmaxshow;
                       if(oldliststart!=liststart) paint();
                       else paintItem(selected - liststart);
                   }
               }
               else if ((msg == CRCInput::RC_down || msg == CRCInput::RC_right)&&(mode != 1))
               {
                   int step = 0;
                   int prev_selected = selected;

                   step = (msg == CRCInput::RC_right) ? listmaxshow : 1;
                   selected += step;
                   if(selected>=syscount) selected = 0;
                   if(state==beDefault)
                   {
                       paintItem(prev_selected - liststart);
                       unsigned int oldliststart = liststart;
                       liststart = (selected/listmaxshow)*listmaxshow;
                       if(oldliststart!=liststart) paint();
                       else paintItem(selected - liststart);
                   }
               }
               else if ((msg == CRCInput::RC_red)&&(mode!=1))
               {
                   mode=1;
                   sysinfo();
                   selected = 0;
                   paintHead();
                   paint();
                   paintFoot();

               }
               else if ((msg == CRCInput::RC_green)&&(mode!=2))
               {
                   mode=2;
                   timercount = 0;
                   dmesg();
                   paintHead();
                   paint();
                   paintFoot();
               }
               else if ((msg == CRCInput::RC_yellow)&&(mode!=3))
               {
                   mode=3;
                   cpuinfo();
                   paintHead();
                   paint();
                   paintFoot();
               }
               else if (msg == CRCInput::RC_blue)
               {
                   if (mode==4)
                       refreshIt=!refreshIt;
                   else
                       refreshIt=true;
                   mode=4;

                   if (refreshIt)
                       top();
                   paintHead();
                   paint();
                   paintFoot();
               }
               else
               {
                       CNeutrinoApp::getInstance()->handleMsg( msg, data );
                       // kein canceling...
               }
       }
       hide();
       return res;
}

int CBESysInfoWidget::sysinfo()
{
       static long curCPU[5]={0,0,0,0,0};
       static long prevCPU[5]={0,0,0,0,0};
       double value[5]={0,0,0,0,0};
       float faktor;
       int i, j = 0;
       char strhelp[6];
       FILE *f;
       char line[MAXLINES];
       char *fmt = " %a %d %b %Y %H:%M";
       long t;

       /* Get and Format the SystemTime */
       t = time(NULL);
       struct tm *tp;
       tp = localtime(&t);
       strftime(line, sizeof(line), fmt, tp);
       /* Get and Format the SystemTime end */

       /* Create tmpfile with date /tmp/sysinfo */
       system("echo 'DATE:' > /tmp/sysinfo");
       f=fopen("/tmp/sysinfo","a");
       if(f)
               fprintf(f,"%s\n", line);
       fclose(f);
       /* Create tmpfile with date /tmp/sysinfo end */

       /* Get the statistics from /proc/stat */
       if(prevCPU[0]==0)
       {
               f=fopen("/proc/stat","r");
               if(f)
               {
                       fgets(line,256,f); /* cpu */
                       sscanf(line,"cpu %lu %lu %lu %lu",&prevCPU[1],&prevCPU[2],&prevCPU[3],&prevCPU[4]);
                       for(i=1;i<5;i++)
                               prevCPU[0]+=prevCPU[i];
               }
               fclose(f);
               sleep(1);
       }
       else
       {
               for(i=0;i<5;i++)
                               prevCPU[i]=curCPU[i];
       }

       while(((curCPU[0]-prevCPU[0]) < 100) || (curCPU[0]==0))
       {
               f=fopen("/proc/stat","r");
               if(f)
               {
                       curCPU[0]=0;
                       fgets(line,256,f); /* cpu */
                       sscanf(line,"cpu %lu %lu %lu %lu",&curCPU[1],&curCPU[2],&curCPU[3],&curCPU[4]);
                       for(i=1;i<5;i++)
                               curCPU[0]+=curCPU[i];
               }
               fclose(f);
               if((curCPU[0]-prevCPU[0])<100)
                       sleep(1);
       }
       // some calculations
       if(!(curCPU[0] - prevCPU[0])==0)
       {
               faktor = 100.0/(curCPU[0] - prevCPU[0]);
               for(i=0;i<4;i++)
                       value[i]=(curCPU[i]-prevCPU[i])*faktor;

               value[4]=100.0-value[1]-value[2]-value[3];

               f=fopen("/tmp/sysinfo","a");
               if(f)
               {
                       memset(line,0x20,sizeof(line));
                       for(i=1, j=0;i<5;i++)
                       {
                               memset(strhelp,0,sizeof(strhelp));
                               sprintf(strhelp,"%.1f", value[i]);
                               memcpy(&line[(++j*7)-2-strlen(strhelp)], &strhelp[0], strlen(strhelp));
                               memcpy(&line[(j*7)-2], "%", 1);
                       }
                       line[(j*7)-1]='\0';
                       fprintf(f,"\nPERFORMANCE:\n USER:  NICE:   SYS:  IDLE:\n%s\n", line);
               }
               fclose(f);
       }
       /* Get the statistics from /proc/stat end*/

       /* Get kernel-info from /proc/version*/
       f=fopen("/proc/version","r");
       if(f)
       {
               char* token;
               fgets(line,256,f); // version
               token = strstr(line,") (");
               if(token != NULL)
                       *++token = 0x0;
               fclose(f);
               f=fopen("/tmp/sysinfo","a");
               fprintf(f, "\nKERNEL:\n %s\n %s\n", line, ++token);
       }
       fclose(f);
       /* Get kernel-info from /proc/version end*/

       /* Get uptime-info from /proc/uptime*/
       f=fopen("/proc/uptime","r");
       if(f)
       {
               fgets(line,256,f);
               float ret[4];
               const char* strTage[2] = {"DAYS", "Day"};
               const char* strStunden[2] = {"Hours", "Hour"};
               const char* strMinuten[2] = {"Minutes", "Minute"};
               sscanf(line,"%f",&ret[0]);
               ret[0]/=60;
               ret[1]=long(ret[0])/60/24; // Tage
               ret[2]=long(ret[0])/60-long(ret[1])*24; // Stunden
               ret[3]=long(ret[0])-long(ret[2])*60-long(ret[1])*60*24; // Minuten
               fclose(f);

               f=fopen("/tmp/sysinfo","a");
               if(f)
                       fprintf(f, "UPTIME:\n System UP Since: %.0f %s %.0f %s %.0f %s\n", ret[1], strTage[int(ret[1])==1], ret[2], strStunden[int(ret[2])==1], ret[3], strMinuten[int(ret[3])==1]);
       }
       fclose(f);

       /* Get uptime-info from /proc/uptime end*/

       return(readList(sinbuffer));
}

int CBESysInfoWidget::cpuinfo()
{
char Wert1[30];
char Wert2[10];
char Wert3[10];
char Wert4[10];
char Wert5[6];
char Wert6[30];

FILE *f,*w;
char line[256];
int i = 0;
    /* Get file-info from /proc/cpuinfo*/

    system("df > /tmp/systmp");
    f=fopen("/tmp/systmp","r");
    if(f)
    {
       w=fopen("/tmp/sysinfo","w");
       if(w)
       {
               while((fgets(line,256, f)!=NULL))
               {
                       sscanf(line,"%s %s %s %s %s %s ", &Wert1, &Wert2, &Wert3, &Wert4, &Wert5, &Wert6);
                       if(i++)
                               fprintf(w,"\nFilesystem: %s\n  1-KBlocks: %s\n  Used: %s\n  Free: %s\n  Use%%: %s\nMounted on: %s\n",Wert1,Wert2,Wert3,Wert4,Wert5,Wert6);
               }
               fprintf(w,"\nCPU:\n\n");
               fclose(w);
       }
    }
    fclose(f);
    /* Get file-info from /proc/cpuinfo end*/

    /* Get cpuinfo from /proc/cpuinfo*/
    system("cat /proc/cpuinfo >> /tmp/sysinfo");
    system("cat /proc/bus/dreambox >> /tmp/sysinfo");
    unlink("/tmp/systmp");
    /* Get cpuinfo from /proc/cpuinfo end*/
    return(readList(sinbuffer));

}

int CBESysInfoWidget::dmesg()
{
       /* Get System-Messages from dmesg*/
       system("dmesg > /tmp/sysinfo");
       /* Get System-Messages from dmesg end*/

       return(readList(sinbuffer));
}

int CBESysInfoWidget::top()
{
       /* Get Memory/Processlist from top*/
       system("top -n1 -b > /tmp/sysinfo");
       /* Get Memory/Processlist from top end*/

       return(readList(sinbuffer));
}

int CBESysInfoWidget::caminfo()
{
       /* Get cam info from ecm.info*/
       system("cat /tmp/ecm.info > /tmp/sysinfo");
       /* Get cam info from ecm.info end*/

       return(readList(sinbuffer));
}

void CBESysInfoWidget::correct_string(char *temp)
{
	int z=0;
	while (temp[z]!=0)
	{
		if((temp[z]>0)&&(temp[z]<32)) temp[z]=32;
		z++;
	}
}

//Infos auslesen
int CBESysInfoWidget::readList(struct sfileline *sinbuffer)
{

    FILE *fp;
    char line[256];

    memset(sinbuffer ,0,(3*MAXLINES) * sizeof(struct sfileline));
    memset(sysbuffer ,0,(2*MAXLINES) * sizeof(struct sreadline));

    fp = fopen("/tmp/sysinfo","rb");

    if(fp==NULL)
      return(-1);

    slinecount=0;
    syscount=0;

    while(fgets(line, 256, fp) != NULL)
    {
        line[256]='\0';
	correct_string(line);
        memcpy(sysbuffer[syscount].line,line,256);
        sinbuffer[slinecount].state = true;
        //printf("%s", sysbuffer[syscount].line);
        sinbuffer[slinecount++].addr=sysbuffer[syscount++].line;
    }
    fclose(fp);
    if (selected>=slinecount)
       selected=slinecount-1;
    return(0);
}

