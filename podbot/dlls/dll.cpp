//
// dll.cpp
//
// Links Functions, handles Client Commands, initializes DLL
// and misc Stuff 

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"
#include "entity_state.h"

#include "bot.h"
#include "bot_weapons.h"
#include "bot_sounds.h"
#include "log.h"
#include "waypoint.h"
#include "bot_globals.h"

typedef int (FAR *GETENTITYAPI)(DLL_FUNCTIONS *, int);
// New since HL 1.1.0.4.
typedef int (FAR *GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS *, int *);

extern enginefuncs_t g_engfuncs;
extern globalvars_t  *gpGlobals;
extern char *g_argv;
extern GETENTITYAPI other_GetEntityAPI;
// New since HL 1.1.0.4.
extern GETNEWDLLFUNCTIONS other_GetNewDLLFunctions;
extern unsigned short FixedUnsigned16( float value, float scale );
extern skilldelay_t BotSkillDelays[6];
extern botaim_t BotAimTab[6];
extern bot_weapon_select_t cs_weapon_select[NUM_WEAPONS+1];
extern bot_t bots[MAXBOTSNUMBER];   // max of 32 bots in a game
extern PATH *paths[MAX_WAYPOINTS];


// ptr to Botnames are stored here
STRINGNODE *pBotNames;
// ptr to already used Names
STRINGNODE *pUsedBotNames[8];
// ptr to Kill Messages go here
STRINGNODE *pKillChat;
// ptr to BombPlant Messages go here
STRINGNODE *pBombChat;
// ptr to Deadchats go here
STRINGNODE *pDeadChat;
STRINGNODE *pUsedDeadStrings[8];
// ptr to Keywords & Replies for interactive Chat
replynode_t *pChatReplies = NULL;
// ptr to Strings when no keyword was found
STRINGNODE *pChatNoKeyword;

createbot_t BotCreateTab[32];
threat_t ThreatTab[32];
float next_modelcheat_time = 0.0;


// Sentences used with "speak" to welcome a User
char szSpeachSentences[16][80]={
	{"hello user,communication is acquired"},
	{"your presence is acknowledged"},
	{"high man, your in command now"},
	{"blast your hostile for good"},
	{"high man, kill some idiot here"},
	{"check check, test, mike check, talk device is activated"},
	{"good, day mister, your administration is now acknowledged"},
	{"high amigo, shoot some but"},
	{"hello pal, at your service"},
	{"time for some bad ass explosion"},
	{"high man, at your command"},
	{"bad ass son of a breach device activated"},
	{"high, do not question this great service"},
	{"engine is operative, hello and goodbye"},
	{"high amigo, your administration has been great last day"},
	{"all command access granted, over and out"}};


// Default Folder to load wpts from
char g_szWPTDirname[256]=
{"WPTDEFAULT"};

static FILE *fp;

// Xored Welcome Message to prevent lamers patching this - Text:

/*char szWelcomeMessage[]= {"Welcome to POD-Bot V2.5 by Count Floyd\n"
"Visit http://www.nuclearbox.com/podbot/ or\n"
"      http://www.botepidemic.com/podbot for Updates\n"};*/

unsigned char szWelcomeMessage[] = {
'W'^0x71, 'e'^0x71, 'l'^0x71, 'c'^0x71, 'o'^0x71, 'm'^0x71, 'e'^0x71, ' '^0x71,
't'^0x71, 'o'^0x71, ' '^0x71, 'P'^0x71, 'O'^0x71, 'D'^0x71, '-'^0x71, 'B'^0x71,
'o'^0x71, 't'^0x71, ' '^0x71, 'V'^0x71, '2'^0x71, '5'^0x71, ' '^0x71, 'b'^0x71,
'y'^0x71, ' '^0x71, 'C'^0x71, 'o'^0x71, 'u'^0x71, 'n'^0x71, 't'^0x71, ' '^0x71,
'F'^0x71, 'l'^0x71, 'o'^0x71, 'y'^0x71, 'd'^0x71, 0xa^0x71, 'V'^0x71, 'i'^0x71,
's'^0x71, 'i'^0x71, 't'^0x71, ' '^0x71, 'h'^0x71, 't'^0x71, 't'^0x71, 'p'^0x71,
':'^0x71, '/'^0x71, '/'^0x71, 'w'^0x71, 'w'^0x71, 'w'^0x71, '.'^0x71, 'n'^0x71,
'u'^0x71, 'c'^0x71, 'l'^0x71, 'e'^0x71, 'a'^0x71, 'r'^0x71, 'b'^0x71, 'o'^0x71,
'x'^0x71, '.'^0x71, 'c'^0x71, 'o'^0x71, 'm'^0x71, '/'^0x71, 'p'^0x71, 'o'^0x71,
'd'^0x71, 'b'^0x71, 'o'^0x71, 't'^0x71, '/'^0x71, ' '^0x71, 'o'^0x71, 'r'^0x71,
0xa^0x71, ' '^0x71, ' '^0x71, ' '^0x71, ' '^0x71, ' '^0x71, ' '^0x71, 'h'^0x71,
't'^0x71, 't'^0x71, 'p'^0x71, ':'^0x71, '/'^0x71, '/'^0x71, 'w'^0x71, 'w'^0x71,
'w'^0x71, '.'^0x71, 'b'^0x71, 'o'^0x71, 't'^0x71, 'e'^0x71, 'p'^0x71, 'i'^0x71,
'd'^0x71, 'e'^0x71, 'm'^0x71, 'i'^0x71, 'c'^0x71, '.'^0x71, 'c'^0x71, 'o'^0x71,
'm'^0x71, '/'^0x71, 'p'^0x71, 'o'^0x71, 'd'^0x71, 'b'^0x71, 'o'^0x71, 't'^0x71,
' '^0x71, 'f'^0x71, 'o'^0x71, 'r'^0x71, ' '^0x71, 'U'^0x71, 'p'^0x71, 'd'^0x71,
'a'^0x71, 't'^0x71, 'e'^0x71, 's'^0x71, 0x0^0x71};



DLL_FUNCTIONS other_gFunctionTable;
DLL_GLOBAL const Vector g_vecZero = Vector(0,0,0);

// Ptr to Hosting Edict
edict_t *pHostEdict = NULL;

// Array of connected Clients
client_t clients[32];

// Flag for noclip wpt editing
bool bEditNoclip=FALSE;

// Index of Beam Sprite
int m_spriteTexture = 0;

// Faked Client Command
int isFakeClientCommand = 0;
int fake_arg_count;

float bot_check_time = 30.0;

int num_bots = 0;
int prev_num_bots = 0;

bool g_GameRules = FALSE;
float respawn_time = 0.0;

int iStoreAddbotVars[3];

int iUserInMenu=0;


// Text and Key Flags for Menues
// \y & \w are special CS Colour Tags
menutext_t menuPODMain={
	0x2ff,
	"Please choose:\n\n\n"
	"1. Quick add Bot\n"
	"2. Add specific Bot\n"
	"3. Kill all Bots\n"
	"4. Newround (All Players)\n"};

char szmenuPODMain2[]={
	"5. Fill Server with Bots\n"
	"6. Kick Random Bot\n"
	"7. Remove all Bots\n"
	"8. Select Weapon Mode\n\n"
	"0. Cancel"};

menutext_t menuSelectWeaponMode={
	0x27f,
	"Please choose a Bot Weapon Mode:\n\n\n"
	"1. Knives only (JasonMode!)\n"
	"2. Pistols only\n"
	"3. Shotguns only\n"};

char szmenuSelectWeaponMode2[] = {
	"4. Machine Guns only\n"
	"5. Rifles only\n"
	"6. Sniper Weapons only\n"
	"7. All Weapons (Standard)\n\n"
	"0. Cancel"};

menutext_t menuSelectBotskill={
	0x23f,
	"Please choose a Skill:\n\n\n"
	"1. Stupid (0-20)\n"
	"2. Newbie (20-40)\n"
	"3. Average (40-60)\n"
	"4. Advanced (60-80)\n"
	"5. Professional (80-99)\n"
	"6. Godlike (100)\n\n"
	"0. Cancel"};

menutext_t menuSelectTeam={
	0x213,
	"Please choose a Team:\n\n\n"
	"1. Terrorist\n"
	"2. Counter-Terrorist\n\n"
	"5. Auto-Assign\n\n"
	"0. Cancel"};

menutext_t menuSelectTModel={
	0x21f,
	"Please choose a Terrorist Model:\n\n\n"
	"1. Phoenix Connexion\n"
	"2. L337 Krew\n"
	"3. Arctic Avengers\n"
	"4. Guerilla Warfare\n\n"
	"5. Random"};

menutext_t menuSelectCTModel={
	0x21f,
	"Please choose a CT Model:\n\n\n"
	"1. Seal Team 6\n"
	"2. GSG-9\n"
	"3. SAS\n"
	"4. GIGN\n\n"
	"5. Random"};

menutext_t menuWaypointAdd={
	0x3ff,
	"Select Waypoint Type:\n\n\n1. Normal Waypoint\n\\r2. Terrorist Important Waypoint\n"
	"3. Counter Terrorist Important Waypoint\n\\w4. Ladder Waypoint\n"};
char szWaypointAdd2[]={
	"\\y5. Rescue Waypoint\n"
	"\\w6. Camp Waypoint Start\n7. Camp Waypoint End\n\\r8. Map Goal Waypoint\n\n\\w9. More Types...\n\n"
	"\\w0. Cancel"}; 

menutext_t menuWaypointAdd3={
	0x203,
	"Select Waypoint Type:\n\n\n1. Jump Waypoint\n\n"
	"0. Cancel"};

menutext_t menuWaypointAddFlag={
	0x207,
	"Select Flag to add:\n\n\n1. Block with Hostage\n"
	"2. Terrorists ONLY Campflag\n"
	"3. CT's ONLY Campflag\n\n"
	"0. Cancel"}; 

menutext_t menuWaypointDeleteFlag={
	0x207,
	"Select Flag to delete:\n\n\n1. Block with Hostage\n"
	"2. Terrorists ONLY Campflag\n"
	"3. CT's ONLY Campflag\n\n"
	"0. Cancel"}; 

cvar_t sv_bot = {"pb",""};


void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);

// Helper Function to parse in a Text from File
void FillBufferFromFile(FILE* fpFile,char *pszBuffer,int file_index)
{
	int ch;

	ch = fgetc(fpFile);
	
	// skip any leading blanks
	while (ch == ' ')
		ch = fgetc(fpFile);
	
	while ((ch != EOF) && (ch != '\r') && (ch != '\n'))
	{
		if (ch == '\t')  // convert tabs to spaces
			ch = ' ';
		
		pszBuffer[file_index] = ch;
		
		ch = fgetc(fpFile);
		
		// skip multiple spaces in input file
		while ((pszBuffer[file_index] == ' ') &&
			(ch == ' '))      
			ch = fgetc(fpFile);
		
		file_index++;
	}
	
	if (ch == '\r')  // is it a carriage return?
	{
		ch = fgetc(fpFile);  // skip the linefeed
	}
	
	pszBuffer[file_index] = 0;  // terminate the command line
}


// Prints Message either to DS Console or Host console
void PrintServerMessage(char *pszMessage)
{
	if (IS_DEDICATED_SERVER())
		printf(pszMessage);
	else
		ALERT( at_console, pszMessage );
}


