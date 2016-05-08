/*
 * $Id: frontend.cpp,v 1.72 2012/03/04 08:24:09 rhabarber1848 Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
 * (C) 2007-2009, 2013 Stefan Seyfried
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

/* system c */
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#include <cmath>
/* zapit */
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/settings.h>

#define diff(x,y)	(max(x,y) - min(x,y))
#define min(x,y)	((x < y) ? x : y)
#define max(x,y)	((x > y) ? x : y)

#define SOUTH	0
#define NORTH	1
#define EAST	0
#define WEST	1

extern double gotoXXLatitude;
extern double gotoXXLongitude;
extern int gotoXXLaDirection;
extern int gotoXXLoDirection;

CFrontend::CFrontend(int _uncommitted_switch_mode, int _auto_fec)
{
	tuned = false;
	currentToneMode = SEC_TONE_OFF;
	currentTransponder.polarization = VERTICAL;
	currentTransponder.feparams.frequency = 0;
	diseqcRepeats = 0;
	diseqcType = NO_DISEQC;
	last_inversion = INVERSION_OFF;
	last_qam = QAM_64;
	secfd = -1;

	uncommitted_switch_mode = _uncommitted_switch_mode;
	auto_fec = _auto_fec;
	if ((uncommitted_switch_mode<0) || (uncommitted_switch_mode>2)) uncommitted_switch_mode = 0;
	printf("[frontend] uncommitted_switch_mode %d auto_fec %d\n", uncommitted_switch_mode, auto_fec);

	if ((fd = open(FRONTEND_DEVICE, O_RDWR|O_NONBLOCK)) < 0)
		ERROR(FRONTEND_DEVICE);

	fop(ioctl, FE_GET_INFO, &info);

#if HAVE_DVB_API_VERSION < 3
	fop(ioctl, FE_SET_POWER_STATE, FE_POWER_ON);
	usleep(150000);	/* taken from enigma source code, all FE_SET_POWER_STATE ioctls are
			   accompanied by such a usleep there */
	if (info.type != FE_QAM) {
		printf("[frontend] non-cable box detected\n");
		/* on cable-dreamboxen, opening the sec-device apparently
		   kills the box. See:
		   http://tuxbox-forum.dreambox-fan.de/forum/viewtopic.php?p=340296#340296
		 */
		if ((secfd = open(SEC_DEVICE, O_RDWR)) < 0)
			ERROR(SEC_DEVICE);
	} else {
		printf("[frontend] cable box detected\n");
	}
#else
	secfd = fd;
#endif
}

CFrontend::~CFrontend(void)
{
	if (diseqcType > MINI_DISEQC)
		sendDiseqcStandby();

	/* tested on dm500, VOLTAGE_OFF switched into passthrough mode,
	   FE_POWER_OFF does something else to save some power...
	   It does no harm on dbox2, at least not on philips sat
	   enigma does exactly the same it its savePower() function
	 */
	secSetVoltage(SEC_VOLTAGE_OFF, 1);
	secSetTone(SEC_TONE_OFF, 1);
#if HAVE_DVB_API_VERSION < 3
	fop(ioctl, FE_SET_POWER_STATE, FE_POWER_OFF);
	if (secfd >= 0)
		close(secfd);
#endif
	close(fd);
}

void CFrontend::reset(void)
{
	close(fd);

	if ((fd = open(FRONTEND_DEVICE, O_RDWR|O_NONBLOCK|O_SYNC)) < 0)
		ERROR(FRONTEND_DEVICE);
#if HAVE_DVB_API_VERSION < 3
	fop(ioctl, FE_SET_POWER_STATE, FE_POWER_ON);
	usleep(150000);
#endif
}

fe_code_rate_t CFrontend::getCodeRate(const uint8_t fec_inner)
{
	switch (fec_inner & 0x0F) {
	case 0x01:
		return FEC_1_2;
	case 0x02:
		return FEC_2_3;
	case 0x03:
		return FEC_3_4;
	case 0x04:
		return FEC_5_6;
	case 0x05:
		return FEC_7_8;
	case 0x0F:
		return FEC_NONE;
	default:
		return FEC_AUTO;
	}
}

#if HAVE_DVB_API_VERSION < 3
#define FEC_4_5 FEC_AUTO
#define FEC_6_7 FEC_AUTO
#define FEC_8_9 FEC_AUTO
#endif

/* converts the number from {cables,sattelites,services}.xml to 
   values the frontend can use
   with new API this is a 1:1 conversion */
fe_code_rate_t CFrontend::xml2FEC(const uint8_t fec)
{
	switch (fec) {
	case 0x00:
		return FEC_NONE;
	case 0x01:
		return FEC_1_2;
	case 0x02:
		return FEC_2_3;
	case 0x03:
		return FEC_3_4;
	case 0x04:
		return FEC_4_5;
	case 0x05:
		return FEC_5_6;
	case 0x06:
		return FEC_6_7;
	case 0x07:
		return FEC_7_8;
	case 0x08:
		return FEC_8_9;
	default:
		return FEC_AUTO;
	}
}

uint8_t CFrontend::FEC2xml(const fe_code_rate_t fec)
{
	switch (fec)
	{
	case FEC_NONE:
		return 0;
	case FEC_1_2:
		return 1;
	case FEC_2_3:
		return 2;
	case FEC_3_4:
		return 3;
	case FEC_5_6:
		return 5;
	case FEC_7_8:
		return 7;
#if HAVE_DVB_API_VERSION >= 3
	case FEC_4_5:
		return 4;
	case FEC_6_7:
		return 6;
	case FEC_8_9:
		return 8;
#endif
	default:
		return 9;
	}
}

