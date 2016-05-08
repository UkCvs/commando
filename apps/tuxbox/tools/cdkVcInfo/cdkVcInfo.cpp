/*
 * Tool for printing some image information during bootup.
 *
 * $Id: cdkVcInfo.cpp,v 1.11 2012/04/13 12:15:22 rhabarber1848 Exp $
 *
 * cdkVcInfo - d-box2 linux project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
*/

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define CONSOLE "/dev/vc/0"
#define VERSION_FILE "/.version"
#define VERSION_FILE2 "/proc/version"
#define INTERFACES_FILE "/etc/network/interfaces"
#define NAMENSSERVER_FILE "/etc/resolv.conf"
#define MOUNTS_FILE "/proc/mounts"
#define BUFFERSIZE 255
#define BIGBUFFERSIZE 2000
#define MAXOSD 2

enum {VERSION, TYPE, DATE, TIME, CREATOR, NAME, WWW, NETW, DHCP, ROOT, IP, NETM, BROAD, GATEWAY, DNS, HEADLINE,
		UNKNOWN, ENABLED, DISABLED, INTERN, LINUX, GCC, UPC, LOAD };

char *info[][MAXOSD] = {
	{ "Image Version   :"	, "Image Version   :" },
	{ "Image Typ       :"	, "Image Type      :" },
	{ "Datum           :"	, "Creation Date   :" },
	{ "Uhrzeit         :"	, "Creation Time   :" },
	{ "Erstellt von    :"	, "Creator         :" },
	{ "Image Name      :"	, "Image Name      :" },
	{ "Homepage        :"	, "Homepage        :" },
	{ "Netzwerk Status :"	, "Network State   :" },
	{ "DHCP Status     :"	, "DHCP State      :" },
	{ "Root-Server     :"	, "Root-Server     :" },
	{ "IP Adresse      :"	, "IP Address      :" },
	{ "Netzmaske       :"	, "Subnet Mask     :" },
	{ "Broadcast       :"	, "Broadcast       :" },
	{ "Gateway         :"	, "Gateway         :" },
	{ "Nameserver      :"	, "Nameserver      :" },
	{ "-------- Netzwerkeinstellungen --------"	, "---------- Network Settings -----------" },
	{ "-- unbekannt --"		, "-- unknown --" },
	{ "aktiviert"	, "enabled"		},
	{ "deaktiviert"	, "disabled"	},
	{ "intern"	, "intern"	},
	{ "Linux Version"	, "Linux Version"	},
	{ "gcc Version"	, "gcc Version"	},
	{ "Erstellt mit dem Computer"	, "Built with the computer"	},
	{ "Lade"		, "Loading"		}
};

