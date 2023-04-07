#ifndef LOG_H
#define LOG_H


const int LOG_ENGINE	 		= 0x0001;
const int LOG_MESSAGE			= 0x0004;
const int LOG_DLL				= 0x0008;
const int LOG_BOTBASE			= 0x0010;
const int LOG_BOTNAV			= 0x0020;
const int LOG_BOTCOMBAT			= 0x0040;
const int LOG_BOTMOVE			= 0x0080;
const int LOG_BOTTASK			= 0x0100;
const int LOG_BOTAIM			= 0x0200;

const int LOG_ALWAYS			= 0x4000;
const int LOG_SPAM				= 0x8000;
const int LOG_ALL				= 0xFFFF;
const int LOG_ALLBUTSPAM		= 0x7FFF;
const int LOG_SINGLEBOT			= LOG_BOTBASE|LOG_BOTNAV|LOG_BOTCOMBAT|LOG_BOTTASK;


#define	LOGFILE		"podbot.log"

void	set_log_mask(int mask);
int		get_log_mask();

void set_debuglog_bot(int iIndex);
void set_log_botindex(int iIndex);

void 	add_log (const char *fmt, ...);
void 	report_log (int flags, const char * funcname, const char *fmt, ...);

#endif
