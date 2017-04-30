#include "main.h"

#pragma warning(disable:4996)
int* MSG_ReadCount = nullptr;
#define equali !stricmp
int* MSG_CurrentSize = nullptr;
int* MSG_BadRead = nullptr;
int MSG_SavedReadCount = 0;
sizebuf_t* MSG_Buffer = nullptr;
#define MAX_CMD_LINE 2048

extern int g_blockedCmdCount;
extern char *g_blockedCmds[MAX_CMD_LINE];

extern int g_serverCmdCount;
extern char *g_serverCmds[MAX_CMD_LINE];
char com_token[1024];
extern cvar_t *logsfiles;
HL_MSG_ReadByte MSG_ReadByte = nullptr;
HL_MSG_ReadShort MSG_ReadShort = nullptr;
HL_MSG_ReadLong MSG_ReadLong = nullptr;
HL_MSG_ReadFloat MSG_ReadFloat = nullptr;
HL_MSG_ReadString MSG_ReadString = nullptr;
HL_MSG_ReadCoord MSG_ReadCoord = nullptr;
HL_MSG_ReadBitVec3Coord MSG_ReadBitVec3Coord = nullptr;
HL_MSG_ReadBits MSG_ReadBits = nullptr;
HL_MSG_StartBitReading MSG_StartBitReading = nullptr;
HL_MSG_EndBitReading MSG_EndBitReading = nullptr;
void MSG_SaveReadCount() {
	MSG_SavedReadCount = *MSG_ReadCount;
}

void MSG_RestoreReadCount() {
	*MSG_ReadCount = MSG_SavedReadCount;
}
pfnEngineMessage pSVC_VoiceInit;

pfnEngineMessage pSVC_StuffText;
pfnEngineMessage pSVC_SendCvarValue;
pfnEngineMessage pSVC_SendCvarValue2;
pfnEngineMessage pSVC_Director;


typedef enum cmd_source_s
{
	src_client = 0,		// came in over a net connection as a clc_stringcmd. host_client will be valid during this state.
	src_command = 1,	// from the command buffer.
} cmd_source_t;

void __cdecl ExecuteString(char *text, cmd_source_t src);

HOOKINIT(
	ExecuteString_F,								// the type created 
	ExecuteString,								// the function prototyped
	ExecuteString_Tramp,							// the trampoline to the original function
	ExecuteString_Prologue						// the prologue object of the function used for this hook
)

DWORD ExecuteString_call;
DWORD ExecuteString_jump;
DWORD Cbuf_Addtext_call;
DWORD Cbuf_Addtext_jump;
DWORD Cbuf_Execute_call;
DWORD Cbuf_Execute_jump;

EasyHook::Hook32 hooker; // an object meant to service you

bool ParseList(const char *str) {
	for (DWORD i = 0; i < g_blockedCmdCount; i++) {
		if (!stricmp(str, g_blockedCmds[i])) {
			return true;
		}
	}
	return false;
}//:D more shit code, in reborn no 

bool ParseList2(const char *str) {
	for (DWORD i = 0; i < g_serverCmdCount; i++) {
		if (!stricmp(str, g_serverCmds[i])) {
			return true;
		}
	}
	return false;
}

int ParseListCvar(const char *str) {
	auto found = FindCvar(str, Cvars);
	if (found == -1)return -1;
	else return Cvars[found].mode;
}

bool IsCommandGood(const char *str) {
	char *ret = g_Engine.COM_ParseFile((char *)str, com_token);
	if (ret == NULL || com_token[0] == 0)return true;
	if ((ParseList(com_token)))return false;
	return true;
}

bool IsCommandGood2(const char *str) {
	char *ret = g_Engine.COM_ParseFile((char *)str, com_token);
	if (ret == NULL || com_token[0] == 0)return true;
	if ((ParseList2(com_token)))return false;
	return true;
}

