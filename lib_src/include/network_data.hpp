
#ifndef NETWORK_DATA_H
#define NETWORK_DATA_H

#include "system_features.hpp"
#include "Packed_sample.hpp"
#include <sys/types.h>
#include <sys/socket.h>

#define CONF_PACKET_SIZE 8
#define IP_SIZE 4
#define BTYPE_POS 0 
#define MAX_PACKET_SIZE 1400 /*Around 1437 for UDP connection in ethernet*/
#define SIZE_PACKED_PERCENTAGE 1

//int conf_packet_size = 0;


int obtain_sample_from_packet(unsigned char * buffer, Packed_sample &mon_sample);

int obtain_conf_from_packet(unsigned char * conf_packed, Hw_conf* hw_conf, std::string client_addr);

time_t obtain_time_from_packet(unsigned char * buffer);
/* This function keeps sending bytes from a buffer until the specified amount	*
 * is actually sent. It is designed to overcome the problem of send function	*
 * being able to return having sent only a portion of the specified bytes.	*
 * n: specifies the amount of bytes to send from the first positions 		*
 * of the buffer.								*
 * On success n is returned.							*
 * On error -1 is returned.							*/
int sendn(int socket_descriptor, void *buf, int n, struct sockaddr_in *out_addr);

/* This function keeps receiving bytes from a buffer until the specified amount	*
 * is actually received. It is designed to overcome the problem of recv function*
 * being able to return having received only a portion of the specified bytes.	*
 * n: specifies the amount of bytes to receive in the buffer.			*
 * On success n is returned.							*
 * On premature close/shutdown on peer 0 is returned.				*
 * On error -1 is returned.							*/
int recvn(int socket_descriptor, void *buf, int n);

struct handle_args{
    char client_IP[30];
    unsigned char * buffer;
    ssize_t size;
    int socket;
    struct sockaddr_in* client_addr;

};
int manage_monitoring_packet (unsigned char * buffer, ssize_t size, const std::string &clientIP);
void * manage_allocate_packet(unsigned char * buffer, struct sockaddr_in si_other, int socket);
void * manage_query_packet(unsigned char * bufferstruct, struct sockaddr_in si_other, int socket);
int obtainHostNameByIP(char const * ip, char * host);
void managePrometheusServer();
void managePrometheusServerGeneric();
int manage_generic_packet (struct handle_args *info);
void createCounterFamilyForNode(std::string nodename, int nio, int nnet);
std::vector<int> getCollectedMetrics();
#endif
