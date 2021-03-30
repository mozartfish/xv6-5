#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    goto usage;
  }
  if (chown(argv[2], atoi(argv[1])) < 0) {
    printf(2, "Error: chown");
  }
  exit();
usage:
  printf(2, "Usage: %s <uid> <program>\n", argv[0]);
  exit();
}
