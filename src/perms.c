/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS 1.4 kit.
 * 
 * Checking permissions for the current user in the specified
 * directory.
 * 
 */

#include "cvs.h"

#ifndef _WIN32
#include <sys/types.h>
#include <grp.h>
#include <limits.h>
#endif

#ifndef _MAX_PATH
#define _MAX_PATH PATH_MAX
#endif

#include <getline.h>

#define OWNER_FILE "/.owner"
#define PERMS_FILE "/.perms"

typedef struct valid_names_s
{
    struct valid_names_s *next;
    char *name;
	int isgroup;
} valid_names_t;

/* A list of all names that the user goes by, primarily groups. */
static valid_names_t *valid_names = NULL;
static char perms_cache_dir[_MAX_PATH] = {0};
static char *perms_cache = NULL;

static int perms_getline(char **buffer, int *buffer_len, char **ptr)
{
	char *endptr;
	while(**ptr && **ptr<' ')
		(*ptr)++;
	if(!**ptr)
	{
		*buffer_len = 0;
		return -1;
	}
	endptr = strchr(*ptr,'\n');
	if(endptr)
		*buffer_len = endptr - *ptr;
	else
		*buffer_len = strlen(*ptr);
	*buffer = xmalloc((*buffer_len)+1);
	memcpy(*buffer,*ptr,*buffer_len);
	(*buffer)[*buffer_len]='\0';
	(*ptr)+=*buffer_len;
	return *buffer_len;
}

static void add_valid_name(const char *name, int isgroup)
{
    valid_names_t *new_name = xmalloc(sizeof(*new_name));

#ifdef _WIN32
	assert(name);
#endif
    new_name->name = xstrdup(name);
    new_name->next = valid_names;
	new_name->isgroup = isgroup;
    valid_names = new_name;
}

/* Add the username to the list of valid names, and then find all the
   groups the user is in and add them to the list of valid names. */
static void
get_valid_names()
{
    char   *filename;
    char   *names;
    char   *name;
    char   *group;
    FILE   *fp;
    char   *linebuf = NULL;
    size_t linebuf_len;

    if (valid_names != NULL)
	return;

    add_valid_name(CVS_Username,0);

    filename = xmalloc (strlen (current_parsed_root->directory)
			+ strlen ("/CVSROOT")
			+ strlen ("/group")
			+ 1);

    strcpy (filename, current_parsed_root->directory);
    strcat (filename, "/CVSROOT");
    strcat (filename, "/group");

    fp = CVS_FOPEN (filename, "r");
    if (fp != NULL) {
	while (getline (&linebuf, &linebuf_len, fp) >= 0) {
	    group = cvs_strtok(linebuf, ":\n");
	    if (group == NULL)
		continue;
	    names = cvs_strtok(NULL, ":\n");
	    if (names == NULL)
		continue;

	    name = cvs_strtok(names, ", \t");
	    while (name != NULL) {
		if(!strcasecmp(name,"admin"))
			error(0,0,"The group 'admin' is automatically assigned to repository administrators");
		if (!fncmp (CVS_Username, name)) {
		    add_valid_name(group,1);
		    break;
		}
		name = cvs_strtok(NULL, ", \t");
            }

            if( linebuf != NULL ) {
		xfree (linebuf);
		linebuf = NULL;
            }
	}

	if (ferror (fp))
	    error (1, errno, "cannot read %s", filename);
	if (fclose (fp) < 0)
	    error (0, errno, "cannot close %s", filename);
    }
    /* If we can't open the group file, we just don't do groups. */

	if(verify_admin())
		add_valid_name("admin",1);

    xfree(filename);
}

static int verify_valid_name(char *name)
{
    valid_names_t *names = valid_names;

    while (names != NULL) {
	if (!fncmp (names->name, name)) {
	    return 1;
	}

	names = names->next;
    }

    return 0;
}

