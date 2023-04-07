//
// Handy little Logging Helper Functions
//
// Adapted from the ClientBot framework supplied
// by Vasily Pupkin

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "log.h"


#undef DEBUG_LOG

const int LOGMSG_BUF = 512; // Size of Buffer
const int LOG_MAXSIZE = 10485760; // Maximum Size LogFile can grow until being reset to 0

// Holds the flags allowed to log, see flags in log.h
static int iCurrentLogMask = 0;

//static int iCurrentLogMask = LOG_BOTTASK;

static int iLogSize = 0; // Current Size of Logfile

// Holds the Bots Index for single Logging, -1 = Log all  
static int iDebugBotIndex = -1;
// Holds Bot currently running BotThink
static int iActBotIndex = -1;


void set_log_mask(int mask)
{
	iCurrentLogMask = mask;
}


int	get_log_mask()
{
	return iCurrentLogMask;
}

// Sets Index of Bot for logging
void set_debuglog_bot(int iIndex)
{
	iDebugBotIndex = iIndex;
}

// Notifies logger which Bot is currently running
void set_log_botindex(int iIndex)
{
	iActBotIndex = iIndex;
}

void add_log (const char *fmt, ...)
{
	va_list va_alist;
	static char szLastLogMessage[LOGMSG_BUF];
	static int iRepeatCounter = 0;
	char logbuf[LOGMSG_BUF];
    FILE * fp;

	va_start (va_alist, fmt);
	_vsnprintf (logbuf, sizeof(logbuf), fmt, va_alist);
	va_end (va_alist);

	if ( !strcmp(logbuf, szLastLogMessage) )
	{
		iRepeatCounter++;
		return;
	}
	
   	time_t ticktime = time (NULL);
	struct tm * timestruct = localtime (&ticktime);

	if ( (fp = fopen(LOGFILE, "a")) != NULL )
	{
		if ( iRepeatCounter )
			fprintf ( fp, "%02d:%02d:%02d Last message has been repeated %d times\n", 
					timestruct->tm_hour, 
					timestruct->tm_min, 
					timestruct->tm_sec, 
					iRepeatCounter );

		iRepeatCounter = 0;
		strcpy(szLastLogMessage, logbuf);
		
		fprintf ( fp, "%02d:%02d:%02d %s\n", 
					timestruct->tm_hour, 
					timestruct->tm_min, 
					timestruct->tm_sec, 
					logbuf );

		fclose (fp);
		iLogSize += strlen(logbuf);
		if(iLogSize >= LOG_MAXSIZE)
		{
			fp = fopen(LOGFILE, "w");
			if ( fp )
			{
				fprintf(fp, "--- LOGFILE RESET ---\n");
				fclose(fp);
				iLogSize = 0;
			}
		}
	}
	else
		printf("%s\n", logbuf);
}


void report_log (int flags, const char * funcname, const char *fmt, ...)
{
	char logbuf[LOGMSG_BUF];

	if ( (iCurrentLogMask & flags) != flags && !(flags & LOG_ALWAYS) )
		return;

	if(iDebugBotIndex != -1)
	{
		if(iActBotIndex != iDebugBotIndex)
			return;
	}

	logbuf[0] = '\0';


	if ( flags & LOG_ENGINE )
		strcat(logbuf, "ENGINE CALL: ");
	else if ( flags & LOG_MESSAGE )
		strcat(logbuf, "MESSAGE: ");
	else if ( flags & LOG_DLL )
		strcat(logbuf, "DLL CALL: ");
	else if ( flags & LOG_BOTBASE )
		strcat(logbuf, "BOTBASE: ");
	else if ( flags & LOG_BOTNAV )
		strcat(logbuf, "BOTNAV: ");
	else if ( flags & LOG_BOTCOMBAT )
		strcat(logbuf, "BOTCOMBAT: ");
	else if ( flags & LOG_BOTMOVE )
		strcat(logbuf, "BOTMOVE: ");
	else if ( flags & LOG_BOTTASK )
		strcat(logbuf, "BOTTASK: ");
	else if ( flags & LOG_BOTAIM )
		strcat(logbuf, "BOTAIM: ");

	if ( funcname )
		sprintf(logbuf+strlen(logbuf), "%s(): ", funcname);

	if ( fmt )
	{
		va_list va_alist;
		va_start (va_alist, fmt);
		_vsnprintf (logbuf+strlen(logbuf), sizeof(logbuf)-strlen(logbuf), fmt, va_alist);
        logbuf[sizeof(logbuf)-1] = '\0';
		va_end (va_alist);
	}

	if ( logbuf[0] == '\0' )
		return;

    add_log("%s", logbuf);
}