int main (int argc, char **argv)
{
	switch (fork()) {
		case -1:
				perror("[cdkVcInfo] fork");
				return -1;
		case 0:
				break;
		default:
				return 0;
	}

	if (setsid() < 0) {
		perror("[cdkVcInfo] setsid");
		return 1;
	}

	unsigned int id = 1;
	int opt = -2;
	char buf[BUFFERSIZE] = "";
	int release_type = -1;
	int imageversion = 0;
	int imagesubver = 0;
	int year = 9999;
	int month = 99;
	int day = 99;
	int hour = 99;
	int minute = 99;
	bool delay = false;
	int dhcp = 0;
	int nic_on = 0;
	char* imagetyp = "squashfs";
	char linuxversion[24] = "";
	char gccversion[24] = "";
	char userpc[24];
	char ladename[BUFFERSIZE] = "System";
	char creator[BUFFERSIZE];
	char imagename[BUFFERSIZE];
	char homepage[BUFFERSIZE];
	char root[BUFFERSIZE];	
	char address[BUFFERSIZE];
	char broadcast[BUFFERSIZE];
	char netmask[BUFFERSIZE];
	char nameserver[BUFFERSIZE];
	char gateway[BUFFERSIZE];
	char null[BUFFERSIZE] = "";
	char versioninfo[20];
	char cvs_revision[] = "$Revision: 1.11 $";
	sscanf(cvs_revision, "%*s %s", versioninfo);

	while ((opt = getopt(argc, argv, "hgdn:")) != -1)
	{
		switch (opt)
		{
			case 'h':
					if (argc < 3)
					{
						printf("cdkVcInfo - bootinfo on screen, v%s\n", versioninfo);
						printf("Usage: cdkVcInfo [-d] [-g] [-n name] [-h]\n");
						printf("\nPossible options:\n");
						printf("\t-h\t\tprint this usage information\n");
						printf("\t-g\t\tprint bootinfo in german\n");
						printf("\t-d\t\tdelay on\n");
						printf("\t-n name\t\tspecial output (e.g. -n Neutrino)\n");
						exit(0);
					}
					break;
			case 'g':
					id = 0;
					break;
			case 'd':
						delay = true;
					break;
			case 'n':
					strcpy(ladename, optarg);
					break;
			default:
					break;
		}
	}

	strcpy(creator, info[UNKNOWN][id]);
	strcpy(imagename, info[UNKNOWN][id]);
	strcpy(homepage, info[UNKNOWN][id]);
	strcpy(root, info[INTERN][id]);	
	strcpy(address, info[UNKNOWN][id]);
	strcpy(broadcast, info[UNKNOWN][id]);
	strcpy(netmask, info[UNKNOWN][id]);
	strcpy(nameserver, info[UNKNOWN][id]);
	strcpy(gateway, info[UNKNOWN][id]);

	FILE* fv1 = fopen(VERSION_FILE, "r");
	if (fv1)
	{
		while (fgets(buf, BUFFERSIZE, fv1)) {
			sscanf(buf, "version=%1d%1d%2d%4d%2d%2d%2d%2d", 
			&release_type, &imageversion, &imagesubver,
			&year, &month, &day, &hour, &minute);
			sscanf(buf, "creator=%[^\n]", (char *) &creator);
			sscanf(buf, "imagename=%[^\n]", (char *) &imagename);
			sscanf(buf, "homepage=%[^\n]", (char *) &homepage);
		}
		fclose(fv1);
	}

	FILE* fv2 = fopen(INTERFACES_FILE, "r");
	if (fv2)
	{
		while (fgets(buf, BUFFERSIZE, fv2)) {
			if (nic_on == 0) {
				if (sscanf(buf, "auto eth%[0]", (char *) &null)) {
					nic_on=1;
				}
			}
			if (sscanf(buf, "iface eth0 inet stati%[c]", (char *) &null)) {
				dhcp = 1;
			}
			else if (sscanf(buf, "iface eth0 inet dhc%[p]", (char *) &null)) {
				dhcp = 2;
			}
		}
		fclose(fv2);
	}

	FILE* fv3 = fopen(NAMENSSERVER_FILE, "r");
	if (fv3)
	{
		while (fgets(buf, BUFFERSIZE, fv3)) {
			sscanf(buf, "nameserver %[^\n]", (char *) &nameserver);
		}
		fclose(fv3);
	}

	FILE* fv4 = popen("/sbin/ifconfig eth0", "r");
	if (fv4)
	{
		while (fgets(buf, BUFFERSIZE, fv4)) {
			sscanf(buf, " inet addr:%s  Bcast:%s  Mask:%[^\n]", (char *) &address, (char *) &broadcast, (char *) &netmask);
		}
		pclose(fv4);
	}

	FILE* fv5 = popen("/sbin/route -n", "r");
	if (fv5)
	{
		fscanf(fv5, "%*[^\n]\n%*[^\n]\n%*[^\n]\n");
		while (fgets(buf, BUFFERSIZE, fv5)) {
			sscanf(buf, "%s %[0-9.]", (char *) &null, (char *) &gateway);
		}
		pclose(fv5);
	}

  FILE* fv6 = fopen(MOUNTS_FILE, "r"); //Root-Server IP ermitteln, falls yadd
  if (fv6) {
    while (fgets(buf, BUFFERSIZE, fv6)!=NULL) {
      sscanf(buf, "/dev/root / nfs rw,v2,rsize=4096,wsize=4096,hard,udp,nolock,addr= %s", (char *) &root);
    }
    fclose(fv6);
  }
  
  FILE* fv7 = fopen(VERSION_FILE2, "r"); //Versionsdatei (/proc/version) auswerten
  if (fv7) {
    while (fgets(buf, BUFFERSIZE, fv7)!=NULL) {
      sscanf(buf, "Linux version %s%s%s%s%s", (char *) &linuxversion, (char *) &userpc, (char *) &gccversion, (char *) &gccversion, (char *) &gccversion);
    }
    fclose(fv7);
  }
  
  FILE* fv8 = fopen(VERSION_FILE, "a"); //Versionsdatei (/.version) beschreibbar, dann jffs2
  if (fv8) {
  fclose(fv8);
  imagetyp = "jffs2";
  }

	char message2[BUFFERSIZE];
	strcpy(message2, "");
	if (delay)
		sprintf(message2, "%s %s .... ", info[LOAD][id], ladename);

	char message[BIGBUFFERSIZE];
	strcpy(message, "");
	sprintf(message,
		"\n\n\n\n"
		"\t\t    ---------- Image Information ----------\n\n"
		"\t\t    %s %d.%d\n"						//Image Version
		"\t\t    %s %s\n\n"							//Image Typ
		"\t\t    %s %02d.%02d.%d\n"					//Date
		"\t\t    %s %d:%02d\n"						//Time
		"\t\t    %s %s\n"							//Creator
		"\t\t    %s %s-%s\n"						//Image Name
		"\t\t    %s %s\n\n"							//Homepage
		"\t\t    %s\n\n"
		"\t\t    %s %s\n"							//Network state
		"\t\t    %s %s\n"						//DHCP state
		"\t\t    %s %s\n"						//Root-Server		
		"\t\t    %s %s\n"							//IP Adress
		"\t\t    %s %s\n"							//Subnet
		"\t\t    %s %s\n"							//Broadcast
		"\t\t    %s %s\n"							//Gateway
		"\t\t    %s %s\n\n"						//Nameserver
		"\t\t    %s (%s, %s %s\n "					//Linux Version, gcc Version
		"\t\t    %s %s\n\n"						//User, PC			
		"\t\t\t\t%s",
		info[VERSION][id], imageversion, imagesubver,
		info[TYPE][id], release_type == 0 ? "Release" 
						: release_type == 1 ? "Snapshot" 
						: release_type == 2 ? "Intern" 
						:	info[UNKNOWN][id],
		info[DATE][id], day, month, year,
		info[TIME][id], hour, minute,
		info[CREATOR][id], creator,
		info[NAME][id], imagename, imagetyp,
		info[WWW][id], homepage,
		info[HEADLINE][id],
		info[NETW][id], nic_on == 0 ? info[DISABLED][id] : nic_on == 1 ? info[ENABLED][id] : info[UNKNOWN][id],
		info[DHCP][id], dhcp == 1 ? info[DISABLED][id] : dhcp == 2 ? info[ENABLED][id] : info[UNKNOWN][id],
		info[ROOT][id], root,
		info[IP][id], address,
		info[NETM][id], netmask,
		info[BROAD][id], broadcast,
		info[GATEWAY][id], gateway,
		info[DNS][id], nameserver,
		info[LINUX][id], linuxversion,
		info[GCC][id], gccversion,
		info[UPC][id], userpc,
		message2);

	FILE *fb = fopen(CONSOLE, "w");
	if (fb == 0) {
		perror("[cdkVcInfo] fopen");
		exit(1);
	}

	if (delay)
	{
		for (unsigned int i = 0; i < strlen(message); i++) {
			fputc(message[i], fb);
			fflush(fb);
		}
	}
	else
	{
		sprintf(message2, "%s %s .... ", info[LOAD][id], ladename);
		for (unsigned int i = 0; i < strlen(message); i++) {
			fputc(message[i], fb);
		}
		for (unsigned int i = 0; i < strlen(message2); i++) {
			fputc(message2[i], fb);
			fflush(fb);
			usleep(20000);
		}
	}
	fclose(fb);
	exit(0);
}
