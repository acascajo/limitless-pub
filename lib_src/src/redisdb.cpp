#include <iostream>
#include "redisdb.hpp"
#include <vector>

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

bool SendToRedis_old(redisContext* c, std::string strcmd){
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


bool SendToRedis(redisContext* context, const std::string& hostname, const std::vector<std::string>& labels,
    const std::vector<std::string>& values)
{
    if (context == nullptr) {
        std::cerr << "Redis connection is null\n";
        return false;
    }

    if (context->err) {
        std::cerr << "Redis connection error: "
                  << context->errstr << '\n';
        return false;
    }

    if (labels.size() != values.size()) {
        std::cerr << "Error: there are " << labels.size()
                  << " labels and " << values.size()
                  << " values\n";
        return false;
    }

    if (labels.empty()) {
        std::cerr << "Error: empty labels\n";
        return false;
    }

    std::vector<std::string> arguments;
    arguments.reserve(2 + labels.size() * 2);

    arguments.emplace_back("HSET");
    arguments.emplace_back("monitor:" + hostname);

    for (std::size_t i = 0; i < labels.size(); ++i) {
        if (labels[i].empty()) {
            std::cerr << "Error: empty label in position "
                      << i << '\n';
            return false;
        }

        arguments.push_back(labels[i]);
        arguments.push_back(values[i]);
    }

    std::vector<const char*> argv;
    std::vector<std::size_t> argvLengths;

    argv.reserve(arguments.size());
    argvLengths.reserve(arguments.size());

    for (const std::string& argument : arguments) {
        argv.push_back(argument.data());
        argvLengths.push_back(argument.size());
    }

    redisReply* reply = static_cast<redisReply*>(
        redisCommandArgv(
            context,
            static_cast<int>(argv.size()),
            argv.data(),
            argvLengths.data()
        )
    );

    if (reply == nullptr) {
        std::cerr << "Failed to execute Redis command";

        if (context->err) {
            std::cerr << ": " << context->errstr;
        }

        std::cerr << '\n';
        return false;
    }

    bool success = true;

    if (reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "Redis error: "
                  << (reply->str != nullptr
                          ? reply->str
                          : "Unknown error")
                  << '\n';

        success = false;
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        /*std::cout << "HSET correct. New fields: "
                  << reply->integer << '\n';*/
    } else {
        std::cerr << "Unexpected Redis reply type: "
                  << reply->type << '\n';
        success = false;
    }

    freeReplyObject(reply);
    return success;
}

void FreeRedis(redisContext* c){
//void FreeRedis(){
    if (c != nullptr)
        if (!c->err)
            redisFree(c);
}