fe_modulation_t CFrontend::getModulation(const uint8_t modulation)
{
	switch (modulation) {
	case 0x00:
		return QPSK;
	case 0x01:
		return QAM_16;
	case 0x02:
		return QAM_32;
	case 0x03:
		return QAM_64;
	case 0x04:
		return QAM_128;
	case 0x05:
		return QAM_256;
	default:
#if HAVE_DVB_API_VERSION >= 3
		return QAM_AUTO;
#else
		/* peeking at the enigma code suggests that QAM_64 might be correct */
		return QAM_64;
#endif
	}
}

uint32_t CFrontend::getFrequency(void) const
{
	switch (info.type) {
	case FE_QPSK:
		if (currentToneMode == SEC_TONE_OFF)
			return currentTransponder.feparams.frequency + lnbOffsetsLow[currentTransponder.diseqc];
		else
			return currentTransponder.feparams.frequency + lnbOffsetsHigh[currentTransponder.diseqc];

	case FE_QAM:
	case FE_OFDM:
	default:
		return currentTransponder.feparams.frequency;
	}
}

void CFrontend::setUncommittedSwitchMode(const int mode)
{
	uncommitted_switch_mode = mode;
	if ((uncommitted_switch_mode<0) || (uncommitted_switch_mode>2))
		uncommitted_switch_mode = 0;
	printf("[frontend] uncommitted_switch_mode %d\n", uncommitted_switch_mode);
}

int CFrontend::getUncommittedSwitchMode(void) const
{
	return uncommitted_switch_mode;
}

uint8_t CFrontend::getPolarization(void) const
{
	return currentTransponder.polarization;
}

fe_status_t CFrontend::getStatus(void) const
{
	fe_status_t status;

	fop(ioctl, FE_READ_STATUS, &status);

	return status;
}

uint32_t CFrontend::getBitErrorRate(void) const
{
	uint32_t ber;

	fop(ioctl, FE_READ_BER, &ber);

	return ber;
}

uint16_t CFrontend::getSignalStrength(void) const
{
	uint16_t strength;

	fop(ioctl, FE_READ_SIGNAL_STRENGTH, &strength);

	return strength;
}

uint16_t CFrontend::getSignalNoiseRatio(void) const
{
	uint16_t snr;

	fop(ioctl, FE_READ_SNR, &snr);

	return snr;
}

uint32_t CFrontend::getUncorrectedBlocks(void) const
{
	uint32_t blocks;

	fop(ioctl, FE_READ_UNCORRECTED_BLOCKS, &blocks);

	return blocks;
}

void CFrontend::setFrontend(const dvb_frontend_parameters *feparams)
{
	dvb_frontend_event event;

	if (fd == -1)
		return;

	/* 'const *' is passed in, but we might need to modify it... */
	dvb_frontend_parameters feparams2;
	memcpy(&feparams2, feparams, sizeof(dvb_frontend_parameters));

	if (diseqcType == DISEQC_UNICABLE)
		feparams2.frequency = TuneUnicable(feparams->frequency,
						   currentToneMode == SEC_TONE_ON,
						   currentTransponder.polarization == SEC_VOLTAGE_18,
						   /*!!currentTransponder.diseqc*/ 0); // TODO: mini-a/b switch
#if HAVE_DVB_API_VERSION < 3
	/* the dreambox cable driver likes to get the frequency in kHz */
	if (info.type == FE_QAM) {
		feparams2.Frequency /= 1000;
		DBG("cable box: setting frequency to %d khz\n", feparams2.Frequency);
	}
	if (auto_fec)
	{
		if (info.type == FE_QPSK)
			feparams2.u.qpsk.FEC_inner = FEC_AUTO;
		if (info.type == FE_QAM)
			feparams2.u.qam.FEC_inner = FEC_AUTO;
	}
#endif
	errno = 0;
	while ((errno == 0) || (errno == EOVERFLOW))
		quiet_fop(ioctl, FE_GET_EVENT, &event);

	fop(ioctl, FE_SET_FRONTEND, &feparams2);
}

#define TIME_STEP 200
#define TIMEOUT_MAX_MS (10*TIME_STEP)

dvb_frontend_parameters CFrontend::getFrontend(void) const
{
	return currentTransponder.feparams;
}

