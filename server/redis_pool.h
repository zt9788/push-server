#ifndef REDIS_POOL_H
#define REDIS_POOL_H

//#include "log.c"
int createRedisPool(char* redis_server,int redis_port,int max_thr_num);
redisContext* getRedis(int* redisId);
void returnRedis (int redisID);
void destroy_redis_pool();

#endif /* !REDIS_POOL_H */