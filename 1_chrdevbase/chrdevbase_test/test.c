#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc,char *argv[])
{
    char *filename = argv[1];
    int fd = open(filename, O_RDWR);
    if(fd <= 0)
    {
        printf("open file error\n");
        return -1;
    }

    char buf[100] = {0};

    int ret = read(fd,buf,100);
    if(ret < 0)
    {
        printf("read failed!\n");
        return -1;
    }
    printf("read out: %s\n",buf);

    strcpy(buf,"Hello World");
    ret = write(fd,buf,20);
    if(ret < 0)
    {
        printf("write failed!\n");
        return -1;
    }
    ret = close(fd);
    if(ret < 0)
    {
        printf("close failed!\n");
        return -1;
    }
    return 0;
}