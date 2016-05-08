/*
 * $Id: text.h,v 1.2 2010/09/27 19:40:10 rhabarber1848 Exp $
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

#ifndef __TEXT_H__

#define __TEXT_H__

#include "shellexec.h"

extern int FSIZE_BIG;
extern int FSIZE_MED;
extern int FSIZE_SMALL;

void TranslateString(char *src);
int GetStringLen(int _sx, unsigned char *string);
FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library _library, FT_Pointer request_data, FT_Face *aface);
void RenderString(char *string, int _sx, int _sy, int maxwidth, int layout, int size, int color);
void ShowMessage(char *mtitle, char *message, int wait);
void remove_tabs(char *src);

#endif