// First function called from HL in our replacement
// Server DLL
void GameDLLInit( void )
{
	int file_index;
	FILE *fpText;
	char szError[80];
	char szBuffer[256];
	int iCount;
	int iAllocLen;
	int i;


	// Register Variable for dedicated Server
	CVAR_REGISTER (&sv_bot);

	// Initialise the Client Struct for welcoming
	// and keeping track who's connected
	for (i=0; i<32; i++)
	{
		clients[i].pEntity = NULL;
		clients[i].bNeedWelcome = FALSE;
		clients[i].welcome_time = 0.0;
	}
	
	// Clear Array of used Botnames
	for(i=0;i<8;i++)
		pUsedBotNames[i]=NULL;

	// Load & Initialise Botnames from 'Botnames.txt'
	iCount=0;
#ifndef __linux__
	fpText = fopen("cstrike\\PODBot\\BotNames.txt", "r");
#else
	fpText = fopen("cstrike/PODBot/BotNames.txt", "r");
#endif

	if(fpText==NULL)
	{
		strcpy(szError,"Podbot couldn't find Botnames.txt!\n");
		ALERT( at_error, szError );
		fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
	}
				
	file_index = 0;
	szBuffer[file_index] = 0;  // null out Buffer

	strcpy(szError,"Podbot out of Memory !\n");


	pBotNames = NULL;
	STRINGNODE *pTempNode = NULL;
	STRINGNODE **pNode = &pBotNames;

	while ((fpText != NULL) && (!feof(fpText)))
	{
		FillBufferFromFile(fpText,(char*)&szBuffer,file_index);
		
		file_index = 0;  // reset for next input line
		if ((szBuffer[0] == '#') || (szBuffer[0] == 0) ||
			(szBuffer[0] == '\r') || (szBuffer[0]== '\n'))
		{
			continue;  // ignore comments or blank lines
		}
		szBuffer[21] = 0;
		iAllocLen = strlen(szBuffer)+1;
		if(iAllocLen<3)
			continue;
		pTempNode = new STRINGNODE;
		if(pTempNode==NULL)
		{
			ALERT( at_error, szError );
			fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
		}
		*pNode = pTempNode;
		pTempNode->pszString = new char[iAllocLen];
		pTempNode->Next = NULL;
		strcpy(pTempNode->pszString,&szBuffer[0]);
		pNode = &pTempNode->Next;
		iCount++;
    }      
	if (fpText != NULL)
	{
		fclose(fpText);
		fpText = NULL;
	}
	iNumBotnames=iCount-1;

	// End Botnames


	// Load & Initialise Botchats from 'Botchat.txt'
#ifndef __linux__
	fpText = fopen("cstrike\\PODBot\\BotChat.txt", "r");
#else
	fpText = fopen("cstrike/PODBot/BotChat.txt", "r");
#endif
	if(fpText==NULL)
	{
		strcpy(szError,"Podbot couldn't find BotChat.txt!\n");
		ALERT( at_error, szError );
		fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
	}
				
	strcpy(szError,"Podbot out of Memory !\n");

	file_index = 0;
	szBuffer[file_index] = 0;  // null out Buffer
	int iChatType=-1;
	replynode_t *pTempReply = NULL;
	replynode_t **pReply;
				
	while ((fpText != NULL) && (!feof(fpText)))
	{
		FillBufferFromFile(fpText,(char*)&szBuffer,file_index);
		
		file_index = 0;  // reset for next input line
		if ((szBuffer[0] == '#') || (szBuffer[0] == 0) ||
			(szBuffer[0] == '\r') || (szBuffer[0]== '\n'))
		{
			continue;  // ignore comments or blank lines
		}

		// Killed Chat Section ?
		if(FStrEq(szBuffer,"[KILLED]"))
		{
			iChatType = 0;
			iNumKillChats = 0;
			pKillChat = NULL;
			pNode = &pKillChat;
			pTempNode = NULL;
			continue;
		}
		// Bomb Chat Section ?
		else if(FStrEq(szBuffer,"[BOMBPLANT]"))
		{
			iChatType = 1;
			iNumBombChats = 0;
			pBombChat = NULL;
			pNode = &pBombChat;
			pTempNode = NULL;
			continue;
		}
		// Dead Chat Section ?
		else if(FStrEq(szBuffer,"[DEADCHAT]"))
		{
			iChatType = 2;
			iNumDeadChats = 0;
			pDeadChat = NULL;
			pNode = &pDeadChat;
			pTempNode = NULL;
			continue;
		}
		// Keyword Chat Section ?
		else if(FStrEq(szBuffer,"[REPLIES]"))
		{
			iChatType = 3;
			pReply = &pChatReplies;
			continue;
		}
		// Unknown Keyword Section ?
		else if(FStrEq(szBuffer,"[UNKNOWN]"))
		{
			iChatType = 4;
			iNumNoKeyStrings = 0;
			pChatNoKeyword = NULL;
			pNode = &pChatNoKeyword;
			pTempNode = NULL;
			continue;
		}

		if(iChatType != 3)
		{
			strcat(szBuffer,"\n");
			szBuffer[79]=0;
			iAllocLen=strlen(szBuffer)+1;
			if(iAllocLen<3)
				continue;
		}

		if(iChatType == 0)
		{
			pTempNode = new STRINGNODE;
			if(pTempNode == NULL)
			{
				ALERT( at_error, szError );
				fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
			}
			*pNode = pTempNode;
			pTempNode->pszString = new char[iAllocLen];
			pTempNode->Next = NULL;
			strcpy(pTempNode->pszString,&szBuffer[0]);
			pNode = &pTempNode->Next;
			iNumKillChats++;
		}
		else if(iChatType == 1)
		{
			pTempNode = new STRINGNODE;
			if(pTempNode==NULL)
			{
				ALERT( at_error, szError );
				fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
			}
			*pNode = pTempNode;
			pTempNode->pszString = new char[iAllocLen];
			pTempNode->Next = NULL;
			strcpy(pTempNode->pszString,&szBuffer[0]);
			pNode = &pTempNode->Next;
			iNumBombChats++;
		}
		else if(iChatType == 2)
		{
			pTempNode = new STRINGNODE;
			if(pTempNode==NULL)
			{
				ALERT( at_error, szError );
				fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
			}
			*pNode = pTempNode;
			pTempNode->pszString = new char[iAllocLen];
			pTempNode->Next = NULL;
			strcpy(pTempNode->pszString,&szBuffer[0]);
			pNode = &pTempNode->Next;
			iNumDeadChats++;
		}
		else if(iChatType == 3)
		{
			char *pPattern = strstr(szBuffer,"@KEY");
			if(pPattern != NULL)
			{
				pTempReply = new replynode_t;
				*pReply = pTempReply;
				pTempReply->pNextReplyNode = NULL;
				pTempReply->pReplies = NULL;
				pTempReply->cNumReplies = 0;
				pTempReply->cLastReply = 0;
				pNode = &pTempReply->pReplies;
				pTempNode = NULL;
				memset(pTempReply->pszKeywords, 0, sizeof(pTempReply->pszKeywords));
				int c=0;
				for(int i=0;i<256;i++)
				{
					if(szBuffer[i] == '\"')
					{
						i++;
						while(szBuffer[i] != '\"')
						{
							pTempReply->pszKeywords[c++] = szBuffer[i++];
						}
						pTempReply->pszKeywords[c++] = '@';
					}
					else if(szBuffer[i] == 0x0)
						break;
				}
				pReply = &pTempReply->pNextReplyNode;
			}
			else if(pTempReply)
			{
				strcat(szBuffer,"\n");
				szBuffer[255]=0;
				iAllocLen=strlen(szBuffer)+1;
				pTempNode = new STRINGNODE;
				if(pTempNode==NULL)
				{
					ALERT( at_error, szError );
					fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
				}
				*pNode = pTempNode;
				pTempNode->pszString = new char[iAllocLen];
				pTempNode->Next = NULL;
				strcpy(pTempNode->pszString,&szBuffer[0]);
				pTempReply->cNumReplies++;
				pNode = &pTempNode->Next;
			}
		}
		else if(iChatType == 4)
		{
			pTempNode = new STRINGNODE;
			if(pTempNode==NULL)
			{
				ALERT( at_error, szError );
				fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
			}
			*pNode = pTempNode;
			pTempNode->pszString = new char[iAllocLen];
			pTempNode->Next = NULL;
			strcpy(pTempNode->pszString,&szBuffer[0]);
			pNode = &pTempNode->Next;
			iNumNoKeyStrings++;
		}
    }      
	if (fpText != NULL)
	{
		fclose(fpText);
		fpText = NULL;
	}
	iNumKillChats--;
	iNumBombChats--;
	iNumDeadChats--;
	iNumNoKeyStrings--;

	// End Botchats


	// Load & Initialise Botskill.cfg
#ifndef __linux__
	fpText = fopen("cstrike\\PODBot\\Botskill.cfg", "r");
#else
	fpText = fopen("cstrike/PODBot/Botskill.cfg", "r");
#endif
	if(fpText==NULL)
	{
		strcpy(szError,"No Botskill.cfg ! Using defaults...\n");
		ALERT( at_error, szError );
		fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
	}

	int ch;
	char cmd_line[80];
	char server_cmd[80];
	char *cmd, *arg1, *arg2, *arg3, *arg4;

	int BotTabCount=0;

	while ((fpText != NULL) && !feof(fpText))
	{
		ch = fgetc(fpText);
		
		// skip any leading blanks
		while (ch == ' ')
			ch = fgetc(fpText);
		
		while ((ch != EOF) && (ch != '\r') && (ch != '\n'))
		{
			if (ch == '\t')  // convert tabs to spaces
				ch = ' ';
			
			cmd_line[file_index] = ch;
			
			ch = fgetc(fpText);
			
			// skip multiple spaces in input file
			while ((cmd_line[file_index] == ' ') &&
				(ch == ' '))      
				ch = fgetc(fpText);
			
			file_index++;
		}
		
		if (ch == '\r')  // is it a carriage return?
		{
			ch = fgetc(fpText);  // skip the linefeed
		}
		
		cmd_line[file_index] = 0;  // terminate the command line
		
		// copy the command line to a server command buffer...
		strcpy(server_cmd, cmd_line);
		strcat(server_cmd, "\n");
		
		file_index = 0;
		cmd = cmd_line;
		arg1 = arg2 = arg3 = arg4 = NULL;
		
		// skip to blank or end of string...
		while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
			file_index++;
		
		if (cmd_line[file_index] == ' ')
		{
			cmd_line[file_index++] = 0;
			arg1 = &cmd_line[file_index];
			
			// skip to blank or end of string...
			while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
				file_index++;
			
			if (cmd_line[file_index] == ' ')
			{
				cmd_line[file_index++] = 0;
				arg2 = &cmd_line[file_index];
				
				// skip to blank or end of string...
				while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
					file_index++;
				
				if (cmd_line[file_index] == ' ')
				{
					cmd_line[file_index++] = 0;
					arg3 = &cmd_line[file_index];
					
					// skip to blank or end of string...
					while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
						file_index++;
					
					if (cmd_line[file_index] == ' ')
					{
						cmd_line[file_index++] = 0;
						arg4 = &cmd_line[file_index];
					}
				}
			}
		}
		
		file_index = 0;  // reset for next input line
		
		if ((cmd_line[0] == '#') || (cmd_line[0] == 0))
		{
			continue;  // ignore comments or blank lines
		}
		else if(FStrEq(cmd,"MIN_DELAY"))
		{
			BotSkillDelays[BotTabCount].fMinSurpriseDelay=(float)atof(arg1);
		}
		else if(FStrEq(cmd,"MAX_DELAY"))
		{
			BotSkillDelays[BotTabCount].fMaxSurpriseDelay=(float)atof(arg1);
		}
		else if(FStrEq(cmd,"HEADSHOT_ALLOW"))
		{
			BotAimTab[BotTabCount].iHeadShot_Frequency=atoi(arg1);
		}
		else if(FStrEq(cmd,"HEAR_SHOOTTHRU"))
		{
			BotAimTab[BotTabCount].iHeardShootThruProb=atoi(arg1);
		}
		else if(FStrEq(cmd,"SEEN_SHOOTTHRU"))
		{
			BotAimTab[BotTabCount].iSeenShootThruProb=atoi(arg1);
			BotTabCount++;
			// Prevent Overflow if Errors in cfg
			BotTabCount%=6;
		}
    }
		 
	// if bot.cfg file is open and reached end of file, then close and free it
	if ((fpText != NULL) && feof(fpText))
	{
		fclose(fpText);
		fpText = NULL;
	}

	// End Botskill.cgf


	// Load & Initialise BotLogos from BotLogos.cfg

#ifndef __linux__
	fpText = fopen("cstrike\\PODBot\\BotLogos.cfg", "r");
#else
	fpText = fopen("cstrike/PODBot/BotLogos.cfg", "r");
#endif

	if(fpText==NULL)
	{
		strcpy(szError,"No BotLogos.cfg ! Using Defaults...\n");
		ALERT( at_error, szError );
		fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
	}
	else
	{
		file_index = 0;
		szBuffer[file_index] = 0;  // null out Buffer
		g_iNumLogos=0;
		while ((fpText != NULL) && (!feof(fpText)))
		{
			FillBufferFromFile(fpText,(char*)&szBuffer,file_index);
			file_index = 0;  // reset for next input line
			if ((szBuffer[0] == '#') || (szBuffer[0] == 0) ||
				(szBuffer[0] == '\r') || (szBuffer[0]== '\n'))
			{
				continue;  // ignore comments or blank lines
			}
			strcpy(&szSprayNames[g_iNumLogos][0],&szBuffer[0]);
			g_iNumLogos++;
		}
		g_iNumLogos--;
		if (fpText != NULL)
		{
			fclose(fpText);
			fpText = NULL;
		}
	}

	// End BotLogos


	// Load & initialise Weapon Stuff from 'BotWeapons.cfg'

#ifndef __linux__
	fpText = fopen("cstrike\\PODBot\\BotWeapons.cfg", "r");
#else
	fpText = fopen("cstrike/PODBot/BotWeapons.cfg", "r");
#endif

	if(fpText==NULL)
	{
		strcpy(szError,"No BotWeapons.cfg ! Using Defaults...\n");
		ALERT( at_error, szError );
		fp=fopen("PODERROR.txt","a"); fprintf(fp, szError); fclose(fp);
	}
	else
	{
		file_index = 0;
		szBuffer[file_index] = 0;  // null out Buffer
		int *ptrWeaponPrefs=NULL;
		int iParseWeapons=0;
		int iWeaponPrefsType;
		char *pszStart;
		char *pszEnd;

		while ((fpText != NULL) && (!feof(fpText)))
		{
			FillBufferFromFile(fpText,(char*)&szBuffer,file_index);
			
			file_index = 0;  // reset for next input line
			if ((szBuffer[0] == '#') || (szBuffer[0] == 0) ||
				(szBuffer[0] == '\r') || (szBuffer[0]== '\n'))
			{
				continue;  // ignore comments or blank lines
			}
			if(iParseWeapons<2)
			{
				if(FStrEq(szBuffer,"[STANDARD]"))
					iWeaponPrefsType = MAP_DE;
				else if(FStrEq(szBuffer,"[AS]"))
					iWeaponPrefsType = MAP_AS;
				else
				{
					pszStart = &szBuffer[0];
					pszEnd = NULL;
					if(iWeaponPrefsType == MAP_DE)
					{
						for(i=0;i<NUM_WEAPONS;i++)
						{
							pszEnd=strchr(pszStart,',');
							cs_weapon_select[i].iTeamStandard = atoi(pszStart);
							pszStart = pszEnd+1;
						}
					}
					else
					{
						for(i=0;i<NUM_WEAPONS;i++)
						{
							pszEnd=strchr(pszStart,',');
							cs_weapon_select[i].iTeamAS = atoi(pszStart);
							pszStart = pszEnd+1;
						}
					}
					iParseWeapons++;
				}
			}
			else
			{
				if(FStrEq(szBuffer,"[NORMAL]"))
					ptrWeaponPrefs=&NormalWeaponPrefs[0];
				else if (FStrEq(szBuffer,"[AGRESSIVE]"))
					ptrWeaponPrefs=&AgressiveWeaponPrefs[0];
				else if (FStrEq(szBuffer,"[DEFENSIVE]"))
					ptrWeaponPrefs=&DefensiveWeaponPrefs[0];
				else
				{
					pszStart = &szBuffer[0];
					for(i=0;i<NUM_WEAPONS;i++)
					{
						pszEnd=strchr(pszStart,',');
						*ptrWeaponPrefs++ = atoi(pszStart);
						pszStart = pszEnd+1;
					}
				}
			}
		}
	}
	
	// Decrypt welcome message
	unsigned char byCode = 255;
	i = 0;
	while(byCode != 0)
	{
		byCode = szWelcomeMessage[i];
		byCode ^= 0x71;
		szWelcomeMessage[i] = byCode;
		i++;
	}


	// Initialize the bots array of structures
	memset(bots, 0, sizeof(bots));
	memset(BotCreateTab, 0, sizeof(BotCreateTab));
	(*other_gFunctionTable.pfnGameInit)();
}


