#ifndef _LICENSE_VERIFY_H
#define _LICENSE_VERIFY_H
/*
verify 验证LICENSE签名和磁盘序列号
[IN] pbData:LICENSE文件内容
[IN] czDataLen:LICENSE文件内容长度
返回值：0=成功,-1=签名不正确,-2=磁盘序列号不匹配,-9=参数错误,-10=其他错误
*/
int verifylicense(unsigned char *pbData,int czDataLen);
/*
verifyDate 验证当前日期是否在LICENSE期限内
[IN] pbData:LICENSE文件内容
[IN] pDate:系统当前日期，格式：字符串"20100526"
返回值：0=成功,-1=小于起始日期,-2=大于终止日期,-9=参数错误,-10=其他错误
*/
int verifyDate(unsigned char *pbData,char *pDate);
/*
getIndate 获得有效期
[IN] pbData:LICENSE文件内容
[OUT]pIndate:有效期,"2010052620130526",0-7起始日期,8-15终止日期
返回值：0=成功,-9=参数错误,-10=其他错误
*/
int getIndate (unsigned char *pbData,char *pIndate);
/*
getMaxUserNumA 获得-A最大用户数
[IN] pbData:LICENSE文件内容
[OUT]pzNum:-A最大用户数
返回值：0=成功,-9=参数错误,-10=其他错误
*/
int getMaxUserNumA(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxUserNumB 获得-B最大用户数
[IN] pbData:LICENSE文件内容
[OUT]pzNum:-B最大用户数
返回值：0=成功,-9=参数错误,-10=其他错误
*/
int getMaxUserNumB(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxUserNumC 获得-C最大用户数
[IN] pbData:LICENSE文件内容
[OUT]pzNum:-C最大用户数
返回值：0=成功,-9=参数错误,-10=其他错误
*/
int getMaxUserNumC(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxUserNumD 获得-D最大用户数
[IN] pbData:LICENSE文件内容
[OUT]pzNum:-D最大用户数
返回值：0=成功,-9=参数错误,-10=其他错误
*/
int getMaxUserNumD(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxOnlineNumB 获得-B最大在线用户数
[IN] pbData:LICENSE文件内容
[OUT]pzNum:-B最大在线用户数
返回值：0=成功,-9=参数错误,-10=其他错误
*/
int getMaxOnlineNumB(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxOnlineNumC 获得-C最大用户数
[IN] pbData:LICENSE文件内容
[OUT]pzNum:-C最大用户数
返回值：0=成功,-9=参数错误,-10=其他错误
*/
int getMaxOnlineNumC(unsigned char *pbData,unsigned int *pzNum);
/*
getDeviceNo 获得设备号
[IN] pbData:LICENSE文件内容
[OUT]pDevNo:设备号地址,pDevNo=NULL时只得到设备号长度
[OUT]pzNoLen:设备号长度
返回值：0=成功,-9=参数错误,-10=其他错误
*/
int getDeviceNo(unsigned char *pbData,char *pDevNo,int *pzNoLen);
/*
test 调用测试
返回值：衡等于1
*/
int test();

#endif

