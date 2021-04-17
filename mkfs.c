#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"

#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

#define NINODES 200

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]

int nbitmap = FSSIZE/(BSIZE*8) + 1;
int ninodeblocks = NINODES / IPB + 1;
int nlog = LOGSIZE;
int nmeta;    // Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks;  // Number of data blocks

int fsfd;
struct superblock sb;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;


void balloc(int);
void wsect(uint, void*);
void winode(uint, struct dinode*);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type, ushort uid, ushort perms);
void iappend(uint inum, void *p, int n);

// convert to intel byte order
ushort
xshort(ushort x)
{
  ushort y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint
xint(uint x)
{
  uint y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

struct fobj {
  char *name;
  union {
    int fd;
    struct fobj **children;
  } contents;
  ushort perms;
  ushort owner;
  uchar t;
};

struct fobj*
create_file(char *name, int fd, ushort owner, ushort perms)
{
  struct fobj *f = calloc(sizeof(struct fobj), 1);
  f->name = name;
  f->t = T_FILE;
  f->contents.fd = fd;
  f->perms = perms;
  f->owner = owner;
  return f;
}
struct fobj*
create_dir(char *name, struct fobj **children,
    ushort owner, ushort perms)
{
  struct fobj *d = calloc(sizeof(struct fobj), 1);
  d->name = name;
  d->t = T_DIR;
  d->contents.children = children;
  d->perms = perms;
  d->owner = owner;
  return d;
}

static char buf[BSIZE];
uint
create_fshier(struct fobj *f, uint *parino)
{
  uint off, ino, cino;
  struct dirent de;
  struct dinode din;
  struct fobj **children;
  int cc;
  ino = ialloc(f->t, f->owner, f->perms);
  switch (f->t) {
  case T_DIR:
    bzero(&de, sizeof(de));
    de.inum = xshort(ino);
    strcpy(de.name, ".");
    iappend(ino, &de, sizeof(de));
    bzero(&de, sizeof(de));
    de.inum = xshort(parino ? *parino : /*root*/ ino);
    strcpy(de.name, "..");
    iappend(ino, &de, sizeof(de));
    for (children = f->contents.children; *children; ++children) {
      cino = create_fshier(*children, &ino);
      bzero(&de, sizeof(de));
      de.inum = xshort(cino);
      strncpy(de.name, (*children)->name, DIRSIZ);
      iappend(ino, &de, sizeof(de));
    }
    // fix size of  dirs
    rinode(ino, &din);
    off = xint(din.size);
    off = ((off/BSIZE) + 1) * BSIZE;
    din.size = xint(off);
    winode(ino, &din);
    break;
  case T_FILE:
    while((cc = read(f->contents.fd, buf, sizeof(buf))) > 0)
      iappend(ino, buf, cc);
    close(f->contents.fd);
    break;
  default:
    fprintf(stderr, "Invalid file hierarchy\n");
    exit(EXIT_FAILURE);
  }

  return ino;
}

int
main(int argc, char *argv[])
{
  int i, fd;
  bool binary;
  struct fobj *root_files[NINODES] = {0}, *bin_files[NINODES] = {0},
              *mtdir_files[] = {NULL};
  int rootidx=0, binidx=0;
  char *users[] = {getlogin(), "testuser", NULL};

  static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

  if(argc < 2){
    fprintf(stderr, "Usage: mkfs fs.img files...\n");
    exit(1);
  }

  assert((BSIZE % sizeof(struct dinode)) == 0);
  assert((BSIZE % sizeof(struct dirent)) == 0);

  fsfd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fsfd < 0){
    perror(argv[1]);
    exit(1);
  }

  // 1 fs block = 1 disk sector
  nmeta = 2 + nlog + ninodeblocks + nbitmap;
  nblocks = FSSIZE - nmeta;

  sb.size = xint(FSSIZE);
  sb.nblocks = xint(nblocks);
  sb.ninodes = xint(NINODES);
  sb.nlog = xint(nlog);
  sb.logstart = xint(2);
  sb.inodestart = xint(2+nlog);
  sb.bmapstart = xint(2+nlog+ninodeblocks);

  printf("nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
         nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

  freeblock = nmeta;     // the first free block that we can allocate

  for(i = 0; i < FSSIZE; i++)
    wsect(i, zeroes);

  memset(buf, 0, sizeof(buf));
  memmove(buf, &sb, sizeof(sb));
  wsect(1, buf);

  // Create filessytem object tree
  // First, collect all files
  for(i = 2; i < argc; i++){
    binary = false;
    assert(index(argv[i], '/') == 0);

    if((fd = open(argv[i], 0)) < 0){
      perror(argv[i]);
      exit(1);
    }

    // Skip leading _ in name when writing to file system.
    // The binaries are named _rm, _cat, etc. to keep the
    // build operating system from trying to execute them
    // in place of system binaries like rm and cat.
    if(argv[i][0] == '_') {
      ++argv[i];
      binary=true;
    }
    if (binary) {
      bin_files[binidx++]=create_file(argv[i], fd, 0, S_IROTH | S_IXOTH);
    } else {
      root_files[rootidx++]=create_file(argv[i], fd, 0, S_IROTH);
    }
  }
  // now create bin
  root_files[rootidx++]=create_dir("bin", bin_files, 0, S_IROTH | S_IXOTH);
  // and /etc/passwd
  {
    int fds[2];
    pipe(fds);
    switch (fork()) {
    case -1:
      fprintf(stderr, "Could not fork.img\n");
      exit(EXIT_FAILURE);
    case 0: 
      close(fds[0]);
      FILE *fp = fdopen(fds[1], "w");
      fprintf(fp, "root:0\n");
      for(i = 0; users[i]; i++)
        fprintf(fp, "%s:%d\n", users[i], i+1);
      exit(EXIT_SUCCESS);
    }
    close(fds[1]);
    struct fobj *etc_files[] = {create_file("passwd", fds[0], 0, S_IROTH), NULL};
    root_files[rootidx++]= create_dir("etc", etc_files, 0, S_IROTH | S_IXOTH );
  }
  // home directories-
  root_files[rootidx++] = create_dir("root", mtdir_files, 0, 0);
  {
    struct fobj **homedirs = calloc(sizeof(struct fobj*), sizeof(users)/sizeof(char *));
    for (i = 0; users[i]; ++i) {
      homedirs[i] = create_dir(users[i], mtdir_files, i+1, 0);
    }
    root_files[rootidx++] = create_dir("home", homedirs, 0, S_IROTH | S_IXOTH);
  }
  // create root
  create_fshier(create_dir(NULL, root_files, 0, S_IROTH | S_IXOTH), NULL);

  balloc(freeblock);

  exit(0);
}

void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(write(fsfd, buf, BSIZE) != BSIZE){
    perror("write");
    exit(1);
  }
}

void
winode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}

