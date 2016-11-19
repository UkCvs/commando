// Original AutoBouquets N1 - autobouquetsreader.cpp by LraiZer

// Sun 7 August 2016 - This autobouquets.cpp has been modified and supplied by PaphosAL
// http://www.ukcvs.net/forum/showthread.php?29792-AutoBouquets-N1&p=58678&viewfull=1#post58678

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <string.h>
#include <unistd.h>

#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>

#include "ost/frontend.h"
#include "ost/dmx.h"

#define DVB_BUFFER_SIZE 2*4096

using namespace std;

template <class T>
string to_string(T t, ios_base & (*f)(ios_base&))
{
  ostringstream oss;
  oss << f << t;
  return oss.str();
}

u_int32_t crc_table[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

u_int32_t crc32 (const char *d, int len, u_int32_t crc) {
	register int i;
	const unsigned char *u=(unsigned char*)d;

	for (i=0; i<len; i++)
		crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *u++)];

	return crc;
}

std::string Latin1_to_UTF8(const char * s)
{
    std::string r;
    while((*s) != 0)
    {
        unsigned char c = *s;
        if (c < 0x80)
            r += c;
        else
        {
            unsigned char d = 0xc0 | (c >> 6);
            r += d;
            d = 0x80 | (c & 0x3f);
            r += d;
        }
        s++;
    }
    return r;
}

std::string UTF8_to_UTF8XML(const char * s)
{
	std::string r;
	while ((*s) != 0)
	{
		switch (*s)
		{
		case '<':
			r += "&lt;";
			break;
		case '>':
			r += "&gt;";
			break;
		case '&':
			r += "&amp;";
			break;
		case '\"':
			r += "&quot;";
			break;
		case '\'':
			r += "&apos;";
			break;
		default:
			r += *s;
		}
		s++;
	}
	return Latin1_to_UTF8(r.c_str());
}

struct header_t {
	unsigned short table_id;
	unsigned short variable_id;
	short version_number;
	short current_next_indicator;
	short section_number;
	short last_section_number;
} header;

struct sections_t {
	short version;
	short last_section;
	short received_section[0xff];
	short populated;
} Sections;

struct transport_t {
	unsigned short original_network_id;
	unsigned short transport_stream_id;
	short modulation_system;
	unsigned int frequency;
	unsigned int symbol_rate;
	short polarization;
	short modulation_type;
	short fec_inner;
	short roll_off;
	short orbital_position;
	short west_east_flag;
} Transport;

struct channel_t {
	string skyid;
	string provider;
	string name;
	string type;
	string onid;
	string tsid;
	string sid;
	string ca;
	string nspace;
} channel;

struct epg_t {
	bool enabled;
	short days;
	short desc_hours;
} epg;

sections_t BAT1_SECTIONS, BAT2_SECTIONS, BAT3_SECTIONS, NIT_SECTIONS;
map<string, channel_t> BAT1, BAT2, BAT3, SDT, TV, RADIO, DATA, TEST;
map<string, sections_t> TRANSPONDER_SECTIONS;
map<string, transport_t> NIT;
map<string, epg_t> EPG;

// default Granada SD >< Northern Ireland SD
int BAT_local = 0x1001, BAT_local_region = 0x7;
int BAT_merge = 0x1004, BAT_merge_region = 0x21;
int skyid;
bool dvbloop = true;
unsigned short sdtmax = 0;

int si_parse_nit(unsigned char *data, int length) {

	if (length < 8)
		return -1;

	int network_descriptors_length = ((data[8] & 0x0f) << 8) | data[9];
	int transport_stream_loop_length = ((data[network_descriptors_length + 10] & 0x0f) << 8) | data[network_descriptors_length + 11];
	int offset1 = network_descriptors_length + 12;

	while (transport_stream_loop_length > 0)
	{

		unsigned short tsid;
		tsid = (data[offset1] << 8) | data[offset1 + 1];
		Transport.original_network_id = (data[offset1 + 2] << 8) | data[offset1 + 3];

		int transport_descriptor_length = ((data[offset1 + 4] & 0x0f) << 8) | data[offset1 + 5];
		int offset2 = offset1 + 6;

		offset1 += (transport_descriptor_length + 6);
		transport_stream_loop_length -= (transport_descriptor_length + 6);

		while (transport_descriptor_length > 0)
		{
			unsigned char descriptor_tag = data[offset2];
			unsigned char descriptor_length = data[offset2 + 1];

			if (descriptor_tag == 0x43)
			{
				Transport.frequency = (data[offset2 + 2] >> 4) * 10000000;
				Transport.frequency += (data[offset2 + 2] & 0x0f) * 1000000;
				Transport.frequency += (data[offset2 + 3] >> 4) * 100000;
				Transport.frequency += (data[offset2 + 3] & 0x0f) * 10000;
				Transport.frequency += (data[offset2 + 4] >> 4) * 1000;
				Transport.frequency += (data[offset2 + 4] & 0x0f) * 100;
				Transport.frequency += (data[offset2 + 5] >> 4) * 10;
				Transport.frequency += data[offset2 + 5] & 0x0f;

				Transport.orbital_position = (data[offset2 + 6] << 8) | data[offset2 + 7];
				Transport.west_east_flag = (data[offset2 + 8] >> 7) & 0x01;
				Transport.polarization = (data[offset2 + 8] >> 5) & 0x03;
				Transport.roll_off = (data[offset2 + 8] >> 3) & 0x03;
				Transport.modulation_system = (data[offset2 + 8] >> 2) & 0x01;
				Transport.modulation_type = data[offset2 + 8] & 0x03;

				Transport.symbol_rate = (data[offset2 + 9] >> 4) * 1000000;
				Transport.symbol_rate += (data[offset2 + 9] & 0xf) * 100000;
				Transport.symbol_rate += (data[offset2 + 10] >> 4) * 10000;
				Transport.symbol_rate += (data[offset2 + 10] & 0xf) * 1000;
				Transport.symbol_rate += (data[offset2 + 11] >> 4) * 100;
				Transport.symbol_rate += (data[offset2 + 11] & 0xf) * 10;
				Transport.symbol_rate += data[offset2 + 11] >> 4;

				Transport.fec_inner = data[offset2 + 12] & 0xf;

				NIT[to_string<unsigned short>(tsid, hex)] = Transport;
			}

			offset2 += (descriptor_length + 2);
			transport_descriptor_length -= (descriptor_length + 2);
		}
	}
	return 1;
}

int sections_check(sections_t *sections) {
	for ( int i = 0; i <= header.last_section_number; i++ ) {
		if ( sections->received_section[i] == 0 )
			return 0;
	}
	return 1;
}

void network_check(int variable_id, sections_t *sections, unsigned char *data, int length) {
	if (!sections->received_section[header.section_number])
	{
		if (si_parse_nit(data, length))
		{
			sections->received_section[header.section_number] = 1;
			if (sections_check(sections))
				sections->populated = 1;
		}
	}
}

int si_open(int pid) {
	short fd = 0;

	char filter, mask;
	struct dmxSctFilterParams sfilter;

	filter = 0x40;
	mask = 0xf0;

	memset(&sfilter, 0, sizeof(sfilter));
	sfilter.pid = pid & 0xffff;
	sfilter.filter.filter[0] = filter & 0xff;
	sfilter.filter.mask[0] = mask & 0xff;
	sfilter.timeout = 0;
	sfilter.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	if ((fd = open("/dev/dvb/card0/demux0", O_RDWR | O_NONBLOCK)) < 0)
		printf("Cannot open demuxer '%s'\n", "/dev/dvb/card0/demux0" );

	if (ioctl(fd, DMX_SET_FILTER, &sfilter) == -1) {
		printf("ioctl DMX_SET_FILTER failed\n");
		close(fd);
	}

	return fd;
}

