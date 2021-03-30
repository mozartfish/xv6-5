#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int i;
  char *c;
  for (i=1; i<argc; ++i) {
    if (!(c = strchr(argv[i], '='))) {
      break;
    }
    *c='\0';
    setenv(argv[i], c+1);
  }

  if (i >= argc) {
    exit();
  }
  execv(argv[i], argv+i);
  printf(2, "exec %s failed\n", argv[i]);
  exit();
  return 0;
}
