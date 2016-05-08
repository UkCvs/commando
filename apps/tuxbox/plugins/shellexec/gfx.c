/*
 * $Id: gfx.c,v 1.2 2010/09/27 19:40:10 rhabarber1848 Exp $
 *
 * shellexec - d-box2 linux project
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

#include "shellexec.h"
#include "gfx.h"

/*
char circle[] =
{
	0,0,0,0,0,1,1,0,0,0,0,0,
	0,0,0,1,1,1,1,1,1,0,0,0,
	0,0,1,1,1,1,1,1,1,1,0,0,
	0,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,0,
	1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,
	0,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,0,
	0,0,1,1,1,1,1,1,1,1,0,0,
	0,0,0,1,1,1,1,1,1,0,0,0,
	0,0,0,0,0,1,1,0,0,0,0,0
};
*/
//typedef struct { unsigned char width_lo; unsigned char width_hi; unsigned char height_lo; unsigned char height_hi; 	unsigned char transp; } IconHeader;
struct rawHeader
{
	uint8_t width_lo;
	uint8_t width_hi;
	uint8_t height_lo;
	uint8_t height_hi;
	uint8_t transp;
} __attribute__ ((packed));

char res_iconfile[64];

/******************************************************************************
 * RenderBox
 ******************************************************************************/
void RenderBox(int _sx, int _sy, int _ex, int _ey, int rad, int col)
{
	int F,R=rad,ssx=startx+_sx,ssy=starty+_sy,dxx=_ex-_sx,dyy=_ey-_sy,rx,ry,wx,wy,count;

	unsigned char *pos=(lbb+ssx+var_screeninfo.xres*ssy);
	unsigned char *pos0, *pos1, *pos2, *pos3;
	
	if (dxx<0) 
	{
		printf("[shellexec] RenderBox called with dx < 0 (%d)\n", dxx);
		dxx=0;
	}

	if(--dyy<=0)
	{
		dyy=1;
	}
	if(R)
	{
		if(R==1 || R>(dxx/2) || R>(dyy/2))
		{
			R=dxx/10;
			F=dyy/10;	
			if(R>F)
			{
				if(R>(dyy/3))
				{
					R=dyy/3;
				}
			}
			else
			{
				R=F;
				if(R>(dxx/3))
				{
					R=dxx/3;
				}
			}
		}
		if(!R)
		{
			R=1;
		}
		ssx=0;
		ssy=R;
		F=1-R;

		rx=R-ssx;
		ry=R-ssy;

		pos0=pos+((dyy-ry)*var_screeninfo.xres);
		pos1=pos+(ry*var_screeninfo.xres);
		pos2=pos+(rx*var_screeninfo.xres);
		pos3=pos+((dyy-rx)*var_screeninfo.xres);
		while (ssx <= ssy)
		{
			rx=R-ssx;
			ry=R-ssy;
			wx=rx<<1;
			wy=ry<<1;

			memset(pos0+rx, col, dxx-wx);
			memset(pos1+rx, col, dxx-wx);
			memset(pos2+ry, col, dxx-wy);
			memset(pos3+ry, col, dxx-wy);

			ssx++;
			pos2-=var_screeninfo.xres;
			pos3+=var_screeninfo.xres;
			if (F<0)
			{
				F+=(ssx<<1)-1;
			}
			else   
			{ 
				F+=((ssx-ssy)<<1);
				ssy--;
				pos0-=var_screeninfo.xres;
				pos1+=var_screeninfo.xres;
			}
		}
		pos+=R*var_screeninfo.xres;
	}

	for (count=R; count<(dyy-R); count++)
	{
		memset(pos, col, dxx);
		pos+=var_screeninfo.xres;
	}
}

/*
void RenderBox(int sx, int sy, int ex, int ey, int mode, int color)
{
	int loop;

	if(mode == FILL) for(; sy <= ey; sy++) memset(lbb + startx + sx + var_screeninfo.xres*(starty + sy), color, ex-sx + 1);
	else
	{
		//hor lines

			for(loop = sx; loop <= ex; loop++)
			{
				*(lbb + startx+loop + var_screeninfo.xres*(sy+starty)) = color;
				*(lbb + startx+loop + var_screeninfo.xres*(sy+1+starty)) = color;

				*(lbb + startx+loop + var_screeninfo.xres*(ey-1+starty)) = color;
				*(lbb + startx+loop + var_screeninfo.xres*(ey+starty)) = color;
			}

		//ver lines

			for(loop = sy; loop <= ey; loop++)
			{
				*(lbb + startx+sx + var_screeninfo.xres*(loop+starty)) = color;
				*(lbb + startx+sx+1 + var_screeninfo.xres*(loop+starty)) = color;

				*(lbb + startx+ex-1 + var_screeninfo.xres*(loop+starty)) = color;
				*(lbb + startx+ex + var_screeninfo.xres*(loop+starty)) = color;
			}
	}
}
*/
/******************************************************************************
 * RenderCircle
 ******************************************************************************/
