/* filesubr.c --- subroutines for dealing with files
   Jim Blandy <jimb@cyclic.com>

   This file is part of GNU CVS.

   GNU CVS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

/* These functions were moved out of subr.c because they need different
   definitions under operating systems (like, say, Windows NT) with different
   file system semantics.  */

#include <io.h>
#include <windows.h>

#include "cvs.h"
#include <zlib.h>

extern void _dosmaperr(DWORD dwErr);


static int deep_remove_dir (const char *path);

/* Copies "from" to "to".  Note that the functionality here is similar
   to the win32 function CopyFile, but (1) we copy LastAccessTime and
   CopyFile doesn't, (2) we set file attributes to the default set by
   the C library and CopyFile copies them.  Neither #1 nor #2 was intentional
   as far as I know, but changing them could be confusing, unless there
   is some reason they should be changed (this would need more
   investigation).  */
int copy_file (const char *from, const char *to, int force_overwrite, int must_exist)
{
    struct stat sb;
    struct utimbuf t;
    int fdin, fdout;
#ifdef UTIME_EXPECTS_WRITABLE
    int change_it_back = 0;
#endif

	TRACE(1,"copy(%s,%s)",from,to);
    if (noexec)
		return 0;

    if ((fdin = open (from, O_RDONLY | O_BINARY,0)) < 0)
	{
		if(must_exist)
			error (1, errno, "cannot open %s for copying", from);
		else
			return -1;
	}
    if (fstat (fdin, &sb) < 0)
	{
		if(must_exist)
			error (1, errno, "cannot fstat %s", from);
		else
		{
			close(fdin);
			return -1;
		}
	}
	if (force_overwrite && unlink_file (to) && !existence_error (errno))
	    error (1, errno, "unable to remove %s", to);
		
    if ((fdout = open (to, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0600)) < 0)
		error (1, errno, "cannot create %s for copying", to);
    if (sb.st_size > 0)
    {
	char buf[BUFSIZ];
	int n;

	for (;;) 
	{
	    n = read (fdin, buf, sizeof(buf));
	    if (n == -1)
	    {
#ifdef EINTR
		if (errno == EINTR)
		    continue;
#endif
		error (1, errno, "cannot read file %s for copying", from);
	    }
            else if (n == 0) 
		break;
  
	    if (write(fdout, buf, n) != n) {
		error (1, errno, "cannot write file %s for copying", to);
	    }
	}

#ifdef HAVE_FSYNC
	if (fsync (fdout)) 
	    error (1, errno, "cannot fsync file %s after copying", to);
#endif
    }

    if (close (fdin) < 0) 
	error (0, errno, "cannot close %s", from);
    if (close (fdout) < 0)
	error (1, errno, "cannot close %s", to);

#ifdef UTIME_EXPECTS_WRITABLE
	if (!iswritable (to))
	{
		xchmod (to, 1);
		change_it_back = 1;
	}
#endif  /* UTIME_EXPECTS_WRITABLE  */
    /* now, set the times for the copied file to match those of the original */
    memset ((char *) &t, 0, sizeof (t));
    t.actime = sb.st_atime;
    t.modtime = sb.st_mtime;
    utime (to, &t);
	chmod(to,sb.st_mode);
#ifdef UTIME_EXPECTS_WRITABLE
	if (change_it_back)
		xchmod (to, 0);
#endif  /*  UTIME_EXPECTS_WRITABLE  */
	return 0;
}

/*
 * Copies "from" to "to", compressing "to" on the fly
 */
