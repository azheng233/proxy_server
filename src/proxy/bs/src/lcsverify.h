#ifndef _LICENSE_VERIFY_H
#define _LICENSE_VERIFY_H
/*
verify ��֤LICENSEǩ���ʹ������к�
[IN] pbData:LICENSE�ļ�����
[IN] czDataLen:LICENSE�ļ����ݳ���
����ֵ��0=�ɹ�,-1=ǩ������ȷ,-2=�������кŲ�ƥ��,-9=��������,-10=��������
*/
int verifylicense(unsigned char *pbData,int czDataLen);
/*
verifyDate ��֤��ǰ�����Ƿ���LICENSE������
[IN] pbData:LICENSE�ļ�����
[IN] pDate:ϵͳ��ǰ���ڣ���ʽ���ַ���"20100526"
����ֵ��0=�ɹ�,-1=С����ʼ����,-2=������ֹ����,-9=��������,-10=��������
*/
int verifyDate(unsigned char *pbData,char *pDate);
/*
getIndate �����Ч��
[IN] pbData:LICENSE�ļ�����
[OUT]pIndate:��Ч��,"2010052620130526",0-7��ʼ����,8-15��ֹ����
����ֵ��0=�ɹ�,-9=��������,-10=��������
*/
int getIndate (unsigned char *pbData,char *pIndate);
/*
getMaxUserNumA ���-A����û���
[IN] pbData:LICENSE�ļ�����
[OUT]pzNum:-A����û���
����ֵ��0=�ɹ�,-9=��������,-10=��������
*/
int getMaxUserNumA(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxUserNumB ���-B����û���
[IN] pbData:LICENSE�ļ�����
[OUT]pzNum:-B����û���
����ֵ��0=�ɹ�,-9=��������,-10=��������
*/
int getMaxUserNumB(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxUserNumC ���-C����û���
[IN] pbData:LICENSE�ļ�����
[OUT]pzNum:-C����û���
����ֵ��0=�ɹ�,-9=��������,-10=��������
*/
int getMaxUserNumC(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxUserNumD ���-D����û���
[IN] pbData:LICENSE�ļ�����
[OUT]pzNum:-D����û���
����ֵ��0=�ɹ�,-9=��������,-10=��������
*/
int getMaxUserNumD(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxOnlineNumB ���-B��������û���
[IN] pbData:LICENSE�ļ�����
[OUT]pzNum:-B��������û���
����ֵ��0=�ɹ�,-9=��������,-10=��������
*/
int getMaxOnlineNumB(unsigned char *pbData,unsigned int *pzNum);
/*
getMaxOnlineNumC ���-C����û���
[IN] pbData:LICENSE�ļ�����
[OUT]pzNum:-C����û���
����ֵ��0=�ɹ�,-9=��������,-10=��������
*/
int getMaxOnlineNumC(unsigned char *pbData,unsigned int *pzNum);
/*
getDeviceNo ����豸��
[IN] pbData:LICENSE�ļ�����
[OUT]pDevNo:�豸�ŵ�ַ,pDevNo=NULLʱֻ�õ��豸�ų���
[OUT]pzNoLen:�豸�ų���
����ֵ��0=�ɹ�,-9=��������,-10=��������
*/
int getDeviceNo(unsigned char *pbData,char *pDevNo,int *pzNoLen);
/*
test ���ò���
����ֵ�������1
*/
int test();

#endif

