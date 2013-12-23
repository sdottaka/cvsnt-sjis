/* cvsnt uninstaller

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include "stdafx.h"

static std::string RemoveDirFromPath(LPCTSTR szPath, LPCTSTR szDir)
{
	std::string path = szPath;
	char *p;
	
	while((p=strstr(path.data(),szDir))!=NULL)
	{
		strcpy(p,p+strlen(szDir));
		if(*p==';')
			strcpy(p,p+1);
	}
	path.resize(strlen(path.c_str()));
	return path;
}

// Edit path
static void EditPath(HKEY hRootKey, LPCTSTR szSubKey, LPCTSTR szDir)
{
   int iResult = 0;
   LONG lRegResult;
   DWORD dwType;
   DWORD dwCount;
   std::string Path;
   std::string newPath;
   HKEY hKey = 0;

   try
   {
	// Open our registry key
	lRegResult = RegOpenKeyEx(hRootKey, szSubKey, 0, KEY_WRITE | KEY_READ, &hKey);
	if(lRegResult != ERROR_SUCCESS)
		throw 1;

	// Determine size of path buffer
	lRegResult = RegQueryValueEx(hKey, "Path", NULL, &dwType, NULL, &dwCount);
	if(lRegResult == ERROR_SUCCESS)
	{
		// Allocate buffer
		Path.resize(dwCount+1);

		// Retrieve path variable value
		lRegResult = RegQueryValueEx(hKey, "Path", NULL, &dwType, (LPBYTE)Path.data(), &dwCount);
		if(lRegResult == ERROR_SUCCESS)
		{
			// Remove CvsPath
			newPath = RemoveDirFromPath(Path.c_str(), szDir);

			// Write changed path 
			if(newPath != Path)
				RegSetValueEx(hKey, "Path", 0, dwType, (LPBYTE) newPath.c_str(), newPath.length() + 1);
		}
	}
   }
   catch(int)
   {
   }
   if(hKey != 0)
      RegCloseKey(hKey);
}


int main(int argc, char *argv[])
{
	std::string longDir;
	std::string shortDir;
	int n;

	if(argc<2)
	{
		fprintf(stderr,"Usage: %s <path>",argv[0]);
		return -1;
	}

	for(n=1; n<argc; n++)
	{
		longDir+=argv[n];
		longDir+=" ";
	}
	longDir.resize(longDir.size()-1);

	shortDir.resize(longDir.size()+1);
	GetShortPathName(longDir.c_str(),(LPTSTR)shortDir.data(),shortDir.size());
	shortDir.resize(strlen(shortDir.c_str()));

	// Edit system path
	EditPath(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 
      longDir.c_str());
	EditPath(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 
      shortDir.c_str());

	// Edit user path
	EditPath(HKEY_CURRENT_USER, "Environment", longDir.c_str());
	EditPath(HKEY_CURRENT_USER, "Environment", shortDir.c_str());
	return 0;
}

