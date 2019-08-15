#ifndef __JIT_USB_KEY_H
#define __JIT_USB_KEY_H

// #ifdef __cplusplus
// extern "C"{
// #endif

// #if defined(_MSC_VER) && defined(WIN32)
// #define KEYAPI  __stdcall
// #else 
// #define KEYAPI
// #endif

#define		RET_PAI_OK						0x00000000
#define		RET_PAI_FAIL					0x00000001
#define		RET_PAI_KEY_REMOVED				0x00000002
#define		RET_PAI_INVALID_PARAMETER		0x00000003
#define		RET_PAI_PARAMETER_ERROR			0x00000004
#define		RET_PAI_PARAMETER_LEN_ERROR		0x00000005


//所有编码为der

//得到错误描述
int MAN_GetErrorString(
	int error_code,	//错误码
	char *err_string	//错误描述
	);

//打开设备
int MAN_OpenDevice(
	void **hHandle,  //成功返回设备句柄
	char *pin        //登录设备密码
	);

//关闭设备
int MAN_CloseDevice(
	void *hHandle	//设备句柄
	);

//返回证书/私钥的数量
int CRYOTO_GetCertCounts(
	void *hHandle,	//设备句柄
	unsigned int *counts	//证书数量
	);
//获取证书信息	      
int CRYPTO_GetCert(
	void *hHandle,	//设备句柄
	unsigned int index,  //证书索引号,从0开始
	char *cert,	//证书内容，为der编码
	unsigned int *certlen	//证书长度
	);

//使用内部私钥进行加密/解密--认证类型							
int CRYPTO_Private_EnDecrypt(
	void *hHandle,		//设备句柄
	unsigned int opt,		//0代表加密，1代表解密
	unsigned int  keyIndex,	//证书索引号,从0开始：0表示签名证书，1表示加密证书
	unsigned char *inData,	//输入数据
	unsigned int  inDataLen,	//输入数据长度
	unsigned char *outData,	//输出数据
	unsigned int  *dataLength	//[in/out]数据长度
	);

// 
// #ifdef __cplusplus
// }
// #endif

#endif

