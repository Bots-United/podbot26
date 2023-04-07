//
// bot_chat.cpp
//
// Contains parsing stuff & chat selection for
// chatting Bots 

#include "extdll.h"
#include "util.h"
#include "bot.h"
#include "bot_chat.h"


// Strips out Words between Chars like []
inline void StripClanTags(char* pszTemp1,char* pszReturn,char* cTag1, char* cTag2)
{
	memset(pszReturn,0,sizeof(pszReturn));

	unsigned char ucLen = strlen(pszTemp1);

	char* pszEndPattern;
	char* pszStartPattern = strstr(pszTemp1,cTag1);
	if(pszStartPattern)
	{
		pszStartPattern++;
		if(*pszStartPattern != 0x0)
		{
			pszEndPattern = strstr(pszTemp1,cTag2);
			if(pszEndPattern)
			{
				if(pszEndPattern - pszStartPattern < ucLen)
				{
					if(pszStartPattern-1 != pszTemp1)
						strncpy(pszReturn,pszTemp1,(pszStartPattern - pszTemp1)-1);
					if(pszEndPattern < pszTemp1 + ucLen)
						strcat(pszReturn,pszEndPattern+1);
				}
			}
		}
	}
	else
		strcpy(pszReturn,pszTemp1);
}

// Converts given Names to a more human like
// style for output
void ConvertNameToHuman(char* pszName,char* pszReturn)
{
	char szTemp1[80];
	char szTemp2[80];

	memset(szTemp1,0,sizeof(szTemp1));
	memset(szTemp2,0,sizeof(szTemp1));

	StripClanTags(pszName,szTemp1,"[","]");
	StripClanTags(szTemp1,szTemp2,"(",")");
	StripClanTags(szTemp2,szTemp1,"{","}");

	char* pszToken = strtok( szTemp1," ");
	unsigned char ucLen = 1;
	unsigned char ucActLen = 1;
	while(pszToken)
	{
		ucActLen = strlen(pszToken);
		if(ucActLen > ucLen)
		{
			strcpy(szTemp2,pszToken);
			ucLen = ucActLen;
		}
		pszToken = strtok( NULL," '");
	}
	strcpy(pszReturn,szTemp2);
}

// Parses Messages from the Botchat, replaces Keywords
// and converts Names into a more human style
void BotPrepareChatMessage(bot_t *pBot,char *pszText)
{
	int iLen;
	char szNamePlaceholder[80];

	memset(&pBot->szMiscStrings, 0, sizeof(pBot->szMiscStrings));

	char *pszTextStart = pszText;
	char *pszPattern = pszText;
	edict_t *pTalkEdict = NULL;

	while(pszPattern)
	{
		// all replacement placeholders start with a %
		pszPattern = strstr(pszTextStart,"%");
		if(pszPattern)
		{
			iLen=pszPattern - pszTextStart;
			if(iLen>0)
				strncpy(pBot->szMiscStrings, pszTextStart, iLen);
			pszPattern++;

			// Player with most frags ?
			if(*pszPattern == 'f')
			{
				int iHighestFrags = -9000; // just pick some start value
				int iCurrFrags;
				int iIndex = 0;

				for (int i = 0; i < gpGlobals->maxClients; i++)
				{
					if((ThreatTab[i].IsUsed==FALSE) || (ThreatTab[i].pEdict==pBot->pEdict))
						continue;
					iCurrFrags = ThreatTab[i].pEdict->v.frags;
					if(iCurrFrags > iHighestFrags)
					{
						iHighestFrags = iCurrFrags;
						iIndex = i;
					}
				}

				pTalkEdict=ThreatTab[iIndex].pEdict;
				if(pTalkEdict && !pTalkEdict->free)
				{
					ConvertNameToHuman((char*)STRING(pTalkEdict->v.netname),szNamePlaceholder);
					strcat(pBot->szMiscStrings,szNamePlaceholder);
				}
			}
			// Mapname ?
			else if(*pszPattern == 'm')
				strcat(pBot->szMiscStrings,STRING(gpGlobals->mapname));
			// Roundtime ?
			else if(*pszPattern == 'r')
			{
				char szTime[]="000:00";
				int iTime = (int)(g_fTimeRoundEnd - gpGlobals->time);

				sprintf(szTime,"%02d:%02d",iTime/60,iTime%60);
				strcat(pBot->szMiscStrings,szTime);
			}
			// Chat Reply ?
			else if(*pszPattern == 's')
			{
				pTalkEdict=INDEXENT(pBot->SaytextBuffer.iEntityIndex);
				if(pTalkEdict && !pTalkEdict->free)
				{
					ConvertNameToHuman((char*)STRING(pTalkEdict->v.netname),szNamePlaceholder);
					strcat(pBot->szMiscStrings,szNamePlaceholder);
				}
			}
			// Teammate alive ?
			else if(*pszPattern == 't')
			{
				int iTeam = UTIL_GetTeam(pBot->pEdict);
				for (int i = 0; i < gpGlobals->maxClients; i++)
				{
					if((ThreatTab[i].IsUsed==FALSE) || (ThreatTab[i].IsAlive==FALSE) ||
						(ThreatTab[i].iTeam!=iTeam) || (ThreatTab[i].pEdict==pBot->pEdict))
						continue;
					break;
				}
				pTalkEdict=ThreatTab[i].pEdict;
				if(pTalkEdict && !pTalkEdict->free)
				{
					ConvertNameToHuman((char*)STRING(pTalkEdict->v.netname),szNamePlaceholder);
					strcat(pBot->szMiscStrings,szNamePlaceholder);
				}
			}
			else if(*pszPattern == 'v')
			{
				pTalkEdict = pBot->pLastVictim;
				if(pTalkEdict && !pTalkEdict->free)
				{
					ConvertNameToHuman((char*)STRING(pTalkEdict->v.netname),szNamePlaceholder);
					strcat(pBot->szMiscStrings,szNamePlaceholder);
				}
			}
			pszPattern++;
			pszTextStart = pszPattern;
		}
	}
	strcat(pBot->szMiscStrings,pszTextStart);
}


