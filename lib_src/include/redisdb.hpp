#ifndef REDISDB_HPP
#define REDISDB_HPP
#include <hiredis/hiredis.h>
#include <vector>

//bool InitializeRedisConnection(redisContext* c, std::string dbaddress);
//bool InitializeRedisConnection(std::string dbaddress);
redisContext* InitializeRedisConnection(std::string dbaddress);
redisContext* InitializeRedisConnection_metricai(const std::string& redis_host, const std::string& redis_password, int redis_port);

bool SendToRedis_old(redisContext* c, std::string strcmd);
//bool SendToRedis(std::string strcmd);

bool SendToRedis(
    redisContext* context,
    const std::string& hostname,
    const std::vector<std::string>& labels,
    const std::vector<std::string>& values);

void FreeRedis(redisContext* c);
//void FreeRedis();


#endif //REDISDB_HPP
