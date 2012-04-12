#include "checker.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../trace/trace.h"

int check(char tmpout_abspath[], char ans_abspath[], result_t *presult)
{
    if(access(tmpout_abspath, 0))
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : tmpout file(%s) not found", tmpout_abspath);
        return -1;
    }
    if(access(ans_abspath, 0))
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : answer file(%s) not found", ans_abspath);
        return -1;
    }
    struct stat s1, s2;
    if(stat(tmpout_abspath, &s1) || stat(ans_abspath, &s2))
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : stat() failed");
        return -1;
    }
    if(s1.st_size != s2.st_size)
    {
        *presult = WRONG_ANSWER;
        return 0;
    }
    int fd1, fd2;
    if((fd1 = open(tmpout_abspath, O_RDONLY)) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : tmpout file open failed");
        return -1;
    }
    if((fd2 = open(ans_abspath, O_RDONLY)) < 0)
    {
        close(fd1);
        __TRACE_LN(__TRACE_KEY, "Internal Error : answer file open failed");
        return -1;
    }
    void * ptr1, * ptr2;
    if((ptr1 = mmap(NULL, s1.st_size, PROT_READ, MAP_PRIVATE, fd1, 0)) == MAP_FAILED)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : mmap() on tmpout file failed");
        close(fd1);
        close(fd2);
        return -1;
    }
    if((ptr2 = mmap(NULL, s2.st_size, PROT_READ, MAP_PRIVATE, fd2, 0)) == MAP_FAILED)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : mmap() on answer file failed");
        munmap(ptr1, s1.st_size);
        close(fd1);
        close(fd2);
        return -1;
    }

    /* compare */
    char * pt = (char *)ptr1;
    char *pa = (char *)ptr2;
    int same = 1;
    while(*pt)
    {
        if(*pt != *pa)
        {
            same = 0;
            break;
        }
        pt++;
        pa++;
    }
    if(same)
    {
        *presult = ACCEPTED;
    }
    else
    {
        *presult = WRONG_ANSWER;
    };
    munmap(ptr1, s1.st_size);
    munmap(ptr2, s2.st_size);
    close(fd1);
    close(fd2);
    return 0;
}