bool BotCheckKeywords(char *pszMessage, char *pszReply)
{
	replynode_t *pReply = pChatReplies;
	char szKeyword[128];
	char *pszCurrKeyword;
	char *pszKeywordEnd;
	int iLen,iRandom;
	char cNumRetries;

	while(pReply != NULL)
	{
		pszCurrKeyword = (char*)&pReply->pszKeywords;
		while(pszCurrKeyword)
		{
			pszKeywordEnd = strstr(pszCurrKeyword,"@");
			if(pszKeywordEnd)
			{
				iLen=pszKeywordEnd - pszCurrKeyword;
				strncpy( szKeyword, pszCurrKeyword, iLen);
				szKeyword[iLen] = 0x0;
				// Parse Text for occurences of keywords
				char *pPattern = strstr(pszMessage,szKeyword);
				if(pPattern)
				{
					STRINGNODE *pNode = pReply->pReplies;
					if(pReply->cNumReplies == 1)
						strcpy(pszReply,pNode->pszString);
					else
					{
						cNumRetries = 0;
						do
						{
							iRandom = RANDOM_LONG(1,pReply->cNumReplies);
							cNumRetries++;
						} while (iRandom == pReply->cLastReply
							&& cNumRetries <= pReply->cNumReplies);
						pReply->cLastReply = iRandom;
						cNumRetries = 1;
						while(cNumRetries < iRandom)
						{
							pNode = pNode->Next;
							cNumRetries++;
						}
						strcpy(pszReply,pNode->pszString);
					}
					return TRUE;
				}
				pszKeywordEnd++;
				if(*pszKeywordEnd == 0x0)
					pszKeywordEnd = NULL;
			}
			pszCurrKeyword = pszKeywordEnd;
		}
		pReply = pReply->pNextReplyNode;
	}

	// Didn't find a keyword ?
	// 50% of the time use some universal reply
	if(RANDOM_LONG(1,100)<50)
	{
		STRINGNODE *pTempNode = NULL;

		do
		{
			pTempNode=GetNodeString(pChatNoKeyword,RANDOM_LONG(0,iNumNoKeyStrings));
		} while(pTempNode == NULL);
		assert(pTempNode != NULL);
		assert(pTempNode->pszString != NULL);
		strcpy(pszReply,pTempNode->pszString);
		return TRUE;
	}
	return FALSE;
}

bool BotParseChat(bot_t *pBot, char *pszReply)
{
	char szMessage[512];


	// Copy to safe place
	strcpy(szMessage,pBot->SaytextBuffer.szSayText);
	// Text to uppercase for Keyword parsing
	_strupr(szMessage);

	int iMessageLen = strlen(szMessage);
	int i = 0;
	// Find the : char behind the name to get the
	// start of the real text
	while(i <= iMessageLen)
	{
		if(szMessage[i] == ':')
			break;
		i++;
	}
	return(BotCheckKeywords(&szMessage[i],pszReply));
}


bool BotRepliesToPlayer(bot_t *pBot)
{
	if(pBot->SaytextBuffer.iEntityIndex != -1
		&& pBot->SaytextBuffer.szSayText[0] != 0x0)
	{
		char szText[256];
		if(pBot->SaytextBuffer.fTimeNextChat < gpGlobals->time)
		{
			if(RANDOM_LONG(1,100)<pBot->SaytextBuffer.cChatProbability
				&& BotParseChat(pBot,(char*)&szText))
			{
				BotPrepareChatMessage(pBot,(char*)&szText);
				BotPushMessageQueue(pBot,MSG_CS_SAY);
				pBot->SaytextBuffer.iEntityIndex = -1;
				pBot->SaytextBuffer.szSayText[0] = 0x0;
				pBot->SaytextBuffer.fTimeNextChat = gpGlobals->time + pBot->SaytextBuffer.fChatDelay;
				return TRUE;
			}
			pBot->SaytextBuffer.iEntityIndex = -1;
			pBot->SaytextBuffer.szSayText[0] = 0x0;
		}
	}
	return FALSE;
}