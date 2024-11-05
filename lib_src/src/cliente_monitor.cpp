#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <strings.h>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include "cliente_monitor.hpp"
#include <wordexp.h>

#define BUFFER_SIZE 512

using namespace std;
/*Socket  from which the client will send its information*/
int sd=0, sd1=0, sd2=0;
struct sockaddr_in server_addr;
struct sockaddr_in server_bk_1;
struct sockaddr_in server_bk_2;
int fail_counter = 0;


void usage(char *program_name) {
	printf("Usage: %s [-d] -s <server> -p <port>\n", program_name);
}

int send_monitor_generic(Packed_sample &ps, int tmr){
  int error=0;
  unsigned char * buffer = NULL;
  int bsent = 0;
  buffer = (unsigned char *) calloc(MAX_PACKET_SIZE, sizeof(char));
  if(buffer==NULL){
    //std::cerr << "Error creating monitor packet." << endl;
    error = -1;
  }else{
    ps.sample_size = ps.packed_ptr;
    /*Creating the packet and sending it*/
    //int packet_size = create_generic_packet(ps, buffer);

    // 3 tries to connect on each server
    for (int i = 0; i < 9; i++) {
      struct sockaddr_in socketsend;
      if (i<3)
        socketsend = server_addr;
      else if (i>=3 && i<6)
        socketsend = server_bk_1;
      else
        socketsend = server_bk_2;
      //bsent = sendn(sd, buffer, packet_size, &socketsend);
      bsent = sendn(sd, ps.packed_buffer, ps.sample_size, &socketsend);
      if (bsent != ps.sample_size) {
        error = -1;
      } else {
        error = 0;
        if (tmr != 1)
          break;
      }
    }
  }

  free(buffer);
  return error;
}

int send_monitor_packet(Packed_sample &ps, int tmr){
	int error=0;
	unsigned char * buffer = NULL;
	int bsent = 0;
	buffer = (unsigned char *) calloc(MAX_PACKET_SIZE, sizeof(char));
	if(buffer==NULL){
		//std::cerr << "Error creating monitor packet." << endl;
		error = -1;
	}else{
		ps.calculate_sample_size();
		/*Creating the packet and sending it*/
		int packet_size = create_sample_packet(ps, buffer);

        // 3 tries to connect on each server
		for (int i = 0; i < 9; i++) {
		    struct sockaddr_in socketsend;
		    if (i<3)
		        socketsend = server_addr;
		    else if (i>=3 && i<6)
		        socketsend = server_bk_1;
		    else
		        socketsend = server_bk_2;
            bsent = sendn(sd, buffer, packet_size, &socketsend);
            if (bsent != packet_size) {
                //std::cerr << "Error sending monitoring packet." << endl;
                error = -1;
            } else {
                error = 0;
                if (tmr != 1)
                    break;
            }
        }
    }

	free(buffer);
	return error;
}

/**
	 Send configuration packet trough the socket. 

	 @param hw_conf	Structure containing the information about th hardware configuration of the system.
 	 @return 0 if no error occurred, -1 otherwise. 

*/
int send_conf_packet(Hw_conf* hw_conf) {
    int error = 0;
    char *conf_packet = NULL;
    int bsent = 0;
    conf_packet = (char *) calloc((CONF_PACKET_SIZE + (hw_conf->hostname.length() + 1)),
                                  sizeof(char));
    if (conf_packet == NULL) {
        std::cerr << "Error creating configuration packet." << endl;
        error = -1;
    } else {
        /*Creating the packet and sending it*/
        int size = create_conf_packet(*hw_conf, conf_packet);
        bsent = sendn(sd, conf_packet, (CONF_PACKET_SIZE + (hw_conf->hostname.length() + 1)),
                      &server_addr);
        if (bsent != (CONF_PACKET_SIZE + (hw_conf->hostname.length() + 1))) {
            std::cerr << "Error sending configuration packet." << endl;
            error = -1;
        }

        //TMR with other 2 servers
        bsent = sendn(sd1, conf_packet, (CONF_PACKET_SIZE + (hw_conf->hostname.length() + 1)),
                      &server_bk_1);
        if (bsent != (CONF_PACKET_SIZE + (hw_conf->hostname.length() + 1))) {
            std::cerr << "Error sending configuration packet." << endl;
            error = -1;
        }

        bsent = sendn(sd2, conf_packet, (CONF_PACKET_SIZE + (hw_conf->hostname.length() + 1)),
                      &server_bk_2);
        if (bsent != (CONF_PACKET_SIZE + (hw_conf->hostname.length() + 1))) {
            std::cerr << "Error sending configuration packet." << endl;
            error = -1;
        }
    }
    free(conf_packet);
    return error;
}