dvb_frontend_event CFrontend::getEvent(void)
{
	dvb_frontend_event event;
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	memset(&event, 0, sizeof(dvb_frontend_event));

#if HAVE_DVB_API_VERSION >= 3
 	int msec = TIME_STEP;

	while (msec <= TIMEOUT_MAX_MS )
	{
		poll(&pfd, 1, TIME_STEP);

		if (pfd.revents & POLLIN)
		{
			memset(&event, 0, sizeof(dvb_frontend_event));
			fop(ioctl, FE_GET_EVENT, &event);

			if (event.status & FE_HAS_LOCK)
			{
				currentTransponder.feparams.frequency = event.parameters.frequency;
				INFO("FE_HAS_LOCK: freq %lu", (long unsigned int)currentTransponder.feparams.frequency);
				tuned = true;
				break;
			}
			else if (event.status & FE_TIMEDOUT)
			{
				WARN("FE_TIMEDOUT");
				break;
			}
			else
			{
				if (event.status & FE_HAS_SIGNAL)
					INFO("FE_HAS_SIGNAL");
				if (event.status & FE_HAS_CARRIER)
					INFO("FE_HAS_CARRIER");
				if (event.status & FE_HAS_VITERBI)
					INFO("FE_HAS_VITERBI");
				if (event.status & FE_HAS_SYNC)
					INFO("FE_HAS_SYNC");
				if (event.status & FE_REINIT)
					INFO("FE_REINIT");
				msec = TIME_STEP;
			}
		}
		else if (pfd.revents & POLLHUP)
			reset();
		msec += TIME_STEP;
	}
#else
	int ret = poll(&pfd, 1, 10000);
	if (ret == -1) ERROR("[CFrontend::getEvent] poll");
	else if (ret == 0) ERROR("[CFrontend::getEvent] timeout");
	else {
		if (pfd.revents & POLLIN)
		{
			memset(&event, 0, sizeof(dvb_frontend_event));
			fop(ioctl, FE_GET_EVENT, &event);
                        switch (event.type)
                        {
                        case FE_UNEXPECTED_EV:
                                printf("[CFrontend::getEvent] FE_UNEXPECTED_EV\n");
                                break;

                        case FE_FAILURE_EV:
                                printf("[CFrontend::getEvent] FE_FAILURE_EV ");
#if 0
				if (event.u.failureEvent & FE_HAS_POWER) printf("FE_HAS_POWER ");
				if (event.u.failureEvent & FE_HAS_SIGNAL) printf("FE_HAS_SIGNAL ");
				if (event.u.failureEvent & FE_SPECTRUM_INV) printf("FE_SPECTRUM_INV ");
				if (event.u.failureEvent & FE_HAS_LOCK) printf("FE_HAS_LOCK ");
				if (event.u.failureEvent & FE_HAS_CARRIER) printf("FE_HAS_CARRIER ");
				if (event.u.failureEvent & FE_HAS_VITERBI) printf("FE_HAS_VITERBI ");
				if (event.u.failureEvent & FE_HAS_SYNC) printf("FE_HAS_SYNC ");
				if (event.u.failureEvent & FE_TUNER_HAS_LOCK) printf("FE_TUNER_HAS_LOCK ");
#endif
				printf("\n");
                                break;

                        case FE_COMPLETION_EV:
                                currentTransponder.feparams.Frequency = event.u.completionEvent.Frequency;
				printf("[CFrontend::getEvent] FE_COMPLETION_EV: freq %d inv: %d ", event.u.completionEvent.Frequency, event.u.completionEvent.Inversion);
				switch (info.type) {
				case FE_QPSK:
					printf("sr: %d, fec: %d", event.u.completionEvent.u.qpsk.SymbolRate, event.u.completionEvent.u.qpsk.FEC_inner);
					break;
				default:
					break;
				}
				printf("\n");
				if (1) {
					FrontendParameters f;
					fop(ioctl, FE_GET_FRONTEND, &f);
					printf("[CFrontend::getEvent] FE_GET_FRONTEND: freq %d inv: %d ", f.Frequency, f.Inversion);
					switch (info.type) {
					case FE_QPSK:
						printf("sr: %d, fec: %d", f.u.qpsk.SymbolRate, f.u.qpsk.FEC_inner);
						break;
					default:
						break;
					}
					printf("\n");
				}
				tuned = true;
				break;
			}
		}
	}

#endif
	if (!tuned)
		currentTransponder.feparams.frequency = 0;

	return event;
}

#ifdef DEBUG_SEC_TIMING

#define TIMER_START()									\
	struct timeval tv, tv2;								\
	unsigned int msec;								\
	gettimeofday(&tv, NULL)

#define TIMER_STOP()									\
	gettimeofday(&tv2, NULL);							\
	msec = ((tv2.tv_sec - tv.tv_sec) * 1000) + ((tv2.tv_usec - tv.tv_usec) / 1000); \
	INFO("%u msec", msec)

#else /* DEBUG_SEC_TIMING */

#define TIMER_START() \
	do {} while (0)

#define TIMER_STOP() \
	do {} while (0)

#endif /* DEBUG_SEC_TIMING */

void CFrontend::secSetTone(const fe_sec_tone_mode_t toneMode, const uint32_t ms)
{
	TIMER_START();

	if (diseqcType == DISEQC_UNICABLE) {
		currentToneMode = toneMode;
		fop_sec(ioctl, FE_SET_TONE, SEC_TONE_OFF); /* just to make sure... */
	} else if (fop_sec(ioctl, FE_SET_TONE, toneMode) == 0) {
		currentToneMode = toneMode;
		usleep(1000 * ms);
	}

	TIMER_STOP();
}

void CFrontend::secSetVoltage(const fe_sec_voltage_t voltage, const uint32_t ms)
{
	TIMER_START();

	if (diseqcType == DISEQC_UNICABLE) {
		currentTransponder.polarization = voltage;
		fop_sec(ioctl, FE_SET_VOLTAGE, SEC_VOLTAGE_13); /* voltage must not be 18V */
	} else if (fop_sec(ioctl, FE_SET_VOLTAGE, voltage) == 0) {
		currentTransponder.polarization = voltage;
		usleep(1000 * ms);
	}

	TIMER_STOP();
}

void CFrontend::secResetOverload(void)
{
	TIMER_START();

#if HAVE_DVB_API_VERSION >= 3
	fop(ioctl, FE_DISEQC_RESET_OVERLOAD);
#else
	printf("CFrontend::secResetOverload() not implemented in old API\n"); 
#endif
	TIMER_STOP();
}

void CFrontend::sendDiseqcCommand(const struct dvb_diseqc_master_cmd *cmd, const uint32_t ms)
{
	TIMER_START();

#if HAVE_DVB_API_VERSION >= 3
	if (fop(ioctl, FE_DISEQC_SEND_MASTER_CMD, cmd) == 0)
#else
	secCmdSequence sequence;
	secCommand command;

	sequence.miniCommand = SEC_MINI_NONE;
	if (diseqcType == DISEQC_UNICABLE) {
		sequence.continuousTone = SEC_TONE_OFF;
		sequence.voltage = SEC_VOLTAGE_18;
	} else {
		sequence.continuousTone = currentToneMode;
		sequence.voltage = currentTransponder.polarization;
	}
	command.type = SEC_CMDTYPE_DISEQC_RAW;
	command.u.diseqc.cmdtype	= cmd->msg[0];
	command.u.diseqc.addr	= cmd->msg[1];
	command.u.diseqc.cmd	= cmd->msg[2];
	command.u.diseqc.numParams	= cmd->msg_len - 3;
	for (int i=0; i < (cmd->msg_len - 3); i++)
		command.u.diseqc.params[i] = cmd->msg[3 + i];
	sequence.commands = &command;
	sequence.numCommands = 1;

	if (fop_sec(ioctl, SEC_SEND_SEQUENCE, sequence) == 0)
#endif
		usleep(1000 * ms);

	TIMER_STOP();
}

