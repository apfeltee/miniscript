
#include "utilos.h"
#include "mem.h"

#if defined(__STRICT_ANSI__)
    #if defined(OSFN_ISUNIX)
        int symlink(const char *target, const char *linkpath);
        int symlinkat(const char *target, int newdirfd, const char *linkpath);
        char *realpath(const char *path, char *resolved_path);
        int lstat(const char *pathname, struct stat *statbuf);
        int truncate(const char *path, off_t length);
        int ftruncate(int fd, off_t length);
    #endif
#endif

char* osfn_utilstrndup(const char* src, size_t len)
{
    char* buf;
    buf = (char*)mc_memory_malloc(len+1);
    if(buf == NULL)
    {
        return NULL;
    }
    memset(buf, 0, len+1);
    memcpy(buf, src, len);
    return buf;
}

char* osfn_utilstrdup(const char* src)
{
    return osfn_utilstrndup(src, strlen(src));
}


int osfn_fdopen(const char *path, int flags, int mode)
{
    return open(path, flags, mode);
}

int osfn_fdcreat(const char* path, int mode)
{
    return creat(path, mode);
}

int osfn_fdclose(int fd)
{
    return close(fd);
}

size_t osfn_fdread(int fd, void* buf, size_t count)
{
    return read(fd, buf, count);
}

size_t osfn_fdwrite(int fd, const void* buf, size_t count)
{
    return write(fd, buf, count);
}

size_t osfn_fdput(int fd, const void* buf, size_t count)
{
    return osfn_fdwrite(fd, buf, count);
}

int osfn_chmod(const char* path, int mode)
{
    #if defined(OSFN_ISUNIXY)
        return chmod(path, mode);
    #else
        return -1;
    #endif
}

char* osfn_realpath(const char* path, char* respath)
{
    char* copy;
    #if defined(OSFN_ISUNIXY)
        char* rt;
        rt = realpath(path, respath);
        if(rt != NULL)
        {
            return rt;
        }
    #else
    #endif
    copy = osfn_utilstrdup(path);
    respath = copy;
    return copy;

}

char* osfn_dirname(const char* path)
{
    char* copy;
    #if defined(OSFN_ISUNIXY)
        char* rt;
        rt = dirname((char*)path);
        if(rt != NULL)
        {
            return rt;
        }
    #else
    #endif
    copy = osfn_utilstrdup(path);
    return copy;
}

char* osfn_fallbackbasename(const char* opath)
{
    char* strbeg;
    char* strend;
    char* cpath;
    strend = cpath = (char*)opath;
    while (*strend)
    {
        strend++;
    }
    while (strend > cpath && strend[-1] == '/')
    {
        strend--;
    }
    strbeg = strend;
    while (strbeg > cpath && strbeg[-1] != '/')
    {
        strbeg--;
    }
    /* len = (strend - strbeg) */
    cpath = strbeg;
    cpath[(strend - strbeg)] = 0;
    return strbeg;
}

char* osfn_basename(const char* path)
{
    #if defined(OSFN_ISUNIXY)
        char* rt;
        rt = basename((char*)path);
        if(rt != NULL)
        {
            return rt;
        }
    #else
    #endif
    return osfn_fallbackbasename(path);
}


int osfn_isatty(int fd)
{
    #if defined(OSFN_ISUNIXY)
        return isatty(fd);
    #else
        return 0;
    #endif
}

int osfn_symlink(const char* path1, const char* path2)
{
    #if defined(OSFN_ISUNIXY)
        return symlink(path1, path2);
    #else
        return -1;
    #endif
}

int osfn_symlinkat(const char* path1, int fd, const char* path2)
{
    #if defined(OSFN_ISUNIXY)
        return symlinkat(path1, fd, path2);
    #else
        return -1;
    #endif
}

char* osfn_getcwd(char* buf, size_t size)
{
    #if defined(OSFN_ISUNIXY)
        return getcwd(buf, size);
    #else
        return NULL;
    #endif
}

int osfn_lstat(const char* path, struct stat* buf)
{
    #if defined(OSFN_ISUNIXY)
        return lstat(path, buf);
    #else
        return stat(path, buf);
    #endif
}


int osfn_truncate(const char* path, size_t length)
{
    #if defined(OSFN_ISUNIXY)
        return truncate(path, length);
    #else
        return -1;
    #endif
}

unsigned int osfn_sleep(unsigned int seconds)
{
    #if defined(OSFN_ISUNIXY)
        return sleep(seconds);
    #else
        return 0;
    #endif
}

int osfn_gettimeofday(struct timeval* tp, void* tzp)
{
    #if defined(OSFN_ISUNIXY)
        return gettimeofday(tp, tzp);
    #else
        return 0;
    #endif
}

int osfn_mkdir(const char* path, size_t mode)
{
    #if defined(OSFN_ISUNIXY)
        return mkdir(path, mode);
    #else
        return -1;
    #endif
}


int osfn_chdir(const char* path)
{
    #if defined(OSFN_ISUNIXY)
        return chdir(path);
    #else
        return -1;
    #endif
}

bool osfn_fileexists(const char* filepath)
{
    #if defined(OSFN_ISUNIXY)
        return access(filepath, F_OK) == 0;
    #else
        struct stat sb;
        if(stat(filepath, &sb) == -1)
        {
            return false;
        }
        return true;
    #endif
}

bool osfn_stat(const char* path, struct stat* st)
{
    if(stat(path, st) == -1)
    {
        return false;
    }
    return true;
}

bool osfn_statisa(const char* path, struct stat* st, char kind)
{
    int fm;
    int need;
    (void)path;
    if(st == NULL)
    {
        return false;
    }
    need = -1;
    #define caseflag(c, r) case c: need=r; break;
    switch(kind)
    {
        #if defined(S_IFBLK)
            caseflag('b', S_IFBLK);
        #endif
        #if defined(S_IFIFO)
            caseflag('i', S_IFIFO);
        #endif
        #if defined(S_IFSOCK)
            caseflag('s', S_IFSOCK);
        #endif
        #if defined(S_IFCHR)
            caseflag('c', S_IFCHR);
        #endif
        caseflag('d', S_IFDIR);
        caseflag('f', S_IFREG);
    }
    #undef caseflag
    if(need == -1)
    {
        return false;
    }
    fm = (st->st_mode & S_IFMT);
    return (fm == need);
}

bool osfn_pathcheck(const char* path, char mode)
{
    struct stat st;
    if(!osfn_stat(path, &st))
    {
        return false;
    }
    return osfn_statisa(path, &st, mode);
}

bool osfn_pathisfile(const char* path)
{
    return osfn_pathcheck(path, 'f');
}

bool osfn_pathisdirectory(const char* path)
{
    return osfn_pathcheck(path, 'd');
}
