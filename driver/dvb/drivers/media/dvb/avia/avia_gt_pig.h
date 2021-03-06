/*
 *   avia_gt_pig.h - pig driver for AViA (dbox-II-project)
 *
 *   Homepage: http://www.tuxbox.org
 *
 *   Copyright (C) 2002 Florian Schirmer (jolt@tuxbox.org)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef AVIA_GT_PIG_H
#define AVIA_GT_PIG_H

int avia_gt_pig_hide(unsigned char pig_nr);
int avia_gt_pig_set_pos(unsigned char pig_nr, unsigned short x, unsigned short y);
int avia_gt_pig_set_size(unsigned char pig_nr, unsigned short width, unsigned short height, unsigned char stretch);
int avia_gt_pig_set_stack(unsigned char pig_nr, unsigned char stack_order);
int avia_gt_pig_show(unsigned char pig_nr);

int avia_gt_pig_init(void);
void avia_gt_pig_exit(void);
	    
#endif