int
verify_admin ()
{
	char   *filename;
	FILE   *fp;
	char   *linebuf = NULL;
	size_t linebuf_len;
	char   *name;
	static int is_the_admin = 0;
	static int admin_valid = 0;

	if(admin_valid)
		return is_the_admin;

	filename = xmalloc (strlen (current_parsed_root->directory)
		  + strlen ("/CVSROOT")
		  + strlen ("/admin")
		  + 1);

	strcpy (filename, current_parsed_root->directory);
	strcat (filename, "/CVSROOT");
	strcat (filename, "/admin");

	TRACE(1,"Checking admin file %s for user %s",
	   		filename, CVS_Username);

	fp = CVS_FOPEN (filename, "r");
	if (fp != NULL)
	{
		while (getline (&linebuf, &linebuf_len, fp) >= 0)
		{
			name = cvs_strtok(linebuf,"\n");
			if (!fncmp (CVS_Username, name))
			{
				is_the_admin = 1;
				xfree(linebuf);
				break;
			}
			xfree (linebuf);
		}
		if (ferror (fp))
		error (1, errno, "cannot read %s", filename);
		if (fclose (fp) < 0)
		error (0, errno, "cannot close %s", filename);
	}

	if(system_auth && !is_the_admin)
	{
#ifdef WIN32
		if(win32_isadmin())
			is_the_admin = 1;
#else
#ifdef CVS_ADMIN_GROUP
	{
	  struct group *gr;
	  gid_t groups[NGROUPS_MAX];
	  int ngroups,n;

	  gr = getgrnam(CVS_ADMIN_GROUP);
	  if(gr)
	  {
	    ngroups = getgroups(NGROUPS_MAX,groups);
	    for(n=0; n<ngroups; n++)
	    {
	      if(groups[n]==gr->gr_gid)
	      {
	        is_the_admin = 1;
	        break;
	      }
	    }
	  }
	}    
#endif
#endif
	}

	admin_valid = 1;
	return is_the_admin;
}

int
verify_owner (dir)
   const char *dir;
{
   char   *filename;
   char   *linebuf = NULL;
   size_t linebuf_len;
   char   *owner;
   FILE   *fp;
   int    retval = 0;


   if (verify_admin ())
      return 1;

   get_valid_names();

   filename = xmalloc (strlen (dir)
		       + strlen (OWNER_FILE)
		       + 1);

   strcpy (filename, dir);
   {
       int  done = 0;

       strcat (filename, OWNER_FILE);

       fp = CVS_FOPEN (filename, "r");
       if (fp == NULL)
       {
		   if(!existence_error(errno))
			error (0, errno, "cannot read owner file in %s", dir);
       }
       else
       {
	   if (getline (&linebuf, &linebuf_len, fp) >= 0)
	   {
	       owner = cvs_strtok(linebuf, "\n");
	       if (verify_valid_name(owner)) {
		   retval = 1;
		   done = 1;
	       }

	       xfree (linebuf);
	       linebuf = NULL;
	   }

	   if (ferror (fp))
	       error (0, errno, "cannot read %s", filename);
	   if (fclose (fp) < 0)
	       error (0, errno, "cannot close %s", filename);
       }
   }

   xfree (filename);

   return retval;
}

/* Get the permissions from the directory for the given name.
   Returns 1 if successful, 0 if it fails. */
