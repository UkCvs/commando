/*
 *   saa7126_core.h - pal driver (dbox-II-project)
 *
 *   Homepage: http://www.tuxbox.org
 *
 *   Copyright (C) 2000-2001 Gillem (htoa@gmx.net)
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

#define SAAIOGREG	1	/* read registers */
#define SAAIOSINP	2	/* input control */
#define SAAIOSOUT	3	/* output control */
#define SAAIOSENC	4	/* set encoder (pal/ntsc) */
#define SAAIOSMODE	5	/* set mode (rgb/fbas/svideo) */
#define SAAIOSPOWERSAVE	6	/* set power save */
#define SAAIOGPOWERSAVE	7	/* get power save */
#define SAAIOSVPSDATA	8	/* set vps data */
#define SAAIOGVPSDATA	9	/* get vps data */
#define SAAIOSWSS	10	/* set wide screen signaling data */
#define SAAIOGWSS	11	/* get wide screen signaling data */
#define SAA_READREG	12	/* read single register */
#define SAA_WRITEREG	13	/* write single register */
#define SAAIOSCSYNC  14 /* set sync correction in rgb mode */
#define SAAIOGCSYNC  15 /* get sync correction in rgb mode */
#define SAAIOSTTX	16	/* 	setup teletext data reinsertion (1=on,0=off) 
							NB: this seems to shut down the TTX interface which
							obviously annoys the demux so after turning reinsertion
							on again it's necessary to restart the teletext PID
							(f.ex. by switching channels)	*/
#define SAAIOGTTX	17	/* get teletext data reinsertion (1=on,0=off) */
#define SAAIOGMODE	18	/* get mode */

#define SAA_MODE_RGB	0
#define SAA_MODE_FBAS	1
#define SAA_MODE_SVIDEO	2
#define SAA_MODE_YUV_V	3
#define SAA_MODE_YUV_C	4

#define SAA_NTSC	0
#define SAA_PAL		1

#define SAA_INP_MP1	1
#define SAA_INP_MP2	2
#define SAA_INP_CSYNC	4
#define SAA_INP_DEMOFF	6
#define SAA_INP_SYMP	8
#define SAA_INP_CBENB	128

#define SAA_WSS_43F	0	/* full format 4:3 */
#define SAA_WSS_149C	1	/* box 14:9 center */
#define SAA_WSS_149T	2	/* box 14:9 top */
#define SAA_WSS_169C	3	/* box 16:9 center */
#define SAA_WSS_169T	4	/* box 16:9 top */
#define SAA_WSS_GT169C	5	/* box > 16:9 center */
#define SAA_WSS_43_149C	6	/* full format 4:3 with 14:9 center letterbox content */
#define SAA_WSS_169F	7	/* full format 16:9 (anamorphic) */
#define SAA_WSS_OFF	8	/* no wide screen signaling */

// TODO: fix this table

