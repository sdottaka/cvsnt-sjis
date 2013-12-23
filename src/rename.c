/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * Copyright (c) 2004, Tony Hoyle
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * Rename a file
 */

#include "cvs.h"
#include "getline.h"

/* Renames enabled? */
int can_rename = 0;

static const char *const rename_usage[] =
{
    "Usage: %s %s [-q] <source> <target>\n",
	"\t-q\tQuieter output.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

static int quiet;

static int validate_file(const char *filename, char **root, char **repository, const char **file, const char **returned_dir, int must_exist)
{
	char *dir = xmalloc(strlen(filename)+sizeof(CVSADM_REP)+sizeof(CVSADM_ROOT)+1),*p;
	int repos_len;
	FILE *fp;

	strcpy(dir,filename);
	p=(char*)last_component(dir);
	*file=last_component((char*)filename);
	*p='\0';

	strcat(dir,CVSADM);
	if(!isdir(dir))
		error(1,0,"%s is not part of a checked out repository",filename);
	*p='\0';
	strcat(dir,CVSADM_REP);
	if(!isfile(dir) || isdir(dir))
		error(1,0,"%s is not part of a checked out repository",filename);
	fp = fopen(dir,"r");
	if(!fp)
		error(1,errno,"Couldn't open %s", dir);
	*repository=NULL;
	if((repos_len=getline(repository,&repos_len,fp))<0)
		error(1,errno,"Couldn't read %s", dir);
	fclose(fp);
	(*repository)[repos_len-1]='\0';

	*p='\0';
	strcat(dir,CVSADM_ROOT);
	if(!isfile(dir) || isdir(dir))
		error(1,0,"%s is not part of a checked out repository",filename);
	fp = fopen(dir,"r");
	if(!fp)
		error(1,errno,"Couldn't open %s", dir);
	*root=NULL;
	if((repos_len=getline(root,&repos_len,fp))<0)
		error(1,errno,"Couldn't read %s", dir);
	fclose(fp);
	(*root)[repos_len-1]='\0';

	*p='\0';
	if(!strlen(dir))
		strcpy(dir,"./");

	*returned_dir = dir;

	TRACE(3,"%s is in %s/%s",PATCH_NULL(filename),PATCH_NULL(*root),PATCH_NULL(*repository));

	return 0;
}

int cvsrename(int argc, char **argv)
{
    int c;
    int err = 0;
	struct file_info finfo = {0};
	char *repos_file1, *repos_file2;
	char *root1, *root2;
	const char *filename1, *filename2, *dir1, *dir2;
	int rootlen;
	List *ent,*ent2;
	Node *node;
	Entnode *entnode;

	if (argc == -1)
		usage (rename_usage);

	quiet = 0;


    optind = 0;
    while ((c = getopt (argc, argv, "q")) != -1)
    {
	switch (c)
	{
	case 'q':
		quiet = 1;
		break;
    case '?':
    default:
		usage (rename_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

	if(argc!=2)
	{
		usage(rename_usage);
	};

	if(!strcmp(argv[0],argv[1]))
		return 0;

	rootlen = strlen(current_parsed_root->directory);

	if(!isfile(argv[0]))
		error(1,0,"%s does not exist",argv[0]);

	if(isfile(argv[1]) && fncmp(argv[0],argv[1])) /* We allow case renames (on Unix this is redundant) */
		error(1,0,"%s already exists",argv[1]);

	if(isdir(argv[0]))
		error(1,0,"Directory renames are not currently supported");

	if(current_parsed_root->isremote)
	{
		if(!supported_request("Rename"))
			error(1,0,"Remote server does not support rename");
		if(!supported_request("Can-Rename"))
			error(1,0,"Renames are currently disabled");
	}
	else
	{
		if(!can_rename)
			error(1,0,"Renames are currently disabled");
	}

	validate_file(argv[0],&root1, &repos_file1, &filename1, &dir1, 1);
	validate_file(argv[1],&root2, &repos_file2, &filename2, &dir2, 0);

	if(strcmp(root1,root2) || strcmp(root1,current_parsed_root->original))
		error(1,0,"%s and %s are in different repositories",argv[0],argv[1]);

	xfree(root1);
	xfree(root2);

	repos_file1 = xrealloc(repos_file1, strlen(filename1)+strlen(repos_file1)+rootlen+10);
	repos_file2 = xrealloc(repos_file2, strlen(filename2)+strlen(repos_file2)+rootlen+10);
	memmove(repos_file1+rootlen+1,repos_file1,strlen(repos_file1)+1);
	memmove(repos_file2+rootlen+1,repos_file2,strlen(repos_file2)+1);
	strcpy(repos_file1,current_parsed_root->directory);
	strcpy(repos_file2,current_parsed_root->directory);
	repos_file1[rootlen]='/';
	repos_file2[rootlen]='/';
	strcat(repos_file1,"/");
	strcat(repos_file2,"/");
	strcat(repos_file1,filename1);
	strcat(repos_file2,filename2);

	if(fncmp(argv[0],argv[1]))
		set_mapping(dir1,repos_file2+rootlen+1,""); /* Delete old file */
	set_mapping(dir2,repos_file1+rootlen+1,repos_file2+rootlen+1); /* Rename to new file */

	ent = Entries_Open_Dir(0,(char*)dir1, NULL);
	if(fncmp(dir1,dir2))
		ent2 = Entries_Open_Dir(0, (char*)dir2, NULL);
	else
		ent2=ent;
	node = findnode_fn(ent, filename1);
	if(node)
	{	
		entnode=(Entnode*)node->data;
		if(entnode->type==ENT_FILE)
			Register(ent2,(char*)filename2,entnode->version,entnode->timestamp,entnode->options,entnode->tag,entnode->date,entnode->conflict,entnode->merge_from_tag_1,entnode->merge_from_tag_2,entnode->rcs_timestamp);
		else if(entnode->type==ENT_SUBDIR)
			Subdir_Register(ent2,NULL,filename2);
		else
			error(1,0,"Unknown entry type %d in entries file",node->type);
		if(ent!=ent2 || fncmp(filename1,filename2))
			Scratch_Entry(ent,filename1);

		Entries_Close_Dir(ent,dir1);
		if(ent!=ent2)
			Entries_Close_Dir(ent2,dir2);
	}

	CVS_RENAME(argv[0],argv[1]);

	if(isdir(argv[1]))
	{
		char *tmp=xmalloc(strlen(argv[1])+strlen(CVSADM_VIRTREPOS)+10);
		FILE *fp;
		sprintf(tmp,"%s/%s",argv[1],CVSADM_VIRTREPOS);
		fp = fopen(tmp,"w");
		if(!fp)
			error(0,errno,"Couldn't write %s",tmp);
		fprintf(fp,"%s\n",repos_file2+rootlen+1);
		fclose(fp);
	}

	xfree(repos_file1);
	xfree(repos_file2);
	xfree(dir1);
	xfree(dir2);

    return (err);
}