// Something gets spawned in the game
int DispatchSpawn( edict_t *pent )
{
   if (gpGlobals->deathmatch)
   {
      char *pClassname = (char *)STRING(pent->v.classname);

#ifdef DEBUG
	  report_log (LOG_DLL, "DispatchSpawn", "%s", pClassname);
#endif

      if (strcmp(pClassname, "worldspawn") == 0)
      {
		 // BOTMAN START
         // do level initialization stuff here...

         WaypointInit();
         WaypointLoad(NULL);

         PRECACHE_SOUND("weapons/xbow_hit1.wav");      // waypoint add
         PRECACHE_SOUND("weapons/mine_activate.wav");  // waypoint delete
         PRECACHE_SOUND("common/wpn_hudoff.wav");      // path add/delete start
         PRECACHE_SOUND("common/wpn_hudon.wav");       // path add/delete done
         PRECACHE_SOUND("common/wpn_moveselect.wav");  // path add/delete cancel
         PRECACHE_SOUND("common/wpn_denyselect.wav");  // path add/delete error

         m_spriteTexture = PRECACHE_MODEL( "sprites/lgtning.spr");

         g_GameRules = TRUE;
         prev_num_bots = num_bots;
         num_bots = 0;
         respawn_time = 0.0;
         bot_check_time = gpGlobals->time + 30.0;
		 // END BOTMAN
      }
   }

   return (*other_gFunctionTable.pfnSpawn)(pent);
}

void DispatchThink( edict_t *pent )
{
   (*other_gFunctionTable.pfnThink)(pent);
}

void DispatchUse( edict_t *pentUsed, edict_t *pentOther )
{
   (*other_gFunctionTable.pfnUse)(pentUsed, pentOther);
}

void DispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
   (*other_gFunctionTable.pfnTouch)(pentTouched, pentOther);
}

void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther )
{
   (*other_gFunctionTable.pfnBlocked)(pentBlocked, pentOther);
}

void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
   (*other_gFunctionTable.pfnKeyValue)(pentKeyvalue, pkvd);
}

void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData )
{
   (*other_gFunctionTable.pfnSave)(pent, pSaveData);
}

int DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity )
{
   return (*other_gFunctionTable.pfnRestore)(pent, pSaveData, globalEntity);
}

void DispatchObjectCollsionBox( edict_t *pent )
{
   (*other_gFunctionTable.pfnSetAbsBox)(pent);
}