void si_close(int fd) {
	if (fd > 0) {
		ioctl (fd, DMX_STOP, 0);
		close(fd);
	}
}

void si_parse_header(unsigned char *data) {
	header.table_id = data[0];
	header.variable_id = (data[3] << 8) | data[4];
	header.version_number = (data[5] >> 1) & 0x1f;
	header.current_next_indicator = data[5] & 0x01;
	header.section_number = data[6];
	header.last_section_number = data[7];
}

void si_parse_sdt(unsigned char *data, int length) {
	unsigned short transport_stream_id = (data[3] << 8) | data[4];
	unsigned short original_network_id = (data[8] << 8) | data[9];

	int offset = 11;
	length -= 11;

	while (length >= 5)
	{
		unsigned short service_id = (data[offset] << 8) | data[offset + 1];
		short free_ca = (data[offset + 3] >> 4) & 0x01;
		int descriptors_loop_length = ((data[offset + 3] & 0x0f) << 8) | data[offset + 4];
		char service_name[256];
		char provider_name[256];
		unsigned short service_type = 0;
		memset(service_name, '\0', 256);
		memset(provider_name, '\0', 256);

		length -= 5;
		offset += 5;

		int offset2 = offset;

		length -= descriptors_loop_length;
		offset += descriptors_loop_length;

		while (descriptors_loop_length >= 2)
		{
			int tag = data[offset2];
			int size = data[offset2 + 1];

			if (tag == 0x48) // service_descriptor
			{
				service_type = data[offset2 + 2];
				int service_provider_name_length = data[offset2 + 3];
				if (service_provider_name_length == 255)
					service_provider_name_length--;

				int service_name_length = data[offset2 + 4 + service_provider_name_length];
				if (service_name_length == 255)
					service_name_length--;

				memset(service_name, '\0', 256);
				memcpy(provider_name, data + offset2 + 4, service_provider_name_length);
				memcpy(service_name, data + offset2 + 5 + service_provider_name_length, service_name_length);
			}
			if (tag == 0xc0) // nvod + adult service descriptor
			{
				memset(service_name, '\0', 256);
				memcpy(service_name, data + offset2 + 2, size);
			}

			descriptors_loop_length -= (size + 2);
			offset2 += (size + 2);
		}

		char *provider_name_ptr = provider_name;
		if (strlen(provider_name) == 0)
			strcpy(provider_name, "BSkyB");
		else if (provider_name[0] == 0x05)
			provider_name_ptr++;

		char *service_name_ptr = service_name;
		if (strlen(service_name) == 0)
			strcpy(service_name, to_string<unsigned short>(service_id, dec).c_str());
		else if (service_name[0] == 0x05)
			service_name_ptr++;

		string sid = to_string<unsigned short>(service_id, hex);
		if (strlen(SDT[sid].provider.c_str()) == 0)
			SDT[sid].provider = provider_name_ptr;
		if (strlen(SDT[sid].name.c_str()) == 0) {
			SDT[sid].name = service_name_ptr;
		}

		SDT[sid].ca = free_ca ? "NDS" : "FTA";
		SDT[sid].type = to_string<unsigned short>(service_type, hex);
		SDT[sid].sid = to_string<unsigned short>(service_id, hex);
		SDT[sid].onid = to_string<unsigned short>(original_network_id, hex);
		SDT[sid].tsid = to_string<unsigned short>(transport_stream_id, hex);
	}
}

int si_parse_bat(unsigned char *data, int length) {

	if (length < 8)
		return -1;

	int bouquet_descriptors_length = ((data[8] & 0x0f) << 8) | data[9];
	int transport_stream_loop_length = ((data[bouquet_descriptors_length + 10] & 0x0f) << 8) | data[bouquet_descriptors_length + 11];
	int offset1 = 10;

	while (bouquet_descriptors_length > 0)
	{
		unsigned char descriptor_tag = data[offset1];
		unsigned char descriptor_length = data[offset1 + 1];

		if (descriptor_tag == 0x47)
		{
			char description[descriptor_length + 1];
			memset(description, '\0', descriptor_length + 1);
			memcpy(description, data + offset1 + 2, descriptor_length);
		}
		offset1 += (descriptor_length + 2);
		bouquet_descriptors_length -= (descriptor_length + 2);
	}

	offset1 += 2;

	while (transport_stream_loop_length > 0)
	{
		unsigned short transport_stream_id = (data[offset1] << 8) | data[offset1 + 1];
		unsigned short original_network_id = (data[offset1 + 2] << 8) | data[offset1 + 3];
		int transport_descriptor_length = ((data[offset1 + 4] & 0x0f) << 8) | data[offset1 + 5];
		int offset2 = offset1 + 6;

		offset1 += (transport_descriptor_length + 6);
		transport_stream_loop_length -= (transport_descriptor_length + 6);

		while (transport_descriptor_length > 0)
		{
			unsigned char descriptor_tag = data[offset2];
			unsigned char descriptor_length = data[offset2 + 1];
			int offset3 = offset2 + 2;

			offset2 += (descriptor_length + 2);
			transport_descriptor_length -= (descriptor_length + 2);

			if (descriptor_tag == 0xb1)
			{
				unsigned char region_id;
				region_id = data[offset3 + 1];

				offset3 += 2;
				descriptor_length -= 2;
				while (descriptor_length > 0)
				{
					unsigned short channel_id;
					unsigned short sky_id;
					unsigned short service_id;
					unsigned short service_type;

					string epg_id;

					channel_id = (data[offset3 + 3] << 8) | data[offset3 + 4];
					sky_id = ( data[offset3 + 5] << 8 ) | data[offset3 + 6];
					service_id = (data[offset3] << 8) | data[offset3 + 1];
					service_type = data[offset3 + 2];
					epg_id = to_string<unsigned short>(channel_id, dec);

					if ( header.variable_id == BAT_local && ( region_id == BAT_local_region || region_id == 0xff ))
					{
						BAT1[epg_id].skyid = to_string<unsigned short>(sky_id, dec);
						BAT1[epg_id].type = to_string<unsigned short>(service_type, hex);
						BAT1[epg_id].onid = to_string<unsigned short>(original_network_id, hex);
						BAT1[epg_id].tsid = to_string<unsigned short>(transport_stream_id, hex);
						BAT1[epg_id].sid = to_string<unsigned short>(service_id, hex);
					}
					else if ( header.variable_id == BAT_merge && ( region_id == BAT_merge_region || region_id == 0xff ))
					{
						BAT2[epg_id].skyid = to_string<unsigned short>(sky_id, dec);
						BAT2[epg_id].type = to_string<unsigned short>(service_type, hex);
						BAT2[epg_id].onid = to_string<unsigned short>(original_network_id, hex);
						BAT2[epg_id].tsid = to_string<unsigned short>(transport_stream_id, hex);
						BAT2[epg_id].sid = to_string<unsigned short>(service_id, hex);
					}
					else if ( header.variable_id == 0x100d && BAT3.find(epg_id) == BAT3.end() )
					{
						BAT3[epg_id].skyid = to_string<unsigned short>(sky_id, dec);
						BAT3[epg_id].type = to_string<unsigned short>(service_type, hex);
						BAT3[epg_id].onid = to_string<unsigned short>(original_network_id, hex);
						BAT3[epg_id].tsid = to_string<unsigned short>(transport_stream_id, hex);
						BAT3[epg_id].sid = to_string<unsigned short>(service_id, hex);
					}
					offset3 += 9;
					descriptor_length -= 9;
				}
			}
		}
	}
	return 1;
}