/*
void RenderCircle(int sx, int sy, char type)
{
	int x, y, color;

	//set color

		switch(type)
		{
			case 'G':	color = GREEN;
					break;

			case 'Y':	color = YELLOW;
					break;

			case 'R':	color = RED;
					break;

			case 'B':	color = BLUE1;
					break;

			default:	return;
		}

	//render

		for(y = 0; y < 12; y++)
		{
			for(x = 0; x < 12; x++) if(circle[x + y*12]) memset(lbb + startx + sx + x + var_screeninfo.xres*(starty + sy + y), color, 1);
		}
}
*/
/******************************************************************************
 * PaintIcon
 ******************************************************************************/
void PaintIcon(const char * const filename, int x, int y, unsigned char offset)
{
	struct rawHeader iheader;
	unsigned int  width, height,count,count2;
	unsigned char pixbuf[768],*pixpos,compressed,pix1,pix2;
	unsigned char * d = (lbb+(startx+x)+var_screeninfo.xres*(starty+y));
	unsigned char * d2;
	int fd;

	getIconFilePath(filename, res_iconfile);

	fd = open(res_iconfile, O_RDONLY);

	if (fd == -1)
	{
		printf("[shellexec] %s: <unable to load icon: %s>\n", __FUNCTION__, res_iconfile);
		return;
	}

	read(fd, &iheader, sizeof(struct rawHeader));

	width  = (iheader.width_hi  << 8) | iheader.width_lo;
	height = (iheader.height_hi << 8) | iheader.height_lo;


	for (count=0; count<height; count ++ )
	{
		read(fd, &pixbuf, width >> 1 );
		pixpos = (unsigned char*) &pixbuf;
		d2 = d;
		for (count2=0; count2<width >> 1; count2 ++ )
		{
			compressed = *pixpos;
			pix1 = (compressed & 0xf0) >> 4;
			pix2 = (compressed & 0x0f);

			if (pix1 != iheader.transp)
			{
				*d2=pix1 + offset;
			}
			d2++;
			if (pix2 != iheader.transp)
			{
				*d2=pix2 + offset;
			}
			d2++;
			pixpos++;
		}
		d += var_screeninfo.xres;
	}
	close(fd);
	return;
}

void getIconFilePath(const char * const filename, char* res_buffer)
{
/*    	
 *  	filename can be a single filename eg. "<filename>" 
 *  	or absolute path eg. "/var/dir/<filename>" 
 */
	if (access(filename, 0) != -1)
	{
		strcpy(res_buffer, filename);
		return;
	}

	char* defaultIconPath = (char*)calloc(strlen(ICON_BASE_PATH) + strlen(filename) +1, sizeof(char));
	strcpy(defaultIconPath, ICON_BASE_PATH);
	strcat(defaultIconPath, filename);
	char* alterIconPath = (char*)calloc(strlen(ICON_VAR_PATH) + strlen(filename) +1, sizeof(char));
	strcpy(alterIconPath, ICON_VAR_PATH);
	strcat(alterIconPath, filename);

	if (access(alterIconPath, 0) != -1)
		strcpy(res_buffer, alterIconPath);
	else
		strcpy(res_buffer, defaultIconPath);

	free(defaultIconPath);
	free(alterIconPath);

	return;
}

void getIconSize(const char * const filename, int* width, int* height)
{
	*width           = 0;
	*height          = 0;
	struct rawHeader header;
	int              icon_fd;

	getIconFilePath(filename, res_iconfile);

	icon_fd = open(res_iconfile, O_RDONLY);

	if (icon_fd == -1)
	{
		printf("[shellexec] %s: error while loading icon: %s %s\n", __FUNCTION__, res_iconfile, strerror(errno));
		return;
	}

	read(icon_fd, &header, sizeof(struct rawHeader));
	*height = (header.height_hi << 8) | header.height_lo;
	*width = (header.width_hi << 8) | header.width_lo;

	close(icon_fd);
}