void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
   (*other_gFunctionTable.pfnSaveWriteFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
   (*other_gFunctionTable.pfnSaveReadFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveGlobalState( SAVERESTOREDATA *pSaveData )
{
   (*other_gFunctionTable.pfnSaveGlobalState)(pSaveData);
}

void RestoreGlobalState( SAVERESTOREDATA *pSaveData )
{
   (*other_gFunctionTable.pfnRestoreGlobalState)(pSaveData);
}

void ResetGlobalState( void )
{
   (*other_gFunctionTable.pfnResetGlobalState)();
}


// Client connects to this Server
BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{
	int i;
	int count = 0;
	
#ifdef DEBUG
	report_log (LOG_DLL, "ClientConnect", "pent=%x name=%s", pEntity,pszName);
#endif
	
	// check if this client is the listen server client
	if (strcmp(pszAddress, "loopback") == 0)
	{
		// save the edict of the listen server client...
		pHostEdict = pEntity;
	}
	// check if this is NOT a bot joining the server...
	if (strcmp(pszAddress, "127.0.0.1") != 0)
	{
		// don't try to add bots for 30 seconds, give client time to get added
		bot_check_time = gpGlobals->time + 30.0;
		
		for (i=0; i <MAXBOTSNUMBER; i++)
		{
			if (bots[i].is_used)  // count the number of bots in use
				count++;
		}
		
		// if there are currently more than the minimum number of bots running
		// then kick one of the bots off the server...
		if ((count > min_bots) && (min_bots != -1))
		{
			for (i=0; i < 32; i++)
			{
				if (bots[i].is_used)  // is this slot used?
				{
					char cmd[80];
					
					sprintf(cmd, "kick \"%s\"\n", bots[i].name);
					
					SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
					
					break;
				}
			}
		}
	}
	
	return (*other_gFunctionTable.pfnClientConnect)(pEntity, pszName, pszAddress, szRejectReason);
}


// Client disconnects from this Server
void ClientDisconnect( edict_t *pEntity )
{

	int i;
		
#ifdef DEBUG
		report_log (LOG_DLL, "ClientDisconnect", "pent=%x", pEntity);
#endif

	if (gpGlobals->deathmatch)
	{

		// Find & remove this Client from our list of Clients
		// connected
		i = 0;
		while ((i < 32) && (clients[i].pEntity != pEntity))
			i++;
		if (i < 32)
			clients[i].pEntity = NULL;
		
		
		
		// Check if its a Bot
		for (i=0; i < 32; i++)
		{
			if (bots[i].pEdict == pEntity)
			{
				// Delete Nodes from Pathfinding
				DeleteSearchNodes(&bots[i]);
				BotResetTasks(&bots[i]);
				bots[i].is_used = FALSE;  // this slot is now free to use
				bots[i].f_kick_time = gpGlobals->time;  // save the kicked time

				// Suggested fix from Leon Hartwig 
				FREE_PRIVATE(pEntity);
				break;
			}
		}
		
		
	}
	(*other_gFunctionTable.pfnClientDisconnect)(pEntity);
}


void ClientKill( edict_t *pEntity )
{
#ifdef DEBUG
	report_log (LOG_DLL, "ClientKill", "pent=%x", pEntity);
#endif
   (*other_gFunctionTable.pfnClientKill)(pEntity);
}

// Client is finally put into the Server
void ClientPutInServer( edict_t *pEntity )
{
#ifdef DEBUG
	report_log (LOG_DLL, "ClientPutInServer", "pent=%x", pEntity);
#endif

	int i = 0;
	while ((i < 32) && (clients[i].pEntity != NULL))
		i++;
	if (i < 32)
	{
		clients[i].pEntity = pEntity;
		if(pEntity->v.flags & FL_FAKECLIENT)
		{
			clients[i].bNeedWelcome=FALSE;
			clients[i].welcome_time=0.0;
		}
		else
		{
			clients[i].bNeedWelcome=TRUE;
			clients[i].welcome_time=0.0;
		}
	}
   (*other_gFunctionTable.pfnClientPutInServer)(pEntity);
}


// Executed if a client typed some sort of command
// into the console
void ClientCommand( edict_t *pEntity )
{
	if ((gpGlobals->deathmatch))
    {
		const char *pcmd = Cmd_Argv(0);
		const char *arg1 = Cmd_Argv(1);
		const char *arg2 = Cmd_Argv(2);
		const char *arg3 = Cmd_Argv(3);
		const char *arg4 = Cmd_Argv(4);
		char msg[80];

#ifdef DEBUG
	report_log (LOG_DLL, "ClientCommand", "cmd=%s arg1=%s arg2=%s", pcmd,arg1,arg2);
#endif
	// don't search ClientCommands of Bots or other Edicts than Host!
		if((isFakeClientCommand==0)	&& (pEntity == pHostEdict)
			&& (!IS_DEDICATED_SERVER()))
		{
			// "addbot"
			if (FStrEq(pcmd,CONSOLE_CMD_ADDBOT))
			{
				// Search Creation Tab for a free slot
				int index = 0;
				while ((BotCreateTab[index].bNeedsCreation) && (index < MAXBOTSNUMBER))
					index++;
			  
				// Needs to be created
				BotCreateTab[index].bNeedsCreation=TRUE;
				// Entity who issued it
				BotCreateTab[index].pCallingEnt=pEntity;
				// copy arguments
				if(arg3 != NULL)
					strcpy(BotCreateTab[index].name,arg3);
				else
					memset(BotCreateTab[index].name,0,BOT_NAME_LEN+1);

				if(arg2 != NULL)
					strcpy(BotCreateTab[index].team,arg2);
				else
					strcpy(BotCreateTab[index].team,"5");
				
				if(arg1 != NULL)
					strcpy(BotCreateTab[index].skill,arg1);
				else
					BotCreateTab[index].skill[0] = '\0';
				// create soon...
				if(botcreation_time==0.0)
					botcreation_time=gpGlobals->time;
				return;
			}
			// "removebots"
			else if (FStrEq(pcmd,CONSOLE_CMD_REMOVEALLBOTS))
			{
				UserRemoveAllBots(pEntity);
				return;
			}
			// "killbots"
			else if (FStrEq(pcmd,CONSOLE_CMD_KILLALLBOTS))
			{
				UserKillAllBots(pEntity);
				return;
			}

#ifdef _DEBUG
			// Shows Bots on the Server
			else if (FStrEq(pcmd,"listbots"))
			{
				UserListBots(pEntity);
				return;
			}
			// Turns on logging for a single Bot
			// spectated by Host
			else if (FStrEq(pcmd,"dbl"))
			{
			    if(!FNullEnt(pHostEdict))
				{
					edict_t *pDebugEdict = INDEXENT(pHostEdict->v.iuser2);
					if(!FNullEnt(pDebugEdict))
					{
						set_log_mask(LOG_SINGLEBOT);
						set_debuglog_bot(UTIL_GetBotIndex(pDebugEdict));
					}
					return;
				}
			}
			// Turns on signle step debugging for a single Bot
			// (breaks on assert)
			else if (FStrEq(pcmd,"dbs"))
			{
			    if(!FNullEnt(pHostEdict))
				{
					edict_t *pDebugEdict = INDEXENT(pHostEdict->v.iuser2);
					if(!FNullEnt(pDebugEdict))
					{
						bot_t *pDebugBot = UTIL_GetBotPointer(pDebugEdict);
						if(pDebugBot != NULL)
							pDebugBot->bDebug = TRUE;
					}
				}
				return;
			}
			// Turns on Debug displaying for a single Bot
			// -1 turns it off
			else if (FStrEq(pcmd,"db"))
			{
				g_iDebugBotIndex = atoi(arg1);
				return;
			}
			// Turns on Debug displaying for a specific Task
			// -1 turns it off
			else if (FStrEq(pcmd,"dbt"))
			{
				g_iDebugTaskNumber = atoi(arg1);
				return;
			}
			else if (FStrEq(pcmd,"dba"))
			{
			    if(!FNullEnt(pHostEdict))
				{
					edict_t *pDebugEdict = INDEXENT(pHostEdict->v.iuser2);
					if(!FNullEnt(pDebugEdict))
					{
						set_log_mask(LOG_BOTAIM);
						set_debuglog_bot(UTIL_GetBotIndex(pDebugEdict));
					}
					return;
				}
			}
			else if (FStrEq(pcmd,"dbkillme"))
			{
				g_bKillMe = TRUE;
				return;
			}
			// Shows the shortest Path from A to B
			else if (FStrEq(pcmd,"dbp1"))
			{
				int iSource,iDest;
				if(arg2 == NULL)
				{
					iSource = WaypointFindNearestToMove(pEntity->v.origin);
					iDest = atoi(arg1);
				}
				else
				{
					iSource = atoi(arg1);
					iDest = atoi(arg2);
				}
				ShowPath(iSource,iDest);
				return;
			}
			else if (FStrEq(pcmd,"dbp2"))
			{
				int iSource,iDest;
				iSource = WaypointFindNearestToMove(pEntity->v.origin);
				iDest = atoi(arg1);
				unsigned char type = atoi(arg2);
				TestAPath(pEntity,iSource,iDest,type);
				return;
			}
			else if (FStrEq(pcmd,"dbshowents"))
			{
				DebugShowEntities(pEntity);
				return;
			}
			else if (FStrEq(pcmd,"dbentinfo"))
			{
				if(arg1 != NULL)
					DebugShowEntInfo(atoi(arg1));
				return;
			}
			else if (FStrEq(pcmd,"dbshowtrace"))
			{
				DebugTraceEntities(pEntity);
				return;
			}
#endif
			// Forces Bots to use a specified Waypoint as a Goal
			else if (FStrEq(pcmd,"debuggoal"))
			{
				g_iDebugGoalIndex=atoi(arg1);
				sprintf(msg, "Goal for Bot set to:%d\n",g_iDebugGoalIndex);
				UTIL_HostPrint(HUD_PRINTNOTIFY, msg);
				return;
			}
			// "waypoint"
			else if (FStrEq(pcmd,CONSOLE_CMD_WAYPOINT))
			{
				if(arg1 != NULL)
				{
					if (FStrEq(arg1, "on"))
					{
						g_bWaypointOn = TRUE;
						UTIL_HostPrint(HUD_PRINTNOTIFY, "Waypoints Editing is ON\n");
						if (arg2 != NULL && FStrEq(arg2, "noclip"))
						{
							bEditNoclip = TRUE;
							UTIL_HostPrint(HUD_PRINTNOTIFY, "Noclip Cheat is ON\n");
						}
						
					}
					else if (FStrEq(arg1, "off"))
					{
						g_bWaypointOn = FALSE;
						bEditNoclip=FALSE;
						pEntity->v.movetype = MOVETYPE_WALK;
						UTIL_HostPrint(HUD_PRINTNOTIFY, "Waypoint Editing turned OFF\n");
					}
					else if (FStrEq(arg1, "find"))
					{
						g_iFindWPIndex=atoi(arg2);
						if(g_iFindWPIndex<g_iNumWaypoints)
							UTIL_HostPrint(HUD_PRINTNOTIFY, "Showing Direction to Waypoint\n");
						else
							g_iFindWPIndex=-1;
					}
					else if (FStrEq(arg1, "add"))
					{
						if (!g_bWaypointOn)
							g_bWaypointOn = TRUE;  // turn waypoints on if off
						iUserInMenu=MENU_WAYPOINT_ADD;
						UTIL_ShowMenu(pEntity, menuWaypointAdd.ValidSlots, -1, TRUE, menuWaypointAdd.szMenuText);
						UTIL_ShowMenu(pEntity, menuWaypointAdd.ValidSlots, -1, FALSE,szWaypointAdd2);
					}
					else if (FStrEq(arg1, "delete"))
					{
						if (!g_bWaypointOn)
							g_bWaypointOn = TRUE;  // turn waypoints on if off
						
						WaypointDelete(pEntity);
					}
					else if (FStrEq(arg1, "save"))
					{
						if(arg2 != NULL && FStrEq(arg2, "nocheck"))
						{
							WaypointSave(pEntity);
							UTIL_HostPrint(HUD_PRINTNOTIFY, "Waypoints saved\n");
						}
						else if(WaypointNodesValid(pEntity))
						{
							WaypointSave(pEntity);
							UTIL_HostPrint(HUD_PRINTNOTIFY, "Waypoints saved\n");
						}
						else
							UTIL_HostPrint(HUD_PRINTNOTIFY, "Waypoints not saved - too many errors !\n");
					}
					else if (FStrEq(arg1, "load"))
					{
						if (WaypointLoad(pEntity))
							UTIL_HostPrint(HUD_PRINTNOTIFY, "Waypoints loaded\n");
					}
					else if (FStrEq(arg1, "stats"))
					{
						sprintf(msg,"Waypoint Statistics:\n"
							"--------------------\n");
						UTIL_HostPrint(HUD_PRINTCONSOLE, msg);
						int iTPoints=0;
						int iCTPoints=0;
						int iGoalPoints=0;
						int iRescuePoints=0;
						int iCampPoints=0;
						int iNoHostagePoints=0;
						for (int i=0; i < g_iNumWaypoints; i++)
						{
							if(paths[i]->flags & W_FL_TERRORIST)
								iTPoints++;
							else if(paths[i]->flags & W_FL_COUNTER)
								iCTPoints++;
							else if(paths[i]->flags & W_FL_GOAL)
								iGoalPoints++;
							else if(paths[i]->flags & W_FL_RESCUE)
								iRescuePoints++;
							else if(paths[i]->flags & W_FL_CAMP)
								iCampPoints++;
							else if(paths[i]->flags & W_FL_NOHOSTAGE)
								iNoHostagePoints++;
						}
						sprintf(msg,"Waypoints: %d - T Points: %d - CT Points: %d - Goal Points: %d\n"
							,g_iNumWaypoints,iTPoints,iCTPoints,iGoalPoints);
						UTIL_HostPrint(HUD_PRINTCONSOLE, msg);
						sprintf(msg,"Rescue Points: %d - Camp Points: %d - Block Hostage Points: %d\n"
							,iRescuePoints,iCampPoints,iNoHostagePoints);
						UTIL_HostPrint(HUD_PRINTCONSOLE, msg);
					}
					else if (FStrEq(arg1, "check"))
					{
						if(WaypointNodesValid(pEntity))
							UTIL_HostPrint(HUD_PRINTNOTIFY, "All Nodes work fine !\n");
					}
					else if (FStrEq(arg1, "showflags"))
					{
						int index;
						
						index=WaypointFindNearest(pEntity);
						if(index!=-1)
						{
							sprintf(msg,"Waypoint Flags:\n"
								"--------------------\n");
							UTIL_HostPrint(HUD_PRINTCONSOLE, msg);
							if(paths[index]->flags & W_FL_NOHOSTAGE)
								UTIL_HostPrint(HUD_PRINTCONSOLE, "W_FL_NOHOSTAGE\n");
						}
						else
							UTIL_HostPrint(HUD_PRINTCONSOLE, "No Near Waypoint!\n");
					}
					else if (FStrEq(arg1, "addflag"))
					{
						iUserInMenu=MENU_WAYPOINT_ADDFLAG;
						UTIL_ShowMenu(pEntity, menuWaypointAddFlag.ValidSlots, -1, FALSE, menuWaypointAddFlag.szMenuText);
					}
					else if (FStrEq(arg1, "delflag"))
					{
						iUserInMenu=MENU_WAYPOINT_DELETEFLAG;
						UTIL_ShowMenu(pEntity, menuWaypointDeleteFlag.ValidSlots, -1, FALSE, menuWaypointDeleteFlag.szMenuText);
					}
					else if (FStrEq(arg1, "setradius"))
					{
						WaypointSetRadius(pEntity,atoi(arg2));
					}
					else if (FStrEq(arg1, "cache"))
					{
						g_iCacheWaypointIndex=WaypointFindNearestToMove(pEntity->v.origin);
						sprintf(msg, "Waypoint No. %d now cached\n",g_iCacheWaypointIndex);
						UTIL_HostPrint(HUD_PRINTNOTIFY, msg);
					}
					else if (FStrEq(arg1, "teleport"))
					{
						int iTelIndex = atoi(arg2);
						if(iTelIndex < g_iNumWaypoints)
							pEntity->v.origin = paths[iTelIndex]->origin;
					}
					else
					{
						if (g_bWaypointOn)
							UTIL_HostPrint(HUD_PRINTNOTIFY, "Waypoints are ON\n");
						else
							UTIL_HostPrint(HUD_PRINTNOTIFY, "Waypoints are OFF\n");
					}
				}
				return;
			}
			// "experience"
			else if (FStrEq(pcmd,CONSOLE_CMD_EXPERIENCE))
			{
				if(arg1 != NULL)
				{
					if (FStrEq(arg1, "save"))
					{
						SaveExperienceTab(pEntity);
					}
					else if (FStrEq(arg1, "on"))
					{
						g_bDangerDirection = TRUE;
						g_bWaypointOn= TRUE;  // turn this on just in case
					}
					else if (FStrEq(arg1, "off"))
						g_bDangerDirection = FALSE;
				}
				return;
			}
			// "autowaypoint"
			else if (FStrEq(pcmd,CONSOLE_CMD_AUTOWAYPOINT))
			{
				if(arg1 != NULL)
				{
					if (FStrEq(arg1, "on"))
					{
						g_bAutoWaypoint = TRUE;
						g_bWaypointOn = TRUE;  // turn this on just in case
					}
					else if (FStrEq(arg1, "off"))
					{
						g_bAutoWaypoint = FALSE;
					}
					
					if (g_bAutoWaypoint)
						sprintf(msg, "Auto-Waypoint is ON\n");
					else
						sprintf(msg, "Auto-Waypoint is OFF\n");
					
					UTIL_HostPrint(HUD_PRINTNOTIFY, msg);
				}
				return;
			}
			// "pathwaypoint"
			else if (FStrEq(pcmd,CONSOLE_CMD_PATHWAYPOINT))
			{
				if(arg1 != NULL)
				{
					if (FStrEq(arg1, "on"))
					{
						g_bPathWaypoint = TRUE;
						g_bWaypointOn= TRUE;  // turn this on just in case
						UTIL_HostPrint(HUD_PRINTNOTIFY, "Path-Waypoint is ON\n");
					}
					else if (FStrEq(arg1, "off"))
					{
						g_bPathWaypoint = FALSE;
						UTIL_HostPrint(HUD_PRINTNOTIFY, "Path-Waypoint is OFF\n");
					}
					else if (FStrEq(arg1, "add"))
					{
						if(arg2 != NULL && *arg2!=0)
							WaypointCreatePath(pEntity, atoi(arg2));
						else
							WaypointCreatePath(pEntity, g_iCacheWaypointIndex);
					}
					else if (FStrEq(arg1, "delete"))
					{
						if(arg2 != NULL && *arg2!=0)
							WaypointRemovePath(pEntity, atoi(arg2));
						else
							WaypointRemovePath(pEntity, g_iCacheWaypointIndex);
					}
				}
				
				return;
			}
			// "wayzone"
			// Obsolete by now...
			else if (FStrEq(pcmd, CONSOLE_CMD_WAYZONE))
			{
				if(arg1 != NULL)
				{
					if (FStrEq(arg1, "calcall"))
						CalculateWayzones(pEntity);
				}
				return;
			}
			// "botchat"
			else if (FStrEq(pcmd, CONSOLE_CMD_BOTCHAT))
			{
				if(arg1 != NULL)
				{
					if (FStrEq(arg1, "on"))
						g_bBotChat=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bBotChat=FALSE;
				}
				return;
			}
			// "newmap"
			else if (FStrEq(pcmd, CONSOLE_CMD_NEWMAP))
			{
				if(arg1 != NULL && *arg1!=0)
				{
					if(IS_MAP_VALID((char*)arg1))
					{
						// This should toggle away the console,
						// doesn't work for some reason ?!?
						SERVER_COMMAND("toggleconsole\n");
						pfnChangeLevel((char*)arg1,NULL);
					}
					else
						UTIL_HostPrint(HUD_PRINTCONSOLE, "Not a valid Map!\n");
				}
				return;
			}
			// "jasonmode"
			else if (FStrEq(pcmd,CONSOLE_CMD_JASONMODE))
			{
				if(arg1 != NULL && *arg1!=0)
				{
					if (FStrEq(arg1, "on"))
					{
						UTIL_HostPrint(HUD_PRINTCONSOLE, "Scary Knifemode on!\n");
						g_bJasonMode=TRUE;
					}
					else if (FStrEq(arg1, "off"))
					{
						UTIL_HostPrint(HUD_PRINTCONSOLE, "Bots are sane again...\n");
						g_bJasonMode=FALSE;
					}
				}
				return;
			}
			// "podbotmenu"
			else if (FStrEq(pcmd,CONSOLE_CMD_PODMENU))
			{
				iUserInMenu=MENU_PODMAIN;
				UTIL_ShowMenu(pEntity, menuPODMain.ValidSlots, -1, TRUE, menuPODMain.szMenuText);
				UTIL_ShowMenu(pEntity, menuPODMain.ValidSlots, -1, FALSE,szmenuPODMain2);
				return;
			}
			// "imaborg"
			else if (FStrEq(pcmd,CONSOLE_CMD_BORG))
			{
				if(arg1 != NULL)
				{
					if (FStrEq(arg1, "on"))
					{
						// Turn off client prediction
						CVAR_SET_FLOAT("cl_lw",0);
						UTIL_HostPrint(HUD_PRINTCONSOLE, "You're assimilated !\n");
						CreateBorgBot(pEntity);
					}
					else if (FStrEq(arg1, "off"))
					{
						CVAR_SET_FLOAT("cl_lw",1);
						int index=UTIL_GetBotIndex(pEntity);
						if (index != -1)
						{
							DeleteSearchNodes(&bots[index]);
							bots[index].is_used = FALSE;  // this slot is now free to use
							pEntity->v.flags &= ~FL_FAKECLIENT;
							pEntity->v.flags &= ~FL_FROZEN;
							pEntity->v.flags |= FL_CLIENT;
							pEntity->v.spawnflags &= ~FL_FAKECLIENT;
							pEntity->v.spawnflags |= FL_CLIENT;
							pEntity->v.fixangle = 0;
							UTIL_HostPrint(HUD_PRINTCONSOLE, "You freed yourself from the Borg community !\n");
							g_bIsBorg = FALSE;
						}
					}
				}
				return;
			}
			// "wptfolder"
			else if (FStrEq(pcmd, CONSOLE_CMD_WPTFOLDER))
			{
				if(arg1 != NULL && *arg1!=0)
					strcpy(g_szWPTDirname,arg1);
				else
				{
					char dirname[256];
					
					sprintf(dirname,"Current Waypoint-Folder:\n"
						"\"cstrike\\PODBot\\%s\"\n",g_szWPTDirname);
					UTIL_HostPrint(HUD_PRINTCONSOLE, dirname);
				}
				return;
			}
			// "botsvotemap"
			else if (FStrEq(pcmd,CONSOLE_CMD_BOTVOTEMAP))
			{
				if(arg1 != NULL)
				{
					int iMap=atoi(arg1);
					for (int i=0; i < 32; i++)
					{
						if(bots[i].is_used)
							bots[i].iVoteMap=iMap;
					}
					sprintf(msg, "All dead Bots will vote for Map %d\n",iMap);
					UTIL_HostPrint(HUD_PRINTCONSOLE, msg);
				}
				return;
			}
			// "omg"
			else if (FStrEq(pcmd, CONSOLE_CMD_FUN))
			{
				if(arg1 != NULL)
				{
					TurnOffFunStuff(pEntity);
					if (FStrEq(arg1, "imsober"))
					{
						UTIL_HostPrint(HUD_PRINTNOTIFY, "Back to Life, back to Reality\n");
						g_cFunMode=0;
					}
					else if (FStrEq(arg1, "tronisback"))
					{
						UTIL_HostPrint(HUD_PRINTNOTIFY, "from the eighties with Love\n");
						g_cFunMode=1;
					}
					else if (FStrEq(arg1, "itsnewyear"))
					{
						UTIL_HostPrint(HUD_PRINTNOTIFY, "Really ? That soon ?\n");
						g_cFunMode=2;
					}
					else if (FStrEq(arg1, "imhaunted"))
					{
						UTIL_HostPrint(HUD_PRINTNOTIFY, "and the Ghosts are coming for you\n");
						g_cFunMode=3;
					}
					else if (FStrEq(arg1, "itstoodark"))
					{
						UTIL_HostPrint(HUD_PRINTNOTIFY, "the Bots will light your way\n");
						g_cFunMode=4;
					}
					else if (FStrEq(arg1, "stonedagain"))
					{
						UTIL_HostPrint(HUD_PRINTNOTIFY, "feeling dizzy now ?\n");
						g_cFunMode=5;
					}
					else if (FStrEq(arg1, "imonmars"))
					{
						CVAR_SET_FLOAT("sv_gravity",325.0);
						UTIL_HostPrint(HUD_PRINTNOTIFY, "feel that Gravity...\n");
						g_cFunMode=6;
					}
					else if (FStrEq(arg1, "chickenagain"))
					{
						UTIL_HostPrint(HUD_PRINTNOTIFY, "Vyper Mode on\n");
						g_cFunMode=100;
					}
				}
				return;
			}
			// Care for Menus instead...
			else if(iUserInMenu)
			{
				if (FStrEq(pcmd, "menuselect"))
				{
					if(arg1 != NULL && *arg1!=0)
					{
						int iSelection=atoi(arg1);
						
						// Waypoint Menu ?
						if(iUserInMenu==MENU_WAYPOINT_ADD)
						{
							switch(iSelection)
							{
							case 1:
							case 2:
							case 3:
							case 4:
							case 5:
							case 6:
							case 7:
								WaypointAdd(pEntity,iSelection-1);
								iUserInMenu=0;
								break;
							case 8:
								WaypointAdd(pEntity,100);
								iUserInMenu=0;
								break;
							case 9:
								UTIL_ShowMenu(pEntity, menuWaypointAdd3.ValidSlots, -1, FALSE, menuWaypointAdd3.szMenuText);
								iUserInMenu=MENU_WAYPOINT_ADD2;
								break;
							case 10:
								iUserInMenu=0;
								break;
							}
						}
						else if(iUserInMenu==MENU_WAYPOINT_ADD2)
						{
							switch(iSelection)
							{
							case 1:
								g_bLearnJumpWaypoint=TRUE;
								UTIL_HostPrint(HUD_PRINTNOTIFY, "Observation on !\n");
								UTIL_SpeechSynth(pEntity,"move to point,any time,now ");
								iUserInMenu=0;
								break;
							case 2:
								iUserInMenu=0;
								break;
							case 10:
								iUserInMenu=0;
								break;
							default:
								break;
							}
						}
						
						// Waypoint Flag Menu ?
						else if(iUserInMenu==MENU_WAYPOINT_ADDFLAG)
						{
							switch(iSelection)
							{
							case 1:
								WaypointChangeFlags(pEntity,W_FL_NOHOSTAGE,1);
								iUserInMenu=0;
								break;
							case 2:
								WaypointChangeFlags(pEntity,W_FL_TERRORIST,1);
								iUserInMenu=0;
								break;
							case 3:
								WaypointChangeFlags(pEntity,W_FL_COUNTER,1);
								iUserInMenu=0;
								break;
							case 10:
								iUserInMenu=0;
								break;
							default:
								break;
							}
						}
						
						// Waypoint Delete Flag Menu ?
						else if(iUserInMenu==MENU_WAYPOINT_DELETEFLAG)
						{
							switch(iSelection)
							{
							case 1:
								WaypointChangeFlags(pEntity,W_FL_NOHOSTAGE,0);
								iUserInMenu=0;
								break;
							case 2:
								WaypointChangeFlags(pEntity,W_FL_TERRORIST,0);
								iUserInMenu=0;
								break;
							case 3:
								WaypointChangeFlags(pEntity,W_FL_COUNTER,0);
								iUserInMenu=0;
								break;
							case 10:
								iUserInMenu=0;
								break;
							default:
								break;
							}
						}
						
						// Main Menu ?
						else if(iUserInMenu==MENU_PODMAIN)
						{
							int index;
							
							switch(iSelection)
							{
							case 1:
								BotCreate(pEntity,NULL,NULL,NULL,NULL);
								iUserInMenu=0;
								break;
							case 2:
								UTIL_ShowMenu(pEntity, menuSelectBotskill.ValidSlots, -1, FALSE, menuSelectBotskill.szMenuText);
								iUserInMenu=MENU_SKILLSELECT;
								break;
							case 3:
								UserKillAllBots(pEntity);
								iUserInMenu=0;
								break;
							case 4:
								UserNewroundAll(pEntity);
								iUserInMenu=0;
								break;
							case 5:
								UTIL_ShowMenu(pEntity, menuSelectTeam.ValidSlots, -1, FALSE, menuSelectTeam.szMenuText);
								iUserInMenu=MENU_SERVERTEAMSELECT;
								break;
							case 6:
								for (index = 0; index <MAXBOTSNUMBER; index++)
								{
									if (bots[index].is_used)  // is this slot used?
									{
										char cmd[40];
										sprintf(cmd, "kick \"%s\"\n", bots[index].name);
										bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
										SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
										break;
									}
								}
								iUserInMenu=0;
								break;
							case 7:
								UserRemoveAllBots(pEntity);
								iUserInMenu=0;
								break;
							case 8:
								UTIL_ShowMenu(pEntity, menuSelectWeaponMode.ValidSlots, -1, TRUE, menuSelectWeaponMode.szMenuText);
								UTIL_ShowMenu(pEntity, menuSelectWeaponMode.ValidSlots, -1, FALSE, szmenuSelectWeaponMode2);
								iUserInMenu=MENU_WEAPONMODESELECT;
								break;

							case 10:
								iUserInMenu=0;
								break;
							default:
								break;
							}
						}
						
						// Bot Skill menu ?
						else if(iUserInMenu==MENU_SKILLSELECT)
						{
							switch(iSelection)
							{
							case 1:
								iStoreAddbotVars[0]=RANDOM_LONG(0,20);
								UTIL_ShowMenu(pEntity, menuSelectTeam.ValidSlots, -1, FALSE, menuSelectTeam.szMenuText);
								iUserInMenu=MENU_TEAMSELECT;
								break;
							case 2:
								iStoreAddbotVars[0]=RANDOM_LONG(20,40);
								UTIL_ShowMenu(pEntity, menuSelectTeam.ValidSlots, -1, FALSE, menuSelectTeam.szMenuText);
								iUserInMenu=MENU_TEAMSELECT;
								break;
							case 3:
								iStoreAddbotVars[0]=RANDOM_LONG(40,60);
								UTIL_ShowMenu(pEntity, menuSelectTeam.ValidSlots, -1, FALSE, menuSelectTeam.szMenuText);
								iUserInMenu=MENU_TEAMSELECT;
								break;
							case 4:
								iStoreAddbotVars[0]=RANDOM_LONG(60,80);
								UTIL_ShowMenu(pEntity, menuSelectTeam.ValidSlots, -1, FALSE, menuSelectTeam.szMenuText);
								iUserInMenu=MENU_TEAMSELECT;
								break;
							case 5:
								iStoreAddbotVars[0]=RANDOM_LONG(80,99);
								UTIL_ShowMenu(pEntity, menuSelectTeam.ValidSlots, -1, FALSE, menuSelectTeam.szMenuText);
								iUserInMenu=MENU_TEAMSELECT;
								break;
							case 6:
								iStoreAddbotVars[0]=100;
								UTIL_ShowMenu(pEntity, menuSelectTeam.ValidSlots, -1, FALSE, menuSelectTeam.szMenuText);
								iUserInMenu=MENU_TEAMSELECT;
								break;
							case 10:
								iUserInMenu=0;
								break;
							default:
								break;
							}
						}
						
						// Bot Team Select Menu ?
						else if(iUserInMenu==MENU_TEAMSELECT)
						{
							switch(iSelection)
							{
							case 1:
							case 2:
							case 5:
								iStoreAddbotVars[1]=iSelection;
								if(iSelection==5)
								{
									iStoreAddbotVars[2]=5;
									char arg1[4];
									char arg2[4];
									char arg3[4];
									sprintf(&arg1[0],"%d",iStoreAddbotVars[0]);
									sprintf(&arg2[0],"%d",iStoreAddbotVars[1]);
									sprintf(&arg3[0],"%d",iStoreAddbotVars[2]);
									BotCreate(pEntity,(char*) &arg1[0],(char*) &arg2[0],(char*) &arg3[0],NULL);
									iUserInMenu=0;
								}
								else
								{
									if(iSelection==1)
										UTIL_ShowMenu(pEntity, menuSelectTModel.ValidSlots, -1, FALSE, menuSelectTModel.szMenuText);
									else
										UTIL_ShowMenu(pEntity, menuSelectCTModel.ValidSlots, -1, FALSE, menuSelectCTModel.szMenuText);
									iUserInMenu=MENU_MODEL_SELECT;
								}
								break;
								
								// 0 - Cancel Menu
							case 10:
								iUserInMenu=0;
								break;
							default:
								break;
							}
						}
						
						// Fill Server Menu Team Select ?
						else if(iUserInMenu==MENU_SERVERTEAMSELECT)
						{
							int index;
							switch(iSelection)
							{
							case 1:
							case 2:
							case 5:
								if(iSelection==1 || iSelection==2)
								{
									// Turn off CVARS if specified Team 
									CVAR_SET_STRING("mp_limitteams","0");
									CVAR_SET_STRING("mp_autoteambalance","0");
								}
								for (index = 0; index <MAXBOTSNUMBER; index++)
								{
									if(!BotCreateTab[index].bNeedsCreation)
									{
										BotCreateTab[index].bNeedsCreation=TRUE;
										BotCreateTab[index].pCallingEnt=pEntity;
										BotCreateTab[index].name[0]='\0';
										sprintf(BotCreateTab[index].team,"%d",iSelection);
										BotCreateTab[index].skill[0]='\0';
									}
								}
								if(botcreation_time==0.0)
									botcreation_time=gpGlobals->time;
								iUserInMenu=0;
							case 10:
								iUserInMenu=0;
								break;
							default:
								break;
							}
						}
						
						// Model Selection Menu ?
						else if(iUserInMenu==MENU_MODEL_SELECT)
						{
							switch(iSelection)
							{
							case 1:
							case 2:
							case 3:
							case 4:
							case 5:
								iStoreAddbotVars[2]=iSelection;
								char arg1[4];
								char arg2[4];
								char arg3[4];
								sprintf(&arg1[0],"%d",iStoreAddbotVars[0]);
								sprintf(&arg2[0],"%d",iStoreAddbotVars[1]);
								sprintf(&arg3[0],"%d",iStoreAddbotVars[2]);
								BotCreate(pEntity,(char*) &arg1[0],(char*) &arg2[0],(char*) &arg3[0],NULL);
								iUserInMenu=0;
								break;
							case 10:
								iUserInMenu=0;
								break;
							default:
								break;
							}
						}
						// Model Selection Menu ?
						else if(iUserInMenu==MENU_WEAPONMODESELECT)
						{
							switch(iSelection)
							{
							case 1:
							case 2:
							case 3:
							case 4:
							case 5:
							case 6:
								UserSelectWeaponMode(pEntity,iSelection);
								iUserInMenu=0;
								break;
							case 10:
								iUserInMenu=0;
								break;
							default:
								break;
							}
						}
						
					}
				}
			return;
			}
	  
		}
	  
		// Check Radio Commands
		int iClientIndex = ENTINDEX(pEntity);
		iClientIndex--;
	  
		if(iRadioSelect[iClientIndex]>0)
		{
			if(FStrEq(pcmd, "menuselect"))
			{
				int iRadioCommand = atoi(arg1);
				if(iRadioCommand != 0)
				{
					if(iRadioSelect[iClientIndex] == 3)
						iRadioCommand+=10;
					else if(iRadioSelect[iClientIndex] == 2)
						iRadioCommand+=20;
					int iTeam = ThreatTab[iClientIndex].iTeam;
					if(iRadioCommand != RADIO_AFFIRMATIVE
					&& iRadioCommand != RADIO_NEGATIVE
					&& iRadioCommand != RADIO_REPORTINGIN)
					{
						for (int i=0; i <MAXBOTSNUMBER; i++)
						{
							if (bots[i].is_used && UTIL_GetTeam(bots[i].pEdict) == iTeam
								&& pEntity != bots[i].pEdict)
							{
								if(bots[i].iRadioOrder == 0)
								{
									bots[i].iRadioOrder = iRadioCommand;
									bots[i].pRadioEntity = pEntity;
								}
							}
						}
					}
					g_rgfLastRadioTime[iTeam] = gpGlobals->time;
				}
				iRadioSelect[iClientIndex] = 0;
			}
		}
		else if (FStrEq(pcmd, "radio1"))
			iRadioSelect[iClientIndex] = 1;
		else if (FStrEq(pcmd, "radio2"))
			iRadioSelect[iClientIndex] = 3;
		else if (FStrEq(pcmd, "radio3"))
			iRadioSelect[iClientIndex] = 2;
		// End Radio Commands
	}
   
	(*other_gFunctionTable.pfnClientCommand)(pEntity);
}

void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
#ifdef DEBUG
	report_log (LOG_DLL, "ClientUserInfoChanged", "pent=%x infobuffer=%s", pEntity,infobuffer);
#endif

   (*other_gFunctionTable.pfnClientUserInfoChanged)(pEntity, infobuffer);
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
#ifdef DEBUG
	report_log (LOG_DLL, "ServerActivate","pEdictList=%x edictCount=%d clientMax=%d",
		pEdictList,edictCount,clientMax);
#endif

	(*other_gFunctionTable.pfnServerActivate)(pEdictList, edictCount, clientMax);
}

void ServerDeactivate( void )
{
#ifdef DEBUG
	report_log (LOG_DLL, "ServerDeactivate", NULL);
#endif
	// Save collected Experience on Shutdown
	SaveExperienceTab(NULL);
   (*other_gFunctionTable.pfnServerDeactivate)();
}

void PlayerPreThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnPlayerPreThink)(pEntity);
}

void PlayerPostThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnPlayerPostThink)(pEntity);
}

// Called each Server frame at the very beginning 
void StartFrame( void )
{
	if (gpGlobals->deathmatch)
	{
#ifdef DEBUG
	report_log (LOG_DLL, "StartFrame",NULL);
#endif
		edict_t *pPlayer;
		static float check_server_cmd = 0.0;
		static int i, index, player_index;
		static float respawn_time = 0.0;
		static float previous_time = -1.0;
		static BOOL file_opened = FALSE;
		static int file_index, length;
		static FILE *bot_cfg_fp;
		static int ch;
		static char cmd_line[80];
		static char server_cmd[80];
		char *cmd, *arg1, *arg2, *arg3, *arg4;
		static float pause_time;
		static float client_update_time = 0.0;

		int count;
		clientdata_s cd;
		char msg[256];

		int iIsDedicatedServer = IS_DEDICATED_SERVER();
		
		// if a new map has started then (MUST BE FIRST IN StartFrame)...
		if ((gpGlobals->time + 0.1) < previous_time)
		{

			count = 0;
			
			// mark the bots as needing to be respawned...
			for (index = 0; index < 32; index++)
			{
				if (count >= prev_num_bots)
				{
					bots[index].is_used = FALSE;
					bots[index].respawn_state = 0;
					bots[index].f_kick_time = 0.0;
				}
				
				if (bots[index].is_used)  // is this slot used?
				{
					bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
					count++;
				}
				
				// check for any bots that were very recently kicked...
				if ((bots[index].f_kick_time + 5.0) > previous_time)
				{
					bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
					count++;
				}
				else
					bots[index].f_kick_time = 0.0;  // reset to prevent false spawns later
			}
			
			// set the respawn time
			if (iIsDedicatedServer)
				respawn_time = gpGlobals->time + 5.0;
			else
				respawn_time = gpGlobals->time + 20.0;
			
			client_update_time = gpGlobals->time + 10.0;  // start updating client data again
			
			bot_check_time = gpGlobals->time + 30.0;
			check_server_cmd = 0.0;
		}



// This is used to fill the correct bitmask of weapons
// not needed for CS
	/*	  if (client_update_time <= gpGlobals->time)
		{
			client_update_time = gpGlobals->time + 0.5;
			
			for (i=0; i <MAXBOTSNUMBER; i++)
			{
				if (bots[i].is_used)
				{
					memset(&cd, 0, sizeof(cd));
					
					UpdateClientData( bots[i].pEdict, 1, &cd );
					
					// see if a weapon was dropped...
					if (bots[i].bot_weapons != cd.weapons)
					{
						bots[i].bot_weapons = cd.weapons;
					}
				}
			}
		}
		*/

		// Record some Stats of all Players on the Server
		int iStoreIndex;
		for (player_index = 1; player_index <= gpGlobals->maxClients;player_index++)
		{
			pPlayer = INDEXENT(player_index);
			iStoreIndex=player_index-1;
			if (pPlayer && !pPlayer->free && 
				(pPlayer->v.flags & (FL_CLIENT | FL_FAKECLIENT)))
			{

				// Does Client need to be shocked by the ugly red
				// welcome message ?
				if(clients[iStoreIndex].bNeedWelcome)
				{
					if(clients[iStoreIndex].welcome_time == 0.0)
						clients[iStoreIndex].welcome_time = gpGlobals->time + 10.0;
					else if(clients[iStoreIndex].welcome_time < gpGlobals->time)
					{
						// Hacked together Version of HUD_DrawString
						MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY,NULL,pPlayer);
						WRITE_BYTE(TE_TEXTMESSAGE);
						WRITE_BYTE(1);
						WRITE_SHORT(FixedSigned16(-1, 1<<13));
						WRITE_SHORT(FixedSigned16(0, 1<<13));
						WRITE_BYTE(2);
						WRITE_BYTE(255);
						WRITE_BYTE(0);
						WRITE_BYTE(0);
						WRITE_BYTE(0);
						WRITE_BYTE(255);
						WRITE_BYTE(255);
						WRITE_BYTE(255);
						WRITE_BYTE(200);
						WRITE_SHORT( FixedUnsigned16(0.0078125, 1<<8 ) );
						WRITE_SHORT( FixedUnsigned16(2, 1<<8 ) );
						WRITE_SHORT( FixedUnsigned16(6, 1<<8 ) );
						WRITE_SHORT( FixedUnsigned16(0.1, 1<<8 ) );
						WRITE_STRING((const char*)&szWelcomeMessage[0]);
						MESSAGE_END();
						
						// If this is the Host, scare him even more with
						// a spoken Message
						if(pPlayer == pHostEdict)
							UTIL_SpeechSynth(pPlayer,(char*)&szSpeachSentences[RANDOM_LONG(0,15)]);
						clients[iStoreIndex].bNeedWelcome=FALSE;
					}
				}
				ThreatTab[iStoreIndex].pEdict=pPlayer;
				ThreatTab[iStoreIndex].IsUsed=TRUE;
				ThreatTab[iStoreIndex].IsAlive=IsAlive(pPlayer);
				if(ThreatTab[iStoreIndex].IsAlive)
				{
					ThreatTab[iStoreIndex].iTeam=UTIL_GetTeam(pPlayer);
					ThreatTab[iStoreIndex].vOrigin=pPlayer->v.origin;
					SoundSimulateUpdate(iStoreIndex);
				}
			}
			else
				ThreatTab[iStoreIndex].IsUsed=FALSE;
		}

		// Go through all active Bots, calling their Think fucntion
		count = 0;


#ifdef DEBUG
		if(g_bKillMe)
		{
			if(IsAlive(pHostEdict))
				ClientKill(pHostEdict);
		}
#endif


		for (int bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
		{
			if ((bots[bot_index].is_used) && (bots[bot_index].respawn_state == RESPAWN_IDLE))  // not respawning
			{
				BotThink(&bots[bot_index]);
				count++;
			}
		}

		if (count > num_bots)
			num_bots = count;

		// Show Waypoints to host if turned on and no dedicated Server
		if(!iIsDedicatedServer)
		{
			pPlayer = pHostEdict;
			if (pPlayer && !pPlayer->free)
			{
				if (g_bWaypointOn)
				{
					WaypointThink(pPlayer);
					// Noclip cheat on ?
					if(bEditNoclip)
						pPlayer->v.movetype = MOVETYPE_NOCLIP;
#ifdef _DEBUG
					// I stuff all Routines I want to check out here
					ShowPlayerView(pPlayer);
#endif
				}
	
			}
		}
		if(g_bMapInitialised)
		{
			if(g_bRecalcVis)
				WaypointCalcVisibility(pHostEdict);
			if(g_bNeedHUDDisplay)
			{
				if(!g_bCreatorShown)
				{
					g_fTimeHUDDisplay = gpGlobals->time + 2.0;
					g_bCreatorShown = TRUE;
				}
				else if(g_fTimeHUDDisplay < gpGlobals->time)
				{
					g_fTimeHUDDisplay = gpGlobals->time + 999999;
					if(!iIsDedicatedServer)
					{
						MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY,NULL,pHostEdict);
						WRITE_BYTE(TE_TEXTMESSAGE);
						WRITE_BYTE(2);
						WRITE_SHORT(FixedSigned16(-1, 1<<13));
						WRITE_SHORT(FixedSigned16(-1, 1<<13));
						WRITE_BYTE(2);
						WRITE_BYTE(0x0);
						WRITE_BYTE(0x0);
						WRITE_BYTE(0xff);
						WRITE_BYTE(0x0);
						WRITE_BYTE(0xff);
						WRITE_BYTE(0xff);
						WRITE_BYTE(0xff);
						WRITE_BYTE(0xff);
						WRITE_SHORT( FixedUnsigned16(0.1, 1<<8 ) );
						WRITE_SHORT( FixedUnsigned16(0.5, 1<<8 ) );
						WRITE_SHORT( FixedUnsigned16(5.0, 1<<8 ) );
						WRITE_SHORT( FixedUnsigned16(0.25, 1<<8 ) );
						WRITE_STRING((const char*)&g_szWaypointMessage[0]);
						MESSAGE_END();
					}
					else
						UTIL_HostPrint(HUD_PRINTCENTER,&g_szWaypointMessage[0]);
				}
			}
		}
	  

      // are we currently respawning bots and is it time to spawn one yet?
	  if ((botcreation_time != 0.0) && (botcreation_time+1.0 <= gpGlobals->time))
	  {
		  index = 0;
		  while ((!BotCreateTab[index].bNeedsCreation) && (index < MAXBOTSNUMBER))
			  index++;
			  
		  if (index < MAXBOTSNUMBER)
		  {
			  BotCreate(BotCreateTab[index].pCallingEnt, BotCreateTab[index].skill, BotCreateTab[index].team,
				  BotCreateTab[index].model,BotCreateTab[index].name);
			  BotCreateTab[index].bNeedsCreation=FALSE;
			  if(botcreation_time!=0.0)
				  botcreation_time=gpGlobals->time;
		  }
		  else
			  botcreation_time=0.0;
	  }


      // are we currently respawning bots and is it time to spawn one yet?
      if ((respawn_time > 1.0) && (respawn_time <= gpGlobals->time))
      {
         int index = 0;

         // set the time to check for automatically adding bots to dedicated servers
         bot_check_time = gpGlobals->time + 5.0;

         // find bot needing to be respawned...
         while ((index < MAXBOTSNUMBER) &&
                (bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN))
            index++;

         if (index < MAXBOTSNUMBER)
         {
            bots[index].respawn_state = RESPAWN_IS_RESPAWNING;
            bots[index].is_used = FALSE;      // free up this slot

            // respawn 1 bot then wait a while (otherwise engine crashes)
			char c_team[2];
			char c_skill[4];
			char c_model[4];
			
			sprintf(c_team, "%d", bots[index].bot_team);
			sprintf(c_skill, "%d", bots[index].bot_skill);
			sprintf(c_model, "%d", bots[index].bot_class);
			BotCreate(NULL, c_skill, c_team, c_model, bots[index].name);

            respawn_time = gpGlobals->time + 1.0;  // set next respawn time
         }
         else
         {
            respawn_time = 0.0;
         }
      }


	  // Executing the .cfg file - most work already done by
	  // Botman
	  // Command Names are the same as in ClientCommand
	  if (g_GameRules)
		{
			if (!file_opened)  // have we open podbot.cfg file yet?
			{
				char bot_cfg_filename[256];
				int str_index;
				
				(*g_engfuncs.pfnGetGameDir)(bot_cfg_filename);
				
				strcat(bot_cfg_filename, "/PODBot/podbot.cfg");  // linux filename format
				
#ifndef __linux__
				str_index = 0;
				while (bot_cfg_filename[str_index])
				{
					if (bot_cfg_filename[str_index] == '/')
						bot_cfg_filename[str_index] = '\\';  // MS-DOS format
					str_index++;
				}
#endif
				
				sprintf(msg, "Executing %s\n", bot_cfg_filename);
				PrintServerMessage(msg);
				
				bot_cfg_fp = fopen(bot_cfg_filename, "r");
				
				file_opened = TRUE;
				
				if (bot_cfg_fp == NULL)
					PrintServerMessage("podbot.cfg File not found\n" );
				
				pause_time = gpGlobals->time;
				
				file_index = 0;
				cmd_line[file_index] = 0;  // null out command line
			}
			
			// if the bot.cfg file is still open and time to execute command...
			while ((bot_cfg_fp != NULL) && !feof(bot_cfg_fp) &&
                (pause_time <= gpGlobals->time))
			{
				ch = fgetc(bot_cfg_fp);
				
				// skip any leading blanks
				while (ch == ' ')
					ch = fgetc(bot_cfg_fp);
				
				while ((ch != EOF) && (ch != '\r') && (ch != '\n'))
				{
					if (ch == '\t')  // convert tabs to spaces
						ch = ' ';
					
					cmd_line[file_index] = ch;
					
					ch = fgetc(bot_cfg_fp);
					
					// skip multiple spaces in input file
					while ((cmd_line[file_index] == ' ') &&
						(ch == ' '))      
						ch = fgetc(bot_cfg_fp);
					
					file_index++;
				}
				
				if (ch == '\r')  // is it a carriage return?
				{
					ch = fgetc(bot_cfg_fp);  // skip the linefeed
				}
				
				cmd_line[file_index] = 0;  // terminate the command line
				
				// copy the command line to a server command buffer...
				strcpy(server_cmd, cmd_line);
				strcat(server_cmd, "\n");
				
				file_index = 0;
				cmd = cmd_line;
				arg1 = arg2 = arg3 = arg4 = NULL;
				
				// skip to blank or end of string...
				while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
					file_index++;
				
				if (cmd_line[file_index] == ' ')
				{
					cmd_line[file_index++] = 0;
					arg1 = &cmd_line[file_index];
					
					// skip to blank or end of string...
					while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
						file_index++;
					
					if (cmd_line[file_index] == ' ')
					{
						cmd_line[file_index++] = 0;
						arg2 = &cmd_line[file_index];
						
						// skip to blank or end of string...
						while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
							file_index++;
						
						if (cmd_line[file_index] == ' ')
						{
							cmd_line[file_index++] = 0;
							arg3 = &cmd_line[file_index];
							
							// skip to blank or end of string...
							while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
								file_index++;
							
							if (cmd_line[file_index] == ' ')
							{
								cmd_line[file_index++] = 0;
								arg4 = &cmd_line[file_index];
							}
						}
					}
				}
				
				file_index = 0;  // reset for next input line
				
				if ((cmd_line[0] == '#') || (cmd_line[0] == 0))
				{
					continue;  // ignore comments or blank lines
				}
				else if (strcmp(cmd,CONSOLE_CMD_ADDBOT) == 0)
				{
					BotCreate( NULL, arg1, arg2, arg3, arg4);
					
					// have to delay here or engine gives "Tried to write to
					// uninitialized sizebuf_t" error and crashes...
					
					pause_time = gpGlobals->time + 1;
					
					break;
				}
				else if (strcmp(cmd, CONSOLE_CMD_MINBOTS) == 0)
				{
					min_bots = atoi( arg1 );
					sprintf(msg, "min_bots set to %d\n", min_bots);
					PrintServerMessage(msg);
				}
				else if (strcmp(cmd, CONSOLE_CMD_MAXBOTS) == 0)
				{
					max_bots = atoi( arg1 );
					if(max_bots>32)
						max_bots=32;
					sprintf(msg, "max_bots set to %d\n", max_bots);
					PrintServerMessage(msg);
				}
				else if (strcmp(cmd, CONSOLE_CMD_PAUSE) == 0)
				{
					pause_time = gpGlobals->time + atoi( arg1 );
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_MINBOTSKILL))
				{
					if(arg1 != NULL && *arg1!=0)
						g_iMinBotSkill=atoi(arg1);
					if(g_iMinBotSkill>=g_iMaxBotSkill)
						g_iMinBotSkill--;
					sprintf(msg, "MinBotSkill set to %d\n", g_iMinBotSkill);
					PrintServerMessage(msg);
				}
				else if (FStrEq(cmd, CONSOLE_CMD_MAXBOTSKILL))
				{
					if(arg1 != NULL && *arg1!=0)
						g_iMaxBotSkill=atoi(arg1);
					if(g_iMaxBotSkill<=g_iMinBotSkill)
						g_iMaxBotSkill++;
					sprintf(msg, "MaxBotSkill set to %d\n", g_iMaxBotSkill);
					PrintServerMessage(msg);
				}
				else if (FStrEq(cmd, CONSOLE_CMD_BOTCHAT))
				{
					if (FStrEq(arg1, "on"))
						g_bBotChat=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bBotChat=FALSE;
					sprintf(msg, "BotChat turned %s\n", arg1);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_JASONMODE))
				{
					if (FStrEq(arg1, "on"))
						g_bJasonMode=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bJasonMode=FALSE;
					sprintf(msg, "JasonMode turned %s\n", arg1);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_WPTFOLDER))
				{
					strcpy(g_szWPTDirname,arg1);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_DETAILNAMES))
				{
					if (FStrEq(arg1, "on"))
						g_bDetailNames=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bDetailNames=FALSE;
					sprintf(msg, "DetailedNames turned %s\n", arg1);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_INHUMANTURNS))
				{
					if (FStrEq(arg1, "on"))
						g_bInstantTurns=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bInstantTurns=FALSE;
					sprintf(msg, "Inhumanturns turned %s\n", arg1);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_MAXBOTSFOLLOW))
				{
					g_iMaxNumFollow = atoi( arg1 );
					if(g_iMaxNumFollow<0)
						g_iMaxNumFollow=3;
					sprintf(msg, "Max. Number of Bots following set to: %d\n", g_iMaxNumFollow);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_MAXWEAPONPICKUP))
				{
					g_iMaxWeaponPickup = atoi( arg1 );
					if(g_iMaxWeaponPickup<0)
						g_iMaxWeaponPickup=1;
					sprintf(msg, "Bots pickup a maximum of %d weapons\n", g_iMaxWeaponPickup);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_COLLECTEXP))
				{
					if (FStrEq(arg1, "on"))
						g_bUseExperience=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bUseExperience=FALSE;
					sprintf(msg, "Experience Collection turned %s\n", arg1);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_SHOOTTHRU))
				{
					if (FStrEq(arg1, "on"))
						g_bShootThruWalls=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bShootThruWalls=FALSE;
					sprintf(msg, "Shooting thru Walls turned %s\n", arg1);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_TIMESOUND))
				{
					g_fTimeSoundUpdate=atof(arg1);
					if(g_fTimeSoundUpdate<0.1)
						g_fTimeSoundUpdate=0.1;
					sprintf(msg, "Sound Update Timer set to %f secs\n", g_fTimeSoundUpdate);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_TIMEPICKUP))
				{
					g_fTimeSoundUpdate=atof(arg1);
					if(g_fTimePickupUpdate<0.1)
						g_fTimePickupUpdate=0.1;
					sprintf(msg, "Pickup Update Timer set to %f secs\n", g_fTimePickupUpdate);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_TIMEGRENADE))
				{
					g_fTimeGrenadeUpdate=atof(arg1);
					if(g_fTimeGrenadeUpdate<0.1)
						g_fTimeGrenadeUpdate=0.1;
					sprintf(msg, "Grenade Check Timer set to %f secs\n", g_fTimeGrenadeUpdate);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_SPEECH))
				{
					if (FStrEq(arg1, "on"))
						g_bUseSpeech=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bUseSpeech=FALSE;
					sprintf(msg, "POD Speech turned %s\n", arg1);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_VOTEKICK))
				{
					if (FStrEq(arg1, "on"))
						g_bAllowVotes=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bAllowVotes=FALSE;
					sprintf(msg, "Bot voting turned %s\n", arg1);
					PrintServerMessage(msg);
					break;
				}
				else if (FStrEq(cmd, CONSOLE_CMD_ALLOWSPRAY))
				{
					if (FStrEq(arg1, "on"))
						g_bBotSpray=TRUE;
					else if (FStrEq(arg1, "off"))
						g_bBotSpray=FALSE;
					sprintf(msg, "Bot Logo spraying turned %s\n", arg1);
					PrintServerMessage(msg);
					break;
				}
				else
				{
					sprintf(msg, "executing server command: %s\n", server_cmd);
					PrintServerMessage(msg);
					SERVER_COMMAND(server_cmd);
				}
         }
		 
         // if bot.cfg file is open and reached end of file, then close and free it
         if ((bot_cfg_fp != NULL) && feof(bot_cfg_fp))
         {
			 fclose(bot_cfg_fp);
			 
			 bot_cfg_fp = NULL;
         }
      }      
	  
      // if time to check for server commands then do so...
      if (check_server_cmd <= gpGlobals->time)
      {
		  check_server_cmd = gpGlobals->time + 1.0;
		  
		  char *cvar_bot = (char *)CVAR_GET_STRING(sv_bot.name);
		  
		  if ( cvar_bot && cvar_bot[0] )
		  {
			  char *cmd, *arg1, *arg2, *arg3, *arg4;
			  
			  strcpy(cmd_line, cvar_bot);
			  
			  index = 0;
			  cmd = cmd_line;
			  arg1 = arg2 = arg3 = arg4 = NULL;
			  
			  // skip to blank or end of string...
			  while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
				  index++;
			  
			  if (cmd_line[index] == ' ')
			  {
				  cmd_line[index++] = 0;
				  arg1 = &cmd_line[index];
				  
				  // skip to blank or end of string...
				  while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
					  index++;
				  
				  if (cmd_line[index] == ' ')
				  {
					  cmd_line[index++] = 0;
					  arg2 = &cmd_line[index];
					  
					  // skip to blank or end of string...
					  while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
						  index++;
					  
					  if (cmd_line[index] == ' ')
					  {
						  cmd_line[index++] = 0;
						  arg3 = &cmd_line[index];
						  
						  // skip to blank or end of string...
						  while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
							  index++;
						  
						  if (cmd_line[index] == ' ')
						  {
							  cmd_line[index++] = 0;
							  arg4 = &cmd_line[index];
						  }
					  }
				  }
			  }
			  
			  if (strcmp(cmd,CONSOLE_CMD_ADDBOT) == 0)
			  {
				  printf("Adding new bot...\n");
				  
				  BotCreate( NULL, arg1, arg2, arg3, arg4);
			  }
			  else if (FStrEq(cmd, CONSOLE_CMD_NEWMAP))
			  {
				  if(arg1 != NULL && *arg1!=0)
				  {
					  if(IS_MAP_VALID((char*)arg1))
					  {
						  printf("Loading new map...\n");
						  CVAR_SET_STRING(sv_bot.name, "");
						  pfnChangeLevel((char*)arg1,NULL);
					  }
					  else
						  printf("Not a valid Map!\n");
				  }
			  }
			  else if (FStrEq(cmd,CONSOLE_CMD_REMOVEALLBOTS))
			  {
				  max_bots=0;
				  min_bots=0;
				  for (int index = 0; index <MAXBOTSNUMBER; index++)
				  {
					  if (bots[index].is_used)  // is this slot used?
					  {
						  char cmd[40];
						  
						  sprintf(cmd, "kick \"%s\"\n", bots[index].name);
						  
						  bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
						  
						  SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
					  }
				  }
				  printf("All Bots removed !\n");
			  }
			  else if (FStrEq(cmd,CONSOLE_CMD_KILLALLBOTS))
			  {
				  bot_t *pBot;
				  
				  for(int i=0;i<MAXBOTSNUMBER+1;i++)
				  {
					  pBot = &bots[i];
					  if((pBot->is_used==TRUE) && (IsAlive(pBot->pEdict)))
					  {
						  pBot->need_to_initialize=TRUE;
						  ClientKill(pBot->pEdict);
					  }
				  }
				  printf("All Bots killed !\n");
			  }
			  else if (FStrEq(cmd,"fillserver"))
			  {
				  int iSelection;

				  if(arg1 != NULL && *arg1!=0)
					  iSelection=atoi(arg1);
				  if(iSelection==1 || iSelection==2)
				  {
					  CVAR_SET_STRING("mp_limitteams","0");
					  CVAR_SET_STRING("mp_autoteambalance","0");
				  }
				  else
					  iSelection=5;
				  for (index = 0; index <MAXBOTSNUMBER; index++)
				  {
					  if(!BotCreateTab[index].bNeedsCreation)
					  {
						  BotCreateTab[index].bNeedsCreation=TRUE;
						  BotCreateTab[index].pCallingEnt=NULL;
						  BotCreateTab[index].name[0]='\0';
						  sprintf(BotCreateTab[index].team,"%d",iSelection);
						  BotCreateTab[index].skill[0]='\0';
					  }
				  }
				  if(botcreation_time==0.0)
					  botcreation_time=gpGlobals->time;
			  }
#ifdef _DEBUG
			  // Shows running Bots on the Server
			  else if (FStrEq(cmd,"listbots"))
			  {
				  UserListBots(NULL);
			  }
			  // Turns on logging for a single Bot
			  // arg1 contains the player index number
			  else if (FStrEq(cmd,"debugbot"))
			  {
				  if(arg1 != NULL && *arg1!=0)
				  {
					  set_log_mask(LOG_SINGLEBOT);
					  set_debuglog_bot(atoi(arg1));
				  }
			  }
#endif
			  CVAR_SET_STRING(sv_bot.name, "");
		  }
      }
	  
      // check if time to see if a bot needs to be created...
      if (bot_check_time < gpGlobals->time)
      {
         int count = 0;

         bot_check_time = gpGlobals->time + 5.0;

         for (i = 0; i < 32; i++)
         {
            if (clients[i].pEntity != NULL)
               count++;
         }

         // if there are currently less than the maximum number of "players"
         // then add another bot using the default skill level...
         if ((count < max_bots) && (max_bots != -1))
         {
            BotCreate( NULL, NULL, NULL, NULL, NULL);
         }
      }
	  
      previous_time = gpGlobals->time;
   }
   
   (*other_gFunctionTable.pfnStartFrame)();
}

