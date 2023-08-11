#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define PIPESIZE 512

int
main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "Usage: xargs arguments\n");
    exit(1);
  }

  char * argvs[MAXARG];     //追加参数
  for(int i = 1; i < argc; i++){
    argvs[i-1] = argv[i];
  }
  char buf[PIPESIZE] = {'\0'};
  int n = 0;
  char tmp[PIPESIZE] = {'\0'};
  int tmp_n = 0;
  while((n = read(0, buf, PIPESIZE)) > 0){   //从管道读取数据
    for(int i = 0; i < n; i++){
        if(buf[i] == '\n'){        //读取单个输入行
            tmp[tmp_n] = '\0';
            tmp_n = 0;
            argvs[argc - 1] = tmp;  //追加参数tmp
            if(fork() == 0){
                exec(argv[1], argvs);
            }
            wait(0);
        }
        else{
            tmp[tmp_n++] = buf[i];
        }
    }
  }
  exit(0);
}