#include "card.h"

/*
static char HexByteTable[]=
{
0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
0,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0,0,0,0,0,0,0,0,0
};
static char ByteHexTable[]="0123456789ABCDEF";
static int hexTobyte( char* hex, char *outbuf,int inlen)
{
	if(inlen%2)
		return 0;
	int i=0;
	for(i=0;i<inlen;)
	{  
	   *(outbuf+i/2)=((HexByteTable[hex[i]-0x30])<<4)+HexByteTable[hex[i+1]-0x30];
	   i+=2;
	}
	return 1;
}
static int byteTohex( char* byte, char *hex,int inlen)
{
	
	int i=0;
	for(i=0;i<inlen;i++)
	{  
		hex[i*2]=ByteHexTable[(byte[i]>>4)&0x0f];
        hex[i*2+1]=ByteHexTable[byte[i]&0x0f];
	}
	return 1;
}
*/



#define SENSE_LEN 0x20
#define BLOCK_LEN 8

//执行
unsigned char sense_buffer[SENSE_LEN];
unsigned char data_buffer[BLOCK_LEN*256];

/*
fd 句柄
cmd 命令 16字节长
rw 传输方向 rw = 1写   rw = 0读
buf 数据缓冲     io
plen 数据缓冲长度指针 io
*/
#define WRITE_CMD 1
#define READ_CMD  0
static int exec_scsi_cmd(int fd,unsigned char* cmd,int rw,unsigned char* buf,int *plen)
{
	unsigned char SenseKey,ASC,ASCQ;
	long rRetCode=0,wRetCode;
	int nLen=0x0;
	int ret;
    // unsigned char GET_DATA[16]={0xF7,0x20,0x00,0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char SenseBuf[SENSE_LEN]={0}; //sense 缓冲
    sg_io_hdr_t   scsi_hdr;
	memset(&scsi_hdr,0,sizeof(scsi_hdr));
	//固定设置
	scsi_hdr.interface_id = 'S'; 
    scsi_hdr.flags = 0; //SG_FLAG_UNUSED_LUN_INHIBIT;//
	//数据输入 输出缓冲区
	scsi_hdr.dxferp    = buf; 
	scsi_hdr.dxfer_len = *plen;
	//sense_buf
	scsi_hdr.sbp = SenseBuf;
    scsi_hdr.mx_sb_len = SENSE_LEN;
	scsi_hdr.timeout = 0x200;

	//方向
	if( rw == 0)
	  scsi_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	else
	  scsi_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    scsi_hdr.cmdp = cmd;  //命令缓冲
    scsi_hdr.cmd_len = 12;//固定为16字节

	//执行
    ret = ioctl(fd, SG_IO, &scsi_hdr);

    if (ret <  0) {
		return 0x3FFFFF;
    }

	SenseKey = SenseBuf[ 2 ];
	ASC      = SenseBuf[ 12 ];
	ASCQ     = SenseBuf[ 13 ];
	rRetCode = SenseKey<<16 | ASC<<8 | ASCQ; 

	if( rRetCode == 0x0 )
	{
		return 0;
	}
	if(SenseKey == 0x61)
	{
		nLen=ASC<<8 | ASCQ;
		// GET_DATA[9]=ASCQ;
        
		memset(&scsi_hdr,0,sizeof(scsi_hdr));
		scsi_hdr.interface_id = 'S'; 
		scsi_hdr.flags  = 0;// SG_FLAG_UNUSED_LUN_INHIBIT;//
		scsi_hdr.dxferp    = buf; 
		scsi_hdr.dxfer_len = nLen;
		scsi_hdr.sbp = SenseBuf;
		scsi_hdr.mx_sb_len = SENSE_LEN;
		scsi_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
		scsi_hdr.cmdp = cmd;
		scsi_hdr.cmd_len = 12;
		ret = ioctl(fd, SG_IO, &scsi_hdr);

		if (ret <  0) {
		return 1;
		}
		
		SenseKey = SenseBuf[ 2 ];
		ASC      = SenseBuf[ 12 ];
		ASCQ     = SenseBuf[ 13 ];
		wRetCode = SenseKey<<16 | ASC<<8 | ASCQ;
		if( ret < 0 )
			return 0x2FFFFF;

		if( wRetCode == 0x0 )
		{  
			 *plen=nLen;
			 return 0;
		}
		return wRetCode;
	}
	return rRetCode;
}

