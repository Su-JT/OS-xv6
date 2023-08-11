#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{

  int parent[2], child[2];
  pipe(parent);
  pipe(child);
  char mes = '\0';
  int pid = fork();
  if(pid < 0){
    fprintf(2, "error in fork\n");
    exit(1);
  }
  else if(pid == 0){   //子进程
    read(parent[0], &mes, 1);  //pipe 0读 1写
    printf("%d: received ping\n", getpid());
    write(child[1], &mes, 1);
  } 
  else if(pid > 0){   //父进程
    write(parent[1], &mes, 1);
    read(child[0], &mes, 1);
    printf("%d: received pong\n", getpid());
  }
   
  exit(0);
}