#if HAVE_DVB_API_VERSION >= 3
uint32_t CFrontend::getDiseqcReply(const int timeout_ms) const
{
	struct dvb_diseqc_slave_reply reply;

	reply.timeout = timeout_ms;

	TIMER_START();

	if (fop(ioctl, FE_DISEQC_RECV_SLAVE_REPLY, &reply) < 0)
		return 0;

	TIMER_STOP();

	/* timeout */
	if (reply.msg_len == 0)
		return 1;

	switch (reply.msg[0]) {
	case 0xe4:	/* ok */
		return 0;
	case 0xe5:	/* invalid address or unknown command */
		return 1;
	case 0xe6:	/* parity error */
		return 1;
	case 0xe7:	/* contention flag mismatch */
		return 1;
	default:	/* unexpected reply */
		return 0;
	}
}
#endif

void CFrontend::sendToneBurst(const fe_sec_mini_cmd_t burst, const uint32_t ms)
{
	TIMER_START();
#if HAVE_DVB_API_VERSION >= 3
	if (fop(ioctl, FE_DISEQC_SEND_BURST, burst) == 0)
#else
	secCmdSequence sequence;
	memset(&sequence, 0, sizeof(sequence));

	sequence.miniCommand = burst;
	sequence.continuousTone = (secToneMode)currentToneMode;
	sequence.voltage = (secVoltage)currentTransponder.polarization;

	if (fop_sec(ioctl, SEC_SEND_SEQUENCE, &sequence) == 0)
#endif
		usleep(1000 * ms);
	else
		perror("SEC_SEND_SEQUENCE");

	TIMER_STOP();
}

void CFrontend::setDiseqcType(const diseqc_t newDiseqcType)
{
	switch (newDiseqcType) {
	case NO_DISEQC:
		INFO("NO_DISEQC");
		break;
	case MINI_DISEQC:
		INFO("MINI_DISEQC");
		break;
	case SMATV_REMOTE_TUNING:
		INFO("SMATV_REMOTE_TUNING");
		break;
	case DISEQC_1_0:
		INFO("DISEQC_1_0");
		break;
	case DISEQC_1_1:
		INFO("DISEQC_1_1");
		break;
	case DISEQC_1_2:
		INFO("DISEQC_1_2");
		break;
	case DISEQC_2_0:
		INFO("DISEQC_2_0");
		break;
	case DISEQC_2_1:
		INFO("DISEQC_2_1");
		break;
	case DISEQC_2_2:
		INFO("DISEQC_2_2");
		break;
	case DISEQC_UNICABLE:
		INFO("DISEQC_UNICABLE");
		break;
	default:
		WARN("Invalid DiSEqC type");
		return;
	}

	if (diseqcType != newDiseqcType)
	{
		/* make sure that the switch gets reset, so that he will
		   accept the new mode */
		sendDiseqcReset();
		sendDiseqcPowerOn();
	}

	diseqcType = newDiseqcType;
}

void CFrontend::setLnbOffset(const bool high, const uint8_t index, int offset)
{
	if (index < MAX_LNBS) {
		if (high)
			lnbOffsetsHigh[index] = offset;
		else
			lnbOffsetsLow[index] = offset;
	}
}

void CFrontend::sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t command, uint8_t num_parameters, uint8_t parameter1, uint8_t parameter2)
{
	struct dvb_diseqc_master_cmd cmd;

	printf("[frontend] sendMotorCommand: cmdtype   = %x, address = %x, cmd   = %x\n", cmdtype, address, command);
	printf("[frontend] sendMotorCommand: num_parms = %d, parm1   = %x, parm2 = %x\n", num_parameters, parameter1, parameter2);

	cmd.msg[0] = cmdtype; //command type
	cmd.msg[1] = address; //address
	cmd.msg[2] = command; //command
	cmd.msg[3] = parameter1;
	cmd.msg[4] = parameter2;
	cmd.msg_len = 3 + num_parameters;

	sendDiseqcCommand(&cmd, 15);
	printf("[frontend] motor positioning command sent.\n");
}

void CFrontend::positionMotor(uint8_t motorPosition)
{
	struct dvb_diseqc_master_cmd cmd;

	if (motorPosition != 0)
	{
		cmd.msg[0] = 0xE0; //command type
		cmd.msg[1] = 0x31; //address
		cmd.msg[2] = 0x6B; //command: goto stored motor position
		cmd.msg[3] = motorPosition;
		cmd.msg_len = 4;
		sendDiseqcCommand(&cmd, 15);
		printf("[frontend] motor positioning command sent.\n");
	}
}

int CFrontend::setParameters(dvb_frontend_parameters *feparams, const uint8_t polarization, const uint8_t diseqc )
{
	TP_params TP;
	memcpy(&TP.feparams, feparams, sizeof(dvb_frontend_parameters));
	TP.polarization = polarization;
	TP.diseqc = diseqc;
	return setParameters(&TP);

}

