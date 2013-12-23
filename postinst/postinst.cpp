// postinst.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#ifdef SJIS
#include <mbstring.h>
#endif

using namespace std;

static void MigrateCvsPass();

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MigrateCvsPass();
	return 0;
}

static string GetHomeDirectory()
{
	string path;
    char *hd, *hp;

    if ((hd = getenv ("HOME")))
		path=hd;
    else if ((hd = getenv ("HOMEDRIVE")) && (hp = getenv ("HOMEPATH")))
	{
		path=hd;
		path+=hp;
	}
    else
		path="";
	
#ifdef SJIS
	const unsigned char *cpath = (const unsigned char *)path.c_str();
	if(path.size() && ((path[path.size()-1]=='\\' && !_ismbstrail(cpath, cpath + path.size()-1)) || path[path.size()-1]=='//'))
#else
	if(path.size() && (path[path.size()-1]=='\\' || path[path.size()-1]=='//'))
#endif
		path.resize(path.size()-1);
	return path;
}

static bool WriteRegistryKey(LPCTSTR key, LPCTSTR value, LPCTSTR buffer)
{
	HKEY hKey,hSubKey;
	DWORD dwLen;

	if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Cvsnt",0,KEY_READ,&hKey) &&
	   RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Cvsnt",0,NULL,0,KEY_READ,NULL,&hKey,NULL))
	{
		return false; // Couldn't open or create key
	}

	if(key)
	{
		if(RegOpenKeyEx(hKey,key,0,KEY_WRITE,&hSubKey) &&
		   RegCreateKeyEx(hKey,key,0,NULL,0,KEY_WRITE,NULL,&hSubKey,NULL))
		{
			RegCloseKey(hKey);
			return false; // Couldn't open or create key
		}
		RegCloseKey(hKey);
		hKey=hSubKey;
	}

	if(!buffer)
	{
		RegDeleteValue(hKey,value);
	}
	else
	{
		dwLen=strlen(buffer);
		if(RegSetValueEx(hKey,value,0,REG_SZ,(LPBYTE)buffer,dwLen+1))
		{
			RegCloseKey(hKey);
			return false;
		}
	}
	RegCloseKey(hKey);

	return true;
}

void MigrateCvsPass()
{
	string path = GetHomeDirectory();
	char line[1024],*key,*pw;

	if(!path.size())
		return; /* Nothing to migrate */

	path+="\\.cvspass";
	FILE *f = fopen(path.c_str(),"r");
	if(!f)
		return; /* No .cvspass file */

	while(fgets(line,1024,f)>0)
	{
		line[strlen(line)-1]='\0';
		key = strtok(line," ");
		if(key[0]=='/') /* This was added in 1.11.1.  Not sure why. */
			key=strtok(NULL," ");
		if(key)
			pw=key+strlen(key)+1;
		if(key && pw)
			WriteRegistryKey("cvspass",key,pw);
	}

	fclose(f);

	/* Should we delete here?  At the moment I'm assuming people are mixing
	   legacy cvs with cvsnt.. perhaps later */
}

