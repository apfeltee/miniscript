#pragma once

/*
* this file contains platform-specific functions.
* unless target platform is unix-like, they are all non-functional stubs, that default to
* returning an error value.
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>


#if defined(__unix__) || defined(__linux__)
    #define OSFN_ISUNIXY 1
#endif

#define OSFN_PATHSIZE 1024
#if defined(__unix__) || defined(__linux__)
    #define OSFN_ISUNIX
#elif defined(_WIN32) || defined(_WIN64)
    #define OSFN_ISWINDOWS
#endif

#if defined(OSFN_ISUNIX)
    #include <unistd.h>
    #include <dirent.h>
    #include <libgen.h>
    #include <sys/time.h>
#else
    #if defined(OSFN_ISWINDOWS)
        #include <windows.h>
        #include <windef.h>
        #include <winbase.h>
        #include <io.h>
    #endif
#endif

#include "mem.h"

#if !defined(S_IFMT)
    #define S_IFMT  00170000
#endif
#if !defined(S_IFSOCK)
    #define S_IFSOCK 0140000
#endif
#if !defined(S_IFLNK)
    #define S_IFLNK  0120000
#endif
#if !defined(S_IFREG)
    #define S_IFREG  0100000
#endif
#if !defined(S_IFBLK)
    #define S_IFBLK  0060000
#endif
#if !defined(S_IFDIR)
    #define S_IFDIR  0040000
#endif
#if !defined(S_IFCHR)
    #define S_IFCHR  0020000
#endif
#if !defined(S_IFIFO)
    #define S_IFIFO  0010000
#endif

#if !defined(DT_DIR)
    #define DT_DIR 4
#endif

#if !defined(DT_REG)
    #define DT_REG 8
#endif

#ifndef S_IREAD
    #define S_IREAD     0400
#endif /* S_IREAD */
#ifndef S_IWRITE
    #define S_IWRITE    0200
#endif /* S_IWRITE */
#ifndef S_IEXEC
    #define S_IEXEC     0100
#endif /* S_IEXEC */


#if !defined(S_IRUSR)
    #define S_IRUSR (S_IREAD)
#endif
#if !defined(S_IWUSR)
    #define S_IWUSR (S_IWRITE)
#endif
#if !defined(S_IXUSR)
    #define S_IXUSR (S_IEXEC)
#endif
#if !defined(S_IRGRP)
    #define S_IRGRP (S_IRUSR >> 3)
#endif
#if !defined(S_IWGRP)
    #define S_IWGRP (S_IWUSR >> 3)
#endif
#if !defined(S_IXGRP)
    #define S_IXGRP (S_IXUSR >> 3)
#endif
#if !defined(S_IROTH)
    #define S_IROTH (S_IRUSR >> 6)
#endif
#if !defined(S_IWOTH)
    #define S_IWOTH (S_IWUSR >> 6)
#endif
#if !defined(S_IXOTH)
    #define S_IXOTH (S_IXUSR >> 6)
#endif
#if !defined(S_IRWXU)
    #define S_IRWXU (S_IRUSR|S_IWUSR|S_IXUSR)
#endif
#if !defined(S_IRWXG)
    #define S_IRWXG (S_IRGRP|S_IWGRP|S_IXGRP)
#endif
#if !defined(S_IRWXO)
    #define S_IRWXO (S_IROTH|S_IWOTH|S_IXOTH)
#endif

#if !defined(S_IFLNK)
    #define S_IFLNK 0120000
#endif

#if !defined (S_ISDIR)
    #define	S_ISDIR(m)	(((m)&S_IFMT) == S_IFDIR)	/* directory */
#endif
#if !defined (S_ISREG)
    #define	S_ISREG(m)	(((m)&S_IFMT) == S_IFREG)	/* file */
#endif
#if !defined(S_ISLNK)
    #define S_ISLNK(m)    (((m) & S_IFMT) == S_IFLNK)
#endif

#if !defined(DT_DIR)
    #define DT_DIR 4
#endif
#if !defined(DT_REG)
    #define DT_REG 8
#endif

#if !defined(PATH_MAX)
    #define PATH_MAX 1024
#endif

class FSDirReader;


class FSDirReader
{
    public:
        struct Item
        {
            char name[OSFN_PATHSIZE + 1];
            bool isdir;
            bool isfile;
        };

