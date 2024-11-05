#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include "servidor_monitor.hpp"
#include <csignal>
#include <wordexp.h>
#include <sysexits.h>

int opt, port = -1, master_port = -1;
char *server = nullptr, *port_s = nullptr, *master_port_s = nullptr;
int master_retransmitter=0, tmrflag = 0;


void intHandler(int dummy) {
    std::cout << "Finishing tasks..." << std::endl;
    exit(0);
}


int main(int argc, char *argv[]) {
  time_t time;
  int error = 0;

  signal(SIGINT, intHandler);

  if (argc != 7 && argc != 3) {
    cout << "Usage: (LDS) ./Test_server -p <port listener> || (LDA) ./Test_server -s <ip dest> -r <port dest> -p <port listener>" << endl;
    exit(0);
  }

  while ((opt = getopt(argc, argv, "ds:p:r:s:")) != -1) {
    switch (opt) {
      case 'p':
        port_s = optarg;
        port = strtol(optarg, NULL, 10);
        printf("Port %s\n", port_s);
        break;
        /* INTERMEDIATE NODE */
      case 'r':
        master_port_s = optarg;
        master_port = strtol(optarg, NULL, 10);
        printf("Port socket master %s\n", master_port_s);
        master_retransmitter = 1;
        break;
      case 's':
        server = optarg;
        printf("Master Server %s\n", server);
        master_retransmitter = 1;
        break;
        /*INTERMEDIATE NODE */
      default:
        return -1;
    }
  }


  if ((port != -1 && (port < 1024) || (port > 65535)) /*|| (client_port != -1 && (client_port < 1024) || (client_port > 65535))
        || (flexmpi_port != -1 && (flexmpi_port < 1024) || (flexmpi_port > 65535))*/) {
    fprintf(stderr, "Error: Port must be in the range 1024 <= port <= 65535\n");
    exit(EX_USAGE);
  }



  setTMRvalue(tmrflag);

  if (server != nullptr && master_port != -1) {
    //intermediate node
    cout << "Intermediate server" << endl;
    error = init_server(port);
    if (error == -1) {
      exit(error);
    }

    error = initialize_master_socket(server, master_port);
    if (error == -1) {
      exit(error);
    }

    /* Threads to aggregate data and send the messages*/
    intermediate_processor_agg();
    //intermediate_processor_send(); Aggregation also sends the messages
    /*Infinite loop, manage termination signal*/
    error = manage_intermediate_server_udp();


  } else {
    //master server
    cout << "Master server" << endl;

    error = init_server(port);
    if (error == -1) {
      exit(error);
    }

    processor();
    //prometheusWorker();
    prometheusWorkerGeneric();
    /*Infinite loop, manage termination signal*/
    error = manage_server_udp(master_retransmitter);
  }


  return 0;
}
