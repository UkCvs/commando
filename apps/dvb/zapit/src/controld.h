// this hack needs to die, soon.
// variables
extern struct Ssettings settings;
/* the configuration file */
extern CConfigFile * controldconfig;

// function prototypes
void setvideooutput(CControld::video_format format, bool bSaveSettings = true);
void setBoxType();
void setChipInfo();
void setAviaChip();
void setScartMode(bool onoff);
void setRGBCsync(int val);
char getRGBCsync();
void setvcroutput(CControld::video_format format);
void disableVideoOutput(bool disable);
void controldSaveSettings();
int startPlayBack(CZapitChannel *cc);
int stopPlayBack();
void controld_main();
void controld_end();
