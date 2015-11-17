#include <mysql/mysql.h>
#include "mysqlutil.h"
#ifndef NULL
#define NULL 0
#endif
#define __DEBUG(format, ...) printf("FILE: "__FILE__", LINE: %d: "format"/n", __LINE__, ##__VA_ARGS__)
int fetchIOSPushMessageDetailInfo(long id,mysql_row_result_t** result)
{
    //获得一个连接
    int fd = getConnect();
    int row = queryResult(fd,result,"select id, token, message, app_id, app_type, uid, msg_id, source from iOS_push_message where id=%d AND (send_state=%d OR send_state=0)",id,3);    
	returnMysql(fd);
    return row;
}


void insertPushSuccess(char* msgId, char* source, int platform, unsigned char state)
{
    if (msgId == NULL || source == NULL)
    {
        return;
    }
    //获得一个连接
    int fd = getConnect();
    mysql_row_result_t* result = NULL;
    int row = queryResult(fd,&result,"SELECT id, state FROM push_success \
					WHERE source=%s AND msg_id = %s",source,msgId);
    int i=0;
    for(i=0; i<row; i++)
    {
        char* temp = getContentById(i,0,result);
        int id = temp==NULL?0:atoi(temp);
        temp = getContentById(i,1,result);
        int s = temp==NULL?0:atoi(temp);
        if (s == 0 || s == state)
        {
            goto return_result;
        }
        else
        {
            int eret = executeEx(fd,"UPDATE push_success SET state = %d, update_time = UNIX_TIMESTAMP() WHERE id=%d",state,id);
            goto return_result;
        }
    }
    int eret = insert(fd,"INSERT INTO push_success \
		  (msg_id,source,platform,state,in_time,update_time) VALUE \
		  (%s,%s,%d,%d,UNIX_TIMESTAMP(),UNIX_TIMESTAMP())" ,
                      msgId,source,platform,state);
return_result:
    freeResult(result);
    returnMysql(fd);
}

void updatePushMessageDetailInfoStatus(mysql_row_result_t* info) 
{
    if(info == NULL)
        return;
    int fd = getConnect();
    char* temp = getContentByName(0,"sendState",info);
    int sendstate = temp==NULL?0:atoi(temp);
    temp = getContentByName(0,"id",info);
    int id = temp==NULL?0:atoi(temp);
    executeEx(fd,"update iOS_push_message set send_state = %d,\
		 op_time = UNIX_TIMESTAMP(CURRENT_TIMESTAMP) where id = %d",sendstate,id);
	returnMysql(fd);

}