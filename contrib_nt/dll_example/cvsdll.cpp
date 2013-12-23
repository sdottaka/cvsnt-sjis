// dllinfo.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "d:\cvsbin\infolib.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

int init(struct library_callback_t* cb, const char *repository, const char *username, const char *prefix, const char *sessionid, const char *hostname)
{
	return 0;
}

int close(struct library_callback_t* cb)
{
	return 0;
}

int pretag(struct library_callback_t* cb, const char *tag, const char *action, const char *repository, int pretag_list_count, const char **pretag_list)
{
	return 0;
}

int verifymsg(struct library_callback_t* cb, const char *filename)
{
	return 0;
}

int loginfo(struct library_callback_t* cb, const char *repository, const char *hostname, const char *directory, const char *message, const char *status, int change_list_count, change_info *change_list)
{
	char str[1024];
	sprintf(str,"Loginfo called on directory %s\n",directory);
	MessageBox(NULL,str,"dllinfo",MB_OK);
	return 0;
}

int history(struct library_callback_t* cb, const char *repository, const char *history_line)
{
	return 0;
}
int notify(struct library_callback_t* cb, const char *short_repository, const char *file, const char *type, const char *repository, const char *who)
{
	return 0;
}

int precommit(struct library_callback_t* cb, const char *repository, int precommit_list_count, const char **precommit_list)
{
	return 0;
}

int postcommit(struct library_callback_t* cb, const char *repository)
{
	return 0;
}

static library_callback callbacks =
{
	init,
	close,
	pretag,
	verifymsg,
	loginfo,
	history,
	notify,
	precommit,
	postcommit
};

extern "C" __declspec(dllexport) library_callback *GetCvsInfo()
{
	return &callbacks;
}

