#include "client.h"
#include <iostream>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <iostream>     // std::cout
#include <algorithm>    // std::copy
#include "minIni.h"
#include <vector>

#pragma warning(disable:4996)

extern TCHAR g_settingsFileName[MAX_PATH];
bool FirstFrame = false;
pfnUserMsgHook pMOTD;
GameInfo_t BuildInfo;
cvar_t *random;
map<string, string> g_modelsHashMap;
cvar_t *logsfiles;
cvar_t *ex_thud;
cvar_t *motd_block;
cvar_t *type;
void HookEngineMessages(){
	pEngineMsgBase = (PEngineMsg)offset.FindSVCMessages();
	pSVC_StuffText = HookEngineMsg("svc_stufftext", SVC_StuffText);
	pSVC_SendCvarValue = HookEngineMsg("svc_sendcvarvalue", SVC_SendCvarValue);
	pSVC_SendCvarValue2 = HookEngineMsg("svc_sendcvarvalue2", SVC_SendCvarValue2);
	pSVC_Director = HookEngineMsg("svc_director", SVC_Director);
//	pSVC_Resourcelist = HookEngineMsg("svc_resourcelist", SVC_Resourcelist);
}

void ConsolePrintColor(BYTE R, BYTE G, BYTE B, char* string){
	TColor24 DefaultColor;PColor24 Ptr;Ptr = Console_TextColor;DefaultColor = *Ptr;Ptr->R = R;Ptr->G = G;Ptr->B = B;g_Engine.Con_Printf("%s", string);*Ptr = DefaultColor;
}

void models(){
	for (DWORD i = 0; i < 32; i++){
		player_info_s* player = g_pStudio->PlayerInfo(i);
		if (player && (lstrlenA(player->name)>1) && player->model){
			char buffer[128];
			sprintf_s(buffer, "NAME -> [ %s ]  | MODEL -> [ %s ]\n", player->name, player->model);
			ConsolePrintColor(255, 255, 15, buffer);
		}
	}
}
void Credits(){
	ConsolePrintColor(255, 255, 255, "-- Thank's to");ConsolePrintColor(0, 255, 0, " [2010] Team\n");ConsolePrintColor(255, 255, 255, "-- Thank's to");
	ConsolePrintColor(0, 255, 0, " madotsuki-team < *\n");ConsolePrintColor(255, 255, 255, "-- Thank's to ");ConsolePrintColor(0, 255, 0, "or_75\n");
}
int g_blockedCmdCount, g_serverCmdCount;
char *g_blockedCmds[1024], *g_serverCmds[2048];

struct models_replace_s{char name[32];char repl[32];};
vector<models_replace_s> models_list;
int Callback(const char *section, const char *key, const char *value, const void *userdata){
	if (lstrcmpA(section, "Models") == 0){
		models_replace_s model_d;lstrcpyA(model_d.name, key);lstrcpyA(model_d.repl, value);models_list.push_back(model_d);
	}
	return 1;
}
void Inject(){LoadLibraryA(g_Engine.Cmd_Argv(1)); }
int g_blockedCvarCount;
char *g_blockedCvars[512];