int  usb_apdu(int fd,unsigned char* cmd,int clen,unsigned char* response,unsigned int* rlen)
{    
	unsigned char metaRes[2500];
	memset(metaRes,0,2500);
	int  ret=1;
	int  len = 0 ;
	unsigned char scsi_id[16]={0xf7,0x80,0x00,0x00,0x00,0x00,0xe0,0x3f,0x00,0x10,0x00,0x00,0x00,0x000,0x00,0x00};
	scsi_id[5]=*cmd;
	scsi_id[6]=*(cmd+1);
	scsi_id[7]=*(cmd+2);
	scsi_id[8]=*(cmd+3);
	scsi_id[9]=*(cmd+4);
	
	if(clen>5)
	{ 
		len = clen-5;
		memcpy(metaRes,cmd+5,len);
		ret = exec_scsi_cmd(fd,scsi_id,WRITE_CMD,metaRes,&len);
                printf( "exec write cmd return %08x\n", ret );
		//返回9000
		if(ret==0)
		{
			metaRes[len]=0x90;
			metaRes[len+1]=0x00;
			len+=2;
			memcpy(response,metaRes,len);
			*rlen=len;
			return ret;
		}
		if((ret==0x1fffff)||(ret==0x2fffff)||(ret==0x3fffff))
		{
			return 1;
		}
		*rlen=2;
		response[0]=(((unsigned int)ret)&0x00ff0000)>>16;
		response[1]=(((unsigned int)ret)&0x0000ff00)>>8;
		return 0;

	}
	else   
	{ 
		len = cmd[4];
		memset(metaRes,0x01,2500);
		ret = exec_scsi_cmd(fd,scsi_id,READ_CMD,metaRes,&len);
                printf( "exec read cmd return %08x\n", ret );
		if(ret==0)
		{
			metaRes[len]=0x90;
			metaRes[len+1]=0x00;
			len+=2;
			memcpy(response,metaRes,len);
			*rlen=len;
			return ret;
		}
		if((ret==0x1fffff)||(ret==0x2fffff)||(ret==0x3fffff))
		{
			return 1;
		}
		*rlen=2;
		response[0]=(((unsigned int)ret)&0x00ff0000)>>16;
		response[1]=(((unsigned int)ret)&0x0000ff00)>>8;
		return 0;
	}
}