int CFrontend::setParameters(TP_params *TP)
{
	int ret, freq_offset = 0;
	bool high_band = false;
#if HAVE_DVB_API_VERSION >= 3
	bool can_not_auto_qam = !(info.caps & FE_CAN_QAM_AUTO);
	bool can_not_auto_inversion = !(info.caps & FE_CAN_INVERSION_AUTO);
	bool do_auto_qam = TP->feparams.u.qam.modulation == QAM_AUTO;
	bool do_auto_inversion = TP->feparams.inversion == INVERSION_AUTO;
#else
	bool can_not_auto_qam = true;
	bool can_not_auto_inversion = true;
	bool do_auto_qam = false;
	bool do_auto_inversion = false;
#endif
	

	if (info.type == FE_QPSK)
	{
		if (TP->feparams.frequency < 11700000)
		{
			high_band = false;
			freq_offset = lnbOffsetsLow[TP->diseqc];
		}
		else
		{
			high_band = true;
			freq_offset = lnbOffsetsHigh[TP->diseqc];
		}

		TP->feparams.frequency -= freq_offset;
		setSec(TP->diseqc, TP->polarization, high_band, TP->feparams.frequency);
	}

	/*
	 * frontends which can not handle auto inversion but
	 * are set to auto inversion will try with stored value first
	 */
	if (do_auto_inversion && can_not_auto_inversion)
		TP->feparams.inversion = last_inversion;

	/*
	 * frontends which can not handle auto qam but
	 * are set to auto qam will try with stored value first
	 */
	if (do_auto_qam && can_not_auto_qam)
		TP->feparams.u.qam.modulation = last_qam;

	dvb_frontend_event event;
	int tryagain = 0; //flame on derget for this 

	do
	{
		do
		{
			tuned = false;
			setFrontend (&TP->feparams);
			event = getEvent();	/* check if tuned */

			if (!tuned && !tryagain)
			{
				tryagain = 1;
				WARN("TUNE FAILED I TRY IT ONE TIME AGAIN");
				continue;
			}

			if (do_auto_inversion && can_not_auto_inversion && !tuned)
			{
				switch (TP->feparams.inversion)
				{
					case INVERSION_OFF:
						TP->feparams.inversion = INVERSION_ON;
						break;
					case INVERSION_ON:
					default:
						TP->feparams.inversion = INVERSION_OFF;
						break;
				}
				if (TP->feparams.inversion == last_inversion) /* can�t tune */
					break;
			}
			else
				break; /* tuned */
		} while(1);

		if (tuned && tryagain) /* i've never seen this trigger */
			WARN("==============> TRY AGAIN actually helped <=======================================================");
#if HAVE_DVB_API_VERSION >= 3
		if (do_auto_qam && can_not_auto_qam && !tuned)
		{
			switch (TP->feparams.u.qam.modulation)
			{
				case QAM_16:
					TP->feparams.u.qam.modulation = QAM_32;
					break;
				case QAM_32:
					TP->feparams.u.qam.modulation = QAM_64;
					break;
				case QAM_64:
					TP->feparams.u.qam.modulation = QAM_128;
					break;
				case QAM_128:
					TP->feparams.u.qam.modulation = QAM_256;
					break;
				case QAM_256:
				default:
					TP->feparams.u.qam.modulation = QAM_16;
					break;
			}
			if (TP->feparams.u.qam.modulation == last_qam) /* can`t tune */
				break;

			continue; /* QAM changed, next try to tune */
		}
		else
#else
		if (tuned || tryagain)
#endif
			break; /* tuned */
	} while (1);

	if (tuned)
	{
		last_inversion = TP->feparams.inversion; /* store good value */
#if HAVE_DVB_API_VERSION < 3
		last_qam = TP->feparams.u.qam.QAM; /* store good value */
		ret = diff(event.u.completionEvent.frequency, TP->feparams.frequency);
#if 1
		/*
		 * if everything went ok, then it is a good idea to copy the real
		 * frontend parameters, so we can update the service list, if it differs.
		 *
		 * TODO: set a flag to indicate a change in the service list
		 */
		memcpy(&currentTransponder.feparams, &event.u.completionEvent, sizeof(FrontendParameters));
#endif
#else
		last_qam = TP->feparams.u.qam.modulation; /* store good value */
		ret = diff(event.parameters.frequency, TP->feparams.frequency);
#if 1
		/*
		 * if everything went ok, then it is a good idea to copy the real
		 * frontend parameters, so we can update the service list, if it differs.
		 *
		 * TODO: set a flag to indicate a change in the service list
		 */
		memcpy(&currentTransponder.feparams, &event.parameters, sizeof(dvb_frontend_parameters));
#endif
#endif
	}
	else
	{
		ret = -1;
	}



	/*
	 * add the frequency offset to the frontend parameters again
	 * because they are used for the channel list and were given
	 * to this method as a pointer
	 */

	if (info.type == FE_QPSK)
		TP->feparams.frequency += freq_offset;

	return ret;
}

