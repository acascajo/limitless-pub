#ifndef SERVER_MONITOR_H
#define SERVER_MONITOR_H

#include "network_data.hpp"

#define CPU_HOT 1
#define MEM_HOT 2
#define IO_HOT 3
#define NET_HOT 4
#define CACHE_HOT 5

typedef struct req_inf{
	int socket;
	char address [16];//max address length for ipv4 is strlen("255.255.255.255") 15 characters+null terminator = 16
	int port;
}req_inf;


int init_server(int port);
int manage_server();

void recoveryMode(char* elasticsearch_add);
int manage_server_udp(int master_ret);
int manage_intermediate_server_udp();
int initialize_master_socket(char *server, int port);
int initialize_backup1_socket(char *server, int port);
int initialize_backup2_socket(char *server, int port);
int init_client_server(int port);
int init_flexmpi_server(int port);
void setTMRvalue(int val);
int manage_client_server();
void sendConfNow(struct handle_args ha);//unsigned char * mes);
void sendAllMessages();
void retransmitter(void * global);
void *dispatcher(struct handle_args* global);//void * global);
void * processor();
void * intermediate_processor_send();
void * intermediate_processor_agg();
void packetAggregation();
int queue_empty();
void hotspotNotification(std::string ip, int profile, char* mes);
void sendConfNow(struct handle_args *global);
void setHotspotsValueNotification(int mem, int cache, int net);
void setElasticSearchAdd(char * add);
char* getElasticSearchAdd();
int getMemHotValue();
int getNetHotValue();
int getCacheHotValue();
void analyticFunction(int hbTime);
void manage_analytic_server_udp();
void prometheusWorker();
void prometheusWorkerGeneric();
void managePrometheusServer();

#endif