void Reload(){
	models_list.clear();
	ini_browse(Callback,NULL,g_settingsFileName);
	memset(g_blockedCmds,0,sizeof(g_blockedCmds));memset(g_blockedCvars, 0, sizeof(g_blockedCvars)); memset(g_serverCmds, 0, sizeof(g_serverCmds));
	g_blockedCvarCount=0; g_blockedCmdCount=0; g_serverCmdCount = 0;
	static TCHAR sKeyNames[4096*3];
	GetPrivateProfileSection(TEXT("Commands"),sKeyNames,ARRAYSIZE(sKeyNames),g_settingsFileName);
	char *psKeyName = sKeyNames;
	while (psKeyName[0]!='\0'){g_blockedCmds[g_blockedCmdCount++]=strdup(psKeyName);psKeyName+=strlen(psKeyName)+1;}

	GetPrivateProfileSection(TEXT("Send Commands"), sKeyNames, ARRAYSIZE(sKeyNames), g_settingsFileName);
	char *psKeyName3 = sKeyNames;
	while (psKeyName3[0] != '\0') { g_serverCmds[g_serverCmdCount++] = strdup(psKeyName3); psKeyName3 += strlen(psKeyName3) + 1; }

	GetPrivateProfileSection(TEXT("Blocked cvars"),sKeyNames,ARRAYSIZE(sKeyNames),g_settingsFileName);
	char *psKeyName2=sKeyNames;
	while (psKeyName2[0]!='\0'){g_blockedCvars[g_blockedCvarCount++]=strdup(psKeyName2);psKeyName2+=strlen(psKeyName2)+1;}
	TCHAR value[16];char cvarname[32];
	GetPrivateProfileString(TEXT("Settings"), TEXT("sid_random"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	sprintf(cvarname, "sid_random %s", value);g_Engine.pfnClientCmd(cvarname); memset(value, 0, sizeof(value)); memset(cvarname, 0, sizeof(cvarname));
	GetPrivateProfileString(TEXT("Settings"), TEXT("cust_hud"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	sprintf(cvarname, "cust_hud %s", value); g_Engine.pfnClientCmd(cvarname);memset(value, 0, sizeof(value)); memset(cvarname, 0, sizeof(cvarname));
	GetPrivateProfileString(TEXT("Settings"), TEXT("logs"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	sprintf(cvarname, "logs %s", value); g_Engine.pfnClientCmd(cvarname);memset(value, 0, sizeof(value)); memset(cvarname, 0, sizeof(cvarname));
	GetPrivateProfileString(TEXT("Settings"), TEXT("motd_block"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	sprintf(cvarname, "motd_block %s", value); g_Engine.pfnClientCmd(cvarname);memset(value, 0, sizeof(value)); memset(cvarname, 0, sizeof(cvarname));
	GetPrivateProfileString(TEXT("Settings"), TEXT("type"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	sprintf(cvarname, "type %s", value); g_Engine.pfnClientCmd(cvarname); memset(value, 0, sizeof(value)); memset(cvarname, 0, sizeof(cvarname));
}
void InitHack(){
	static TCHAR sKeyNames[4096 * 3];
	GetPrivateProfileSection(TEXT("Commands"), sKeyNames, ARRAYSIZE(sKeyNames), g_settingsFileName);
	char *psKeyName = sKeyNames;
	g_blockedCmdCount = 0;
	while (psKeyName[0] != '\0') {
		g_blockedCmds[g_blockedCmdCount++] = strdup(psKeyName);
		psKeyName += strlen(psKeyName) + 1;
	}

	GetPrivateProfileSection(TEXT("Send Commands"), sKeyNames, ARRAYSIZE(sKeyNames), g_settingsFileName);
	psKeyName = sKeyNames;
	g_serverCmdCount = 0;
	while (psKeyName[0] != '\0') {
		g_serverCmds[g_serverCmdCount++] = strdup(psKeyName);
		psKeyName += strlen(psKeyName) + 1;
	}


	GetPrivateProfileSection(TEXT("AutoInject"), sKeyNames, ARRAYSIZE(sKeyNames), g_settingsFileName);
	psKeyName = sKeyNames;
	while (psKeyName[0] != '\0') {
		LoadLibraryA(psKeyName);
		psKeyName += strlen(psKeyName) + 1;
	}
	GetPrivateProfileSection(TEXT("Custom Commands"), sKeyNames, ARRAYSIZE(sKeyNames), g_settingsFileName);
	psKeyName = sKeyNames;
	while (psKeyName[0] != '\0') {
		g_pEngine->pfnAddCommand(strdup(psKeyName), DRC_CMD_NONE);
		psKeyName += strlen(psKeyName) + 1;
	}
	ini_browse(Callback, NULL, g_settingsFileName);
	GetPrivateProfileSection(TEXT("Blocked cvars"), sKeyNames, ARRAYSIZE(sKeyNames), g_settingsFileName);
	char *psKeyName2 = sKeyNames;
	g_blockedCvarCount = 0;
	while (psKeyName2[0] != '\0') {
		g_blockedCvars[g_blockedCvarCount++] = strdup(psKeyName2);
		psKeyName2 += strlen(psKeyName2) + 1;
	}
	if (!(g_Engine.Con_IsVisible() != 0))g_Engine.pfnClientCmd("toggleconsole");
	ConsolePrintColor(0, 255, 11, "-- Extra Mirror v1.4\n");
	ConsolePrintColor(255, 255, 255, "-- Use 'credits' for more information\n");
	ConsolePrintColor(255, 255, 255, "-- Thank's to Realwar for title\n");
	ConsolePrintColor(255, 255, 255, "-- Thank's to FightMagister for functions\n");
	ConsolePrintColor(255, 255, 255, "-- Thank's to Spawner { Kiass }\n");
	g_pEngine->pfnAddCommand("credits", Credits); g_pEngine->pfnAddCommand("inject", Inject);	g_pEngine->pfnAddCommand("modelsn", models); g_pEngine->pfnAddCommand("update", Reload);TCHAR value[16];
	GetPrivateProfileString(TEXT("Settings"), TEXT("sid_random"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	random = g_pEngine->pfnRegisterVariable("sid_random", strdup(value), 0);memset(value, 0, sizeof(value));
	GetPrivateProfileString(TEXT("Settings"), TEXT("cust_hud"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	ex_thud = g_pEngine->pfnRegisterVariable("cust_hud", value, 0);memset(value, 0, sizeof(value));
	GetPrivateProfileString(TEXT("Settings"), TEXT("logs"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	logsfiles = g_pEngine->pfnRegisterVariable("logs", value, 0);memset(value, 0, sizeof(value));
	GetPrivateProfileString(TEXT("Settings"), TEXT("motd_block"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	motd_block = g_pEngine->pfnRegisterVariable("motd_block", value, 0);memset(value, 0, sizeof(value));
	GetPrivateProfileString(TEXT("Settings"), TEXT("type"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
	type = g_pEngine->pfnRegisterVariable("type", value, 0); memset(value, 0, sizeof(value));
	GetPrivateProfileString(TEXT("Settings"), TEXT("type"), TEXT("0"), value, ARRAYSIZE(value), g_settingsFileName);
}
//Shel be there;((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((
void HUD_Frame(double time){
	if (!FirstFrame){
		g_Screen.iSize = sizeof(SCREENINFO);offset.HLType = g_Studio.IsHardware() + 1;offset.ConsoleColorInitalize();
		offset.GetGameInfo(&BuildInfo);HookUserMessages();HookEngineMessages();InitHack();FirstFrame = true;
	}
	g_Engine.pfnGetScreenInfo(&g_Screen);
	g_Client.HUD_Frame(time);
}

void CL_CreateMove(float frametime, struct usercmd_s *cmd, int active){
	for (DWORD i = 1; i < 32; i++){
		player_info_s* player = g_pStudio->PlayerInfo(i);
		if (player && player->name && player->model){
			for (auto m : models_list){
				if (lstrcmpA(player->model, m.name) == 0){
					lstrcpyA(player->model, m.repl);
				}
			}
		}
	}
	g_Client.CL_CreateMove(frametime, cmd,active);
}
int MOTD(const char *pszName, int iSize, void *pbuf);
int MOTD(const char *pszName, int iSize, void *pbuf){
	if (logsfiles->value > 0){
		BEGIN_READ(pbuf, iSize);
		int konez = READ_BYTE();
		char* buff = READ_STRING();
		char str[1024];strncpy(str, buff, sizeof(str));
		str[sizeof(str) - 1] = 0;
		ConsolePrintColor(255, 255, 155, str);
		if (konez == 1)ConsolePrintColor(255, 255, 155, "\n");
	}
	if (motd_block->value > 0)return 1;
	
	return pMOTD(pszName, iSize, pbuf);
}
void HookUserMessages(){
	pUserMsgBase=(PUserMsg)offset.FindUserMsgBase();pMOTD=HookUserMsg("MOTD",MOTD);
} 
int pfnDrawUnicodeCharacter(int x, int y, short number, int r, int g, int b, unsigned long hfont) {
	if (ex_thud->value>0)return 1;return g_Engine.pfnDrawUnicodeCharacter(x,y,number,r,g,b,hfont);
}

void HookFunction(){
	g_pClient->CL_CreateMove = CL_CreateMove;
	g_pClient->HUD_Frame = HUD_Frame;
	g_pEngine->pfnDrawUnicodeCharacter = pfnDrawUnicodeCharacter; 
}
