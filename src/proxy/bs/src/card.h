#include <stddef.h>
#include <scsi/sg.h>
#include <stdlib.h>
#include <string.h> 
#include <stdio.h>
#include <sys/ioctl.h>
#include <scsi/scsi_ioctl.h>
#include <unistd.h>
#include <fcntl.h> 

int get_cert(unsigned char *buf,int *len);

