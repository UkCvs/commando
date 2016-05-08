/*
 * $Id: udpstreamts.c,v 1.4 2012/04/13 12:15:00 rhabarber1848 Exp $
 *
 * a lightweight videolan server replacement
 *
 * Copyright (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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
 * usage:
 * udpstreamts <ip-address> <udp-port> <pid> [<pid> ...]
 * 
 * e.g. "udpstreamts 192.168.1.7 1234 0x0 0x64 0xff 0x100" to
 * send a transport stream containing the channel prosieben.
 *
 * the pids used in the example are:
 * - its transponder's program association table (always zero),
 * - the channel's program map table (see pat)
 * - the channel's video pes (see pmt)
 * - the channel's audio pes (see pmt)
 *
 * these are required to make videolan client work.
 *
 */

#include <config.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <unistd.h>
#include <errno.h>

#if HAVE_DVB_API_VERSION < 3
	#include <ost/dmx.h>
	#define dmx_pes_filter_params dmxPesFilterParams
	#define pes_type pesType
	#ifdef HAVE_DREAMBOX_HARDWARE
		#define DVR "/dev/dvb/card0/dvr1"
		#define DMX "/dev/dvb/card0/demux1"
	#else
		#define DVR "/dev/dvb/card0/dvr0"
		#define DMX "/dev/dvb/card0/demux0"
	#endif
#else
	#include <linux/dvb/dmx.h>
	#define DVR "/dev/dvb/adapter0/dvr0"
	#define DMX "/dev/dvb/adapter0/demux0"
#endif

#define TS_SYNC_BYTE (0x47)
#define TS_PACKET_SIZE (188)
#define UDP_PACKET_SIZE (TS_PACKET_SIZE * 7)
#define READ_BUFFER_SIZE (UDP_PACKET_SIZE * 32)
#define MAX_RETRIES 10

#undef UDPSTREAMTS_DEBUG
#undef USE_SEND_RETRY


char exiting = 0;

static int
dmx_pid_filter_start(unsigned short pid)
{
	struct dmx_pes_filter_params flt;
	int fd;

	flt.pid = pid;
	flt.input = DMX_IN_FRONTEND;
	flt.output = DMX_OUT_TS_TAP;
	flt.pes_type = DMX_PES_OTHER;
	flt.flags = DMX_IMMEDIATE_START;

	if ((fd = open(DMX, O_RDWR)) < 0) {
		perror(DMX);
		return -1;
	}

	if (ioctl(fd, DMX_SET_PES_FILTER, &flt) < 0) {
		perror("DMX_SET_PES_FILTER");
		close(fd);
		return -1;
	}

	return fd;
}


static void
dmx_pid_filter_stop(int fd)
{
	if (fd > 0) {
		if (ioctl(fd, DMX_STOP) < 0)
			perror("DMX_STOP");

		close(fd);
	}
}


static int
write_packets(int fd, unsigned char *buf, int count, unsigned char *writebuf, int writebuf_size)
{
        unsigned int size;
        unsigned char *bp;
        ssize_t written;

	#ifdef USE_SEND_RETRY
	unsigned int retries=0;
	#endif

	if (buf[0] != TS_SYNC_BYTE) {
		/*
		 * What happen?
		 * Somebody set up us the bomb
		 */
		fprintf(stderr, "%s:%s: Internal Error!\n", __FILE__, __FUNCTION__);
		exit(EXIT_FAILURE);
	}

        /*
         * ensure, that there is always at least one complete
         * packet inside of the send buffer
         */
        while (writebuf_size + count >= UDP_PACKET_SIZE) {

                /*
                 * how many bytes are to be sent from the input buffer?
                 */
                size = UDP_PACKET_SIZE - writebuf_size;

                /*
                 * send buffer is not empty, so copy from
                 * input buffer to get a complete packet
                 */
                if (writebuf_size) {
                        memcpy(writebuf + writebuf_size, buf, size);
                        bp = writebuf;
                }

                /*
                 * if send buffer is empty, then do not memcopy,
                 * but send directly from input buffer
                 */
                else {
                        bp = buf;
                }

                /*
                 * write the packet, count the amount of really
                 * written bytes
                 */
                written = write(fd, bp, UDP_PACKET_SIZE);

		#ifdef USE_SEND_RETRY
		/*
		* retry to send
		*/
                retries = 0;
		while((written == -1) && (retries < MAX_RETRIES)){
			retries++;
			fprintf(stderr,"Error writing to socket, still trying..(%i/%i);",retries,MAX_RETRIES);
			perror(" Reason");
			written = write(fd, bp, UDP_PACKET_SIZE);
		}
		#endif


                /*
                 * exit on error
                 */
                if (written == -1) {

			perror("Error writing to socket, giving up");
                        return -1;

		}

                /*
                 * if the packet could not be written completely, then
                 * how many bytes must be stored in the send buffer
                 * until the next packet is to be sent?
                 */
                writebuf_size = UDP_PACKET_SIZE - written;

                /*
                 * move all bytes of the packet which were not sent
                 * to the beginning of the send buffer
                 */
                if (writebuf_size)
                        memmove(writebuf, bp + written, writebuf_size);

                /*
                 * advance in the input buffer
                 */
                buf += size;

                /*
                 * decrease the todo size
                 */
               count -= size;
        }

        /*
         * if there are still some bytes left in the input buffer,
         * then store them in the send buffer and increase send
         * buffer size
         */
        if (count) {
                memcpy(writebuf + writebuf_size, buf, count);
                writebuf_size += count;
        }

	/*
	 * We get signal
	 */
	return writebuf_size;
}