int copy_and_zip_file (const char *from, const char *to, int force_overwrite, int must_exist)
{
    struct stat sb;
    struct utimbuf t;
    int fdin, fdout;
    z_stream zstr = {0};
    int zstatus;
    char buf[BUFSIZ*10];
    char zbuf[BUFSIZ*20];
    int n,zlen;
#ifdef UTIME_EXPECTS_WRITABLE
    int change_it_back = 0;
#endif

	TRACE(1,"copy_and_zip(%s,%s)",from,to);
    if (noexec)
		return 0;

    if ((fdin = open (from, O_RDONLY | O_BINARY,0)) < 0)
	{
		if(must_exist)
			error (1, errno, "cannot open %s for copying", from);
		else
			return -1;
	}
	if (fstat (fdin, &sb) < 0)
	{
		if(must_exist)
			error (1, errno, "cannot fstat %s", from);
		else
		{
			close(fdin);
			return -1;
		}
	}
	if (force_overwrite && unlink_file (to) && !existence_error (errno))
	    error (1, errno, "unable to remove %s", to);

    if ((fdout = open (to, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0600)) < 0)
	    error (1, errno, "cannot create %s for copying", to);

	zstatus = deflateInit2 (&zstr, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
	if(zstatus != Z_OK)
	   error(1, 0, "compression error (INIT): (%d)%s", zstatus,zstr.msg);
     
	if (sb.st_size > 0)
	{
	    for (;;) 
	    {
		n = read (fdin, buf, sizeof(buf));
		if (n == -1)
		{
#ifdef EINTR
		    if (errno == EINTR)
			continue;
#endif
		    error (1, errno, "cannot read file %s for copying", from);
		}
		else if (n == 0) 
		    break;

		zlen = deflateBound(&zstr,n);
		if(zlen>sizeof(zbuf))
			error(1,0,"compression error: zlen=%d (> %d)",zlen,sizeof(zbuf));
		zstr.next_in = buf;
		zstr.avail_in = n;	
		zstr.next_out = zbuf;
		zstr.avail_out = zlen;
		zstatus = deflate (&zstr, Z_SYNC_FLUSH);
		if(zstatus != Z_OK)	
		  error(1,0, "compression error (deflate): (%d)%s", zstatus,zstr.msg);
		
		n = zlen-zstr.avail_out;	
		if (n && write(fdout, zbuf, n) != n) {
		    error (1, errno, "cannot write file %s for copying", to);
		}
	    }

#ifdef HAVE_FSYNC
	    if (fsync (fdout)) 
		error (1, errno, "cannot fsync file %s after copying", to);
#endif
	
	}

	zstr.next_in = buf;
	zstr.avail_in = 0;
	zstr.next_out = zbuf;
	zstr.avail_out = sizeof(zbuf);
	zstatus = deflate (&zstr, Z_FINISH);
	if(zstatus != Z_OK && zstatus != Z_STREAM_END)	
	      error(1,0, "compression error (Z_FINISH): (%d)%s", zstatus,zstr.msg);
	n = sizeof(zbuf)-zstr.avail_out;	
	if (n && write(fdout, zbuf, n) != n) {
	   error (1, errno, "cannot write file %s for copying", to);
	}

	zstr.next_in = buf;
	zstr.avail_in = 0;
	zstr.next_out = zbuf;
	zstr.avail_out = sizeof(zbuf);
	zstatus = deflateEnd(&zstr);
	if(zstatus != Z_OK)
	  error(1,0, "compression error (deflateEnd): (%d) %s", zstatus, zstr.msg);

	if (close (fdin) < 0) 
	    error (0, errno, "cannot close %s", from);
	if (close (fdout) < 0)
	    error (1, errno, "cannot close %s", to);

#ifdef UTIME_EXPECTS_WRITABLE
	if (!iswritable (to))
	{
		xchmod (to, 1);
		change_it_back = 1;
	}
#endif  /* UTIME_EXPECTS_WRITABLE  */
    /* now, set the times for the copied file to match those of the original */
    memset ((char *) &t, 0, sizeof (t));
    t.actime = sb.st_atime;
    t.modtime = sb.st_mtime;
    utime (to, &t);
	chmod(to,sb.st_mode);
#ifdef UTIME_EXPECTS_WRITABLE
	if (change_it_back)
		xchmod (to, 0);
#endif  /*  UTIME_EXPECTS_WRITABLE  */
	return 0;
}

/*
 * Copies "from" to "to", decompressing "from" on the fly
 */
int copy_and_unzip_file (const char *from, const char *to, int force_overwrite, int must_exist)
{
    struct stat sb;
    struct utimbuf t;
    int fdin, fdout;
    z_stream zstr = {0};
    int zstatus;
    char buf[BUFSIZ*10];
    char zbuf[BUFSIZ*20];
    int n;
#ifdef UTIME_EXPECTS_WRITABLE
    int change_it_back = 0;
#endif

	TRACE(1,"copy_and_unzip(%s,%s)",from,to);
    if (noexec)
		return 0;

    if ((fdin = open (from, O_RDONLY | O_BINARY,0)) < 0)
	{
		if(must_exist)
			error (1, errno, "cannot open %s for copying", from);
		else
			return -1;
	}
	if (fstat (fdin, &sb) < 0)
	{
		if(must_exist)
			error (1, errno, "cannot fstat %s", from);
		else
		{
			close(fdin);
			return -1;
		}
	}
	if (force_overwrite && unlink_file (to) && !existence_error (errno))
	    error (1, errno, "unable to remove %s", to);

    if ((fdout = open (to, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0600))<0)
	    error (1, errno, "cannot create %s for copying", to);

	zstatus = inflateInit2 (&zstr, 47);
	if(zstatus != Z_OK)
	   error(1, 0, "expansion error (INIT): (%d)%s", zstatus,zstr.msg);
     
	if (sb.st_size > 0)
	{
	    for (;;) 
	    {
		n = read (fdin, buf, sizeof(buf));
		if (n == -1)
		{
#ifdef EINTR
		    if (errno == EINTR)
			continue;
#endif
		    error (1, errno, "cannot read file %s for copying", from);
		}
		else if (n == 0) 
		    break;

		zstr.next_in = buf;
		zstr.avail_in = n;	
		while(zstr.avail_in)
		{
			zstr.next_out = zbuf;
			zstr.avail_out = sizeof(zbuf);
			zstatus = inflate (&zstr, 0);
			if(zstatus != Z_OK && zstatus != Z_STREAM_END)	
			error(1,0, "expansion error (inflate): (%d)%s", zstatus,zstr.msg);
			
			n = sizeof(zbuf)-zstr.avail_out;	
			if (n && write(fdout, zbuf, n) != n) {
				error (1, errno, "cannot write file %s for copying", to);
			}
		}
		if(zstatus == Z_STREAM_END)
			break;
		}

#ifdef HAVE_FSYNC
	    if (fsync (fdout)) 
		error (1, errno, "cannot fsync file %s after copying", to);
#endif
	
	}

	zstr.next_out = zbuf;
	zstr.avail_out = sizeof(zbuf);
	zstatus = inflate (&zstr, Z_FINISH);
	if(zstatus != Z_OK && zstatus != Z_STREAM_END)	
	      error(1,0, "expansion error (Z_FINISH): (%d)%s", zstatus,zstr.msg);
	n = sizeof(zbuf)-zstr.avail_out;	
	if (n && write(fdout, zbuf, n) != n) {
	   error (1, errno, "cannot write file %s for copying", to);
	}
	
	zstr.next_in = buf;
	zstr.avail_in = 0;
	zstr.next_out = zbuf;
	zstr.avail_out = sizeof(zbuf);
	zstatus = inflateEnd(&zstr);
	if(zstatus != Z_OK)
	  error(1,0, "expansion error: %s", zstr.msg);

	if (close (fdin) < 0) 
	    error (0, errno, "cannot close %s", from);
	if (close (fdout) < 0)
	    error (1, errno, "cannot close %s", to);

#ifdef UTIME_EXPECTS_WRITABLE
	if (!iswritable (to))
	{
		xchmod (to, 1);
		change_it_back = 1;
	}
#endif  /* UTIME_EXPECTS_WRITABLE  */
    /* now, set the times for the copied file to match those of the original */
    memset ((char *) &t, 0, sizeof (t));
    t.actime = sb.st_atime;
    t.modtime = sb.st_mtime;
    utime (to, &t);
	chmod(to,sb.st_mode);
#ifdef UTIME_EXPECTS_WRITABLE
	if (change_it_back)
		xchmod (to, 0);
#endif  /*  UTIME_EXPECTS_WRITABLE  */
	return 0;
}

/* FIXME-krp: these functions would benefit from caching the char * &
   stat buf.  */

char *repository_name(struct file_info *file)
{
	char *fn = xmalloc(strlen(file->repository) + strlen(file->file) + 10);

	sprintf(fn,"%s/%s",file->repository,file->file);
	return fn;
}

/*
 * Returns non-zero if the argument file is a directory, or is a symbolic
 * link which points to a directory.
 */
int
isdir (file)
    const char *file;
{
    struct stat sb;

    if (stat (file, &sb) < 0)
	return (0);
    return (S_ISDIR (sb.st_mode));
}

/*
 * Returns non-zero if the argument file is a directory, or is a symbolic
 * link which points to a directory.
 */
int
isdir_f (struct file_info *file)
{
    struct stat sb;
	char *fn = repository_name(file);

    if (stat (fn, &sb) < 0)
	{
		free(fn);
		return (0);
	}
	free(fn);
    return (S_ISDIR (sb.st_mode));
}

/*
 * Returns non-zero if the argument file is a symbolic link.
 */
int
islink (file)
    const char *file;
{
#ifdef S_ISLNK
    struct stat sb;

    if (lstat (file, &sb) < 0)
	return (0);
    return (S_ISLNK (sb.st_mode));
#else
    return (0);
#endif
}

/*
 * Returns non-zero if the argument file exists.
 */
int
isfile (file)
    const char *file;
{
    return isaccessible(file, F_OK);
}

/*
 * Returns non-zero if the argument file is readable.
 */
int
isreadable (file)
    const char *file;
{
    return isaccessible(file, R_OK);
}

/*
 * Returns non-zero if the argument file is writable.
 */
int
iswritable (file)
    const char *file;
{
    return isaccessible(file, W_OK);
}

/*
 * Returns non-zero if the argument file is accessable according to
 * mode.  If compiled with SETXID_SUPPORT also works if cvs has setxid
 * bits set.
 */
int
isaccessible (file, mode)
    const char *file;
    const int mode;
{
#ifdef SETXID_SUPPORT
    struct stat sb;
    int umask = 0;
    int gmask = 0;
    int omask = 0;
    int uid;
    
    if (stat(file, &sb) == -1)
	return 0;
    if (mode == F_OK)
	return 1;

    uid = geteuid();
    if (uid == 0)		/* superuser */
    {
	if (mode & X_OK)
	    return sb.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH);
	else
	    return 1;
    }
	
    if (mode & R_OK)
    {
	umask |= S_IRUSR;
	gmask |= S_IRGRP;
	omask |= S_IROTH;
    }
    if (mode & W_OK)
    {
	umask |= S_IWUSR;
	gmask |= S_IWGRP;
	omask |= S_IWOTH;
    }
    if (mode & X_OK)
    {
	umask |= S_IXUSR;
	gmask |= S_IXGRP;
	omask |= S_IXOTH;
    }

    if (sb.st_uid == uid)
	return (sb.st_mode & umask) == umask;
    else if (sb.st_gid == getegid())
	return (sb.st_mode & gmask) == gmask;
    else
	return (sb.st_mode & omask) == omask;
#else
    return access(file, mode) == 0;
#endif
}

