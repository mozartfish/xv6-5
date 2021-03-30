#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if (argc < 3)
    goto usage;

  int u = atoi(argv[1]);

  int r = setuid(u);
  if (r < 0) {
    printf(2, "setuid failed\n");
    exit();
  }

  r = execv(argv[2], argv + 2);
  if (r < 0) {
    printf(2, "exec failed\n");
    exit();
  }

  exit();

 usage:
  printf(2, "Usage: %s <uid> <program>\n", argv[0]);
  exit();
}