static int find_perms (const char *dir, char *perm, char *username, const char *tag, int allow_default)
{
	char   *filename;
	char   *linebuf = NULL;
	size_t linebuf_len;
	char   *name;
	char   *permptr;
	FILE   *fp;
	int    retval = 0;
	char   all_perms[4];
	char *namebra;
	off_t len;

	TRACE(3,"find_perms(%s,%s,%s,%s,%d)",dir,perm,username,tag,allow_default);

	if(tag && !strcmp(tag,"HEAD"))
		tag = NULL;

	if(fncmp(perms_cache_dir,dir))
	{
		filename = xmalloc (strlen (dir)
		   + strlen (PERMS_FILE)
		   + 1);

		strcpy (filename, dir);
		strcat (filename, PERMS_FILE);

		fp = CVS_FOPEN (filename, "r");
		if (fp == NULL)
		{
			if(!existence_error(errno))
			{
				error (0, errno, "cannot open %s", filename);
				retval = 0;
			}
			xfree(perms_cache);
			perms_cache = NULL;
			strcpy(perms_cache_dir,dir);
			retval = 1;
		}
		else
		{
			CVS_FSEEK(fp,0,2);
			len = CVS_FTELL(fp);
			CVS_FSEEK(fp,0,0);
			perms_cache = xrealloc(perms_cache, len+1);
			len = fread(perms_cache,1,len,fp);
			fclose(fp);
			perms_cache[len]='\0';

			strcpy(perms_cache_dir,dir);
		}
		xfree (filename);
	}

	all_perms[0] = '\0';

	if(!retval && perms_cache)
	{
		char *ptr = perms_cache;

		while (perms_getline (&linebuf, &linebuf_len, &ptr) >= 0)
		{
			name = cvs_strtok(linebuf, ":");

			if(!name)
			{
				xfree(linebuf);
				continue; /* Illegal line in .perms */
			}

			if(name[0]=='{')
			{
				namebra = name+1;
#ifdef SJIS
				name=_mbschr(name,'}')+1;
#else
				name=strchr(name,'}')+1;
#endif
			}
			else
				namebra=NULL;

			if(!name)
			{
				xfree(linebuf);
				continue; /* Illegal line in .perms */
			}

			if (!strcasecmp(name, username) && 
				((namebra && tag && !strncmp(namebra,tag,(name-namebra)-1)) ||
				((!namebra || !strncmp(namebra, "HEAD}", 5)) && !tag)))
			{
				permptr = cvs_strtok(NULL, "\n :");
				if (permptr != NULL)
				{
					strncpy(perm, permptr, 3);
					perm[3] = '\0';
					retval = 1;
				}
				xfree (linebuf);
				break;
			}
			if (!strcasecmp(name, "DEFAULT") && 
				((namebra && tag && !strncmp(namebra,tag,(name-namebra)-1)) ||
				((!namebra || !strncmp(namebra, "HEAD}", 5)) && !tag)))
			{
				permptr = cvs_strtok(NULL, "\n :");
				if (permptr != NULL)
				{
				   strncpy(all_perms, permptr, 3);
				   all_perms[3] = '\0';
				}
			}

			xfree (linebuf);
			linebuf = NULL;
		}

		/* If we didn't find the user but found ALL, set it to ALL's perms */
		if (allow_default && retval == 0)
		{
			if(all_perms[0] != '\0')
				strcpy(perm, all_perms);
			else 
				strcpy(perm, "rwc");	/* Assumed default:rwc */
			retval = 1;
		}
	}

	/* If no perms file, always allow access */
	if(!perms_cache)
	{
		strcpy(perm,"rwc");
		retval = 1;
	}

	return retval;
}

static int verify_perm (const char *dir, char perm, const char *tag)
{
   char perms[4]={0};
   char *currdir;
   const char *p;
   int  retval;
   int name_found;

    /* The whole cvs code assumes any directory called 'EmptyDir'
	   is invalid wherever it is. */
#ifdef SJIS
    for(p=dir+strlen(dir)-1; p>dir && !isslashmb(dir,p); p--)
#else
    for(p=dir+strlen(dir)-1; p>dir && !isslash(*p); p--)
#endif
		;
#ifdef SJIS
	if(isslashmb(dir,p))
#else
	if(isslash(*p))
#endif
		p++;
	if(!fncmp(p,CVSNULLREPOS))
		return 1; // Don't do a verify on CVSROOT/Emptydir

   get_valid_names();

   currdir = xstrdup(dir);

   do
   {
       char *slashptr;
	   int pass;
       valid_names_t *names = valid_names;

	   name_found = 0;
	   retval = 0;
	   for(pass=0; pass<2; pass++)
	   {
			names = valid_names;
			while (names != NULL)
			{
				if((pass==0 && !names->isgroup)
					|| (pass==1 && names->isgroup))
				{
					names = names->next;
					continue;
				}

				if (find_perms (currdir, perms, names->name, tag, pass==1 && !name_found))
				{
					if (strchr(perms, perm) != NULL)
					{
						retval = 1;
						name_found = 1;
						break; /* Break out of loop on the first positive ACL */
					}
					else
					{
						retval = 0;
						name_found = 1;
					}
				}
				names = names->next;
			}
	   }
	   if(name_found && !retval)
		   break;

	   perm = 'r'; /* Parent directories only need read access */

       slashptr = strrchr(currdir, '/');
       if (slashptr == NULL)
		break;

       *slashptr = '\0';

   } while (strlen(currdir) > strlen(current_parsed_root->directory));

   xfree(currdir);
   return retval;
}