static int
sync_byte_offset(char *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		if (buf[i] == TS_SYNC_BYTE)
			return i;
	/*
	 * What!
	 */
	return -1;
}

void
sig_handler(int sig) /* signal handler */
{
	switch(sig)
	{
		case SIGTERM:
		/* do the cleanup */
		printf("Got kill signal. Cleaning and bailing out...\n");
		exiting = 1;
		break;
	}
}

int
main (int argc, char **argv)
{
	struct sockaddr_in sin;
	int udp_socket;

	int dmx[argc - 3];
	int dvr;

	int i,j;

	int argp = 1, retries=0;

	#ifdef UDPSTREAMTS_DEBUG
	int min_read = READ_BUFFER_SIZE+1, max_read = -1;
	#endif

	char readbuf[READ_BUFFER_SIZE];
	char writebuf[UDP_PACKET_SIZE];
	ssize_t rsize = 0;
	ssize_t wsize = 0;

	#ifdef UDPSTREAMTS_DEBUG
		printf("udpstreamts (compiled in debug mode) starting... \n");
		printf("Size of readbuffer: %d bytes\n", READ_BUFFER_SIZE);
		printf("Size of writebuffer: %d bytes\n", UDP_PACKET_SIZE);
		printf("\n");
	#else
		printf("udpstreamts starting... \n");
		printf("\n");		
	#endif

	if (argc < 4) {
		/*
		 * Main screen turn on
		 */
		fprintf(stderr, "usage: %s [-b <logfile>] <ip> <port> <pid> [<pid> ...]\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* inserted A.Wacker 19.07.2007 for starting udpstreamts in background */
	if (!strncmp(argv[1],"-b",2)) {
        	printf("Daemonizing %s and logging to %s...\n", argv[0], argv[2]);
		fflush(stdout);
        	
		/* daemonize */
		j=fork();
		if (j<0) exit(1); /* fork error */
		if (j>0) exit(0); /* parent exits */
		/* child daemon continous */
		
		setsid(); /* obtain a new process group */

		for (j=getdtablesize();j>=0;--j) close(j); /* close all descriptors */

		j = open(argv[2], O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR); /*open stdin */
		dup(j); /* stdout */
		dup(j); /* stderr */

		if (j == -1)
        	{
			perror("Cannot open logfile for writing");
        	}
		
		chdir ("/");

		signal(SIGCHLD,SIG_IGN); /* ignore child terminate signal */
		signal(SIGTERM,sig_handler); /* termination signal from kill */
		
		
		/* check again if there are enough arguments */
		if (argc < 6) {
			fprintf(stderr, "usage: %s [-b <logfile>] <ip> <port> <pid> [<pid> ...]\n", argv[0]);
		return EXIT_FAILURE;
	}

		/* finally shift the remaining arguments, so that the rest of the code remains untouched */
		argp = 3;
	}
	printf("Preparing to send the transport stream to %s:%s ...\n",argv[argp],argv[argp+1]);
    	/* end changes 19.07.2007 */


	/*
	 * It's You!!
	 */
	if ((udp_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(argv[argp]);
	sin.sin_port = htons(strtoul(argv[argp+1], NULL, 0));

	/*
	 * How are you gentlemen!!
	 */
	if (connect(udp_socket, (struct sockaddr *) &sin, sizeof(struct sockaddr)) < 0) {
		perror("connect");
		return EXIT_FAILURE;
	}

	if ((dvr = open(DVR, O_RDONLY)) < 0) {
		perror(DVR);
		return EXIT_FAILURE;
	}

	/*
	 * All your base are belong to us
	 */
	for (i = 0; i < (argc - (argp+2)); i++)
	{
		printf("Preparing demuxer for streaming PID %i: %s ...\n",i ,argv[i + (argp + 2)]);
		dmx[i] = dmx_pid_filter_start(strtoul(argv[i + (argp + 2)], NULL, 0));
	}
	fflush(stdout);

	rsize = read(dvr, readbuf, TS_PACKET_SIZE);

	if (rsize == -1) {
		/*
		 * You are on the way to destruction
		 */
		perror("Error reading from dvr");
	}

	else {
		int read_offset = sync_byte_offset(readbuf, TS_PACKET_SIZE);
		int size_offset = 0;

		/*
		 * What you say!!
		 */
		if (read_offset == -1)
			fprintf(stderr, "%s: not a valid ts\n", __FILE__);

		else do {
			/*
			 * read as much bytes as needed to fill the buffer
			 * as much as possible and to end with a complete
			 * transport stream packet
			 */
			rsize = read(dvr, readbuf + read_offset, sizeof(readbuf) - read_offset - size_offset);


			/*
			* retry to read
			*/
			retries = 0;
			while((rsize == -1) && (retries < MAX_RETRIES) && (errno==EOVERFLOW)){
				retries++;
				#ifdef UDPSTREAMTS_DEBUG
				fprintf(stderr,"Error reading from dvr, still trying..(%i/%i)",retries,MAX_RETRIES);
				perror("; Reason");
				#endif
				rsize = read(dvr, readbuf + read_offset, sizeof(readbuf) - read_offset - size_offset);
			}


			if (rsize == -1) {
				perror("Error reading from dvr. Giving up");
				break;
			}

			else if ((rsize + read_offset) % TS_PACKET_SIZE) {
				/*
				 * You have no chance to survive make your time
				 * HA HA HA HA ...
				 */
				fprintf(stderr, "%s: bad ts packet size %d, r_off %d, s_off %d\n", __FILE__, (rsize + read_offset) % TS_PACKET_SIZE, read_offset, size_offset);
				exit(EXIT_FAILURE);
			}

			/*
			 * Take off every 'zig'
			 */
			read_offset = sync_byte_offset(readbuf, rsize - TS_PACKET_SIZE);


			#ifdef UDPSTREAMTS_DEBUG
			/* just for debugging*/
			if(rsize>max_read)
			{
				max_read = rsize;
				printf("New max read size: %d\n",max_read);
			}
			if(rsize<min_read)
			{
				min_read = rsize;
				printf("New min read size: %d\n",min_read);
			}
			if ((rsize!=max_read) && (rsize!=min_read))
			{
				printf("Current read size: %d\n",rsize);
			}
			#endif


			if (!read_offset) {
				/*
				 * All your TS are belong to us
				 */
				wsize = write_packets(udp_socket, readbuf, rsize, writebuf, wsize);
				size_offset = 0;

			}

			else if (read_offset == -1) {
				fprintf(stderr, "%s: sync now absolutely fucked up\n", __FILE__);
				/*
				 * You know what you doing
				 */
				read_offset = size_offset = 0;
			}

			else {
				fprintf(stderr, "%s: ts out of sync: r_offs %d, len: %d\n", __FILE__, read_offset, rsize);
				/*
				 * Move 'zig'
				 */
				/* FIXME size_offset = ((rsize - read_offset) % TS_PACKET_SIZE); */
				rsize -= read_offset + ((rsize - read_offset) % TS_PACKET_SIZE);

				if (rsize % TS_PACKET_SIZE) {
					/*
					 * Somebody set up us the bomb
					 */
					fprintf(stderr, "%s:%s: Internal Error!\n", __FILE__, __FUNCTION__);
					exit(EXIT_FAILURE);
				}

				wsize = write_packets(udp_socket, readbuf + read_offset, rsize, writebuf, wsize);
				read_offset = 0;
			}

		} while (wsize >= 0 && !exiting);
	}

	printf("Prepare for shutdown ");

	for (i = 0; i < (argc - (argp+2)); i++)
	{
		fprintf(stdout,".");
		dmx_pid_filter_stop(dmx[i]);
	}

	close(dvr);

	/* just to be sure, that everything is closed */
	for (j=getdtablesize();j>=3;--j) close(j); /* close all descriptors, except std dscr. */

	printf(" done. Goodby!\n");

	/*
	 * For great justice
	 */
	return EXIT_SUCCESS;
}