void bouquet_check(int variable_id, sections_t *sections, unsigned char *data, int length) {
	if (!sections->received_section[header.section_number])
	{
		if (si_parse_bat(data, length))
		{
			sections->received_section[header.section_number] = 1;
			if (sections_check(sections))
				sections->populated = 1;
		}
	}
}

int si_read_bouquets(int fd) {

	unsigned char buffer[DVB_BUFFER_SIZE];
	bool SDT_SECTIONS_populated = false;

	while ( !BAT1_SECTIONS.populated || !BAT2_SECTIONS.populated ||
	        !BAT3_SECTIONS.populated || !SDT_SECTIONS_populated ) {

		int size = read(fd, buffer, sizeof(buffer));

		if (size < 3) {
			usleep(100000);
			return -1;
		}

		int section_length = ((buffer[1] & 0x0f) << 8) | buffer[2];

		if (size != section_length + 3)
			return -1;

		int calculated_crc = crc32((char *) buffer, section_length + 3, 0xffffffff);

		if (calculated_crc)
			calculated_crc = 0;

		if (calculated_crc != 0)
			return -1;

		si_parse_header(buffer);

		if (header.table_id == 0x4a)
		{
			if ( !BAT1_SECTIONS.populated || !BAT2_SECTIONS.populated || !BAT3_SECTIONS.populated )
			{
				if ( header.variable_id == BAT_local )		// regional bat
					bouquet_check(BAT_local, &BAT1_SECTIONS, buffer, section_length);
				else if ( header.variable_id == BAT_merge )	// merge bat (english >< irish)
					bouquet_check(BAT_merge, &BAT2_SECTIONS, buffer, section_length);
				else if ( header.variable_id == 0x100d )	// testing bat
					bouquet_check(0x100d, &BAT3_SECTIONS, buffer, section_length);
			}
		}
		else if (header.table_id == 0x42 || header.table_id == 0x46)
		{
			si_parse_sdt(buffer, section_length);

			SDT_SECTIONS_populated = true;

			sdtmax++;
			if (sdtmax < 0x1f4)
				SDT_SECTIONS_populated = false;
			else
				SDT_SECTIONS_populated = true;
		}
	}

	if ( BAT1_SECTIONS.populated && BAT2_SECTIONS.populated && BAT3_SECTIONS.populated && SDT_SECTIONS_populated )
		return 1;
	else
		return 0;
}

int si_read_network(int fd) {

	unsigned char buffer[DVB_BUFFER_SIZE];

	int size = read(fd, buffer, sizeof(buffer));

	if (size < 3) {
		usleep(100000);
		return -1;
	}

	int section_length = ((buffer[1] & 0x0f) << 8) | buffer[2];

	if (size != section_length + 3)
		return -1;

	int calculated_crc = crc32((char *) buffer, section_length + 3, 0xffffffff);

	if (calculated_crc)
		calculated_crc = 0;

	if (calculated_crc != 0)
		return -1;

	si_parse_header(buffer);

	if ( header.variable_id == 0x20 )
		network_check(0x20, &NIT_SECTIONS, buffer, section_length);

	if ( NIT_SECTIONS.populated )
		return 1;

	return 0;
}

void write_bouquet_service(ofstream& bq_stream, string ss_sid, string ss_name, string ss_tsid) {
	bq_stream << "\t\t<channel serviceID=\"" << hex << right << setw(4) << setfill('0') << ss_sid << "\" name=\"";
	bq_stream << UTF8_to_UTF8XML(ss_name.c_str()) << "\" tsid=\"";
	bq_stream << hex << right << setw(4) << setfill('0') << ss_tsid << "\" onid=\"0002\" sat=\"282\"/>" << endl;
}

void write_bouquet_service_numbered(ofstream& bq_stream, string ss_sid, string ss_name, string ss_tsid, int skyid_count) {
	bq_stream << "\t\t<channel serviceID=\"" << hex << right << setw(4) << setfill('0') << ss_sid << "\" name=\"";
	bq_stream << dec << skyid_count << " - " << UTF8_to_UTF8XML(ss_name.c_str()) << "\" tsid=\"";
	bq_stream << hex << right << setw(4) << setfill('0') << ss_tsid << "\" onid=\"0002\" sat=\"282\"/>" << endl;
}

void write_bouquet_service_channel(ofstream& bq_stream, string ss_sid, string ss_name, string ss_tsid, int channelid) {
	bq_stream << "\t\t<channel serviceID=\"" << hex << right << setw(4) << setfill('0') << ss_sid << "\" name=\"";
	bq_stream << UTF8_to_UTF8XML(ss_name.c_str()) << " " << dec << channelid << "\" tsid=\"";
	bq_stream << hex << right << setw(4) << setfill('0') << ss_tsid << "\" onid=\"0002\" sat=\"282\"/>" << endl;
}

void write_bouquet_service_placeholder(ofstream& bq_stream, ofstream& nit_stream, int skyid_count) {
	bq_stream << "\t\t<channel serviceID=\"" << hex << right << setw(4) << setfill('0');
	bq_stream << skyid_count << "\" name=\"" << dec << skyid_count << " - placeholder\" tsid=\"0000\" onid=\"0000\" sat=\"282\"/>" << endl;
	nit_stream << "\t\t\t<channel service_id=\"" << hex << right << setw(4) << setfill('0');
	nit_stream << skyid_count << "\" name=\"\" service_type=\"01\"/>" << endl;
}

void write_database_service(ofstream& db_stream, int ss_pos, string ss_skyid, string ss_type, string ss_sid, string ss_tsid, string ss_ca, string ss_name) {
	db_stream << dec << ss_pos << "," << dec << ss_skyid << ",0x" << hex << ss_type << ",0x" << hex << ss_sid;
	db_stream << ",0x" << hex << ss_tsid << ",\"" << ss_ca << "\",\"" << UTF8_to_UTF8XML(ss_name.c_str()) << "\"" << endl;
}

