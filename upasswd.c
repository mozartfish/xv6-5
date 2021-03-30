#include "types.h"
#include "stat.h"
#include "user.h"
#include "pwd.h"

static int fd = -1;
// 32chars for username +2sep and hopefully not more than 30 digits for uid
static char BF[64];
// username pointer points here
static char user[33];
// buffer full from 0....BFIDX
static int BFLEN = 0;

// Reset
void
setpwent(void) {
  if (fd != -1) {
    close(fd);
    fd=-1;
  }
  BFLEN=0;
}

struct passwd *
getpwent(void) {
  static struct passwd pw = {user, 0};
  if (fd<0 && (fd = open("/etc/passwd", 0)) < 0) {
      return 0;
  }
  char ch=':'; // looking for username
  int i, n;

  while((n = read(fd, BF, sizeof(BF)-BFLEN)) >= 0) {
    BFLEN += n;
    if (BFLEN==0 && ch==':') {
      return 0;
    }
    // search for ch
    for (i = 0; i < BFLEN && BF[i] != ch; ++i);
    if (i == BFLEN) {
      printf(1, "upasswd: passwd file ill-formatted");
      exit();
    }
    BF[i]='\0';
    switch (ch) {
    case ':':
      strcpy(user, BF);
      // look for uid next
      ch='\n';
      break;
    case '\n':
      pw.pw_uid = atoi(BF);
      ch='\0'; //ret
      break;
    }
    // reset buffer & index
    BFLEN-=i+1;
    memmove(BF, BF+i+1, BFLEN);
    if (ch == '\0') {
      return &pw;
    }
  }
  return 0;
}

struct passwd*
getpwnam(const char *name)
{
  setpwent();
  struct passwd *pw;
  while((pw = getpwent())) {
    if (strcmp(name, pw->pw_name) == 0) {
      return pw;
    }
  }
  return 0;
}
struct passwd*
getpwuid(uid_t uid)
{
  setpwent();
  struct passwd *pw;
  while((pw = getpwent())) {
    if (uid == pw->pw_uid) {
      return pw;
    }
  }
  return 0;
}
