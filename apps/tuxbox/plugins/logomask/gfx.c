/*
 * $Id: gfx.c,v 1.1 2010/03/03 20:28:45 rhabarber1848 Exp $
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

#include "logomask.h"

/******************************************************************************
 * RenderBox
 ******************************************************************************/

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
