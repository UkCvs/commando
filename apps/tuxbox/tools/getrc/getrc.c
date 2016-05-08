/*
 * $Id: getrc.c,v 1.1 2009/12/31 14:52:03 rhabarber1848 Exp $
 *
 * getrc - d-box2 linux project
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
#include "getrc.h"
#include "io.h"

int main (int argc, char **argv)
{
int rv='X',i;
char *key=NULL;
int tmo=0;

	InitRC();
	for(i=1; i<argc; i++)
	{
		if(strstr(argv[i],"key=")==argv[i])
		{
			key=argv[i]+4;
		}
		if(strstr(argv[i],"timeout=")==argv[i])
		{
			if(sscanf(argv[i]+8,"%d",&tmo)!=1)
			{
				tmo=0;
			}
		}
	}

	rv=GetRCCode(key, tmo);
	
	CloseRC();

	printf("%c\n",rv);
	return rv;
}