void ParmsNewLevel( void )
{
   (*other_gFunctionTable.pfnParmsNewLevel)();
}

void ParmsChangeLevel( void )
{
   (*other_gFunctionTable.pfnParmsChangeLevel)();
}

const char *GetGameDescription( void )
{
   return (*other_gFunctionTable.pfnGetGameDescription)();
}

void PlayerCustomization( edict_t *pEntity, customization_t *pCust )
{
#ifdef DEBUG
	report_log (LOG_DLL, "PlayerCustomization","pent=%x",pEntity);
#endif

   (*other_gFunctionTable.pfnPlayerCustomization)(pEntity, pCust);
}

void SpectatorConnect( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnSpectatorConnect)(pEntity);
}

void SpectatorDisconnect( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnSpectatorDisconnect)(pEntity);
}

void SpectatorThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnSpectatorThink)(pEntity);
}

void Sys_Error( const char *error_string )
{
   (*other_gFunctionTable.pfnSys_Error)(error_string);
}

void PM_Move ( struct playermove_s *ppmove, int server )
{
   (*other_gFunctionTable.pfnPM_Move)(ppmove, server);
}

void PM_Init ( struct playermove_s *ppmove )
{
   (*other_gFunctionTable.pfnPM_Init)(ppmove);
}

char PM_FindTextureType( char *name )
{
   return (*other_gFunctionTable.pfnPM_FindTextureType)(name);
}

void SetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas )
{
   (*other_gFunctionTable.pfnSetupVisibility)(pViewEntity, pClient, pvs, pas);
}

void UpdateClientData ( const struct edict_s *ent, int sendweapons, struct clientdata_s *cd )
{
   (*other_gFunctionTable.pfnUpdateClientData)(ent, sendweapons, cd);
}

int AddToFullPack( struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet )
{
   return (*other_gFunctionTable.pfnAddToFullPack)(state, e, ent, host, hostflags, player, pSet);
}

void CreateBaseline( int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs )
{
   (*other_gFunctionTable.pfnCreateBaseline)(player, eindex, baseline, entity, playermodelindex, player_mins, player_maxs);
}

void RegisterEncoders( void )
{
   (*other_gFunctionTable.pfnRegisterEncoders)();
}

int GetWeaponData( struct edict_s *player, struct weapon_data_s *info )
{
   return (*other_gFunctionTable.pfnGetWeaponData)(player, info);
}

void CmdStart( const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed )
{
   (*other_gFunctionTable.pfnCmdStart)(player, cmd, random_seed);
}

void CmdEnd ( const edict_t *player )
{
   (*other_gFunctionTable.pfnCmdEnd)(player);
}

int ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
   return (*other_gFunctionTable.pfnConnectionlessPacket)(net_from, args, response_buffer, response_buffer_size);
}

