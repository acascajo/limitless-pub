#ifndef LIBRARY_H
#define LIBRARY_H

#include <string>
#include "sqlite3.h"
#include <list>
#include <vector>

#define SIZE_LOCAL_BUFFER 8192

/*Profile*/
#define CPU 0
#define MEM 1
#define IO 2
#define GPU 3

typedef std::vector<std::string>  StringVector;


/* Initialization and basic ops. */
int db_open();
int db_close();
//int db_initialization(std::list<std::string> nodes);
int db_initialization();
void indexInit(std::list<std::string> nodes);


/* Insert into tables */
int db_insert_CONF(std::string id, int ncpus, int ncores, int nram, int ngpus, int nio, int nnets);
int db_insert_CR(std::string id, int iosat, int cpusat, int ramsat, int gpusat, int temp, int net);
int db_insert_coreload(std::string id, std::vector<int> core_load);
int db_insertSummary(std::string id, int cpu, int ram, int io, int gpu, int temp, int net);
int db_insert_IOdev(std::string id_node, std::string id_io, int writes, int io_perc);
int db_insert_Net(std::string id_node, std::string id_net, int usage, int speed);
int db_insert_Cores(std::string id_node, std::string id_core, int temperature, int pow, int temp_perc);
int db_insert_Gpu(std::string id_node, std::string id_gpu, int temperature, int mem_usage, int pow, int gen_usage);


/* Update tables */
int db_update_CR(std::string id, int lastTime, int newTime, int iosat, int cpusat, int ramsat, int gpusat, int temp, int net);
int db_update_IOdev(std::string id_node, std::string id_io, int writes, int io_perc);
int db_update_Net(std::string id_node, std::string id_net, int usage);
int db_update_Cores(std::string id_node, std::string id_core, int temperature, int pow, int temp_perc);
int db_update_Gpu(std::string id_node, std::string id_gpu, int temperature, int mem_usage, int pow, int gen_usage);
int db_update_summary(std::string id, int cpusat, int ramsat, int iosat, int gpusat, int tempsat, int netsat) ;


/* Query to get all data or some fields */
int db_query_all();
char* db_query_all_Now();
char* db_query_all_CR();
char* db_query_all_Conf();
int db_query_all_IO();
int db_query_all_Cores();
int db_query_all_GPU();
int db_query_all_Net();
int db_query_table(char* table);
char * db_query_all_Summary();
char* db_query_allocate(int np, int profile);
/*int db_query_count(char *table, int id);
int db_query_spec_CR(int id, int time, int ncpus, int ncores, int nram, int ngpus, int nio, int nnets, int netspeed,
                     int cpusat, int ramsat, int iosat, bool showAll);*/
char* db_query_spec_CR(std::string id, int time, int iouse, int cpuuse, int ramuse);
int db_query_spec_Conf(std::string id, int ncpus, int ncores, int nram, int ngpus, int nio, int nnets);
char* db_query_coreload(std::string ip);


/* Callbacks para query */
static int callback(void *NotUsed, int argc, char **argv, char **azColName);
static int callback_buffer(void *NotUsed, int argc, char **argv, char **azColName);
int callback_avg(void *NotUsed, int argc, char **argv, char **azColName);
//int callbackCount(void *NotUsed, int argc, char **argv, char **azColName);


int sql_exec(sqlite3* db, const char* sql, char * zErrMsg);
int sql_exec_buffer(sqlite3 *db, const char *sql, char *zErrMsg);
//int sql_exec_count(sqlite3 *db, const char *sql, char *zErrMsg);

int backupDb(/*sqlite3 *pDb,*/ const char *zFilename);//, void(*xProgress)(int, int));

/* Methods to test in terminal  */
void processDB();
void optionShowInfo();
void showInfo();
void clearScreen();
StringVector explode(const std::string & str, char separator);
void updateSummary();
void *resetCounter();
void setDefaultTimer(time_t time);


#endif