static void inquiry_cmd(unsigned char* cmd)
{
    unsigned char cdb[16]={0x12,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	memcpy(cmd,cdb,16);
	return ;
}

/*
static void show_char(unsigned char * buffer,int len) {
	int i = 0;
    for (i=0; i<len; ++i) {
        putchar(buffer[i]);
    }
    putchar('\n');
}
*/


static int write_file_once(int fd,int start,int len,unsigned char *outbuf)
{
	unsigned char response[256];
	unsigned int rlen = 2;
	unsigned char cmd[300] ="\x00\xd6\x00\x00\x00";
	cmd[2] = start>>8;
	cmd[3] = start&0xff;
	cmd[4] = len;
	memcpy(cmd+5,outbuf,len);
	if( usb_apdu(fd,cmd,len+5,response,&rlen) )
	{
	    return 1;  
	}
	if(( response[rlen-2] != 0x90) ||(response[rlen-1] != 0x00))
	{
		return 1;
	}
	return 0;
}

static int read_file_once(int fd,int start,int len,unsigned char *outbuf)
{
	unsigned char response[256];
	int rlen = len;
	unsigned char cmd[5] ="\x00\xb0\x00\x00\x00";
	cmd[2] = start>>8;
	cmd[3] = start&0xff;
	cmd[4] = len;
	if( usb_apdu(fd,cmd,5,response,(unsigned int*)&rlen) )
	{
	    return 1;  
	}
	if( rlen !=(len+2))
	{
		return 1;
	}
	if(( response[rlen-2] != 0x90) ||(response[rlen-1] != 0x00))
	{
		return 1;
	}
	memcpy(outbuf,response,len);
	return 0;
}
/*
引出函数
返回0 成功 
其他  失败

参数 设备fd句柄

*/
 int select_file(int fd )
{
	unsigned char response[256];
	unsigned int rlen = 2;
	unsigned char cmd[]="\x00\xA4\x00\x00\x02\x00\x06";

	int ret = usb_apdu(fd,cmd,7,response,&rlen);
	if( ret )
	{
            printf( "usb_apdu failed\n" );
	    return 1;
	}
	if(( response[rlen-2] != 0x90) ||(response[rlen-1] != 0x00))
	{
            printf( "response wrong, len %u, last %hhx %hhx\n", rlen, response[rlen-2], response[rlen-1] );
		return 1;
	}
	return 0;
}

/*
引出函数
读文件 
仅限读取长度为240*30

返回 0 为正确
其他   为错误

fd     设备句柄 open_usbkey获取
start  0
len    240*30
outbuf 输出缓冲区
*/
 int read_file(int fd,int start,int len,unsigned char* outbuf)
{
	int i =0;
	for( i =0; i< len/240;i++ )
	{
		if( read_file_once(fd,start+i*240,240,outbuf+i*240))
			return 1;
	}
	return 0;
}
/*
引出函数
写文件 
仅限写入长度为240*30

返回 0 为正确
其他   为错误

fd     设备句柄 open_usbkey获取
start  0
len    240*30
inbuf 输入缓冲区
*/
int write_file(int fd,int start,int len,unsigned char*inbuf)
{
	int i =0;
	for( i =0; i< len/240;i++ )
	{
		if( write_file_once(fd,start+i*240,240,inbuf+i*240))
			return 1;
	}
	return 0;
}

/*
每次取16字节的随机数
1 可以用来产生真随机数
2 可以用来检查卡是否存在

buf  输出缓冲区 至少为16字节 
返回值
0 成功
1 失败 
*/
int  get_random(int fd,unsigned char *buf)
{
	unsigned int rlen = 16 ;
	unsigned char response[256];
	unsigned char cmd[5] ="\x00\x84\x00\x00\x10";
	if( usb_apdu(fd,cmd,5,response,&rlen) )
	{
	    return 1;  
	}
	if( rlen != 18 )
	{
		return 1;
	}
	if(( response[rlen-2] != 0x90) ||(response[rlen-1] != 0x00))
	{
		return 1;
	}
	memcpy(buf,response,16);
	return 0;

}


/*
被引出的函数 用来打开usbkey设备文件句柄
返回-1 为失败
其他   为正确
*/
int open_usbkey()
{
	int i = 0;
	int fd = -1;
	int len = 0;
	unsigned char cmd[16]={0};
	char buf[33]={0};
	char  keyname[]="USB     AisinoKey";
	char devname[] ="/dev/sg0";
	for( i =0;i<10;i++)
	{
		devname[7]=0x30+i;
		fd = open(devname, O_RDWR);
	    if( fd == -1)
	    {
			continue;
	    }
		inquiry_cmd(cmd);
		len = 32;
		if( exec_scsi_cmd(fd,cmd,READ_CMD,(unsigned char *)buf,&len))
		{
			close(fd);
			continue;
		}
		else
		{
			if( memcmp(keyname,buf+8,strlen(keyname)))
			{
				close(fd);
				continue;
			}
			else
				break;
		}

	}
	if( i == 10)
		return -1;
	else
		return fd;
}




int get_cert(unsigned char *buf,int *len) 
{

 	int fd ;
	unsigned char tmp[8192];
	int length;
	//char  hexBuf[20000];
	//unsigned char cmd[16]={0};
	//unsigned char sensebuf[32];
	//int i = 0;

	//搜索并打开usb设备
	fd  = open_usbkey();
	if( fd == -1)
	{
		printf("open device failed \n");
                return 0;
	}
	//选择文件 
	if( select_file(fd) )
	{
		printf("select file failed\n");
		close(fd);
		return 0;
	}

	//将随机数填入缓冲区，进行读写测试

//	memset(tmp,0x02,240*30);
//	srand(0x33);
//	for(;i<240*30;i++)
//	{
//		buf[i] =(unsigned char*) rand();
//	}


	//写 仅限写240*30大小内容
/*	if( write_file(fd,0,240*30,buf) )
	{
		printf("write failed \r\n");
		close(fd);
		return 0 ;
	}
*/
        tmp[0] = 0;
        tmp[1] = 0;
	//读   仅限读240*30大小
	if( read_file(fd,0,240*30,tmp) )
	{
		printf("read failed \r\n");
		close(fd);
		return 0 ;
	}

    length=tmp[0]*256+tmp[1];
    printf(" length of file store in the key is %d\n",length);
    if( length > *len )
    {
        printf(" bigger than buffer len(%d)\n", *len);
		close(fd);
		return 0 ;
    }

    memcpy(buf,tmp+2,length);
   *len=length;
	//关闭句柄
	close(fd);
	 return 1;
}

