#ifndef DAEMON_MONITOR_H
#define DAEMON_MONITOR_H


#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include  <dirent.h>
#include "common.hpp"

using namespace std;


#define BUFF_SIZE 1024
#define MAX_NAME_LEN 128
#define MAX_SAMPLES 127

/*CODES TO SENT PACKETS*/
#define CONFIG_PACKET 0
#define QUERY_CONFIGURATION 255
#define QUERY_STATE_NOW 254
#define QUERY_AVG 253
#define QUERY_ONE_NOW 252

/*OTHER CODES TO DB*/
#define QUERY_PACKET_SIZE 1
#define QUERY_PACKET_SPECT 17
#define QUERY_ALLOCATE_SIZE 512 //code:Np:profile
#define QUERY_ALLOCATE_RESPONSE 200

#define INIT_FILE "init.dae"
#define CONF_FILE "conf.dae"
#define INIT_SERV_FILE "init.serv"
#define CONF_SERV_FILE "conf.serv"

/**
        Prints manual and interpretation of results.

*/
void print_manual();


/**
        Computes the size of each package from the number of devices, interfaces, and samples that are sent in each packet.

*/
void calculate_packed_size();

#endif
