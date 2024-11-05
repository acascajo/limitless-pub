#ifndef CLIENT_MONITOR_H
#define CLIENT_MONITOR_H


#include "network_data.hpp"
#include "daemon_global_params.hpp"
#define BUFFER_SIZE 512

typedef struct packet{
	char title [256];
	char xlabel [256];
	char ylabel [256];
	char style [8];
	int filesize;
}pckt;


//auxiliar methods, forward declarations

//int debug = 0;


/**
         Send configuration packet trough the socket.

         @param hw_conf Structure containing the information about th hardware configuration of the system.
         @return 0 if no error occurred, -1 otherwise.

*/
int send_conf_packet(Hw_conf* hw_conf);

/**
         Send monitorization packet trough the socket.

         @param hw_conf Structure containing the information about the monitorized variables.
         @return 0 if no error occurred, -1 otherwise.

*/
int send_monitor_packet(Packed_sample &ps, int tmr);
int send_monitor_generic(Packed_sample &ps, int tmr);

/**
	Initialize socket for communication.
        @param server String containing the Ip address to the server towars the client will
        communicate.
        @param port Integer with the number of the port in which the server will be listening.
        @return Error in case the socket was not correctly created.
*/
int init_socket(char *server, int port);
int init_socket_backup1(char *server, int port);
int init_socket_backup2(char *server, int port);

/**
	Close socket used in client.
	@return -1 Error in case the socket could not be closed, otherwise 0.

*/
int close_socket();

#endif