int GetHullBounds( int hullnumber, float *mins, float *maxs )
{
   return (*other_gFunctionTable.pfnGetHullBounds)(hullnumber, mins, maxs);
}

void CreateInstancedBaselines( void )
{
   (*other_gFunctionTable.pfnCreateInstancedBaselines)();
}

int InconsistentFile( const edict_t *player, const char *filename, char *disconnect_message )
{
#ifdef DEBUG
	report_log (LOG_DLL, "InconsistentFile","pent=%x filename=%s",player,filename);
#endif

   return (*other_gFunctionTable.pfnInconsistentFile)(player, filename, disconnect_message);
}

int AllowLagCompensation( void )
{
   return (*other_gFunctionTable.pfnAllowLagCompensation)();
}


DLL_FUNCTIONS gFunctionTable =
{
   GameDLLInit,               //pfnGameInit
   DispatchSpawn,             //pfnSpawn
   DispatchThink,             //pfnThink
   DispatchUse,               //pfnUse
   DispatchTouch,             //pfnTouch
   DispatchBlocked,           //pfnBlocked
   DispatchKeyValue,          //pfnKeyValue
   DispatchSave,              //pfnSave
   DispatchRestore,           //pfnRestore
   DispatchObjectCollsionBox, //pfnAbsBox

   SaveWriteFields,           //pfnSaveWriteFields
   SaveReadFields,            //pfnSaveReadFields

   SaveGlobalState,           //pfnSaveGlobalState
   RestoreGlobalState,        //pfnRestoreGlobalState
   ResetGlobalState,          //pfnResetGlobalState

   ClientConnect,             //pfnClientConnect
   ClientDisconnect,          //pfnClientDisconnect
   ClientKill,                //pfnClientKill
   ClientPutInServer,         //pfnClientPutInServer
   ClientCommand,             //pfnClientCommand
   ClientUserInfoChanged,     //pfnClientUserInfoChanged
   ServerActivate,            //pfnServerActivate
   ServerDeactivate,          //pfnServerDeactivate

   PlayerPreThink,            //pfnPlayerPreThink
   PlayerPostThink,           //pfnPlayerPostThink

   StartFrame,                //pfnStartFrame
   ParmsNewLevel,             //pfnParmsNewLevel
   ParmsChangeLevel,          //pfnParmsChangeLevel

   GetGameDescription,        //pfnGetGameDescription    Returns string describing current .dll game.
   PlayerCustomization,       //pfnPlayerCustomization   Notifies .dll of new customization for player.

   SpectatorConnect,          //pfnSpectatorConnect      Called when spectator joins server
   SpectatorDisconnect,       //pfnSpectatorDisconnect   Called when spectator leaves the server
   SpectatorThink,            //pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

   Sys_Error,                 //pfnSys_Error          Called when engine has encountered an error

   PM_Move,                   //pfnPM_Move
   PM_Init,                   //pfnPM_Init            Server version of player movement initialization
   PM_FindTextureType,        //pfnPM_FindTextureType

   SetupVisibility,           //pfnSetupVisibility        Set up PVS and PAS for networking for this client
   UpdateClientData,          //pfnUpdateClientData       Set up data sent only to specific client
   AddToFullPack,             //pfnAddToFullPack
   CreateBaseline,            //pfnCreateBaseline        Tweak entity baseline for network encoding, allows setup of player baselines, too.
   RegisterEncoders,          //pfnRegisterEncoders      Callbacks for network encoding
   GetWeaponData,             //pfnGetWeaponData
   CmdStart,                  //pfnCmdStart
   CmdEnd,                    //pfnCmdEnd
   ConnectionlessPacket,      //pfnConnectionlessPacket
   GetHullBounds,             //pfnGetHullBounds
   CreateInstancedBaselines,  //pfnCreateInstancedBaselines
   InconsistentFile,          //pfnInconsistentFile
   AllowLagCompensation,      //pfnAllowLagCompensation
};


extern "C" _declspec( dllexport) int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
{
   // check if engine's pointer is valid and version is correct...

   if ( !pFunctionTable || interfaceVersion != INTERFACE_VERSION )
      return FALSE;

   // pass engine callback function table to engine...
   memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );

   // pass other DLLs engine callbacks to function table...
   if (!(*other_GetEntityAPI)(&other_gFunctionTable, INTERFACE_VERSION))
   {
      return FALSE;  // error initializing function table!!!
   }

   return TRUE;
}

extern "C" EXPORT int GetNewDLLFunctions( NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
   if (other_GetNewDLLFunctions == NULL)
      return FALSE;

   // pass other DLLs engine callbacks to function table...
   if (!(*other_GetNewDLLFunctions)(pFunctionTable, interfaceVersion))
   {
      return FALSE;  // error initializing function table!!!
   }

   return TRUE;
}


void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3) 
{ 
  int length,i; 

  i=0; 
  while(i<256) 
  { 
        g_argv[i] = NULL; 
        i++; 
  } 

  isFakeClientCommand = 1; 

  if ((arg1 == NULL) || (*arg1 == 0)) 
     return; 

  if ((arg2 == NULL) || (*arg2 == 0)) 
  { 
     length = sprintf(&g_argv[0], "%s", arg1); 
     fake_arg_count = 1; 
  } 
  else if ((arg3 == NULL) || (*arg3 == 0)) 
  { 
     length = sprintf(&g_argv[0], "%s %s", arg1, arg2); 
     fake_arg_count = 2; 
  } 
  else 
  { 
     length = sprintf(&g_argv[0], "%s %s %s", arg1, arg2, arg3); 
     fake_arg_count = 3; 
  } 

  strcpy(&g_argv[64], arg1); 

  if (arg2) 
     strcpy(&g_argv[128], arg2); 

  if (arg3) 
     strcpy(&g_argv[192], arg3); 

  g_argv[length] = NULL;  // null terminate just in case 
  
  // allow the MOD DLL to execute the ClientCommand... 
  ClientCommand(pBot); 

  isFakeClientCommand = 0; 
} 



const char *Cmd_Args( void )
{
   if (isFakeClientCommand)
   {
      return &g_argv[0];
   }
   else
   {
      return (*g_engfuncs.pfnCmd_Args)();
   }
}


const char *Cmd_Argv( int argc )
{
   if (isFakeClientCommand)
   {
      if (argc == 0)
      {
         return &g_argv[64];
      }
      else if (argc == 1)
      {
         return &g_argv[128];
      }
      else if (argc == 2)
      {
         return &g_argv[192];
      }
      else
      {
         return "???";
      }
   }
   else
   {
      return (*g_engfuncs.pfnCmd_Argv)(argc);
   }
}


int Cmd_Argc( void )
{
   if (isFakeClientCommand)
   {
      return fake_arg_count;
   }
   else
   {
      return (*g_engfuncs.pfnCmd_Argc)();
   }
}

