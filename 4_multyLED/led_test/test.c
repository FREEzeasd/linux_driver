#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc,char *argv[])
{
    if (argc < 2)
    {
        printf("plz input filename\n");
        return 0;
    }
    char *filename = argv[1];
    int fd = open(filename,O_RDWR);
    if(fd < 0)
    {
        printf("file open failed\n");
        return -1;
    }
    while(1)
    {
        int in = 0;
        int exit = 0;
        printf("0 to off,1 to on,-1 to exit\n"); 
        scanf("%d",&in);
        switch (in)
        {
            case 0: write(fd,"0",1);break;
            case 1: write(fd,"1",1);break;
            default: exit = 1;break;
        }
        if(exit)
        {
            break;
        }
    }
    close(fd);
    return 0;
}