int main (int argc, char *argv[]) {

	time_t dvb_loop_start;

	int fd, loop_time = 120, custom_sort = 0;
	bool parentalcontrol = false, custom_swap = false, fta = false, extra = true, placeholder = true;

	if (argv[1] != NULL) {
	 BAT_local = atoi(argv[1]);
	 if (argv[2] != NULL) {
	  BAT_local_region = atoi(argv[2]);
	  if (BAT_local_region == 0x21 || BAT_local_region == 0x32) {
	   BAT_merge = 0x1001;
	   BAT_merge_region = 0x7;
	  }
	  if (argv[3] != NULL) {
	   extra = atoi(argv[3]);
	   if (argv[4] != NULL) {
	    custom_sort = atoi(argv[4]);
	    if (argv[5] != NULL) {
	     parentalcontrol = atoi(argv[5]);
	     if (argv[6] != NULL) {
	      custom_swap = atoi(argv[6]);
	      if (argv[7] != NULL) {
	       fta = atoi(argv[7]);
	       if (argv[8] != NULL) {
	        placeholder = atoi(argv[8]);
	}}}}}}}}
/*
	// Make Hex Editor Hackable so they dont need to recompile for changes ;-)
	string SkySportsActive1 = "1381"; unsigned short ssa1 = atoi(SkySportsActive1.c_str());
	string SkySportsActive2 = "1387"; unsigned short ssa2 = atoi(SkySportsActive2.c_str());
	string SkySportsActive3 = "1470"; unsigned short ssa3 = atoi(SkySportsActive3.c_str());
	string SkySportsActive4 = "1486"; unsigned short ssa4 = atoi(SkySportsActive4.c_str());
	string BBCInteractive1 = "2051"; unsigned short bbci1 = atoi(BBCInteractive1.c_str());
	string BBCInteractive2 = "2052"; unsigned short bbci2 = atoi(BBCInteractive2.c_str());
	string BBCInteractive3 = "3542"; unsigned short bbci3 = atoi(BBCInteractive3.c_str());
	string BBCInteractive4 = "3545"; unsigned short bbci4 = atoi(BBCInteractive4.c_str());
*/
	string SkyAnytime1 = "4094"; unsigned short any1 = atoi(SkyAnytime1.c_str());
	string SkyAnytime2 = "4099"; unsigned short any2 = atoi(SkyAnytime2.c_str());

	fd = si_open(0x11);

	dvb_loop_start = time(NULL);

	while(si_read_bouquets(fd) < 1)
	{
		if (time(NULL) > dvb_loop_start + loop_time)
		{
			printf("[AutoBouquetsReader] read bouquets timeout! %i seconds\n", loop_time);
			si_close(fd);
			return -1;
		}
	}

	si_close(fd);

	fd = si_open(0x10);

	dvb_loop_start = time(NULL);

	while(si_read_network(fd) < 1)
	{
		if (time(NULL) > dvb_loop_start + loop_time)
		{
			printf("[AutoBouquetsReader] read network timeout! %i seconds\n", loop_time);
			si_close(fd);
			return -1;
		}
	}

	si_close(fd);

	pair<map<string, channel_t>::iterator,bool> ret;

	for( map<string, channel_t>::iterator ii = BAT2.begin(); ii != BAT2.end(); ++ii )
	{
		bool foundskyid = false;
		for( map<string, channel_t>::iterator i = BAT1.begin(); i != BAT1.end(); ++i )
		{
			if ( (*i).second.skyid == (*ii).second.skyid )
			{
				foundskyid = true;
				break;
			}
		}

		if ( !foundskyid )
		{
			ret = BAT1.insert ( pair<string, channel_t>((*ii).first,channel) );
			if (ret.second == true)
				BAT1[(*ii).first] = BAT2[(*ii).first];
		}
	}

	BAT2.clear();

	// remove duplicates from bat3 that are already in merged bat1
	for( map<string, channel_t>::iterator i = BAT1.begin(); i != BAT1.end(); ++i )
	{
		if (BAT3.find((*i).first) != BAT3.end())
			BAT3.erase ((*i).first);
	}

	// map remaining BAT3 services into TEST and DATA
	for( map<string, channel_t>::iterator ii = BAT3.begin(); ii != BAT3.end(); ++ii )
	{
		string sid = to_string<string>((*ii).second.sid, hex);
		if ((fta == true && SDT[sid].ca == "FTA") || fta == false)
		{
			unsigned short skyid = atoi((*ii).second.skyid.c_str());

			// hack to re-assign channel numbers for duplicate regionals in transmitted TEST bouquet services
			switch (skyid)
			{
				case 131: // ITV +1
					(*ii).second.skyid = "69";
					break;
			}

			if (skyid != 65535)
			{
				bool foundskyid = false;
				for( map<string, channel_t>::iterator i = BAT1.begin(); i != BAT1.end(); ++i )
				{
					if ( (*i).second.skyid == (*ii).second.skyid )
					{
						foundskyid = true;
						break;
					}
				}

				if ((!foundskyid ) && (skyid > 100 && skyid < 1000))
				{
					ret = BAT1.insert ( pair<string, channel_t>((*ii).first,channel) );
					if (ret.second == true)
						BAT1[(*ii).first] = BAT3[(*ii).first];
				}
				else
				{
					// hack to re-assign channel numbers to 9k+ for duplicate regionals in transmitted TEST bouquet services
					string test_id = (*ii).second.skyid;
					if (skyid > 99 && skyid < 1000)
						test_id = ("9"+(*ii).second.skyid);

					TEST[test_id] = BAT3[(*ii).first];
					TEST[test_id].skyid = (*ii).first;
					TEST[test_id].ca       = SDT[sid].ca;
					TEST[test_id].provider = SDT[sid].provider;
					TEST[test_id].name     = SDT[sid].name;
					TEST[test_id].nspace   = ((*ii).second.tsid == "7e3" ? "11a2f26" : "11a0000");
				}
			}
			else // detect unassigned skyid as DATA
			{
				DATA[(*ii).first] = BAT3[(*ii).first];
				DATA[(*ii).first].ca       = SDT[sid].ca;
				DATA[(*ii).first].provider = SDT[sid].provider;
				DATA[(*ii).first].name     = SDT[sid].name;
				DATA[(*ii).first].nspace   = ((*ii).second.tsid == "7e3" ? "11a2f26" : "11a0000");
			}
		}
	}

	BAT3.clear();

	// add sdt data to merged bat1
	for( map<string, channel_t>::iterator i = BAT1.begin(); i != BAT1.end(); ++i )
	{
		unsigned short skyid = atoi((*i).second.skyid.c_str());
		string sid = to_string<string>((*i).second.sid, hex);

		if ((fta == true && SDT[sid].ca == "FTA") || fta == false)
		{
			BAT1[(*i).first].ca       = SDT[sid].ca;
			BAT1[(*i).first].provider = SDT[sid].provider;
			BAT1[(*i).first].name     = SDT[sid].name;
			BAT1[(*i).first].nspace   = ((*i).second.tsid == "7e3" ? "11a2f26" : "11a0000");
		}

		if (parentalcontrol && ((skyid > 860 && skyid < 881) || (skyid > 899 && skyid < 950)))
			SDT.erase(sid);
	}

	string datfile;
	datfile = "/tmp/autobouquets.csv";
	ofstream database_csv;
	database_csv.open (datfile.c_str());
//	database_csv << "\"POSITION\",\"EPG_ID\",\"TYPE\",\"SID\",\"TSID\",\"ENCRYPTION\",\"NAME\"" << endl;
	database_csv << "# autobouquets.csv master channels x-ref file for C16.0 by LraiZer @ ukcvs.net" << endl;
	database_csv << "# SYNTAX: Sky Ch Slot,EPG_ID,Type,Sid,TSid,Encrypted? (N=FTA Y=NDS),\"Channel Name\"" << endl;

	for( map<string, channel_t>::iterator i = BAT1.begin(); i != BAT1.end(); ++i )
	{
		if ((fta == true && (*i).second.ca == "FTA") || fta == false)
		{
			unsigned short skyid = atoi((*i).second.skyid.c_str());

			if (skyid > 100 && skyid < 1000)
			{
				TV[(*i).second.skyid] = BAT1[(*i).first];
				TV[(*i).second.skyid].skyid = (*i).first;
			}
			else if (skyid > 3100 && skyid < 4000)
			{
				RADIO[(*i).second.skyid] = BAT1[(*i).first];
				RADIO[(*i).second.skyid].skyid = (*i).first;
			}
			else if (skyid == 65535)
				DATA[(*i).first] = BAT1[(*i).first];
			else
			{
				TEST[(*i).second.skyid] = BAT1[(*i).first];
				TEST[(*i).second.skyid].skyid = (*i).first;
			}
		}
	}

	BAT1.clear();

	ofstream bq_22 ("/tmp/bouquet.22");	// placeholder
	ofstream bq_01 ("/tmp/bouquet.01");	// Entertainment
	ofstream bq_02 ("/tmp/bouquet.02");	// Lifestyle-Culture
	ofstream bq_03 ("/tmp/bouquet.03");	// Movies
	ofstream bq_04 ("/tmp/bouquet.04");	// Music
	ofstream bq_05 ("/tmp/bouquet.05");	// Sport
	ofstream bq_06 ("/tmp/bouquet.06");	// News
	ofstream bq_07 ("/tmp/bouquet.07");	// Factual
	ofstream bq_08 ("/tmp/bouquet.08");	// Religious
	ofstream bq_09 ("/tmp/bouquet.09");	// Kids
	ofstream bq_0a ("/tmp/bouquet.0a");	// Shopping
	ofstream bq_0b ("/tmp/bouquet.0b");	// Sky Box Office
	ofstream bq_0c ("/tmp/bouquet.0c");	// International
	ofstream bq_0d ("/tmp/bouquet.0d");	// BT Sport Interactive
	ofstream bq_0e ("/tmp/bouquet.0e");	// Specialist
	ofstream bq_0f ("/tmp/bouquet.0f");	// Adults only!
	ofstream bq_10 ("/tmp/bouquet.10");	// Other
	ofstream bq_11 ("/tmp/bouquet.11");	// My Bouquet 1
	ofstream bq_12 ("/tmp/bouquet.12");	// My Bouquet 2
	ofstream bq_13 ("/tmp/bouquet.13");	// My Bouquet 3
	ofstream bq_14 ("/tmp/bouquet.14");	// Regional
	ofstream bq_15 ("/tmp/bouquet.15");	// Plus1
	ofstream bq_16 ("/tmp/bouquet.16");	// BBC Interactive
	ofstream bq_17 ("/tmp/bouquet.17");	// Sky Sports Interactive
	ofstream bq_19 ("/tmp/bouquet.19");	// FTA
	ofstream f_epg ("/tmp/channels.cfg");


	ifstream supplement_file("supplement.txt");

	if (supplement_file.is_open())
	{
		unsigned int pre, index, skyid;
		string dat_line, epg_id;
		bool is_skyid, is_epgid, is_type, is_sid, is_tsid, is_nspace, is_provider, is_ca, is_name, is_complete;
		bool logging = false;
		ofstream autobouquets_log;

		if (FILE *file = fopen("/var/etc/.logging", "r"))
		{
			fclose(file);
			logging = true;
			autobouquets_log.open("/tmp/autobouquets.log");
		}

		while (!supplement_file.eof() && getline(supplement_file, dat_line))
		{
			if (dat_line.length() > 0 && dat_line[0] != '#')
			{
				index = dat_line.find(':');
				channel.skyid = dat_line.substr(0, index);
				skyid = atoi(channel.skyid.c_str());
				is_skyid = (index == 0) ? false : true;

				pre = index + 1;
				index = dat_line.find(':', pre);
				epg_id = dat_line.substr(pre, index-pre);
				is_epgid = (index == pre) ? false : true;

				pre = index + 1;
				index = dat_line.find(':', pre);
				channel.type = dat_line.substr(pre, index-pre);
				is_type = (index == pre) ? false : true;

				pre = index + 1;
				index = dat_line.find(':', pre);
				channel.sid = dat_line.substr(pre, index-pre);
				is_sid = (index == pre) ? false : true;

				pre = index + 1;
				index = dat_line.find(':', pre);
				channel.tsid = dat_line.substr(pre, index-pre);
				is_tsid = (index == pre) ? false : true;

				pre = index + 1;
				index = dat_line.find(':', pre);
				channel.nspace = dat_line.substr(pre, index-pre);
				is_nspace = (index == pre) ? false : true;

				pre = index + 1;
				index = dat_line.find(':', pre);
				channel.provider = dat_line.substr(pre, index-pre);
				is_provider = (index == pre) ? false : true;

				pre = index + 1;
				index = dat_line.find(':', pre);
				channel.ca = dat_line.substr(pre, index-pre);
				is_ca = (index == pre) ? false : true;

				pre = index + 1;
				channel.name = dat_line.substr(pre);
				is_name = (dat_line.length() == pre) ? false : true;

				is_complete = (is_skyid && is_epgid && is_type && is_sid && is_tsid && is_nspace && is_provider && is_ca && is_name) ? true : false;

				if (is_complete)
				{
					if (logging) autobouquets_log << "NEW CHANNEL: ";

					if ((skyid > 100) && (skyid < 1000))
					{
						TV[channel.skyid] = channel;
						TV[channel.skyid].skyid = epg_id;
						if (logging) autobouquets_log << "TV" << endl;
					}
					else if ((skyid > 3100) && (skyid < 4000))
					{
						RADIO[channel.skyid] = channel;
						RADIO[channel.skyid].skyid = epg_id;
						if (logging) autobouquets_log << "RADIO" << endl;
					}
					else if (skyid == 0xffff)
					{
						DATA[epg_id] = channel;
						if (logging) autobouquets_log << "DATA" << endl;
					}
					else
					{
						TEST[channel.skyid] = channel;
						TEST[channel.skyid].skyid = epg_id;
						if (logging) autobouquets_log << "TEST" << endl;
					}

					SDT[channel.sid] = channel;

					if (logging)
					{
						autobouquets_log << channel.skyid << ":" << epg_id << ":" << channel.type << ":"
						<< channel.sid << ":" << channel.tsid << ":" << channel.nspace << ":"
						<< channel.provider << ":" << channel.ca << ":" << channel.name << endl << endl;
					}
				}
				else if (is_epgid)
				{
					for( map<string, channel_t>::iterator i = TV.begin(); i != TV.end(); ++i )
					{
						if ((*i).second.skyid == epg_id)
						{
							if (logging)
							{
								autobouquets_log << (*i).first  << ":" << (*i).second.skyid << ":" << (*i).second.type << ":"
								<< (*i).second.sid << ":" << (*i).second.tsid << ":" << (*i).second.nspace << ":"
								<< (*i).second.provider << ":" << (*i).second.ca << ":" << (*i).second.name << endl;
							}

							if (is_sid)
								(*i).second.sid = channel.sid;
							else
								channel.sid = (*i).second.sid;

							if (is_name)     { (*i).second.name = channel.name; SDT[channel.sid].name = channel.name; }
							if (is_type)     { (*i).second.type = channel.type; SDT[channel.sid].type = channel.type; }
							if (is_tsid)     { (*i).second.tsid = channel.tsid; SDT[channel.sid].tsid = channel.tsid; }
							if (is_nspace)   (*i).second.nspace = channel.nspace;
							if (is_provider) (*i).second.provider = channel.provider;
							if (is_ca)       (*i).second.ca = channel.ca;

							if (logging)
							{
								autobouquets_log << (*i).first  << ":" << (*i).second.skyid << ":" << (*i).second.type << ":"
								<< (*i).second.sid << ":" << (*i).second.tsid << ":" << (*i).second.nspace << ":"
								<< (*i).second.provider << ":" << (*i).second.ca << ":" << (*i).second.name << endl << endl;
							}
						}
					}
					for( map<string, channel_t>::iterator i = TEST.begin(); i != TEST.end(); ++i )
					{
						if ((*i).second.skyid == epg_id)
						{
							if (logging)
							{
								autobouquets_log << (*i).first  << ":" << (*i).second.skyid << ":" << (*i).second.type << ":"
								<< (*i).second.sid << ":" << (*i).second.tsid << ":" << (*i).second.nspace << ":"
								<< (*i).second.provider << ":" << (*i).second.ca << ":" << (*i).second.name << endl;
							}

							if (is_sid)
								(*i).second.sid = channel.sid;
							else
								channel.sid = (*i).second.sid;

							if (is_name)     { (*i).second.name = channel.name; SDT[channel.sid].name = channel.name; }
							if (is_type)     { (*i).second.type = channel.type; SDT[channel.sid].type = channel.type; }
							if (is_tsid)     { (*i).second.tsid = channel.tsid; SDT[channel.sid].tsid = channel.tsid; }
							if (is_nspace)   (*i).second.nspace = channel.nspace;
							if (is_provider) (*i).second.provider = channel.provider;
							if (is_ca)       (*i).second.ca = channel.ca;

							if (logging)
							{
								autobouquets_log << (*i).first  << ":" << (*i).second.skyid << ":" << (*i).second.type << ":"
								<< (*i).second.sid << ":" << (*i).second.tsid << ":" << (*i).second.nspace << ":"
								<< (*i).second.provider << ":" << (*i).second.ca << ":" << (*i).second.name << endl << endl;
							}
						}
					}
					if (DATA.find(epg_id) != DATA.end())
					{
						if (logging)
						{
							autobouquets_log << DATA[epg_id].skyid << ":" << epg_id << ":" << DATA[epg_id].type << ":"
							<< DATA[epg_id].sid << ":" << DATA[epg_id].tsid << ":" << DATA[epg_id].nspace << ":"
							<< DATA[epg_id].provider << ":" << DATA[epg_id].ca << ":" << DATA[epg_id].name << endl;
						}

						if (is_sid)
							DATA[epg_id].sid = channel.sid;
						else
							channel.sid = DATA[epg_id].sid;

						if (is_name)     { DATA[epg_id].name = channel.name; SDT[channel.sid].name = channel.name; }
						if (is_type)     { DATA[epg_id].type = channel.type; SDT[channel.sid].type = channel.type; }
						if (is_tsid)     { DATA[epg_id].tsid = channel.tsid; SDT[channel.sid].tsid = channel.tsid; }
						if (is_nspace)   DATA[epg_id].nspace = channel.nspace;
						if (is_provider) DATA[epg_id].provider = channel.provider;
						if (is_ca)       DATA[epg_id].ca = channel.ca;

						if (logging)
						{
							autobouquets_log << DATA[epg_id].skyid << ":" << epg_id << ":" << DATA[epg_id].type << ":"
							<< DATA[epg_id].sid << ":" << DATA[epg_id].tsid << ":" << DATA[epg_id].nspace << ":"
							<< DATA[epg_id].provider << ":" << DATA[epg_id].ca << ":" << DATA[epg_id].name << endl << endl;
						}
					}
				}
			}
		}
		if (logging) autobouquets_log.close();
	}
	supplement_file.close();

	ofstream dat_nit;
	dat_nit.open ("/tmp/services.xml");

	dat_nit << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	dat_nit << "<!--" << endl;
	dat_nit << "  This file was automatically generated by an AutoBouquets N1 scan. It" << endl;
	dat_nit << "  will be overwritten by future scans. Therefore, it is NOT advisable to" << endl;
	dat_nit << "  make manual changes to this file! For entries to be added or renamed," << endl;
	dat_nit << "  please edit the helpful /var/etc/supplement.txt file!" << endl;
//	dat_nit << "  or renamed, use the myservices.xml file instead," << endl;
//	dat_nit << "  see http://wiki.tuxbox.org/Neutrino:Senderlisten#myservices.xml." << endl;
	dat_nit << "-->" << endl;
	dat_nit << "<zapit>" << endl;
	dat_nit << "\t<sat name=\"Astra 28.2E\" diseqc=\"0\">" << endl;

	for( map<string, transport_t>::iterator i = NIT.begin(); i != NIT.end(); ++i )
	{
		dat_nit << "\t\t<transponder id=\"";
		dat_nit << hex << right;
		dat_nit << setw(4) << setfill('0') << (*i).first << "\" onid=\"";
		dat_nit << setw(4) << setfill('0') << (*i).second.original_network_id << "\" frequency=\"";
		dat_nit << dec << left;
		dat_nit << setw(8) << setfill('0') << (*i).second.frequency << "\" inversion=\"2\" symbol_rate=\"";
		dat_nit << setw(8) << setfill('0') << (*i).second.symbol_rate << "\" fec_inner=\"";
		dat_nit << hex;
		dat_nit << (*i).second.fec_inner << "\" polarization=\"" << (*i).second.polarization << "\">";

		for( map<string, channel_t>::iterator ii = SDT.begin(); ii != SDT.end(); ++ii )
		{
			if (((*ii).second.tsid == (*i).first) && ( atoi((*ii).second.type.c_str()) != 19 && atoi((*ii).second.type.c_str()) != 87 ))
			{
				if ((fta == true && (*ii).second.ca == "FTA") || fta == false)
				{
					if (!(parentalcontrol && atoi((*ii).second.type.c_str()) == 4))
					{
						dat_nit << endl << "\t\t\t<channel service_id=\"";
						dat_nit << hex << right;
						dat_nit << setw(4) << setfill('0') << (*ii).first << "\" name=\"" << UTF8_to_UTF8XML((*ii).second.name.c_str()) << "\" service_type=\"";
						dat_nit << hex << right;
/*
						if ((*ii).second.name == "EPG Background Audio.")
							dat_nit << "02\"/>";
						else if (((*ii).second.name == "Chl Line-up") || ((*ii).second.name == "Sports Active") || ((*ii).second.type == "0"))
							dat_nit << "01\"/>";
						else
*/							dat_nit << setw(2) << setfill('0') << (*ii).second.type << "\"/>";
					}
				}
			}
		}
		dat_nit << endl << "\t\t</transponder>" << endl;
	}
	dat_nit << "\t\t<transponder id=\"0000\" onid=\"0000\" frequency=\"00000000\" inversion=\"0\" symbol_rate=\"00000000\" fec_inner=\"0\" polarization=\"0\">" << endl;

	NIT.clear();
	SDT.clear();


	ifstream local_swap_file("local_swap");

	if (local_swap_file.is_open())
	{
		string dat_line;
		while (!local_swap_file.eof() && getline(local_swap_file, dat_line))
		{
			if (dat_line.length() > 0 && dat_line[0] != '#')
			{
				bool swap_found = true;
				int index = dat_line.find('=');

				string swap1 = dat_line.substr(0, index);
				string swap2 = dat_line.substr(index + 1);
				int swap1_pos = atoi(swap1.c_str());
				int swap2_pos = atoi(swap2.c_str());

				if (swap1 != swap2)
				{
					if ((swap1_pos > 100) && (swap1_pos < 1000))
					{
						if (TV.find(swap1) != TV.end())
							channel = TV[swap1];
						else
							swap_found = false;
					}
					else
					{
						if (TEST.find(swap1) != TEST.end())
							channel = TEST[swap1];
						else
							swap_found = false;
					}

					if ((swap2_pos > 100) && (swap2_pos < 1000))
					{
						if (TV.find(swap2) != TV.end())
						{
							if ((swap1_pos > 100) && (swap1_pos < 1000))
								TV[swap1] = TV[swap2];
							else
								TEST[swap1] = TV[swap2];
						}
					}
					else
					{
						if (TEST.find(swap2) != TEST.end())
						{
							if ((swap1_pos > 100) && (swap1_pos < 1000))
								TV[swap1] = TEST[swap2];
							else
								TEST[swap1] = TEST[swap2];
						}
					}

					if (swap_found)
					{
						if ((swap2_pos > 100) && (swap2_pos < 1000))
							TV[swap2] = channel;
						else
							TEST[swap2] = channel;
					}
				}
			}
		}
	}

	local_swap_file.close();

	if (custom_swap)
	{
		ifstream custom_swap_file("custom_swap.txt");

		if (custom_swap_file.is_open())
		{
			string dat_line;
			while (!custom_swap_file.eof() && getline(custom_swap_file, dat_line))
			{
				if (dat_line.length() > 0 && dat_line[0] != '#')
				{
					bool swap_found = true;
					int index = dat_line.find('=');

					string swap1 = dat_line.substr(0, index);
					string swap2 = dat_line.substr(index + 1);
					int swap1_pos = atoi(swap1.c_str());
					int swap2_pos = atoi(swap2.c_str());

					if (swap1 != swap2)
					{
						if ((swap1_pos > 100) && (swap1_pos < 1000))
						{
							if (TV.find(swap1) != TV.end())
								channel = TV[swap1];
							else
								swap_found = false;
						}
						else
						{
							if (TEST.find(swap1) != TEST.end())
								channel = TEST[swap1];
							else
								swap_found = false;
						}

						if ((swap2_pos > 100) && (swap2_pos < 1000))
						{
							if (TV.find(swap2) != TV.end())
							{
								if ((swap1_pos > 100) && (swap1_pos < 1000))
									TV[swap1] = TV[swap2];
								else
									TEST[swap1] = TV[swap2];
							}
						}
						else
						{
							if (TEST.find(swap2) != TEST.end())
							{
								if ((swap1_pos > 100) && (swap1_pos < 1000))
									TV[swap1] = TEST[swap2];
								else
									TEST[swap1] = TEST[swap2];
							}
						}

						if (swap_found)
						{
							if ((swap2_pos > 100) && (swap2_pos < 1000))
								TV[swap2] = channel;
							else
								TEST[swap2] = channel;
						}
					}
				}
			}
		}

		custom_swap_file.close();
	}

	ifstream custom_epg("custom_epg.txt");

	if (custom_epg.is_open())
	{
		unsigned int pre, index;
		string dat_line, epg_id;

		while (!custom_epg.eof() && getline(custom_epg, dat_line))
		{
			if (dat_line.length() > 0 && dat_line[0] != '#')
			{
				index = dat_line.find(':');
				epg_id = dat_line.substr(0, index);

				pre = index + 1;
				index = dat_line.find(':', pre);
				epg.enabled = atoi(dat_line.substr(pre, index-pre).c_str());

				pre = index + 1;
				index = dat_line.find(':', pre);
				epg.days = atoi(dat_line.substr(pre, index-pre).c_str());

				pre = index + 1;
				index = dat_line.find(':', pre);
				epg.desc_hours = atoi(dat_line.substr(pre, index-pre).c_str());

				EPG[epg_id] = epg;
			}
		}
	}
	else
	{
		unsigned short count = 101;
		for ( ; count < 1000; count++ )
		{
			epg.enabled = 0;
			epg.days = 1;
			epg.desc_hours = 18;
			EPG[to_string<unsigned short>(count, dec)] = epg;
		}
	}
	custom_epg.close();

	if (custom_sort > 0)
	{
		char sort_file[256];
		memset(sort_file, '\0', 256);
		sprintf(sort_file, "custom_sort_%i.txt", custom_sort);
		ifstream custom_sort_file(sort_file);

		if (custom_sort_file.is_open())
		{
			string dat_line;
			while (!custom_sort_file.eof() && getline(custom_sort_file, dat_line))
			{
				if (dat_line.length() > 0 && dat_line[0] != '#')
				{
					int index = dat_line.find('=');
					string sort_bouquet = dat_line.substr(0, index);
					string sort_skyid = dat_line.substr(index + 1);
					int skyid = atoi(sort_skyid.c_str());
					bool found_sort_skyid = false;

					if ( TV.count(sort_skyid) > 0 )
					{
						channel = TV[sort_skyid];
						found_sort_skyid = true;
					}
					else if ( TEST.count(sort_skyid) > 0 )
					{
						channel = TEST[sort_skyid];
						found_sort_skyid = true;
					}
					else if ( RADIO.count(sort_skyid) > 0 )
					{
						channel = RADIO[sort_skyid];
						found_sort_skyid = true;
					}
					else if ( skyid > 0xffff )
					{
						sort_skyid = to_string<int>((skyid - 0xffff), dec);
						if ( DATA.count(sort_skyid) > 0 )
						{
							channel = DATA[sort_skyid];
							found_sort_skyid = true;
						}
					}

					if (found_sort_skyid)
					{
						if ( atoi(channel.type.c_str()) != 19 && atoi(channel.type.c_str()) != 87 )
						{
							if ((fta == true && channel.ca == "FTA") || fta == false)
							{
								if (!(parentalcontrol && ((skyid > 860 && skyid < 881) || (skyid > 899 && skyid < 950))))
								{
									     if ( sort_bouquet == "01" ) write_bouquet_service(bq_01, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "02" ) write_bouquet_service(bq_02, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "03" ) write_bouquet_service(bq_03, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "04" ) write_bouquet_service(bq_04, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "05" ) write_bouquet_service(bq_05, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "06" ) write_bouquet_service(bq_06, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "07" ) write_bouquet_service(bq_07, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "08" ) write_bouquet_service(bq_08, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "09" ) write_bouquet_service(bq_09, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "0a" ) write_bouquet_service(bq_0a, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "0b" ) write_bouquet_service(bq_0b, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "0c" ) write_bouquet_service(bq_0c, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "0d" ) write_bouquet_service(bq_0d, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "0e" ) write_bouquet_service(bq_0e, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "0f" ) write_bouquet_service(bq_0f, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "10" ) write_bouquet_service(bq_10, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "11" ) write_bouquet_service(bq_11, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "12" ) write_bouquet_service(bq_12, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "13" ) write_bouquet_service(bq_13, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "14" ) write_bouquet_service(bq_14, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "15" ) write_bouquet_service(bq_15, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "16" ) write_bouquet_service(bq_16, channel.sid, channel.name, channel.tsid);
									else if ( sort_bouquet == "17" ) write_bouquet_service(bq_17, channel.sid, channel.name, channel.tsid);

									f_epg << EPG[sort_skyid].enabled << ";" << channel.skyid << ";";
									f_epg << hex << right << setw(4) << setfill('0') << channel.sid << ":";
									f_epg << hex << right << setw(4) << setfill('0') << channel.tsid << ":0002:";
									f_epg << dec << right << setw(3) << setfill('0') << EPG[sort_skyid].desc_hours << ";";
									f_epg << EPG[sort_skyid].days << ";" << sort_skyid << " " << UTF8_to_UTF8XML(channel.name.c_str()) << endl;
								}
							}
						}
					}
				}
			}
		}

		custom_sort_file.close();
	}

	bq_11.close(); bq_12.close(); bq_13.close(); bq_14.close(); bq_15.close();
//	bq_11.close(); bq_12.close(); bq_13.close(); bq_14.close(); bq_15.close(); bq_16.close(); bq_17.close();
	if (placeholder)
	{
		for ( int count = 1; count < 101; count++ )
			write_bouquet_service_placeholder(bq_22, dat_nit, count);
	}

	int count = 101;

	for( map<string, channel_t>::iterator i = TV.begin(); i != TV.end(); ++i )
	{
		unsigned short skyid = atoi((*i).first.c_str());
		bool parentalguidance = false;

		if (placeholder)
		{
			while (count < skyid)
			{
				write_bouquet_service_placeholder(bq_22, dat_nit, count);
				count++;
			}
			count = skyid;
		}

		parentalguidance = (( skyid > 860 && skyid < 881 ) || ( skyid > 899 && skyid < 950 )) ? true : false;

		if ( atoi((*i).second.type.c_str()) != 19 && atoi((*i).second.type.c_str()) != 87 )
		{
			if ((fta == true && (*i).second.ca == "FTA") || fta == false)
			{
				if (placeholder)
				{
					if (parentalguidance && parentalcontrol)
						write_bouquet_service_placeholder(bq_22, dat_nit, skyid);
					else
						write_bouquet_service_numbered(bq_22, (*i).second.sid, (*i).second.name, (*i).second.tsid, skyid);

					count++;
				}

				if (custom_sort == 0)
				{
					if ( skyid > 100 && skyid < 299 ) write_bouquet_service(bq_01, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 298 && skyid < 301 ) write_bouquet_service(bq_02, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 300 && skyid < 350 ) write_bouquet_service(bq_03, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 349 && skyid < 401 ) write_bouquet_service(bq_04, (*i).second.sid, (*i).second.name, (*i).second.tsid);
//					else if ( skyid > 400 && skyid < 501 ) write_bouquet_service(bq_05, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 400 && skyid < 470 ) write_bouquet_service(bq_05, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 500 && skyid < 520 ) write_bouquet_service(bq_06, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 519 && skyid < 580 ) write_bouquet_service(bq_07, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 579 && skyid < 601 ) write_bouquet_service(bq_08, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 600 && skyid < 650 ) write_bouquet_service(bq_09, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 649 && skyid < 700 ) write_bouquet_service(bq_0a, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 699 && skyid < 780 ) write_bouquet_service(bq_0b, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 779 && skyid < 861 ) write_bouquet_service(bq_0c, (*i).second.sid, (*i).second.name, (*i).second.tsid);
/*					else if ( skyid > 860 && skyid < 881 )
					{
						if (!parentalcontrol)
							write_bouquet_service(bq_0d, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					}
*/
					else if ( skyid > 469 && skyid < 501 ) write_bouquet_service(bq_0d, (*i).second.sid, (*i).second.name, (*i).second.tsid);
//					else if ( skyid > 880 && skyid < 900 ) write_bouquet_service(bq_0e, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 985 && skyid < 1000 ) write_bouquet_service(bq_0e, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					else if ( skyid > 860 && skyid < 950 )
					{
						if (!parentalcontrol)
							write_bouquet_service(bq_0f, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					}
					else
						write_bouquet_service(bq_10, (*i).second.sid, (*i).second.name, (*i).second.tsid);

					if (!(parentalguidance && parentalcontrol))
					{
						f_epg << EPG[(*i).first].enabled << ";" << TV[(*i).first].skyid << ";";
						f_epg << hex << right << setw(4) << setfill('0') << TV[(*i).first].sid << ":";
						f_epg << hex << right << setw(4) << setfill('0') << TV[(*i).first].tsid << ":0002:";
						f_epg << dec << right << setw(3) << setfill('0') << EPG[(*i).first].desc_hours << ";";
						f_epg << EPG[(*i).first].days << ";" << (*i).first << " " << UTF8_to_UTF8XML(TV[(*i).first].name.c_str()) << endl;
					}
				}
			}

			if ( ((*i).second.ca == "FTA") && !(parentalguidance && parentalcontrol) )
				write_bouquet_service(bq_19, (*i).second.sid, (*i).second.name, (*i).second.tsid);
		}
		write_database_service(database_csv, skyid, (*i).second.skyid, (*i).second.type, (*i).second.sid, (*i).second.tsid, (*i).second.ca, (*i).second.name);
	}

	TV.clear();
	EPG.clear();

	if (placeholder)
	{
		while (count < 1001)
		{
			write_bouquet_service_placeholder(bq_22, dat_nit, count);
			count++;
		}
	}

	dat_nit << "\t\t</transponder>" << endl;
	dat_nit << "\t</sat>" << endl;
	dat_nit << "</zapit>" << endl;

	dat_nit.close();
	f_epg.close();

	bq_01.close(); bq_02.close(); bq_03.close(); bq_04.close();
	bq_05.close(); bq_06.close(); bq_07.close(); bq_08.close();
	bq_09.close(); bq_0a.close(); bq_0b.close(); bq_0c.close();
	bq_0d.close(); bq_0e.close(); bq_0f.close(); bq_16.close();
	bq_17.close(); bq_22.close();

//	ofstream bq_16; ofstream bq_17; ofstream bq_18;
	ofstream bq_18;

//	bq_16.open ("/tmp/bouquet.16");		// BBC RBs
//	bq_17.open ("/tmp/bouquet.17");		// interactive sports
	bq_18.open ("/tmp/bouquet.18");		// anytime

	for( map<string, channel_t>::iterator i = TEST.begin(); i != TEST.end(); ++i )
	{
		if (extra)
		{
			if ( atoi((*i).second.type.c_str()) != 19 && atoi((*i).second.type.c_str()) != 87 )
			{
				if ((fta == true && (*i).second.ca == "FTA") || fta == false)
				{
					if (!(parentalcontrol && atoi((*i).second.type.c_str()) == 4))
						write_bouquet_service(bq_10, (*i).second.sid, (*i).second.name, (*i).second.tsid);
				}
			}
		}
		write_database_service(database_csv, atoi((*i).first.c_str()), (*i).second.skyid, (*i).second.type, (*i).second.sid, (*i).second.tsid, (*i).second.ca, (*i).second.name);
	}

	TEST.clear();

	for( map<string, channel_t>::iterator i = DATA.begin(); i != DATA.end(); ++i )
	{
		if ( atoi((*i).second.type.c_str()) != 19 && atoi((*i).second.type.c_str()) != 87 )
		{
			if ((fta == true && (*i).second.ca == "FTA") || fta == false)
			{
				unsigned short skyid = atoi((*i).first.c_str());

/*				if (( skyid > (ssa1 -1) && skyid < ssa2 ) || ( skyid > ssa3 && skyid < ssa4 ))
				{
					channel.name = "Sky Sports Active ";
					write_bouquet_service_channel(bq_16, (*i).second.sid, channel.name, (*i).second.tsid, skyid);

					if ( (*i).second.ca == "FTA" )
						write_bouquet_service_channel(bq_19, (*i).second.sid, channel.name, (*i).second.tsid, skyid);
				}
				else if (( skyid > (bbci1 - 1) && skyid < bbci2 ) || ( skyid > bbci3 && skyid < bbci4 ))
				{
					channel.name = "BBCi ";
					write_bouquet_service_channel(bq_17, (*i).second.sid, channel.name, (*i).second.tsid, skyid);;

					if ( (*i).second.ca == "FTA" )
						write_bouquet_service_channel(bq_19, (*i).second.sid, channel.name, (*i).second.tsid, skyid);
				}
				else if ( skyid > (any1 - 1) && skyid < any2 )	*/
				if ( skyid > (any1 - 1) && skyid < any2 )
				{
					write_bouquet_service_channel(bq_18, (*i).second.sid, (*i).second.name, (*i).second.tsid, skyid);
					if ( (*i).second.ca == "FTA" )
						write_bouquet_service_channel(bq_19, (*i).second.sid, (*i).second.name, (*i).second.tsid, skyid);
				}
				else
				{	// write remainder of unassigned services to 'Other' bouquet if extra option is enabled
					if (extra)
					{
						if (!(parentalcontrol && atoi((*i).second.type.c_str()) == 4))
							write_bouquet_service(bq_10, (*i).second.sid, (*i).second.name, (*i).second.tsid);
					}
				}
			}
		}
		write_database_service(database_csv, atoi((*i).second.skyid.c_str()), (*i).first, (*i).second.type, (*i).second.sid, (*i).second.tsid, (*i).second.ca, (*i).second.name);
	}

//	bq_10.close(); bq_16.close(); bq_17.close(); bq_18.close(); bq_19.close();
	bq_10.close(); bq_18.close(); bq_19.close();

	ofstream bq_radio;
//	bq_radio.open ("/tmp/bouquet.radio");
	bq_radio.open ("/tmp/bouquet.20");

	DATA.clear();

	for( map<string, channel_t>::iterator i = RADIO.begin(); i != RADIO.end(); ++i )
	{
		if ( atoi((*i).second.type.c_str()) == 2 )
			write_bouquet_service(bq_radio, (*i).second.sid, (*i).second.name, (*i).second.tsid);

		write_database_service(database_csv, atoi((*i).first.c_str()), (*i).second.skyid, (*i).second.type, (*i).second.sid, (*i).second.tsid, (*i).second.ca, (*i).second.name);
	}

	bq_radio.close();
	database_csv.close();

	RADIO.clear();

	return 0;
}

