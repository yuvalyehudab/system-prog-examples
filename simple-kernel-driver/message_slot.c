
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");

#include "message_slot.h"

/*******************************VARIABLES********************************/
struct open_devices* devices_list_head;

/*******************************LIST FUNCTIONS***********************************/
struct open_devices* get_device_by_value(struct open_devices* head, int value)
{
    while (head != NULL){
        if (head->value == value){
            return head;
        }
        head = head->next;
    }
    return NULL;
}
struct channel_list* get_channel_by_value(struct channel_list* head, int value)
{
    while (head != NULL){
        if (head->value == value){
            return head;
        }
        head = head->next;
    }
    return NULL;
}
void delete_channel_list(struct channel_list* head)
{
    struct channel_list* tmp;
    while (head != NULL){
        tmp = head;
        head = head->next;
        kfree(tmp);
    }
}
void delete_devices_list(struct open_devices* head)
{
    struct open_devices* tmp;
    
    while(head != NULL){
        delete_channel_list(head->channels);
        tmp = head;
        head = head->next;
        kfree(tmp);
    }
}
/********************************DEVICE FUNCTIONS************************/

static int device_open(struct inode* inode, struct file* file)
{
    struct open_devices* tmp;
    file->private_data = NULL;
    
    tmp = get_device_by_value(devices_list_head, iminor(inode));
    if (tmp == NULL){
        tmp = (struct open_devices*)kmalloc(sizeof(struct open_devices), GFP_KERNEL);
        if (tmp == NULL){
            return -EFAULT;
        }
        tmp->value = iminor(inode);
        tmp->channels = NULL;
        tmp->next = devices_list_head;
        devices_list_head = tmp;
    }
    return 0;
}

static int device_release(struct inode* inode, struct file* file)
{
    return 0;
}

static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset)
{
    int i, c;
    struct open_devices* tmp_device;
    struct channel_list* tmp_channel;
    if (file->private_data == NULL){
        return -EINVAL;
    }
    tmp_device = get_device_by_value(devices_list_head, iminor(file_inode(file)));
    if (tmp_device == NULL){
        return -EWOULDBLOCK;
    }
    
    tmp_channel = get_channel_by_value(tmp_device->channels, (int)((long)file->private_data));
    if (tmp_channel == NULL){
        return -EWOULDBLOCK;
    }
    if (tmp_channel->size == 0){
        return -EWOULDBLOCK;
    }
    if (length < tmp_channel->size){
        return -ENOSPC;
    }
    
    for (i = 0; i < tmp_channel->size; i++){
        c = put_user((tmp_channel->message)[i], &buffer[i]);
        if (c != 0){
            return -EFAULT;
        }
    }
    if (i != tmp_channel->size){
        return -EFAULT;
    }
    return i;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    int minor, chan;
    int i, c;
    struct open_devices* tmp_dev;
    struct channel_list* tmp_channel;
    if (file->private_data == NULL){
        return -EINVAL;
    }
    if (length > MAX_MSG_LEN || length < MIN_MSG_LEN){
        return -EMSGSIZE;
    }
    
    minor = iminor(file_inode(file));
    tmp_dev = get_device_by_value(devices_list_head, minor);
    if (tmp_dev == NULL){
        return -EFAULT;
    }
    chan = (int)((long)file->private_data);
    tmp_channel = get_channel_by_value(tmp_dev->channels, chan);
    if (tmp_channel == NULL){
        tmp_channel = (struct channel_list *)kmalloc(sizeof(struct channel_list), GFP_KERNEL);
        if (tmp_channel == NULL){
            return -EFAULT;
        }
        tmp_channel->value = chan;
        tmp_channel->next = tmp_dev->channels;
        tmp_dev->channels = tmp_channel;
    }
    tmp_channel->size = length;
    
    for (i = 0; i < length; i++){
        c = get_user(tmp_channel->message[i], &buffer[i]);
        if (c != 0){
            return -EFAULT;
        }
    }
    return i;
}

static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param)
{
    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0){
        return -EINVAL;
    }
    file->private_data = (void *)ioctl_param;
    return 0;
}

/**********************************DEVICE SETUP******************************************************/

struct file_operations fops = 
{
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .release = device_release
};

static int __init device_init(void)
{
    int rc = register_chrdev(MAJOR_NUMBER, DEVICE_RANGE_NAME, &fops);
    if (rc < 0)
    {
        printk(KERN_ERR "registration failed for message_slot\n");
        return rc;
    }
    devices_list_head = NULL;
    printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUMBER);
    return 0;
}

static void __exit device_cleanup(void)
{
    delete_devices_list(devices_list_head);
    unregister_chrdev(MAJOR_NUMBER, DEVICE_RANGE_NAME);
}

/**********************************************************************************************/

module_init(device_init);
module_exit(device_cleanup);

/***********************************ALL DONE*****************************************************/