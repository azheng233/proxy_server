#include <stdio.h>

int main()
{
	char a[300];

	printf("POST http://pv.csdn.net/csdnbi HTTP/1.1\r\n"
		"Host: pv.csdn.net\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:38.0) Gecko/20100101 Firefox/38.0\r\n"
		"Accept: */*\r\n"
		"Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3\r\n"
		"Accept-Encoding: gzip, deflate\r\n"
		"Content-Type: text/plain;charset=UTF-8\r\n"
		"Referer: http://blog.csdn.net/zztfj/article/details/5297233\r\n"
		"Content-Length: 225\r\n"
		"Origin: http://blog.csdn.net\r\n"
		"Cookie: Hm_lvt_6bcd52f51e9b3dce32bec4a3997715ac=1517821461,1517821554; uuid_tt_dd=10_20259810860-1520921756876-9\r\n"
		"55403; dc_session_id=10_1520921756876.299249; dc_tos=p5ty9u\r\n"
		"Connection: keep-alive\r\n"
		"Pragma: no-cache\r\n"
		"Cache-Control: no-cache\r\n\r\n");
	return 1;
}