void CFrontend::setSec(const uint8_t sat_no, const uint8_t pol, const bool high_band, const uint32_t frequency)
{
	DBG("sat_no: %d pol: %d high: %d freq: %d diseqc_type: %d", sat_no, pol, (int)high_band, frequency, diseqcType);
	uint8_t repeats = diseqcRepeats;

	fe_sec_voltage_t v = (pol & 1) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	fe_sec_tone_mode_t t = high_band ? SEC_TONE_ON : SEC_TONE_OFF;
	fe_sec_mini_cmd_t b = (sat_no & 1) ? SEC_MINI_B : SEC_MINI_A;

	/*
	 * [0] from master, no reply, 1st transmission
	 * [1] any lnb switcher or smatv
	 * [2] write to port group 0 (committed switches)
	 * [3] high nibble: reset bits, low nibble set bits
	 *     bits are: option, position, polarizaion, band
	 */

	struct dvb_diseqc_master_cmd cmd = {
		{ 0xe0, 0x10, 0x38, 0x00, 0x00, 0x00 }, 4
	};

	cmd.msg[3] = 0xf0 | (((sat_no * 4) & 0x0f) | (high_band ? 1 : 0) | ((pol & 1) ? 0 : 2));

	/*
	 * set all SEC / DiSEqC parameters
	 */

	secSetTone(SEC_TONE_OFF, 1); // this resets currentToneMode!
	secSetVoltage(v, 30);

	if (diseqcType == DISEQC_UNICABLE)
		goto out;	/* shortcut */

	if ((diseqcType == DISEQC_1_1) && (uncommitted_switch_mode > 0))
	{
		static uint8_t prevSatNo = 255; // initialised with greater than max Satellites (64)
		// because we only want to send uncommitted switch
		// command if necessary to save zap time
		DBG("new Sat %d previous Sat %d", sat_no, prevSatNo);
		//DBG("new Sat/4 %d previous Sat/4 %d", sat_no/4, prevSatNo/4);

		if ((prevSatNo/4 != sat_no/4) && (1 == uncommitted_switch_mode))
		{
			sendUncommittedSwitchesCommand(0xF0 + sat_no/4);
		}
		else if ((prevSatNo != sat_no) && (2 == uncommitted_switch_mode))
		{
			sendUncommittedSwitchesCommand(0xF0 + sat_no);
		}
		prevSatNo = sat_no;
	}

	if (diseqcType >= SMATV_REMOTE_TUNING) {
#if HAVE_DVB_API_VERSION >= 3
		if (diseqcType >= DISEQC_2_0)
			cmd.msg[0] |= 0x02;	/* reply required */

		sendDiseqcCommand(&cmd, 15);

		if (diseqcType >= DISEQC_2_0)
			repeats += getDiseqcReply(50);
#else
		sendDiseqcCommand(&cmd, 15);
#endif
	}

	if ((diseqcType >= DISEQC_1_1) && (repeats)) {
		for (uint16_t i = 0; i < repeats; i++) {

			usleep(1000 * 100);	/* wait at least 100ms before retransmission */

			if (0 == uncommitted_switch_mode) {
				cmd.msg[2] |= 0x01;	/* uncommitted switches */
				sendDiseqcCommand(&cmd, 15);
			}

#if HAVE_DVB_API_VERSION >= 3
			uint8_t again = 0;
			if (diseqcType >= DISEQC_2_0)
				again += getDiseqcReply(50);

			cmd.msg[0] |= 0x01;	/* repeated transmission */
			cmd.msg[2] &= 0xFE;	/* committed switches */
			sendDiseqcCommand(&cmd, 15);

			if (diseqcType >= DISEQC_2_0)
				again += getDiseqcReply(50);

			if (again == 2)
				repeats++;
#else
			cmd.msg[0] |= 0x01;	/* repeated transmission */
			cmd.msg[2] &= 0xFE;	/* committed switches */
			sendDiseqcCommand(&cmd, 15);
#endif
		}
	}

	if (diseqcType == SMATV_REMOTE_TUNING)
		sendDiseqcSmatvRemoteTuningCommand(frequency);

 out:
	currentToneMode = t;
	if (diseqcType == MINI_DISEQC)
		sendToneBurst(b, 15);

	secSetTone(t, 15);

	currentTransponder.diseqc = sat_no;
}

void CFrontend::sendDiseqcPowerOn(void)
{
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x03);
}

void CFrontend::sendDiseqcReset(void)
{
	/* Reset && Clear Reset */
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x00);
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x01);
}

void CFrontend::sendDiseqcStandby(void)
{
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x02);
}

void CFrontend::sendDiseqcZeroByteCommand(uint8_t framing_byte, uint8_t address, uint8_t command)
{
	struct dvb_diseqc_master_cmd diseqc_cmd = {
		{ framing_byte, address, command, 0x00, 0x00, 0x00 }, 3
	};

	sendDiseqcCommand(&diseqc_cmd, 15);
}

void CFrontend::sendDiseqcSmatvRemoteTuningCommand(const uint32_t frequency)
{
 	/*
	 * [0] from master, no reply, 1st transmission
	 * [1] intelligent slave interface for multi-master bus
	 * [2] write channel frequency
	 * [3] frequency
	 * [4] frequency
	 * [5] frequency
	 */

	struct dvb_diseqc_master_cmd cmd = {
		{ 0xe0, 0x71, 0x58, 0x00, 0x00, 0x00 }, 6
	};

	cmd.msg[3] = (((frequency / 10000000) << 4) & 0xF0) | ((frequency / 1000000) & 0x0F);
	cmd.msg[4] = (((frequency / 100000) << 4) & 0xF0) | ((frequency / 10000) & 0x0F);
	cmd.msg[5] = (((frequency / 1000) << 4) & 0xF0) | ((frequency / 100) & 0x0F);

	sendDiseqcCommand(&cmd, 15);
}