    public:
        #if defined(OSFN_ISUNIX)
            DIR* m_handle;
        #else
            WIN32_FIND_DATA m_finddata;
            HANDLE m_findhnd;
        #endif

    public:
        bool openDir(const char* path)
        {
            #if defined(OSFN_ISUNIX)
                if((this->m_handle = opendir(path)) == NULL)
                {
                    return false;
                }
                return true;
            #else
                size_t msz;
                size_t plen;
                char* itempattern;
                /*
                * dumb-as-shit windows has AI, but retarded API:
                * unlike dirent.h, the method for reading items in a directory
                * requires '<path>' + "/" "*"', that is; one must add a glob character.
                * no idea if this interferes with dot files.
                */
                plen = strlen(path);
                msz = (sizeof(char) * (plen + 5));
                itempattern = (char*)mc_memory_malloc(msz);
                memset(itempattern, 0, msz);
                strncat(itempattern, path, plen);
                {
                    strncat(itempattern, "/*", 2);
                }
                this->m_findhnd = FindFirstFile(itempattern, &this->m_finddata);
                mc_memory_free(itempattern);
                if(INVALID_HANDLE_VALUE == this->m_findhnd)
                {
                   return false;
                }
                return true;
            #endif
            return false;
        }

        bool readItem(Item* itm)
        {
            #if defined(OSFN_ISUNIX)
                struct dirent* ent;
            #else
                int ok;
            #endif
            itm->isdir = false;
            itm->isfile = false;
            memset(itm->name, 0, OSFN_PATHSIZE);
            #if defined(OSFN_ISUNIX)
                if((ent = readdir((DIR*)(this->m_handle))) == NULL)
                {
                    return false;
                }
                if(ent->d_type == DT_DIR)
                {
                    itm->isdir = true;
                }
                if(ent->d_type == DT_REG)
                {
                    itm->isfile = true;
                }
                strcpy(itm->name, ent->d_name);
                return true;
            #else
                ok = FindNextFile(this->m_findhnd, &this->m_finddata);
                if(ok != 0)
                {
                    if(INVALID_HANDLE_VALUE == this->m_findhnd)
                    {
                        return false;
                    }
                    if(this->m_finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        itm->isfile = false;
                        itm->isdir = true;
                    }
                    else
                    {
                        itm->isfile = true;
                        itm->isdir = false;
                    }
                    strcpy(itm->name, this->m_finddata.cFileName);
                    return true;
                }
            #endif
            return false;
        }

        bool closeDir()
        {
            #if defined(OSFN_ISUNIX)
                closedir((DIR*)(this->m_handle));
            #else
                FindClose(this->m_findhnd);
            #endif
            return false;
        }

};



char *osfn_utilstrndup(const char *src, size_t len);
char *osfn_utilstrdup(const char *src);

int osfn_fdopen(const char *path, int flags, int mode);
int osfn_fdcreat(const char *path, int mode);
int osfn_fdclose(int fd);
size_t osfn_fdread(int fd, void *buf, size_t count);
size_t osfn_fdwrite(int fd, const void *buf, size_t count);
size_t osfn_fdput(int fd, const void *buf, size_t count);
int osfn_chmod(const char *path, int mode);
char *osfn_realpath(const char *path, char *respath);
char *osfn_dirname(const char *path);
char *osfn_fallbackbasename(const char *opath);
char *osfn_basename(const char *path);
int osfn_isatty(int fd);
int osfn_symlink(const char *path1, const char *path2);
int osfn_symlinkat(const char *path1, int fd, const char *path2);
char *osfn_getcwd(char *buf, size_t size);
int osfn_lstat(const char *path, struct stat *buf);
int osfn_truncate(const char *path, size_t length);
unsigned int osfn_sleep(unsigned int seconds);
int osfn_gettimeofday(struct timeval *tp, void *tzp);
int osfn_mkdir(const char *path, size_t mode);
int osfn_chdir(const char *path);
bool osfn_fileexists(const char *filepath);
bool osfn_stat(const char *path, struct stat *st);
bool osfn_statisa(const char *path, struct stat *st, char kind);
bool osfn_pathcheck(const char *path, char mode);
bool osfn_pathisfile(const char *path);
bool osfn_pathisdirectory(const char *path);

