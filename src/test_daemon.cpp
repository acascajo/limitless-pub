/**
* DaemonMonitor		Provides information about CPU, IO, Networks and GPUs
+ stats in a node.
* @version              v0.1
*/


#ifndef DAEMON_INFO
#define DAEMON_INFO

#include <string>
#include <iostream>
#include "daemon_monitor.hpp"
#include "Packed_sample.hpp"
#include "cliente_monitor.hpp"

using namespace std;

/**************************************************************************************************************/

int mode = -1;

Packed_sample *ps;

int threshold = 0;
int opt, port = 0;
char *server = nullptr;
int error = 0;
int tmrd = 0;


int n_devices_io = 0, n_cpu = 0,  n_cores = 0, n_interfaces = 0, n_samples = 0, n_core_temps = 0;

int main(int argc, char *argv[]){
  char*port_s = nullptr;

  if (argc != 5 ) {
    cout << "Usage: (LDS) ./DaeMon -p <port dest> -s <ip dest>" << endl;
    exit(0);
  }

  while ((opt = getopt(argc, argv, "ds:p:s:")) != -1) {
    switch (opt) {
      case 'p':
        port_s = optarg;
        port = strtol(optarg, NULL, 10);
        printf("Dest Port %s\n", port_s);
        break;
      case 's':
        server = optarg;
        printf("Dest Server %s\n", server);
        break;
      default:
        return -1;
    }
  }

  if (server == NULL) {
    std::cerr << "ERROR: NO address for master was given." << std::endl;
    exit(-1);
  }
  if ((port < 1024) || (port > 65535)) {
    fprintf(stderr, "Error: Port must be in the range 1024 <= port <= 65535");
    exit(-1);
  }

  error = init_socket(server, port);
  if (error == -1) {
    std::cerr << "Socket could not be initialized" << endl;
    return error;
  }

  while(1) {
    ps = new Packed_sample();
    ps->pack_sample_generic(
      "capo14;18446744073709551615;campo2;543;campo3;18446744073709551615;campo4;1111;campo5;123;campo6;543;");
    ps->packed_ptr++;
    send_monitor_generic(*ps, tmrd);
    sleep(3);
  }

}


#endif