bool CheckExecute(char *text)
{
	bool isGood = IsCommandGood(text);
	bool isGood2 = IsCommandGood2(text);
	bool isSet = CheckAndSetCvar(text);
	bool isFake = CheckIsFake(text);
	char *x = text;
	if (!isGood2) {
		g_Engine.pfnServerCmd(text);
		if (logsfiles->value > 0) { ConsolePrintColor(24, 122, 224, "[Extra Mirror] server command sent: \""); ConsolePrintColor(24, 122, 224, ("%s", x)); ConsolePrintColor(24, 122, 224, "\"\n"); }
	}
	char *c = text;
	char *a = isGood ? "[Extra Mirror] execute: \"" : "[Extra Mirror] blocked: \"";
	if (logsfiles->value > 0) { ConsolePrintColor(255, 255, 255, ("%s", a)); ConsolePrintColor(255, 255, 255, ("%s", c)); ConsolePrintColor(255, 255, 255, "\"\n"); }
	/*else*/if (isSet)a = "[Extra Mirror] update server-side cvar: \"";

	if (isGood)
	{
		return true;
	}
	return false;
}
// experimental
__declspec(naked) void Cmd_ExecuteString_CallHook( )
{	
	static char *text;
	__asm MOV text, ECX
	bool Test;
	Test = CheckExecute(text);
	if (Test)
	{
		__asm PUSH EBP
		__asm MOV EBP, ESP
		__asm MOV ECX, [EBP + 0x8]
		__asm MOV EAX, [EBP + 0xC]
		__asm JMP[ExecuteString_jump]
	}
	else
	{
		__asm ret;
	}
	
}/*
__declspec(naked) void Cmd_ExecuteString_CallHook()
{
	char *text;
	cmd_source_t src;
	__asm {
		PUSH EBP
		MOV EBP, ESP
		MOV ECX, [EBP + 0x8]
		MOV EAX, [EBP + 0xC]
		PUSH EAX
		PUSH ECX
		MOV text, ECX
		MOV src, EAX
		POP ECX
		POP EAX
		POP EBP
	}
	__asm {
		PUSH EBP
		MOV EBP, ESP
		MOV ECX, [EBP + 0x8]
		MOV EAX, [EBP + 0xC]
		jmp[ExecuteString_jump]
	}
	ConsolePrintColor(0, 255, 255, "%s", text);
	hooker.unhook(ExecuteString_Tramp, ExecuteString_Prologue);
}
/*__declspec(naked) void Cmd_ExecuteString_CallHook()
{
	char *text;
	cmd_source_t src;
	__asm {
		PUSH EBP
		MOV EBP, ESP
		MOV ECX, [EBP + 0x8]
		MOV EAX, [EBP + 0xC]
		PUSH EAX
		PUSH ECX
		MOV text, ECX
		MOV src, EAX
		call ExecuteString
		POP ECX
		POP EAX
		POP EBP
	}
	//bool Test;
	//Test = CheckExecute((char*)&text);

	//if (Test)
	__asm {
		PUSH EBP
		MOV EBP, ESP
		MOV ECX, [EBP + 0x8]
		MOV EAX, [EBP + 0xC]
		jmp[ExecuteString_jump]
	}
	hooker.unhook(ExecuteString_Tramp, ExecuteString_Prologue);
}*/
/*
void __cdecl ExecuteString(char *text, cmd_source_t src)
{
	if (FirstFrame)
		ConsolePrintColor(0, 255, 0, "%s %d \n", text, src);

	//MessageBox(NULL, text, NULL, MB_OK);
}
*/
void ExecuteString_Test(const char *str, pfnEngineMessage Func) {
	ExecuteString_Tramp = (ExecuteString_F)hooker.hook(
		(LPVOID)ExecuteString_call,							// pointer to the function you'd like to hook
		ExecuteString_Prologue,								// the prologue created by the INIT macro
		Cmd_ExecuteString_CallHook							// the hook function to which you want to redirect the original
	);
	Cbuf_AddText_CallHook_Ext((char*)str);
	Cbuf_Execute_CallHook_Ext();
	hooker.unhook(ExecuteString_Tramp, ExecuteString_Prologue);
}


bool BlackList(char *str) {
	bool changed = false;
	char *text = str;
	char command[MAX_CMD_LINE];
	int i, quotes;
	int len = strlen(str);
	while (text[0] != 0) {
		quotes = 0;
		for (i = 0; i < len; i++) {
			if (text[i] == '\"') quotes++;
			if (text[i] == '\n')break;
			if (!(quotes & 1) && text[i] == ';')break;
			if (text[i] == 0x00 || text[i] == ' ' )break;
		}
		if (i >= MAX_CMD_LINE)i = MAX_CMD_LINE;
		strncpy(command, text, i); command[i] = 0;
		bool isGood = IsCommandGood(command);
		bool isGood2 = IsCommandGood2(command);
		bool isSet = CheckAndSetCvar(command);
		bool isFake = CheckIsFake(command);
		char *x = command;
		if (!isGood2) {
			g_Engine.pfnServerCmd(command);
			if (logsfiles->value > 0) { ConsolePrintColor(24, 122, 224, "[Extra Mirror] server command sent: \""); ConsolePrintColor(24, 122, 224, ("%s", x)); ConsolePrintColor(24, 122, 224, "\"\n"); }
		}
		char *c = command;
		char *a = isGood ? "[Extra Mirror] execute: \"" : "[Extra Mirror] blocked: \"";
		if (logsfiles->value > 0) { ConsolePrintColor(255, 255, 255, ("%s", a)); ConsolePrintColor(255, 255, 255, ("%s", c)); ConsolePrintColor(255, 255, 255, "\"\n"); }
		//		if (isFake) a = isGood ? "[Extra Mirror] set fake cvar: \"" : "[Extra Mirror] block fake cvar: \"";
		/*else*/if (isSet)a = "[Extra Mirror] update server-side cvar: \"";
		if (isGood)g_Engine.pfnClientCmd(c);
		if (isSet) { if (logsfiles->value > 0) { ConsolePrintColor(255, 255, 255, ("%s", a)); ConsolePrintColor(255, 255, 255, ("%s", c)); ConsolePrintColor(255, 255, 255, "\"\n"); } }
		len -= i;
		if (!isGood) { strncpy(text, text + i, len); text[len] = 0; text++; changed = true; }
		else { text += i + 1; }
	}
	return true;
}