/*
 * Open a file and die if it fails
 */
FILE *
open_file (name, mode)
    const char *name;
    const char *mode;
{
    FILE *fp;

    if ((fp = fopen (name, mode)) == NULL)
	error (1, errno, "cannot open %s", name);
    return (fp);
}

/*
 * Make a directory and die if it fails
 */
void
make_directory (name)
    const char *name;
{
    struct stat sb;

    if (stat (name, &sb) == 0 && (!S_ISDIR (sb.st_mode)))
	    error (0, 0, "%s already exists but is not a directory", fn_root(name));
    if (!noexec && mkdir (name) < 0)
	error (1, errno, "cannot make directory %s", fn_root(name));
}

/*
 * Make a path to the argument directory, printing a message if something
 * goes wrong.
 */
void make_directories (char *name)
{
    char *cp;

    if (noexec)
		return;

    if (mkdir (name) == 0 || errno == EEXIST)
		return;
    if (errno != ENOENT)
    {
		error (0, errno, "cannot make path to %s", fn_root(name));
		return;
    }
	for(cp=name+strlen(name);cp>name && !ISDIRSEP(*cp); --cp)
		;
	if(cp==name)
		return;
    *cp = '\0';
    make_directories (name);
    *cp++ = '/';
    if (*cp == '\0')
		return;
    mkdir (name);
}

