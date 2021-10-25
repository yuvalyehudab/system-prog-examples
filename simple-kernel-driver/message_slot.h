
#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUMBER 241

#define MSG_SLOT_CHANNEL _IOR(MAJOR_NUMBER, 0, unsigned int)
#define DEVICE_RANGE_NAME "m_slot"

#define DEVICE_FILE_NAME "m_slot"
#define MAX_MSG_LEN 128
#define MIN_MSG_LEN 1

struct channel_list {
    int size;
    int value;
    char message[MAX_MSG_LEN];
    struct channel_list* next;
};

struct open_devices {
    int value;
    struct channel_list* channels;
    struct open_devices* next;
};

#endif