void
rinode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, BSIZE) != BSIZE){
    perror("read");
    exit(1);
  }
}

uint
ialloc(ushort type, ushort uid, ushort perms)
{
  uint inum = freeinode++;
  struct dinode din;

  bzero(&din, sizeof(din));
  // *** STAGE2b ADD YOUR CODE HERE ***
  din.type = xshort(type);
  din.nlink = xshort(1);
  din.size = xint(0);
  winode(inum, &din);
  return inum;
}

void
balloc(int used)
{
  uchar buf[BSIZE];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < BSIZE*8);
  bzero(buf, BSIZE);
  for(i = 0; i < used; i++){
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  printf("balloc: write bitmap block at sector %d\n", sb.bmapstart);
  wsect(sb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

void
iappend(uint inum, void *xp, int n)
{
  char *p = (char*)xp;
  uint fbn, off, n1;
  struct dinode din;
  char buf[BSIZE];
  uint indirect[NINDIRECT];
  uint x;

  rinode(inum, &din);
  off = xint(din.size);
  // printf("append inum %d at off %d sz %d\n", inum, off, n);
  while(n > 0){
    fbn = off / BSIZE;
    if(fbn >= MAXFILE) {
      fprintf(stderr, "file full appending to inode %u, %u blocks %u bytes\n", inum, fbn, fbn*BSIZE);
    }
    assert(fbn < MAXFILE);
    if(fbn < NDIRECT){
      if(xint(din.addrs[fbn]) == 0){
        din.addrs[fbn] = xint(freeblock++);
      }
      x = xint(din.addrs[fbn]);
    } else {
      if(xint(din.addrs[NDIRECT]) == 0){
        din.addrs[NDIRECT] = xint(freeblock++);
      }
      rsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      if(indirect[fbn - NDIRECT] == 0){
        indirect[fbn - NDIRECT] = xint(freeblock++);
        wsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      }
      x = xint(indirect[fbn-NDIRECT]);
    }
    n1 = min(n, (fbn + 1) * BSIZE - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * BSIZE), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}