/* Create directory NAME if it does not already exist; fatal error for
   other errors.  Returns 0 if directory was created; 1 if it already
   existed.  */
int
mkdir_if_needed (char *name)
{
    if (mkdir (name) < 0)
    {
	if (errno != EEXIST
#ifdef EACCESS
	    /* This was copied over from the OS/2 code; I would guess it
	       isn't needed here but that has not been verified.  */
	    && errno != EACCESS
#endif
#ifdef EACCES
	    /* This is said to be needed by NT on Alpha or PowerPC
	       (not sure what version) --August, 1996.  */
	    && errno != EACCES
#endif
	    )
	    error (1, errno, "cannot make directory %s", fn_root(name));
	return 1;
    }
    return 0;
}

/*
 * Change the mode of a file, either adding write permissions, or removing
 * all write permissions.  Adding write permissions honors the current umask
 * setting.
 */
void xchmod (char *fname, int writable)
{
	struct stat sb;
    mode_t mode, oumask;

    if (stat (fname, &sb) < 0)
    {
	if (!noexec)
	    error (0, errno, "cannot stat %s", fn_root(fname));
	return;
    }
    oumask = 022; /* NT Default umask */
    (void) umask (oumask);
    if (writable)
    {
		mode = sb.st_mode | (~oumask
					& (((sb.st_mode & S_IRUSR) ? S_IWUSR : 0)
					| ((sb.st_mode & S_IRGRP) ? S_IWGRP : 0)
					| ((sb.st_mode & S_IROTH) ? S_IWOTH : 0)));
    }
    else
    {
		mode = sb.st_mode & ~(S_IWRITE | S_IWGRP | S_IWOTH) & ~oumask;
    }

	TRACE(1,"chmod(%s,%o)",PATCH_NULL(fname),mode);
    if (noexec)
		return;

    if (chmod (fname, mode) < 0)
		error (0, errno, "cannot change mode of file %s", fn_root(fname));
}


/* Read the value of a symbolic link.
   Under Windows NT, this function always returns EINVAL.  */
int
readlink (char *path, char *buf, int buf_size)
{
    errno = EINVAL;
    return -1;
}

/* Rename for NT which works for read only files.  Apparently if we are
   accessing FROM and TO via a Novell network, this is an issue.  */
