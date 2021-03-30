#include "types.h"
#include "stat.h"
#include "user.h"
#include "pwd.h"

int
main(int argc, char *argv[])
{
  struct passwd *pw;
  if (!(pw = getpwuid(getuid()))) {
      printf(2, "Error: whoami cannot find current user.");
  } else {
      printf(1, "%s\n", pw->pw_name);
  }
  exit();
}
