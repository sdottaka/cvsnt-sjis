#include <stdio.h>
#include <malloc.h>
#include <string>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include "common.h"

int rcs_main(const char *command, int argc, char *argv[])
{
#ifdef _WIN32
	std::string s;
	LPSTR szCmd = GetCommandLine();
	char *p=szCmd;
	int inquote=0;

	while(inquote || !isspace(*p))
	{
		if(*p=='"')
			inquote=!inquote;
		p++;
	}
	szCmd=p+1;
	s="cvs rcsfile ";
	s+=command;
	s+=" ";
	s+=szCmd;

	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi = {0};
	DWORD exit;

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	if(!CreateProcess(NULL,(LPSTR)s.c_str(),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
		return -1;
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess,INFINITE);
	GetExitCodeProcess(pi.hProcess,&exit);
	CloseHandle(pi.hProcess);
	return (int)exit;
#else
	int n;
	char **nargv = (char**)malloc((argc+3)*sizeof(char*));

	nargv[0]="cvs";
	nargv[1]="rcsfile";
	nargv[2]=(char*)command;
	for(n=1; n<argc; n++)
		nargv[n+2]=argv[n];
	nargv[n+2]=NULL;

	execvp(nargv[0],nargv);
	perror("Couldn't run cvs");
	return -1;
#endif
}

