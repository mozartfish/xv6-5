# Initial process execs /init.
# This code runs in user space.

#include "syscall.h"
#include "traps.h"


# exec(init, argv)
.globl start
start:
  pushl $envp
  pushl $argv
  pushl $init
  pushl $0  // where caller pc would be
  movl $SYS_execve, %eax
  int $T_SYSCALL

# for(;;) exit();
exit:
  movl $SYS_exit, %eax
  int $T_SYSCALL
  jmp exit

# char init[] = "/bin/init\0";
init:
  .string "/bin/init\0"

# char *argv[] = { init, 0 };
# char *envp[] = argv+1 = { 0 };
.p2align 2
argv:
  .long init
envp:
  .long 0

