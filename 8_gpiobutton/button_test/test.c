#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc,char *argv[])
{
    unsigned char keyvalue;
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
    int flag = 0;
    while(1)
    {
        read(fd, &keyvalue, sizeof(keyvalue));
        if(keyvalue == '1' && flag == 0)
        {
            printf("key pressed!\r\n");
            flag = 1;
        }
        else if (keyvalue != '1')
            flag = 0;
    }
    close(fd);
    return 0;
}