uint32_t CFrontend::TuneUnicable(const uint32_t frequency, const int high_band,
				 const int horizontal, const int bank)
{
	struct dvb_diseqc_master_cmd cmd = { { 0xe0, 0x10, 0x5a, 0x00, 0x00, 0x00 }, 5 };
	unsigned int t = (frequency / 1000 + uni_qrg + 2) / 4 - 350;
	if (t >= 1024 || uni_scr < 0 || uni_scr > 7 || info.type != FE_QPSK)
	{
		WARN("ooops. t > 1024? (%d) or uni_scr out of range? (%d)", t, uni_scr);
		return 0;
	}

	uint32_t ret = (t + 350) * 4000 - frequency;
	INFO("[unicable] 18V=%d TONE=%d, freq=%d qrg=%d scr=%d bank=%d ret=%d",
			horizontal, high_band, frequency, uni_qrg, uni_scr, bank, ret);
	cmd.msg[3] = (t >> 8)		|	/* highest 3 bits of t */
		(uni_scr << 5)		|	/* adress */
		(bank << 4)		|	/* input 0/1 */
		(horizontal << 3)	|	/* horizontal == 0x08 */
		(high_band) << 2;		/* high_band  == 0x04 */
	cmd.msg[4] = t & 0xFF;
	fop_sec(ioctl, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
	usleep(15 * 1000);		/* en50494 says: >4ms and < 22 ms */
	sendDiseqcCommand(&cmd, 50);	/* en50494 says: >2ms and < 60 ms */
	fop_sec(ioctl, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
	return ret;
}

void CFrontend::sendUncommittedSwitchesCommand(uint8_t usCommand)
{
	struct dvb_diseqc_master_cmd cmd = {{ 0xe0, 0x10, 0x39, 0x00, 0x00, 0x00 }, 4};

	//command from master, no reply required, first transmission
	//all families (don't care adress)
	//write to port group 1 (uncommitted switches)
	cmd.msg[3] = usCommand;
	sendDiseqcCommand(&cmd, 15);

	if (diseqcRepeats)
	{
		cmd.msg[0] = 0xe1; /* repeated transmission */
		for (uint16_t i = 0; i < diseqcRepeats; i++)
		{
			usleep(1000 * 100); /* wait at least 100ms before retransmission */
			sendDiseqcCommand(&cmd, 15);
		}
	}
	//DBG"[frontend] uncommitted switches command (0x%x) sent", cmd.msg[3]);
}

static const int gotoXTable[10] = { 0x00, 0x02, 0x03, 0x05, 0x06, 0x08, 0x0A, 0x0B, 0x0D, 0x0E };

/*----------------------------------------------------------------------------*/
double factorial_div(double value, int x)
{
	if (!x)
		return 1;
	else {
		while (x > 1) {
			value = value / x--;
		}
	}
	return value;
}

/*----------------------------------------------------------------------------*/
double powerd(double x, int y)
{
	int i = 0;
	double ans = 1.0;

	if (!y)
		return 1.000;
	else {
		while (i < y) {
			i++;
			ans = ans * x;
		}
	}
	return ans;
}

/*----------------------------------------------------------------------------*/
double SIN(double x)
{
	int i = 0;
	int j = 1;
	int sign = 1;
	double y1 = 0.0;
	double diff = 1000.0;

	if (x < 0.0) {
		x = -1 * x;
		sign = -1;
	}

	while (x > 360.0 * M_PI / 180) {
		x = x - 360 * M_PI / 180;
	}

	if (x > (270.0 * M_PI / 180)) {
		sign = sign * -1;
		x = 360.0 * M_PI / 180 - x;
	} else if (x > (180.0 * M_PI / 180)) {
		sign = sign * -1;
		x = x - 180.0 * M_PI / 180;
	} else if (x > (90.0 * M_PI / 180)) {
		x = 180.0 * M_PI / 180 - x;
	}

	while (powerd(diff, 2) > 1.0E-16) {
		i++;
		diff = j * factorial_div(powerd(x, (2 * i - 1)), (2 * i - 1));
		y1 = y1 + diff;
		j = -1 * j;
	}
	return (sign * y1);
}

double COS(double x)
{
	return SIN(90 * M_PI / 180 - x);
}

double ATAN(double x)
{
	int i = 0;		/* counter for terms in binomial series */
	int j = 1;		/* sign of nth term in series */
	int k = 0;
	int sign = 1;		/* sign of the input x */
	double y = 0.0;		/* the output */
	double deltay = 1.0;	/* the value of the next term in the series */
	double addangle = 0.0;	/* used if arctan > 22.5 degrees */

	if (x < 0.0) {
		x = -1 * x;
		sign = -1;
	}

	while (x > 0.3249196962) {
		k++;
		x = (x - 0.3249196962) / (1 + x * 0.3249196962);
	}
	addangle = k * 18.0 * M_PI / 180;

	while (powerd(deltay, 2) > 1.0E-16) {
		i++;
		deltay = j * powerd(x, (2 * i - 1)) / (2 * i - 1);
		y = y + deltay;
		j = -1 * j;
	}
	return (sign * (y + addangle));
}

double ASIN(double x)
{
	return 2 * ATAN(x / (1 + std::sqrt(1.0 - x * x)));
}

double Radians(double number)
{
	return number * M_PI / 180;
}

double Deg(double number)
{
	return number * 180 / M_PI;
}

double Rev(double number)
{
	return number - std::floor(number / 360.0) * 360;
}

double calcElevation(double SatLon, double SiteLat, double SiteLon, int Height_over_ocean = 0)
{
	const double a0 = 0.58804392, a1 = -0.17941557, a2 = 0.29906946E-1, a3 = -0.25187400E-2, a4 = 0.82622101E-4;
	const double f = 1.00 / 298.257;	// Earth flattning factor
	const double r_sat = 42164.57;		// Distance from earth centre to satellite
	const double r_eq = 6378.14;		// Earth radius
	double sinRadSiteLat = SIN(Radians(SiteLat)), cosRadSiteLat = COS(Radians(SiteLat));
	double Rstation = r_eq / (std::sqrt(1.00 - f * (2.00 - f) * sinRadSiteLat * sinRadSiteLat));
	double Ra = (Rstation + Height_over_ocean) * cosRadSiteLat;
	double Rz = Rstation * (1.00 - f) * (1.00 - f) * sinRadSiteLat;
	double alfa_rx = r_sat * COS(Radians(SatLon - SiteLon)) - Ra;
	double alfa_ry = r_sat * SIN(Radians(SatLon - SiteLon));
	double alfa_rz = -Rz, alfa_r_north = -alfa_rx * sinRadSiteLat + alfa_rz * cosRadSiteLat;
	double alfa_r_zenith = alfa_rx * cosRadSiteLat + alfa_rz * sinRadSiteLat;
	double El_geometric = Deg(ATAN(alfa_r_zenith / std::sqrt(alfa_r_north * alfa_r_north + alfa_ry * alfa_ry)));
	double x = std::fabs(El_geometric + 0.589);
	double refraction = std::fabs(a0 + a1 * x + a2 * x * x + a3 * x * x * x + a4 * x * x * x * x);
	double El_observed = 0.00;

	if (El_geometric > 10.2)
		El_observed = El_geometric + 0.01617 * (COS(Radians(std::fabs(El_geometric))) / SIN(Radians(std::fabs(El_geometric))));
	else {
		El_observed = El_geometric + refraction;
	}

	if (alfa_r_zenith < -3000)
		El_observed = -99;

	return El_observed;
}

double calcAzimuth(double SatLon, double SiteLat, double SiteLon, int Height_over_ocean = 0)
{
	const double f = 1.00 / 298.257;	// Earth flattning factor
	const double r_sat = 42164.57;		// Distance from earth centre to satellite
	const double r_eq = 6378.14;		// Earth radius

	double sinRadSiteLat = SIN(Radians(SiteLat)), cosRadSiteLat = COS(Radians(SiteLat));
	double Rstation = r_eq / (std::sqrt(1 - f * (2 - f) * sinRadSiteLat * sinRadSiteLat));
	double Ra = (Rstation + Height_over_ocean) * cosRadSiteLat;
	double Rz = Rstation * (1 - f) * (1 - f) * sinRadSiteLat;
	double alfa_rx = r_sat * COS(Radians(SatLon - SiteLon)) - Ra;
	double alfa_ry = r_sat * SIN(Radians(SatLon - SiteLon));
	double alfa_rz = -Rz;
	double alfa_r_north = -alfa_rx * sinRadSiteLat + alfa_rz * cosRadSiteLat;
	double Azimuth = 0.00;

	if (alfa_r_north < 0)
		Azimuth = 180 + Deg(ATAN(alfa_ry / alfa_r_north));
	else
		Azimuth = Rev(360 + Deg(ATAN(alfa_ry / alfa_r_north)));

	return Azimuth;
}

double calcDeclination(double SiteLat, double Azimuth, double Elevation)
{
	return Deg(ASIN(SIN(Radians(Elevation)) * SIN(Radians(SiteLat)) + COS(Radians(Elevation)) * COS(Radians(SiteLat)) + COS(Radians(Azimuth))
		   )
	    );
}

double calcSatHourangle(double Azimuth, double Elevation, double Declination, double Lat)
{
	double a = -COS(Radians(Elevation)) * SIN(Radians(Azimuth));
	double b = SIN(Radians(Elevation)) * COS(Radians(Lat)) - COS(Radians(Elevation)) * SIN(Radians(Lat)) * COS(Radians(Azimuth));

	// Works for all azimuths (northern & sourhern hemisphere)
	double returnvalue = 180 + Deg(ATAN(a / b));

	(void)Declination;

	if (Azimuth > 270) {
		returnvalue = ((returnvalue - 180) + 360);
		if (returnvalue > 360)
			returnvalue = 360 - (returnvalue - 360);
	}

	if (Azimuth < 90)
		returnvalue = (180 - returnvalue);

	return returnvalue;
}

void CFrontend::gotoXX(t_satellite_position pos)
{
	int RotorCmd;
	int satDir = pos < 0 ? WEST : EAST;
	double SatLon = abs(pos) / 10.00;
	double SiteLat = gotoXXLatitude;
	double SiteLon = gotoXXLongitude;

	if (gotoXXLaDirection == SOUTH)
		SiteLat = -SiteLat;
	if (gotoXXLoDirection == WEST)
		SiteLon = 360 - SiteLon;
	if (satDir == WEST)
		SatLon = 360 - SatLon;
	printf("siteLatitude = %f, siteLongitude = %f, %f degrees\n", SiteLat, SiteLon, SatLon);

	double azimuth = calcAzimuth(SatLon, SiteLat, SiteLon);
	double elevation = calcElevation(SatLon, SiteLat, SiteLon);
	double declination = calcDeclination(SiteLat, azimuth, elevation);
	double satHourAngle = calcSatHourangle(azimuth, elevation, declination, SiteLat);
	printf("azimuth=%f, elevation=%f, declination=%f, PolarmountHourAngle=%f\n", azimuth, elevation, declination, satHourAngle);
	if (SiteLat >= 0) {
		int tmp = (int)round(fabs(180 - satHourAngle) * 10.0);
		RotorCmd = (tmp / 10) * 0x10 + gotoXTable[tmp % 10];
		if (satHourAngle < 180)	// the east
			RotorCmd |= 0xE000;
		else
			RotorCmd |= 0xD000;
	} else {
		if (satHourAngle < 180) {
			int tmp = (int)round(fabs(satHourAngle) * 10.0);
			RotorCmd = (tmp / 10) * 0x10 + gotoXTable[tmp % 10];
			RotorCmd |= 0xD000;
		} else {
			int tmp = (int)round(fabs(360 - satHourAngle) * 10.0);
			RotorCmd = (tmp / 10) * 0x10 + gotoXTable[tmp % 10];
			RotorCmd |= 0xE000;
		}
	}

	printf("RotorCmd = %04x\n", RotorCmd);
	sendMotorCommand(0xE0, 0x31, 0x6E, 2, ((RotorCmd & 0xFF00) / 0x100), RotorCmd & 0xFF);
}

