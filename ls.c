#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "pwd.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  struct passwd *pw;

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  char perms[4];

  switch(st.type){
  case T_FILE:
    perms[0] = (st.perms & S_IROTH) ? 'r' : '-';
    perms[1] = (st.perms & S_IWOTH) ? 'w' : '-';
    perms[2] = (st.perms & S_IXOTH) ? 'x' : '-';
    perms[3] = '\0';
    pw = getpwuid(st.uid);
    if (pw) {
      printf(1, "%s %s %s %d %d %d\n", fmtname(path), pw->pw_name, perms, st.type, st.ino, st.size);
    } else {
      printf(1, "%s %d %s %d %d %d\n", fmtname(path), st.uid, perms, st.type, st.ino, st.size);
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      perms[0] = (st.perms & S_IROTH) ? 'r' : '-';
      perms[1] = (st.perms & S_IWOTH) ? 'w' : '-';
      perms[2] = (st.perms & S_IXOTH) ? 'x' : '-';
      perms[3] = '\0';
      pw = getpwuid(st.uid);
      if (pw) {
        printf(1, "%s %s %s %d %d %d\n", fmtname(buf), pw->pw_name, perms, st.type, st.ino, st.size);
      } else {
        printf(1, "%s %d %s %d %d %d\n", fmtname(buf), st.uid, perms, st.type, st.ino, st.size);
      }
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit();
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit();
}
