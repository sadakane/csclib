#ifndef _MYMMAP_H_
 #define _MYMMAP_H_


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#ifndef caddr_t
 #define caddr_t char *
#endif

typedef struct {
  void *addr;
  size_t len;
  int fd;
} MMAP;

MMAP *mymmap (char *fname);
int mymunmap (MMAP *m);




#ifndef min
 #define min(x,y) (((x)<(y))?(x):(y))
#endif

MMAP *mymmap (char *fname)
{
int fd;
size_t len;
MMAP *m;
struct stat statbuf;
caddr_t base;
  m = (MMAP *)malloc(sizeof(*m));
  if (m==NULL) {perror("mymmap malloc");  exit(1);}

  stat(fname,&statbuf);
  len = statbuf.st_size;
  fd = open(fname,O_RDONLY);
//  fd = open(fname,O_RDWR);
  if (fd == -1) {
    perror("open2\n");
    printf("fname %s\n", fname);
    exit(1);
  }
  base = (caddr_t)mmap(0,len,PROT_READ,MAP_SHARED,fd,0);
//  base = (void *)mmap(0,len,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
  if (base==(caddr_t)-1) {
    perror("mmap1\n");
    exit(1);
  }
  m->addr = (void *)base;
  m->fd = fd;
  m->len = len;
  return m;
}

int mymunmap (MMAP *m)
{
  if (munmap(m->addr,m->len)==-1) {
    perror("munmap 1:");
  }
  close(m->fd);
  free(m);
  return 0;
}                

#endif