int
verify_read (const char *dir, const char *tag)
{
    return verify_perm(dir, 'r', tag);
}

int
verify_write (const char *dir, const char *tag)
{
    return verify_perm(dir, 'w', tag);
}

int
verify_create (const char *dir, const char *tag)
{
    return verify_perm(dir, 'c', tag);
}

int
change_owner (dir, user)
   const char *dir;
   const char *user;
{
   char *filename;
   FILE *fp;
   int retval = 0;

   filename = xmalloc (strlen (dir)
		       + strlen (OWNER_FILE)
		       + 1);

   strcpy (filename, dir);
   strcat (filename, OWNER_FILE);

   fp = CVS_FOPEN (filename, "w");
   if (fp == NULL)
   {
      error (0, errno, "cannot open %s for writing", filename);
      retval = 0;
   }
   else
   {
      fprintf (fp, "%s\n", user);
      if (ferror (fp))
	 error (0, errno, "cannot write %s", filename);
      if (fclose (fp) < 0)
	 error (0, errno, "cannot close %s", filename);
   }

   xfree (filename);

   perms_cache_dir[0]='\0'; /* Invalidate cache */

   return retval;
}

static void
node_deleted(p)
   Node *p;
{
   if (p->data != NULL)
      xfree (p->data);
}

static int
write_node(p, v)
   Node *p;
   void *v;
{
   FILE *fp;

   fp = v;
   fprintf(fp, "%s:%s\n", p->key, p->data);
   return 0;
}

int copy_perms_from_parent(const char *dir)
{
	char *parent = xmalloc(strlen(dir)+strlen(PERMS_FILE)+1);
	char *child = xmalloc(strlen(dir)+strlen(PERMS_FILE)+1);
	strcpy(parent,dir);	
	strcpy(child,dir);
	if(strrchr(parent,'/'))
	{
		*strrchr(parent,'/')='\0';
		strcat(parent,PERMS_FILE);
		strcat(child,PERMS_FILE);
		if(!copy_file(parent,child,1, 0))
		{
			xfree(parent);
			xfree(child);
			return 0;
		}
	}
	xfree(parent);
	xfree(child);
	return change_perms(dir,"", NULL, NULL);
}

int change_perms(const char *dir, const char *user, const char *perm, const char *tag)
{
   char   *filename;
   char   *linebuf = NULL;
   size_t linebuf_len;
   char   *name;
   FILE   *fp;
   int    retval = 1;
   List   *userlist;
   char   *permptr;
   Node   *namenode;
   char	  *key;

   filename = xmalloc (strlen (dir)
		       + strlen (PERMS_FILE)
		       + 1);

   strcpy (filename, dir);
   strcat (filename, PERMS_FILE);

   userlist = getlist();

   fp = CVS_FOPEN (filename, "r");
   if (fp != NULL)
   {
      while (getline (&linebuf, &linebuf_len, fp) >= 0)
      {
		 name = cvs_strtok(linebuf, ":");
		 permptr = cvs_strtok(NULL, "\n :");
	 if ((permptr != NULL) && strlen(permptr) != 0)
	 {
	    namenode = getnode();
	    namenode->type = LIST;
	    namenode->key = xstrdup(name);
	    namenode->data = xstrdup (permptr);
	    namenode->delproc = node_deleted;
	    addnode(userlist, namenode);
	 }

	 xfree (linebuf);
	 linebuf = NULL;
      }
      if (ferror (fp))
		error (1, errno, "cannot read %s", filename);
      if (fclose (fp) < 0)
		 error (0, errno, "cannot close %s", filename);
   }
   else
   {
	   namenode = getnode();
	   namenode->type = LIST;
	   namenode->key = xstrdup("default");
	   namenode->data = xstrdup("rwc");
	   namenode->delproc = node_deleted;
	   addnode(userlist, namenode);
   }

   if(tag)
   {
	   key = xmalloc(strlen(tag)+3+strlen(user));
	   sprintf(key,"{%s}%s",tag,user);
   }
   else
	   key=xstrdup(user);

   namenode = findnode(userlist, key);
   if (namenode == NULL)
   {
      if (perm != NULL || !strcasecmp(key,"DEFAULT"))
      {
		 namenode = getnode();
		 namenode->type = LIST;
		 namenode->key = xstrdup (key);
		 namenode->data = xstrdup (perm?perm:"rwc");
		 namenode->delproc = node_deleted;
		 addnode(userlist, namenode);
      }
   }
   else if (perm != NULL || !strcasecmp(key,"DEFAULT"))
   {
      xfree (namenode->data);
	  namenode->data = xstrdup (perm?perm:"rwc");
   }
   else
   {
      delnode (namenode);
   }
   xfree(key);
   
   fp = CVS_FOPEN (filename, "w");
   if (fp == NULL)
   {
      error (0, errno, "cannot open %s for writing", filename);
   }
   else
   {
      walklist(userlist, write_node, fp);
      retval = 0;
   }
    if (ferror (fp))
	error (1, errno, "cannot write %s", filename);
    if (fclose (fp) < 0)
		error (0, errno, "cannot close %s", filename);

   dellist(&userlist);

   xfree (filename);

   perms_cache_dir[0]='\0'; /* Invalidate cache */

   return retval;
}

