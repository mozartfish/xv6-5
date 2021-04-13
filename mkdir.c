#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    printf(2, "Usage: mkdir files...\n");
    exit();
  }

  for(i = 1; i < argc; i++){
    if(mkdir(argv[i], S_IROTH | S_IXOTH | S_IWOTH) < 0){
      printf(2, "mkdir: %s failed to create\n", argv[i]);
      break;
    }
  }

  exit();
}
