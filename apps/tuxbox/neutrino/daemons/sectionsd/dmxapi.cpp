/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/daemons/sectionsd/dmxapi.cpp,v 1.10 2011/06/19 12:22:29 rhabarber1848 Exp $
 *
 * DMX low level functions (sectionsd) - d-box2 linux project
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "global.h"
#include <dmxapi.h>

#include <stdio.h>         /* perror      */
#include <string.h>        /* memset      */
#include <sys/ioctl.h>     /* ioctl       */
#include <fcntl.h>         /* open        */
#include <unistd.h>        /* close, read */
#include <arpa/inet.h>     /* htons */

#include "SIutils.hpp"
#include "debug.h"

bool setfilter(const int fd, const uint16_t pid, const uint8_t filter, const uint8_t mask, const uint32_t flags)
{
	struct dmx_sct_filter_params flt;

	memset(&flt, 0, sizeof(struct dmx_sct_filter_params));

	flt.pid              = pid;
#ifndef HAVE_TRIPLEDRAGON
	flt.filter.filter[0] = filter;
	flt.filter.mask  [0] = mask;
#else
	flt.filter[0] = filter;
	flt.mask[0] = mask;
	flt.filter_length = 1 + 2;
#endif
	flt.timeout          = 0;
	flt.flags            = flags;

	if (::ioctl(fd, DMX_SET_FILTER, &flt) == -1)
	{
		perror("[sectionsd] DMX: DMX_SET_FILTER");
		return false;
	}
	return true;
}

struct SI_section_TOT_header
{
	unsigned char      table_id                 :  8;
	unsigned char      section_syntax_indicator :  1;
	unsigned char      reserved_future_use      :  1;
	unsigned char      reserved1                :  2;
	unsigned short     section_length           : 12;
/*	unsigned long long UTC_time                 : 40;*/
	UTC_t              UTC_time;
#if __BYTE_ORDER == __BIG_ENDIAN
	unsigned char      reserved2                :  4;
	unsigned char      descr_loop_length_hi     :  4;
#else
	unsigned char      descr_loop_length_hi     :  4;
	unsigned char      reserved2                :  4;
#endif
	unsigned short     descr_loop_length_lo     :  8;
}
__attribute__ ((packed)); /* 10 bytes */

struct SI_section_TDT_header
{
	unsigned char      table_id                 :  8;
	unsigned char      section_syntax_indicator :  1;
	unsigned char      reserved_future_use      :  1;
	unsigned char      reserved1                :  2;
	unsigned short     section_length           : 12;
/*	unsigned long long UTC_time                 : 40;*/
	UTC_t              UTC_time;
}
__attribute__ ((packed)); /* 8 bytes */

struct descrLocalTimeOffset
{
	unsigned char      country_code[3];
#if __BYTE_ORDER == __BIG_ENDIAN
	unsigned char      country_region_id          :  6;
	unsigned char      reserved_1                 :  1;
	unsigned char      local_time_offset_polarity :  1;
#else
	unsigned char      local_time_offset_polarity :  1;
	unsigned char      reserved_1                 :  1;
	unsigned char      country_region_id          :  6;
#endif
	unsigned int       local_time_offset          : 16;
	unsigned int       time_of_change_MJD         : 16;
	unsigned int       time_of_change_UTC         : 24;
	unsigned int       next_time_offset           : 16;
} __attribute__ ((packed)); /* 13 bytes */;


bool getUTC(UTC_t * const UTC, const bool TDT)
{
	int fd;
	struct dmx_sct_filter_params flt;
	struct SI_section_TOT_header tdt_tot_header;
	char cUTC[5];
	bool ret = true;

	unsigned char buf[1023+3];

	if ((fd = ::open(DEMUX_DEVICE, O_RDWR)) < 0)
	{
		perror("[sectionsd] getUTC: open");
		return false;
	}

	memset(&flt, 0, sizeof(struct dmx_sct_filter_params));

	flt.pid              = 0x0014;
#ifndef HAVE_TRIPLEDRAGON
	flt.filter.filter[0] = TDT ? 0x70 : 0x73;
	flt.filter.mask  [0] = 0xFF;
#else
	flt.filter[0] = TDT ? 0x70 : 0x73;
	flt.mask  [0] = 0xFF;
	flt.filter_length = 1 + 2;
#endif
	flt.timeout          = 31000;
	flt.flags            = TDT ? (DMX_ONESHOT | DMX_IMMEDIATE_START | XPDF_NO_CRC) : (DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START);

	if (::ioctl(fd, DMX_SET_FILTER, &flt) == -1)
	{
		perror("[sectionsd] getUTC: set filter");
		::close(fd);
		return false;
	}

	int size = TDT ? sizeof(struct SI_section_TDT_header) : sizeof(tdt_tot_header);
	if (::read(fd, buf, TDT ? size : sizeof(buf)) < size)
	{
		if (TDT || debug) /* not having TOT is common, no need to log */
			perror("[sectionsd] getUTC: read");
		::close(fd);
		return false;
	}
	memset(&tdt_tot_header, 0, sizeof(tdt_tot_header));
	memcpy(&tdt_tot_header, buf, size);

	uint64_t tmp = tdt_tot_header.UTC_time.time;
	memcpy(cUTC, &tdt_tot_header.UTC_time, 5);
	if ((cUTC[2] > 0x23) || (cUTC[3] > 0x59) || (cUTC[4] > 0x59)) // no valid time
	{
		printf("[sectionsd] getUTC: invalid %s section received: %02x %02x %02x %02x %02x\n", 
			TDT ? "TDT" : "TOT", cUTC[0], cUTC[1], cUTC[2], cUTC[3], cUTC[4]);
		ret = false;
	}

	(*UTC).time = tmp;

	short loop_length = tdt_tot_header.descr_loop_length_hi << 8 | tdt_tot_header.descr_loop_length_lo;
	if (loop_length >= 15) {
		int off = sizeof(tdt_tot_header);
		int rem = loop_length;
		while (rem >= 15)
 		{
			unsigned char *b2 = &buf[off];
			if (b2[0] == 0x58) {
				struct descrLocalTimeOffset *to;
				to = (struct descrLocalTimeOffset *)&b2[2];
				unsigned char cc[4];
				cc[3] = 0;
				memcpy(cc, to->country_code, 3);
				time_t t = changeUTCtoCtime(&b2[2+6],0);
				char tbuf[26];

				xprintf("getUTC(TOT): len=%d cc=%s reg_id=%d "
					"pol=%d offs=%04x new=%04x when=%s",
					b2[1], cc, to->country_region_id,
					to->local_time_offset_polarity, htons(to->local_time_offset),
					htons(to->next_time_offset), ctime_r(&t, tbuf));
			} else {
				xprintf("getUTC(TOT): descriptor != 0x58: 0x%02x\n", b2[0]);
			}
			off += b2[1] + 2;
			rem -= b2[1] + 2;
			if (off + rem > (int)sizeof(buf))
			{
				xprintf("getUTC(TOT): not enough buffer space? (%d/%d)\n", off+rem, sizeof(buf));
				break;
			}
 		}
 	}

	/* TOT without descriptors seems to be not better than a plain TDT, such TOT's are */
	/* found on transponders which also have wrong time in TDT etc, so don't trust it. */
	if (loop_length < 15 && !TDT)
		ret = false;

	::close(fd);
	return ret;
}
