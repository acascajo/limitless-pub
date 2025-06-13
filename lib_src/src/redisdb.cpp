#include <iostream>
#include "redisdb.hpp"

//Hiredis link
//#include <hiredis/hiredis.h>

using namespace std;

//redisContext* c;

//bool InitializeRedisConnection(redisContext* c, std::string dbaddress){
redisContext* InitializeRedisConnection(std::string dbaddress){
    redisContext* c = redisConnect(dbaddress.c_str(), 6379); //localhost for debug
    if (c == nullptr || c->err) {
        if (c) {
            cerr << "Connection error: " << c->errstr << endl;
            redisFree(c);
            return nullptr;
        } else {
            cout << "Connection error: can't allocate redis context\n";
            return nullptr;
        }
    } else {
        cout << "Redis connected \n";
        return c;
    }
}

bool SendToRedis(redisContext* c, std::string strcmd){
//bool SendToRedis(std::string strcmd){
    if (c == nullptr) {
	    cerr << "Redis connection is null\n";
	    return false;
    }

    if (!c->err) {
        redisReply * reply = static_cast<redisReply *>(redisCommand(c, strcmd.c_str()));
        if (reply == nullptr) {
            std::cerr << "Failed to execute Redis command\n";
            freeReplyObject(reply);
            return false;
        }
        freeReplyObject(reply);
        return true;
    }
    return false;
}

void FreeRedis(redisContext* c){
//void FreeRedis(){
    if (c != nullptr)
        if (!c->err)
            redisFree(c);
}
