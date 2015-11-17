#ifndef MYSQLUTIL_H_
#define MYSQLUTIL_H_


typedef struct mysql_row_result{
	int rowid;
	int columncount;
	struct mysql_column_result_t* columns;
	struct mysql_row_result* next;
}mysql_row_result_t;

typedef struct mysql_column_result{
	int columnid;
	char columnname[255];
	char* content;
	struct mysql_result* next;
}mysql_column_result_t;
//创建连接池 
int createMysqlPool(int max_pool);
void destroyMysqlPool();
//默认连接 
void init(char* serverip,
					char* dbuser,
					char* pass,
					char* dbname, 
					int port,
					int attri,
					int masterOrSalve);
//添加主从列表
//void addserverList(char* serverip,char* dbuser,char* pass,char* dbname, int port,	int attri,int masterOrSalve);
//获得一个连接 
int getConnection(char* serverip,
					char* dbuser,
					char* pass,
					char* dbname,
				    int port,
					int attri,
					int masterOrSalve);
					
int getConnectByDBType(int masterOrSalve);
int getConnect(); 
//执行update和insert 
int execute(int fd,char* sql);
int executeEx(int fd,char* sqlformat,...);	
//插入并返回key 
long long  insert(int fd,char* sql,...);
int commit(int fd);
//执行select ,返回记录条数,和结果集 
int queryResult(int fd,mysql_row_result_t** result,char* sqlformat,...);
//结果集处理方法 
char* getContentById(int row,int columnid,const mysql_row_result_t* result);
char* getContentByName(int row,char* columnname,const mysql_row_result_t* result);
char** getContent2Array(int row,const mysql_row_result_t* result,char** des);
void freeArray(char** src,int columcount);
void freeResult(mysql_row_result_t* result);
//返回连接池 
void returnMysql(int fd);//close mysql

#endif /* !MYSQLUTIL */
				