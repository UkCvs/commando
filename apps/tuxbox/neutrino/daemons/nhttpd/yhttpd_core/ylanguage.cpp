//=============================================================================
// YHTTPD
// Language
//=============================================================================

// c
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

// yhttpd
#include <yconfig.h>
#include <yhttpd.h>
#include "ytypes_globals.h"
#include "ylanguage.h"
#include "yconnection.h"

//=============================================================================
// Instance Handling - like Singelton Pattern
//=============================================================================
//-----------------------------------------------------------------------------
// Init as Singelton
//-----------------------------------------------------------------------------
CLanguage* CLanguage::instance = NULL;
CConfigFile* CLanguage::DefaultLanguage = NULL;
CConfigFile* CLanguage::ConfigLanguage = NULL;
std::string CLanguage::language = "";
std::string CLanguage::language_dir = "";
//-----------------------------------------------------------------------------
// There is only one Instance
//-----------------------------------------------------------------------------
CLanguage *CLanguage::getInstance(void)
{
	if (!instance)
		instance = new CLanguage();
	return instance;
}

//-----------------------------------------------------------------------------
void CLanguage::deleteInstance(void)
{
	if (instance)
		delete instance;
	instance = NULL;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CLanguage::CLanguage(void)
{
	DefaultLanguage = new CConfigFile(',');
	ConfigLanguage = new CConfigFile(',');
	language = "";
	language_dir =getLanguageDir();
}

//-----------------------------------------------------------------------------
CLanguage::~CLanguage(void)
{
	delete DefaultLanguage;
	delete ConfigLanguage;
}

//=============================================================================

//-----------------------------------------------------------------------------
void CLanguage::setLanguage(std::string _language)
{
	language=_language;
	ConfigLanguage->loadConfig(language_dir + "/" + _language);
	DefaultLanguage->loadConfig(language_dir + "/" + HTTPD_DEFAULT_LANGUAGE);
}

//-----------------------------------------------------------------------------
// return translation for "id" if not found use default language
//-----------------------------------------------------------------------------
std::string CLanguage::getTranslation(std::string id)
{
	std::string trans=ConfigLanguage->getString(id,"");
	if(trans.empty())
		trans=DefaultLanguage->getString(id,"");
	if (trans.empty())
		trans = "# L:" + id + " #"; 
	return trans;
}
//-----------------------------------------------------------------------------
// Find language directory
//-----------------------------------------------------------------------------
std::string CLanguage::getLanguageDir(void)
{
	std::string tmpfilename = "/"+Cyhttpd::ConfigList["Language.directory"], dir = "";

	if( access((Cyhttpd::ConfigList["WebsiteMain.override_directory"] + tmpfilename).c_str(), R_OK) == 0)
		dir = Cyhttpd::ConfigList["WebsiteMain.override_directory"] + tmpfilename;
	else if(access((Cyhttpd::ConfigList["WebsiteMain.directory"] + tmpfilename).c_str(), R_OK) == 0)
		dir = Cyhttpd::ConfigList["WebsiteMain.directory"] + tmpfilename;
	return dir;
}

