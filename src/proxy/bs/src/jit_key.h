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


//���б���Ϊder

//�õ���������
int MAN_GetErrorString(
	int error_code,	//������
	char *err_string	//��������
	);

//���豸
int MAN_OpenDevice(
	void **hHandle,  //�ɹ������豸���
	char *pin        //��¼�豸����
	);

//�ر��豸
int MAN_CloseDevice(
	void *hHandle	//�豸���
	);

//����֤��/˽Կ������
int CRYOTO_GetCertCounts(
	void *hHandle,	//�豸���
	unsigned int *counts	//֤������
	);
//��ȡ֤����Ϣ	      
int CRYPTO_GetCert(
	void *hHandle,	//�豸���
	unsigned int index,  //֤��������,��0��ʼ
	char *cert,	//֤�����ݣ�Ϊder����
	unsigned int *certlen	//֤�鳤��
	);

//ʹ���ڲ�˽Կ���м���/����--��֤����							
int CRYPTO_Private_EnDecrypt(
	void *hHandle,		//�豸���
	unsigned int opt,		//0������ܣ�1�������
	unsigned int  keyIndex,	//֤��������,��0��ʼ��0��ʾǩ��֤�飬1��ʾ����֤��
	unsigned char *inData,	//��������
	unsigned int  inDataLen,	//�������ݳ���
	unsigned char *outData,	//�������
	unsigned int  *dataLength	//[in/out]���ݳ���
	);

// 
// #ifdef __cplusplus
// }
// #endif

#endif

