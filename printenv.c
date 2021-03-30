#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char **s;
  for(s=environ; *s; ++s) {
    printf(1, "%s\n", *s);
  }
  exit();
  return 0;
}