void SVC_SendCvarValue() {
	MSG_SaveReadCount();
	char* cvar = MSG_ReadString();
	char str[1024];
	strncpy(str, cvar, sizeof(str));
	str[sizeof(str) - 1] = 0;
	cvar_t *pCvar = g_Engine.pfnGetCvarPointer(str);
	if (pCvar != NULL) {
		int mode = ParseListCvar(str);
		if (mode == cvar_fake || mode == cvar_open) {
			if (logsfiles->value > 0) {
				ConsolePrintColor(255, 255, 255, "[Extra Mirror] request %s cvar: ", mode == cvar_fake ? "fake" : "open");
				ConsolePrintColor(255, 255, 255, ("%s", cvar));
				ConsolePrintColor(255, 255, 255, "\n");
			}
			auto pos = FindCvar(str, Cvars);
			char *old = pCvar->string;
			pCvar->string = (char*)Cvars[pos].value.c_str();
			MSG_RestoreReadCount();
			pSVC_SendCvarValue();
			pCvar->string = old;
		}
		else if (mode == cvar_bad) {
			if (logsfiles->value > 0) {
				ConsolePrintColor(255, 255, 255, "[Extra Mirror] request blocked cvar: ");
				ConsolePrintColor(255, 255, 255, ("%s", cvar));
				ConsolePrintColor(255, 255, 255, "\n");
			}
			char *old = pCvar->string;
			pCvar->string = "Bad CVAR request";
			MSG_RestoreReadCount();
			pSVC_SendCvarValue();
			pCvar->string = old;
		}
		else {
			if (logsfiles->value > 0) {
				ConsolePrintColor(255, 255, 255, "[Extra Mirror] request cvar: ");
				ConsolePrintColor(255, 255, 255, ("%s", cvar));
				ConsolePrintColor(255, 255, 255, "\n");
			}
			MSG_RestoreReadCount();
			pSVC_SendCvarValue();
		}
	}
	else {
		if (logsfiles->value > 0) {
			ConsolePrintColor(255, 255, 255, "[Extra Mirror] request non-exist cvar: ");
			ConsolePrintColor(255, 255, 255, (" %s", cvar));
			ConsolePrintColor(255, 255, 255, "\n");
		}
		MSG_RestoreReadCount();
		pSVC_SendCvarValue();
	}
}
void SVC_SendCvarValue2() {
	MSG_SaveReadCount();
	MSG_ReadLong();
	char* cvar = MSG_ReadString();
	char str[1024];
	strncpy(str, cvar, sizeof(str));
	str[sizeof(str) - 1] = 0;
	cvar_t *pCvar = g_Engine.pfnGetCvarPointer(str);
	if (pCvar != NULL) {
		int mode = ParseListCvar(str);
		if (mode == cvar_fake || mode == cvar_open) {
			if (logsfiles->value > 0) {
				ConsolePrintColor(255, 255, 255, "[Extra Mirror] request %s cvar2: ", mode == cvar_fake ? "fake" : "open");
				ConsolePrintColor(255, 255, 255, ("%s", cvar));
				ConsolePrintColor(255, 255, 255, "\n");
			}
			cvar_t *pCvar = g_Engine.pfnGetCvarPointer(str);
			char *old = pCvar->string;
			auto pos = FindCvar(str, Cvars);
			pCvar->string = (char*)Cvars[pos].value.c_str();
			MSG_RestoreReadCount();
			pSVC_SendCvarValue2();
			pCvar->string = old;
		}
		else if (mode == cvar_bad) {
			if (logsfiles->value > 0) {
				ConsolePrintColor(255, 255, 255, "[Extra Mirror] request blocked cvar2: ");
				ConsolePrintColor(255, 255, 255, ("%s", cvar));
				ConsolePrintColor(255, 255, 255, "\n");
			}
			cvar_t *pCvar = g_Engine.pfnGetCvarPointer(str);
			char *old = pCvar->string;
			pCvar->string = "Bad CVAR request";
			MSG_RestoreReadCount();
			pSVC_SendCvarValue2();
			pCvar->string = old;
		}
		else {
			if (logsfiles->value > 0) {
				ConsolePrintColor(255, 255, 255, "[Extra Mirror] request cvar2: ");
				ConsolePrintColor(255, 255, 255, ("%s", cvar));
				ConsolePrintColor(255, 255, 255, "\n");
			}
			MSG_RestoreReadCount();
			pSVC_SendCvarValue2();
		}
	}
	else {
		if (logsfiles->value > 0) {
			ConsolePrintColor(255, 255, 255, "[Extra Mirror] request non-exist cvar2: ");
			ConsolePrintColor(255, 255, 255, (" %s", cvar));
			ConsolePrintColor(255, 255, 255, "\n");
		}
		MSG_RestoreReadCount();
		pSVC_SendCvarValue2();
	}
}
bool CheckIsFake(string FullCmd) {
	// Find first space character
	size_t p = FullCmd.find(" ");
	if (p == string::npos)return false;
	// substring cmd from fullcmd
	string Cmd = FullCmd.substr(0, p);
	auto pos = FindCvar(Cmd, Cvars);
	if (pos == -1)return false;
	if (Cvars[pos].mode == cvar_fake)return true;
	return false;
}

