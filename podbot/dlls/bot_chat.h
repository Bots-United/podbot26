//
// bot_chat.h
//

#ifndef BOTCHAT_H
#define BOTCHAT_H

extern replynode_t *pChatReplies;
extern STRINGNODE *pChatNoKeyword;
extern int  iNumNoKeyStrings;
extern float g_fTimeRoundEnd;
extern threat_t ThreatTab[32];

void BotPrepareChatMessage(bot_t *pBot,char *pszText);
bool BotRepliesToPlayer(bot_t *pBot);

#endif