#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define MAX 35
#define MAX_NUM 34

void 
findprime(int prime[], int num)
{
    if(num <= 0){
        return;
    }
    int base = prime[0];
    printf("prime %d\n", base);     //经筛选，第一个一定为素数
    int p[2];
    pipe(p);
    if(fork() == 0){
        for(int i = 0; i < num; i++){
            if(prime[i] > 0){
                write(p[1], &prime[i], sizeof(int));
            }
        }
        exit(0);
    }
    close(p[1]);
    if(fork() == 0){
        int tmp = 0;
        int count = 0;
        while(read(p[0], &tmp, sizeof(int)) != 0){
            if(tmp % base != 0){
                prime[count++] = tmp;
            }
        }
        findprime(prime, count);
        exit(0);
    }
    wait(0);
}

int
main(int argc, char *argv[])
{
    int prime[MAX_NUM]={0};
    for(int i=0;i<MAX_NUM;i++){
        prime[i]=i+2;
    }
    findprime(prime, MAX_NUM);
    exit(0);
}