// postinst.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#ifdef SJIS
#include <mbstring.h>
#endif

static void MigrateCvsPass();
static void MigrateRepositories();

static HKEY hServerKey;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MigrateCvsPass();

	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Software\\CVS\\PServer",0,KEY_READ|KEY_WRITE,&hServerKey))
	{
		MigrateRepositories();
		RegCloseKey(hServerKey);
	}
	return 0;
}

static std::string GetHomeDirectory()
{
	std::string path;
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
	std::string path = GetHomeDirectory();
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

#define MAX_REPOSITORIES 1024
struct RootStruct
{
	std::string root;
	std::string name;
	bool valid;
};
std::vector<RootStruct> Roots;

bool GetRootList()
{
	TCHAR buf[MAX_PATH],buf2[MAX_PATH],tmp[MAX_PATH];
	std::string prefix;
	DWORD bufLen;
	DWORD dwType;
	int drive;
	bool bModified = false;

	bufLen=sizeof(buf);
	if(!RegQueryValueEx(hServerKey,_T("RepositoryPrefix"),NULL,&dwType,(BYTE*)buf,&bufLen))
	{
		TCHAR *p = buf;
		while((p=_tcschr(p,'\\'))!=NULL)
			*p='/';
		p=buf+_tcslen(buf)-1;
		if(*p=='/')
			*p='\0';
		prefix = buf;
		bModified = true; /* Save will delete this value */
	}

	drive = _getdrive() + 'A' - 1;

	for(int n=0; n<MAX_REPOSITORIES; n++)
	{
		_sntprintf(tmp,sizeof(tmp),_T("Repository%d"),n);
		bufLen=sizeof(buf);
		if(RegQueryValueEx(hServerKey,tmp,NULL,&dwType,(BYTE*)buf,&bufLen))
			continue;
		if(dwType!=REG_SZ)
			continue;

		TCHAR *p = buf;
		while((p=_tcschr(p,'\\'))!=NULL)
			*p='/';

		_sntprintf(tmp,sizeof(tmp),_T("Repository%dName"),n);
		bufLen=sizeof(buf2);
		if(RegQueryValueEx(hServerKey,tmp,NULL,&dwType,(BYTE*)buf2,&bufLen))
		{
			_tcscpy(buf2,buf);
			if(prefix.size() && !_tcsnicmp(prefix.c_str(),buf,prefix.size()))
				_tcscpy(buf2,&buf[prefix.size()]);
			else
				_tcscpy(buf2,buf);
			if(buf[1]!=':')
				_sntprintf(buf,sizeof(buf),_T("%c:%s"),drive,buf2);

			p=buf2+_tcslen(buf2)-1;
			if(*p=='/')
				*p='\0';

			bModified = true;
		}
		else if(dwType!=REG_SZ)
			continue;

		RootStruct r;
		r.root = buf;
		r.name = buf2;
		r.valid = true;

		Roots.push_back(r);
	}
	return bModified;
}

void RebuildRootList()
{
	std::string path,desc;
	TCHAR tmp[64];
	int j;
	size_t n;

	for(n=0; n<MAX_REPOSITORIES; n++)
	{
		_sntprintf(tmp,sizeof(tmp),_T("Repository%d"),n);
		RegDeleteValue(hServerKey,tmp);
	}

	for(n=0,j=0; n<Roots.size(); n++)
	{
		path=Roots[n].root;
		desc=Roots[n].name;
		if(Roots[n].valid)
		{
			_sntprintf(tmp,sizeof(tmp),_T("Repository%d"),j);
			RegSetValueEx(hServerKey,tmp,NULL,REG_SZ,(BYTE*)path.c_str(),(path.length()+1)*sizeof(TCHAR));
			_sntprintf(tmp,sizeof(tmp),_T("Repository%dName"),j);
			RegSetValueEx(hServerKey,tmp,NULL,REG_SZ,(BYTE*)desc.c_str(),(desc.length()+1)*sizeof(TCHAR));
			j++;
		}
	}

	RegDeleteValue(hServerKey,_T("RepositoryPrefix"));
}

void MigrateRepositories()
{
	if(GetRootList())
		RebuildRootList();
}