bool CheckAndSetCvar(string FullCmd) {
	// Find first space character
	size_t p = FullCmd.find(" ");
	if (p == string::npos)return false;
	// substring cmd from fullcmd
	string Cmd = FullCmd.substr(0, p);
	auto pos = FindCvar(Cmd, Cvars);
	if (pos == -1)return false;
	if (Cvars[pos].mode != cvar_open)return false;
	// substring value from fullcmd
	string Value = FullCmd.substr(p + 1);
	Cvars[pos].value = Value;
	return true;
}
void SVC_StuffText() {	
	//MSG_SaveReadCount();
	char* command = MSG_ReadString();
	//MSG_RestoreReadCount();
	ExecuteString_Test(command, pSVC_StuffText);
		/*char str[1024];
	strncpy(str, command, sizeof(str));
	str[sizeof(str) - 1] = 0;
	if (BlackList(str))return;
	MSG_RestoreReadCount();*/
	//ConsolePrintColor(0, 255, 0, "%s", command);	
}
void SVC_Director() {
	/*MSG_SaveReadCount();
	int msglen = MSG_ReadByte();
	int msgtype = MSG_ReadByte();
	char* DirectCommand = MSG_ReadString();
	if (msgtype == 10) {
		char str[1024];
		strncpy(str, DirectCommand, sizeof(str));
		str[sizeof(str) - 1] = 0;
		if (BlackList(str))return;
	}
	MSG_RestoreReadCount();
	pSVC_Director();*/
}
void SVC_VoiceInit() {
	MSG_SaveReadCount();
	char* codec = MSG_ReadString(); int bitz = MSG_ReadByte(); bool blocked;
	if (!stricmp(codec, "voice_miles") || !stricmp(codec, "voice_speex"))blocked = false;
	else blocked = true;
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "[Extra Mirror] [VoiceInit] %s [%s]\n", codec, blocked ? "Blocked" : "Execute");
	ConsolePrintColor(255, 255, 255, buffer);
	if (blocked)return;
	MSG_RestoreReadCount();
	pSVC_VoiceInit();
}
/*
void SVC_Resourcelist() {
MSG_SaveReadCount();
int NumResources, Type, Index, DownloadSize, HasExtraInfo, ExtraInfo, HasConsistency, Flags, Flags;
MSG_StartBitReading(MSG_Buffer);
NumResources = MSG_ReadBits(12);

for (int i = 1; i <= NumResources; i++) {
Type = MSG_ReadBits(4);
char* szFileName[64];
//		szFileName = MSG_ReadBitString();
Index = MSG_ReadBits(12);
DownloadSize = MSG_ReadBits(24);
unsigned char Flags = READ_CHAR();
unsigned char rgucMD5_hash[16];
for (int i = 0; i < 16; i++)(BYTE)rgucMD5_hash[i] = READ_CHAR();
HasExtraInfo = MSG_ReadBits(1);
if (HasExtraInfo)ExtraInfo = MSG_ReadBits(256);
}
HasConsistency = MSG_ReadBits(1);


}
*/