typedef struct s_saa_data {
	unsigned char version   : 3;
	unsigned char ccrdo     : 1;
	unsigned char ccrde     : 1;
	unsigned char res01     : 1;
	unsigned char fseq      : 1;
	unsigned char o_2       : 1;

	unsigned char res02[0x26-0x01];	// NULL

	unsigned char wss_7_0   : 8;

	unsigned char wsson     : 1;
	unsigned char res03     : 1;
	unsigned char wss_13_8  : 6;

	unsigned char deccol    : 1;
	unsigned char decfis    : 1;
	unsigned char bs        : 6;

	unsigned char sres      : 1;
	unsigned char res04     : 1;
	unsigned char be        : 6;

	unsigned char cg_7_0    : 8;

	unsigned char cg_15_8   : 8;

	unsigned char cgen      : 1;
	unsigned char res05     : 3;
	unsigned char cg_19_16  : 4;

	unsigned char vbsen     : 2;
	unsigned char cvbsen    : 1;
	unsigned char cen       : 1;
	unsigned char cvbstri   : 1;
	unsigned char rtri      : 1;
	unsigned char gtri      : 1;
	unsigned char btri      : 1;

	unsigned char res06[0x38-0x2e];	// NULL

	unsigned char res07     : 3;
	unsigned char gy        : 5;

	unsigned char res08     : 3;
	unsigned char gcd       : 5;

	unsigned char vbenb     : 1;
	unsigned char res09     : 2;
	unsigned char symp      : 1;
	unsigned char demoff    : 1;
	unsigned char csync     : 1;
	unsigned char mp2c      : 2;

	unsigned char res10[0x54-0x3b];	// ???

	unsigned char vpsen     : 1;
	unsigned char ccirs     : 1;
	unsigned char res11     : 4;
	unsigned char edge      : 2;

	unsigned char vps5      : 8;
	unsigned char vps11     : 8;
	unsigned char vps12     : 8;
	unsigned char vps13     : 8;
	unsigned char vps14     : 8;

	unsigned char chps      : 8;
	unsigned char gainu_7_0 : 8;
	unsigned char gainv_7_0 : 8;

	unsigned char gainu_8   : 1;
	unsigned char decoe     : 1;
	unsigned char blckl     : 6;

	unsigned char gainv_8   : 1;
	unsigned char decph     : 1;
	unsigned char blnnl     : 6;

	unsigned char ccrs      : 2;
	unsigned char blnvb     : 6;

	unsigned char res12     : 8; // NULL

	unsigned char downb     : 1;
	unsigned char downa     : 1;
	unsigned char inpi      : 1;
	unsigned char ygs       : 1;
	unsigned char res13     : 1;
	unsigned char scbw      : 1;
	unsigned char pal       : 1;
	unsigned char fise      : 1;

	// 62h
	unsigned char rtce      : 1;
	unsigned char bsta      : 7;

	unsigned char fsc0      : 8;
	unsigned char fsc1      : 8;
	unsigned char fsc2      : 8;
	unsigned char fsc3      : 8;

	unsigned char l21o0     : 8;
	unsigned char l21o1     : 8;
	unsigned char l21e0     : 8;
	unsigned char l21e1     : 8;

	unsigned char srcv0     : 1;
	unsigned char srcv1     : 1;
	unsigned char trcv2     : 1;
	unsigned char orcv1     : 1;
	unsigned char prcv1     : 1;
	unsigned char cblf      : 1;
	unsigned char orcv2     : 1;
	unsigned char prcv2     : 1;

	// 6ch
	unsigned char htrig0    : 8;
	unsigned char htrig1    : 8;

	unsigned char sblbn     : 1;
	unsigned char blckon    : 1;
	unsigned char phres     : 2;
	unsigned char ldel      : 2;
	unsigned char flc       : 2;

	unsigned char ccen      : 2;
	unsigned char ttxen     : 1;
	unsigned char sccln     : 5;

	unsigned char rcv2s_lsb : 8;
	unsigned char rcv2e_lsb : 8;

	unsigned char res14     : 1;
	unsigned char rvce_mbs  : 3;
	unsigned char res15     : 1;
	unsigned char rvcs_mbs  : 3;

	unsigned char ttxhs     : 8;
	unsigned char ttxhl     : 4;
	unsigned char ttxhd     : 4;

	unsigned char csynca    : 5;
	unsigned char vss       : 3;

	unsigned char ttxovs    : 8;
	unsigned char ttxove    : 8;
	unsigned char ttxevs    : 8;
	unsigned char ttxeve    : 8;

	// 7ah
	unsigned char fal       : 8;
	unsigned char lal       : 8;

	unsigned char ttx60     : 1;
	unsigned char lal8      : 1;
	unsigned char ttx0      : 1;
	unsigned char fal8      : 1;
	unsigned char ttxeve8   : 1;
	unsigned char ttxove8   : 1;
	unsigned char ttxevs8   : 1;
	unsigned char ttxovs8   : 1;

	unsigned char res16     : 8;

	unsigned char ttxl12    : 1;
	unsigned char ttxl11    : 1;
	unsigned char ttxl10    : 1;
	unsigned char ttxl9     : 1;
	unsigned char ttxl8     : 1;
	unsigned char ttxl7     : 1;
	unsigned char ttxl6     : 1;
	unsigned char ttxl5     : 1;

	unsigned char ttxl20    : 1;
	unsigned char ttxl19    : 1;
	unsigned char ttxl18    : 1;
	unsigned char ttxl17    : 1;
	unsigned char ttxl16    : 1;
	unsigned char ttxl15    : 1;
	unsigned char ttxl14    : 1;
	unsigned char ttxl13    : 1;
} __attribute__ ((__packed__)) s_saa_data;

#define SAA_DATA_SIZE		sizeof(s_saa_data)

#ifdef __KERNEL__

#endif
