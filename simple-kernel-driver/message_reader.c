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
    int i, io_ret, channel_number;
    char buffer[MAX_MSG_LEN + 1];
    
    for(i = 0; i < 3; i++){
        if(argv[i] == NULL || argv[i][0] == '\0'){
            exit(-1);
        }
    }
    int fd = open(argv[1], O_RDWR);
    if (fd == -1){
        exit(-1);
    }
    channel_number = string_to_number(argv[2]);
    io_ret = ioctl(fd, MSG_SLOT_CHANNEL, channel_number);
    if (io_ret != 0){
        exit(-1);
    }
    int bytes_read = read(fd, buffer, MAX_MSG_LEN);
    close(fd);
    if (bytes_read < MIN_MSG_LEN){
        exit(-1);
    }
    buffer[bytes_read] = '\0';
    printf("The message: %s\n", buffer);
    printf("Status: fine. Bytes read: %d\n",bytes_read);

    return 0;
}