int wnt_rename (const char *from, const char *to)
{
    int result, save_errno;
    int readonly = !iswritable (from);

	if(!strcmp(from,to))
		return 0;

	if(isfile(to))
		xchmod(to,1);

    if(readonly)
		xchmod (from,1);

#ifdef CVS95
	/* Win95 doesn't support atomic rename over existing files */
	DeleteFile(to);
	result = MoveFile(from,to);
    save_errno = GetLastError();
#else
	result = MoveFileExA(from,to,MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    save_errno = GetLastError();
#endif
    if (readonly)
    {
		if (result)
			xchmod (to, 0);
		else
			/* We have a choice of which error to report, if there is
			one here too; report the one from rename ().  */
			xchmod (from, 0);
    }
	if(!result)
	{
		_dosmaperr(save_errno);
	}
    return result?0:-1;
}

/*
 * Rename a file and die if it fails
 */
void
rename_file (from, to)
    const char *from;
    const char *to;
{
	TRACE(1,"rename(%s,%s)",from,to);

    if (noexec)
		return;

    if (CVS_RENAME (from, to) < 0)
		error (1, errno, "cannot rename file %s to %s", fn_root(from), fn_root(to));
}

/*
 * unlink a file, if possible.
 */
int
unlink_file (f)
    const char *f;
{
	TRACE(1,"unlink(%s)",f);
    if (noexec)
		return (0);

    /* Win32 unlink is stupid - it fails if the file is read-only */
    chmod (f, _S_IWRITE);
    return (unlink (f));
}

/*
 * Unlink a file or dir, if possible.  If it is a directory do a deep
 * removal of all of the files in the directory.  Return -1 on error
 * (in which case errno is set).
 */
int
unlink_file_dir (f)
    const char *f;
{
	TRACE(1,"unlink_file_dir(%s)",f);
    if (noexec)
		return (0);

    /* Win32 unlink is stupid - it fails if the file is read-only */
    chmod (f, _S_IWRITE);
    if (unlink (f) != 0)
    {
	/* under Windows NT, unlink returns EACCES if the path
	   is a directory.  Under Windows 95, ENOENT.  */
        if (errno == EISDIR || errno == EACCES || errno == ENOENT || errno == ENOTEMPTY)
                return deep_remove_dir (f);
        else
		/* The file wasn't a directory and some other
		 * error occured
		 */
                return -1;
    }
    /* We were able to remove the file from the disk */
    return 0;
}

/* Remove a directory and everything it contains.  Returns 0 for
 * success, -1 for failure (in which case errno is set).
 */

static int
deep_remove_dir (path)
    const char *path;
{
    DIR		  *dirp;
    struct dirent *dp;
    char	   buf[PATH_MAX];

    /* ENOTEMPTY for NT (obvious) but EACCES for Win95 (not obvious) */
    if (rmdir (path) != 0
	&& (errno == ENOTEMPTY || errno == EACCES))
    {
	if ((dirp = opendir (path)) == NULL)
	    /* If unable to open the directory return
	     * an error
	     */
	    return -1;

	while ((dp = readdir (dirp)) != NULL)
	{
	    if (strcmp (dp->d_name, ".") == 0 ||
			strcmp (dp->d_name, "..") == 0)
		continue;

	    sprintf (buf, "%s/%s", path, dp->d_name);

	    /* Win32 unlink is stupid - it fails if the file is read-only */
	    chmod (buf, _S_IWRITE);
	    if (unlink (buf) != 0 )
	    {
		/* Under Windows NT, unlink returns EACCES if the path
		   is a directory.  Under Windows 95, ENOENT.  It
		   isn't really clear to me whether checking errno is
		   better or worse than using _stat to check for a directory.
		   We aren't really trying to prevent race conditions here
		   (e.g. what if something changes between readdir and
		   unlink?)  */
		if (errno == EISDIR || errno == EACCES || errno == ENOENT || errno == ENOTEMPTY)
		{
		    if (deep_remove_dir (buf))
		    {
			closedir (dirp);
			return -1;
		    }
		}
		else
		{
		    /* buf isn't a directory, or there are
		     * some sort of permision problems
		     */
		    closedir (dirp);
		    return -1;
		}
	    }
	}
	closedir (dirp);
	return rmdir (path);
    }
    /* Was able to remove the directory return 0 */
    return 0;
}

/* Read NCHARS bytes from descriptor FD into BUF.
   Return the number of characters successfully read.
   The number returned is always NCHARS unless end-of-file or error.  */
static size_t
block_read (fd, buf, nchars)
    int fd;
    char *buf;
    size_t nchars;
{
    char *bp = buf;
    size_t nread;

    do 
    {
	nread = read (fd, bp, nchars);
	if (nread == (size_t)-1)
	{
#ifdef EINTR
	    if (errno == EINTR)
		continue;
#endif
	    return (size_t)-1;
	}

	if (nread == 0)
	    break; 

	bp += nread;
	nchars -= nread;
    } while (nchars != 0);

    return bp - buf;
} 

    
/*
 * Compare "file1" to "file2". Return non-zero if they don't compare exactly.
 */
int
xcmp (file1, file2)
    const char *file1;
    const char *file2;
{
    char *buf1, *buf2;
    struct stat sb1, sb2;
    int fd1, fd2;
    int ret;

    if ((fd1 = open (file1, O_RDONLY | O_BINARY,0)) < 0)
	error (1, errno, "cannot open file %s for comparing", fn_root(file1));
    if ((fd2 = open (file2, O_RDONLY | O_BINARY,0)) < 0)
	error (1, errno, "cannot open file %s for comparing", fn_root(file2));
    if (fstat (fd1, &sb1) < 0)
	error (1, errno, "cannot fstat %s", fn_root(file1));
    if (fstat (fd2, &sb2) < 0)
	error (1, errno, "cannot fstat %s", fn_root(file2));

    /* A generic file compare routine might compare st_dev & st_ino here 
       to see if the two files being compared are actually the same file.
       But that won't happen in CVS, so we won't bother. */

    if (sb1.st_size != sb2.st_size)
	ret = 1;
    else if (sb1.st_size == 0)
	ret = 0;
    else
    {
	/* FIXME: compute the optimal buffer size by computing the least
	   common multiple of the files st_blocks field */
	size_t buf_size = 8 * 1024;
	size_t read1;
	size_t read2;

	buf1 = xmalloc (buf_size);
	buf2 = xmalloc (buf_size);

	do 
	{
	    read1 = block_read (fd1, buf1, buf_size);
	    if (read1 == (size_t)-1)
		error (1, errno, "cannot read file %s for comparing", fn_root(file1));

	    read2 = block_read (fd2, buf2, buf_size);
	    if (read2 == (size_t)-1)
		error (1, errno, "cannot read file %s for comparing", fn_root(file2));

	    /* assert (read1 == read2); */

	    ret = memcmp(buf1, buf2, read1);
	} while (ret == 0 && read1 == buf_size);

	free (buf1);
	free (buf2);
    }
	
    (void) close (fd1);
    (void) close (fd2);
    return (ret);
}


/* Return non-zero iff FILENAME is absolute.
   Trivial under Unix, but more complicated under other systems.  */
int
isabsolute (const char *filename)
{
    /* FIXME: This routine seems to interact poorly with
       strip_trailing_slashes.  For example, specify ":local:r:\" as
       CVSROOT.  The CVS/Root file will contain ":local:r:" and then
       isabsolute will complain about the root not being an absolute
       pathname.  My guess is that strip_trailing_slashes is the right
       place to fix this.  */
    return (ISDIRSEP (filename[0])
            || (filename[0] != '\0'
                && filename[1] == ':'
                && ISDIRSEP (filename[2])));
}

int
isabsolute_remote(const char *filename)
{
	return isabsolute(filename);
}

/* Return a pointer into PATH's last component.  */
const char *last_component (const char *path)
{
    const char *scan;
    const char *last = 0;

    for (scan = path; *scan; scan++)
#ifdef SJIS
    {
	if (_ismbblead(*scan)) {
	    scan++;
	    continue;
	}
#endif
        if (ISDIRSEP (*scan))
	    last = scan;
#ifdef SJIS
    }
#endif

    if (last && (last != path))
        return last + 1;
    else
        return path;
}


/* NT has two evironment variables, HOMEPATH and HOMEDRIVE, which,
   when combined as ${HOMEDRIVE}${HOMEPATH}, give the unix equivalent
   of HOME.  Some NT users are just too unixy, though, and set the
   HOME variable themselves.  Therefore, we check for HOME first, and
   then try to combine the other two if that fails.

   Looking for HOME strikes me as bogus, particularly if the only reason
   is to cater to "unixy users".  On the other hand, if the reasoning is
   there should be a single variable, rather than requiring people to
   set both HOMEDRIVE and HOMEPATH, then it starts to make a little more
   sense.

   Win95: The system doesn't set HOME, HOMEDRIVE, or HOMEPATH (at
   least if you set it up as the "all users under one user ID" or
   whatever the name of that option is).  Based on thing overheard on
   the net, it seems that users of the pserver client have gotten in
   the habit of setting HOME (if you don't use pserver, you can
   probably get away without having a reasonable return from
   get_homedir.  Of course you lose .cvsrc and .cvsignore, but many
   users won't notice).  So it would seem that we should be somewhat
   careful if we try to change the current behavior.

   NT 3.51 or NT 4.0: I haven't checked this myself, but I am told
   that HOME gets set, but not to the user's home directory.  It is
   said to be set to c:\users\default by default. 

   Win2k:  Apparently this gets set to c:\ by default (ugh).
 */

char *stripslash(char *path)
{
	char *p=path+strlen(path)-1;
	if(!*path)
		return path; // null string, just in case
#ifdef SJIS
	if(_ismbstrail(path, p))
		return path;
#endif
	if(*p=='\\' || *p=='/')
		*p='\0';
	return path;
}

char *
get_homedir ()
{
    static char *pathbuf;
    char *hd, *hp;

    if (pathbuf != NULL)
		return stripslash(pathbuf);
    else if ((hd = getenv ("HOME")))
		return stripslash(hd);
    else if ((hd = getenv ("HOMEDRIVE")) && (hp = getenv ("HOMEPATH")))
    {
		pathbuf = xmalloc (strlen (hd) + strlen (hp) + 5);
		strcpy (pathbuf, hd);
		strcat (pathbuf, hp);

		return stripslash(pathbuf);
    }
    else
		return NULL;
}

/* See cvs.h for description.  */
void
expand_wild (argc, argv, pargc, pargv)
    int argc;
    char **argv;
    int *pargc;
    char ***pargv;
{
    int i;
    int new_argc;
    char **new_argv;
    /* Allocated size of new_argv.  We arrange it so there is always room for
	   one more element.  */
    int max_new_argc;
    int ugly_dot_hack;

    new_argc = 0;
    /* Add one so this is never zero.  */
    max_new_argc = argc + 1;
    new_argv = (char **) xmalloc (max_new_argc * sizeof (char *));
    for (i = 0; i < argc; ++i)
    {
	HANDLE h;
	WIN32_FIND_DATAA fdata;

	/* These variables help us extract the directory name from the
           given pathname. */

	char *last_forw_slash, *last_back_slash, *end_of_dirname;
	int dirname_length = 0;

	/* FindFirstFile doesn't return pathnames, so we have to do
	   this ourselves.  Luckily, it's no big deal, since globbing
	   characters under Win32s can only occur in the last segment
	   of the path.  For example,
                /a/path/q*.h                      valid
	        /w32/q*.dir/cant/do/this/q*.h     invalid */

	/* Win32 can handle both forward and backward slashes as
           filenames -- check for both. */
	     
	last_forw_slash = strrchr (argv[i], '/');
#ifdef SJIS
	last_back_slash = _mbsrchr (argv[i], '\\');
#else
	last_back_slash = strrchr (argv[i], '\\');
#endif

#define cvs_max(x,y) ((x >= y) ? (x) : (y))

	/* FIXME: this comparing a NULL pointer to a non-NULL one is
	   extremely ugly, and I strongly suspect *NOT* sanctioned by
	   ANSI C.  The code should just use last_component instead.  */
	end_of_dirname = cvs_max (last_forw_slash, last_back_slash);

	if (end_of_dirname == NULL)
	  dirname_length = 0;	/* no directory name */
	else
	  dirname_length = end_of_dirname - argv[i] + 1; /* include slash */

	/* This is an attempt to get around the dotting problem */
	if(!stricmp(argv[i],".") || !stricmp(argv[i],"..")) ugly_dot_hack = 1;
	else ugly_dot_hack = 0;

	h = FindFirstFileA (argv[i], &fdata);
	if (h == INVALID_HANDLE_VALUE)
	{
//	    if (GetLastError () == ENOENT)
//	    {
		/* No match.  The file specified didn't contain a wildcard (in which case
		   we clearly should return it unchanged), or it contained a wildcard which
		   didn't match (in which case it might be better for it to be an error,
		   but we don't try to do that).  */
		new_argv [new_argc++] = xstrdup (argv[i]);
		if (new_argc == max_new_argc)
		{
		    max_new_argc *= 2;
		    new_argv = xrealloc (new_argv, max_new_argc * sizeof (char *));
		}
//	    }
//	    else
//	    {
//			_dosmaperr(GetLastError());
//			error (1, errno, "cannot find %s", argv[i]);
//	    }
	}
	else
	{
	    while (1)
	    {
			if(!strcmp(fdata.cFileName,".") || !strcmp(fdata.cFileName,".."))
				goto skip_dot;
		new_argv[new_argc] =
		    (char *) xmalloc (strlen (fdata.cFileName) + 1
				      + dirname_length);

		/* Copy the directory name, if there is one. */

		if (dirname_length)
		{
		    strncpy (new_argv[new_argc], argv[i], dirname_length);
		    new_argv[new_argc][dirname_length] = '\0';
		}
		else
		    new_argv[new_argc][0] = '\0';

		/* Copy the file name. */
		
			if (ugly_dot_hack ||
				(fncmp (argv[i] + dirname_length, fdata.cFileName) == 0))
		    /* We didn't expand a wildcard; we just matched a filename.
		       Use the file name as specified rather than the filename
		       which exists in the directory (they may differ in case).
		       This is needed to make cvs add on a directory consistently
		       use the name specified on the command line, but it is
		       probably a good idea in other contexts too.  */
		    strcpy (new_argv[new_argc], argv[i]);
		else
		    strcat (new_argv[new_argc], fdata.cFileName);

		new_argc++;

		if (new_argc == max_new_argc)
		{
		    max_new_argc *= 2;
		    new_argv = xrealloc (new_argv, max_new_argc * sizeof (char *));
		}
skip_dot:
		if (!FindNextFileA (h, &fdata))
		{
		    if (GetLastError () == ERROR_NO_MORE_FILES)
			break;
		    else
			error (1, errno, "cannot find %s", argv[i]);
		}
	    }
	    if (!FindClose (h))
		error (1, GetLastError (), "cannot close %s", argv[i]);
	}
    }
    *pargc = new_argc;
    *pargv = new_argv;
}

/* Generate a unique temporary filename.  Returns a pointer to a newly
 * malloc'd string containing the name.  Returns successfully or not at
 * all.
 *
 *     THIS FUNCTION IS DEPRECATED!!!  USE cvs_temp_file INSTEAD!!!
 *
 * and yes, I know about the way the rcs commands use temp files.  I think
 * they should be converted too but I don't have time to look into it right
 * now.
 */
char *
cvs_temp_name ()
{
    char *fn;
    FILE *fp;

    fp = cvs_temp_file (&fn);
    if (fp == NULL)
	error (1, errno, "Failed to create temporary file");
    if (fclose (fp) == EOF)
	error (0, errno, "Failed to close temporary file %s", fn);
    return fn;
}

/* Generate a unique temporary filename and return an open file stream
 * to the truncated file by that name
 *
 *  INPUTS
 *	filename	where to place the pointer to the newly allocated file
 *   			name string
 *
 *  OUTPUTS
 *	filename	dereferenced, will point to the newly allocated file
 *			name string.  This value is undefined if the function
 *			returns an error.
 *
 *  RETURNS
 *	An open file pointer to a read/write mode empty temporary file with the
 *	unique file name or NULL on failure.
 *
 *  ERRORS
 *	on error, errno will be set to some value either by CVS_FOPEN or
 *	whatever system function is called to generate the temporary file name
 */
/* There are at least four functions for generating temporary
 * filenames.  We use mkstemp (BSD 4.3) if possible, else tempnam (SVID 3),
 * else mktemp (BSD 4.3), and as last resort tmpnam (POSIX).  Reason is that
 * mkstemp, tempnam, and mktemp both allow to specify the directory in which
 * the temporary file will be created.
 *
 * And the _correct_ way to use the deprecated functions probably involves
 * opening file descriptors using O_EXCL & O_CREAT and even doing the annoying
 * NFS locking thing, but until I hear of more problems, I'm not going to
 * bother.
 */
FILE *cvs_temp_file (filename)
    char **filename;
{
	char tempdir[MAX_PATH];
	DWORD fa;
	FILE *f;

	if(!(GetEnvironmentVariableA("TEMP",tempdir,sizeof(tempdir)) ||
	     GetEnvironmentVariableA("TMP",tempdir,sizeof(tempdir))))
	{
		// No TEMP or TMP, use default <windir>\TEMP
		GetWindowsDirectoryA(tempdir,sizeof(tempdir));
		strcat(tempdir,"\\TEMP");
	}
	if((fa=GetFileAttributesA(tempdir))==0xFFFFFFFF || !(fa&FILE_ATTRIBUTE_DIRECTORY))
	{
		// Last resort, can't find a valid temp.... use C:\...
		strcpy(tempdir,"C:\\");
	}
	*filename=xmalloc(MAX_PATH);
	GetTempFileNameA(tempdir,"cvs",0,*filename);
	f=fopen(*filename,"wb+");
	return f;
}

void win32ize_root(char *arg)
{
	char *p;

	if(ISDIRSEP(arg[0]) && ISDIRSEP(arg[2]) && ISDIRSEP(arg[3]))
	{
		strcpy(arg,arg+1);
		arg[1]=':';
	}
	for(p=arg; *p; p++)
#ifdef SJIS
		if(*p=='\\' && !_ismbstrail(arg, p))
#else
		if(*p=='\\')
#endif
			*p='/';
}

/* On case insensitive systems, find the real case */
/* On case sensitive ones, just return */
char *normalize_path(char *arg)
{
	if(!arg)
		return NULL;

	TRACE(3,"normalize_path(%s)",arg);
	win32ize_root(arg);
	arg = xrealloc(arg,MAX_PATH*2);
	ConvertFilespecToCorrectCase(arg);
	arg = xrealloc(arg, strlen(arg)+1);
	TRACE(3,"...returns %s",arg);

	return arg;
}
