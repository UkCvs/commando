/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Sven Traenkle 'Zwen'
	License: GPL

	$Id: irsend.h,v 1.2 2009/11/22 15:36:51 rhabarber1848 Exp $
*/
#ifndef __irsend__
#define __irsend__

#include <string>

class CIRSend
{
   public:
		CIRSend(const char * const configfile);
		bool Send();
   private:
		std::string m_configFile;
};
#endif
