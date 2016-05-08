/*
 * $Id: gfx.h,v 1.2 2010/09/27 19:40:10 rhabarber1848 Exp $
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

#ifndef __GFX_H__

#define __GFX_H__

void getIconFilePath(const char * const filename, char* res_buffer);
void getIconSize(const char * const filename, int* width, int* height);
void RenderBox(int _sx, int _sy, int _ex, int _ey, int mode, int color);
//void RenderCircle(int sx, int sy, char type);
void PaintIcon(const char * const filename, int x, int y, unsigned char offset);

#endif
