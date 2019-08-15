

typedef struct startline {
	char method[10];	//请求方式
	char url[2048];		//url
	char dns[128];		//域名
	int port;		//端口
	char resource[2048];	//资源
	char version[10];	//协议版本
}start;

typedef struct request_header {
	char *key;			//字段名
	char *value;			//字段值
	struct request_header *next;	//哈希值相同的下一个字段
}header;

typedef struct hash_table {
	header *index[BUCKET];
} hash_table;
