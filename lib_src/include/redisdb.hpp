#ifndef REDISDB_HPP
#define REDISDB_HPP
#include <hiredis/hiredis.h>

//bool InitializeRedisConnection(redisContext* c, std::string dbaddress);
//bool InitializeRedisConnection(std::string dbaddress);
redisContext* InitializeRedisConnection(std::string dbaddress);

bool SendToRedis(redisContext* c, std::string strcmd);
//bool SendToRedis(std::string strcmd);

void FreeRedis(redisContext* c);
//void FreeRedis();


#endif //REDISDB_HPP