/**

	Initialize socket for communication.
	@param server String containing the Ip address to teh server towars the client will 
	communicate. 
	@param port_s String with the number of the port in which the server will be listening. 
	@return Erroe in case the socket was not correctly created. 
*/
int init_socket(char *server, int port){

	//char *server, *port_s;
	int opt=0;
	struct sockaddr_in  client_addr;
	struct hostent *hp=NULL;
	int error =0;
	int num[2], res;

	if((sd = socket(AF_INET, SOCK_DGRAM, 0))< 0){
		std::cerr << " Could not connect to master. " << std::endl;
		//fprintf(stderr, "\n");
		exit(0);
	}
	hp = gethostbyname (server);
     
    //setting up sockaddr_in to be able to connect using it
    bzero((char *)&server_addr, sizeof(server_addr));
    if(hp != NULL){
        memcpy(&(server_addr.sin_addr), hp->h_addr, sizeof(hp->h_length));
    }else{
        std::cerr << "Can not determine the address." << std::endl;
        error = -1;
		return error;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port  = htons(port);

    //connecting
    if(connect(sd,(struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
        printf("Error connecting to the server...\n");
        error = -1;
    }   
	return error;
}

/**

	Initialize socket for communication when master server fails.
	@param server String containing the Ip address to teh server towars the client will
	communicate.
	@param port_s String with the number of the port in which the server will be listening.
	@return Erroe in case the socket was not correctly created.
*/
int init_socket_backup1(char *server, int port){
    int opt=0;
    struct hostent *hp=NULL;
    int error =0;
    int num[2], res;

    if((sd1 = socket(AF_INET, SOCK_DGRAM, 0))< 0){
        std::cerr << " Could not connect to master. " << std::endl;
        //fprintf(stderr, "\n");
        exit(0);
    }
    hp = gethostbyname (server);

    //setting up sockaddr_in to be able to connect using it
    bzero((char *)&server_bk_1, sizeof(server_bk_1));
    if(hp != NULL){
        memcpy(&(server_bk_1.sin_addr), hp->h_addr, sizeof(hp->h_length));
    }else{
        std::cerr << "Can not determine the address." << std::endl;
        error = -1;
        return error;
    }
    server_bk_1.sin_family = AF_INET;
    server_bk_1.sin_port  = htons(port);

    //connecting
    if(connect(sd,(struct sockaddr *) &server_bk_1, sizeof(server_bk_1)) == -1){
        printf("Error connecting to the server...\n");
        error = -1;
    }
    return error;
}

/**

	Initialize socket for communication when master server and backup server fails. .
	@param server String containing the Ip address to teh server towars the client will
	communicate.
	@param port_s String with the number of the port in which the server will be listening.
	@return Erroe in case the socket was not correctly created.
*/
int init_socket_backup2(char *server, int port){
    int opt=0;
    struct hostent *hp=NULL;
    int error =0;
    int num[2], res;

    if((sd2 = socket(AF_INET, SOCK_DGRAM, 0))< 0){
        std::cerr << " Could not connect to master. " << std::endl;
        //fprintf(stderr, "\n");
        exit(0);
    }
    hp = gethostbyname (server);

    //setting up sockaddr_in to be able to connect using it
    bzero((char *)&server_bk_2, sizeof(server_bk_2));
    if(hp != NULL){
        memcpy(&(server_bk_2.sin_addr), hp->h_addr, sizeof(hp->h_length));
    }else{
        std::cerr << "Can not determine the address." << std::endl;
        error = -1;
        return error;
    }
    server_bk_2.sin_family = AF_INET;
    server_bk_2.sin_port  = htons(port);

    //connecting
    if(connect(sd,(struct sockaddr *) &server_bk_2, sizeof(server_bk_2)) == -1){
        printf("Error connecting to the server...\n");
        error = -1;
    }
    return error;
}

int close_socket(){
	int error=0;
	error=close(sd);
	if (error ==-1){
		cerr << "Error happened when closing socket."<< endl;
	}
	return error;	
}

