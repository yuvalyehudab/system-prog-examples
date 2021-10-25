
#include "message_slot.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

unsigned int string_to_number(const char* str){
    unsigned int result = 0;
    while(*str != '\0'){
        if (!isdigit(*str)){
            return 0;
        }
        result = result * 10 + (*str - '0');
        str++;
    }    
    return result;
}

int main(int argc, char const *argv[])
{
    int i, fd;
    
    for (i = 0; i < 4; i++){
        if (argv[i] == NULL || argv[i][0] == '\0'){
            return -1;
        }
    }
    fd = open(argv[1], O_RDWR);
    
    if (fd == -1){
        close(fd);
        return -1;
    }
    unsigned int channel_number = string_to_number(argv[2]);
    
    if (channel_number == 0){
        close(fd);
        return -1;
    }
    int is_channel_set = (int)ioctl(fd, MSG_SLOT_CHANNEL, channel_number);
    if (is_channel_set != 0){
        close(fd);
        return -1;
    }
    int length = strlen(argv[3]);
    int bytes_written = write(fd, argv[3], length);
    close(fd);
    printf("%s:%d\n", "status-bytes_written", bytes_written);

    return 0;
}