void
list_owner (dir)
   const char *dir;
{
   char   *filename;
   char   *linebuf = NULL;
   size_t linebuf_len;
   char   *owner;
   FILE   *fp;

   filename = xmalloc (strlen (dir)
		       + strlen (OWNER_FILE)
		       + 1);

   strcpy (filename, dir);
   strcat (filename, OWNER_FILE);

   fp = CVS_FOPEN (filename, "r");
   
   /*
    * Don't make noise about the file not existing as that
    * only means that the owner has not been set
    * M. Ferrell
    */
   if( fp == NULL && existence_error(errno) )
   {
     cvs_output( "Owner: <not set>\n", 0);
   }
   else if (fp == NULL)
   {
      error (0, errno, "cannot open %s", filename);
   }
   else
   {
      if (getline (&linebuf, &linebuf_len, fp) >= 0)
      {
	 cvs_output ("Owner: ", 0);
	 if( strlen(linebuf) > 1 )
	 {
	   owner = cvs_strtok(linebuf, "\n");
	   cvs_output (owner , 0);
	 }
	 else
	   cvs_output ("<not set>", 0);

	 cvs_output ("\n", 0);
	 xfree (linebuf);
      }
      else
	 error (0, 0, "Error reading line in %s", filename);

      if (ferror (fp))
	 error (0, errno, "cannot read %s", filename);
      if (fclose (fp) < 0)
	 error (0, errno, "cannot close %s", filename);
   }

   xfree (filename);
}

void
list_perms (dir)
   const char *dir;
{
   char   *filename;
   char   *linebuf = NULL;
   size_t linebuf_len;
   char   *permptr;
   FILE   *fp;


   filename = xmalloc (strlen (dir)
		       + strlen (PERMS_FILE)
		       + 1);

   strcpy (filename, dir);
   strcat (filename, PERMS_FILE);

   fp = CVS_FOPEN (filename, "r");

   /*
    * Don't complain because the file doesn't exist
    * M. Ferrell
    */
   if (fp == NULL && !existence_error(errno) )
   {
      error (0, errno, "cannot open %s", filename);
   }
   else if ( fp != NULL )
   {
      while (getline (&linebuf, &linebuf_len, fp) >= 0)
      {
	 permptr = cvs_strtok(linebuf, "\n");
	 cvs_output ("  ", 0);
	 if(*permptr=='{')
	 {
#ifdef SJIS
		 char *braptr=_mbschr(permptr,'}');
#else
		 char *braptr=strchr(permptr,'}');
#endif
		 if(braptr)
		 {
			cvs_output(braptr+1, 0);
			cvs_output("  tag:",0);
			cvs_output(permptr+1,(braptr-permptr)-1);
		 }
	 }
	 else
		 cvs_output (permptr, 0);
	 cvs_output ("\n", 0);
	 xfree (linebuf);
	 linebuf = NULL;
      }

      if (ferror (fp))
	 error (0, errno, "cannot read %s", filename);
      if (fclose (fp) < 0)
	 error (0, errno, "cannot close %s", filename);
   }

   xfree (filename);
}
