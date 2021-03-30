#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{
  int mode = atoi(argv[1]);
  ushort fmode = 0;
  if (argc != 3) {
    goto usage;
  }
  // we want this to be "octal-ish", so e.g. 17 is all permissions.
  if (mode > 10) {
    mode -=10;
    fmode |= S_ISUID;
  }
  if (mode > 7) {
    printf(2, "Error: mode must be in octal");
    goto usage;
  }
  fmode |= mode;
  if (chmod(argv[2], fmode) < 0) {
    printf(2, "Error: chmod");
  }
  exit();
usage:
  printf(2, "Usage: %s <mode> <program>\n", argv[0]);
  exit();
}
