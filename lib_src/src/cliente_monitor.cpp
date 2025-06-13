#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <chrono>
#include "cliente_monitor.hpp"
#include <memory>
#include <string>
#include <array>

//Inlfuxdb link
#include "influxdb.hpp"

#define BUFFER_SIZE 512

using namespace std;
/*Socket  from which the client will send its information*/
int sd=0, sd1=0, sd2=0;
struct sockaddr_in server_addr;
struct sockaddr_in server_bk_1;
struct sockaddr_in server_bk_2;
int fail_counter = 0;


/**
 * Send generic packet trough the socket.
 * @param ps Structure containing the information
 * @param tmr if TMR is enabled
 * @return 0 if no error occurred, -1 otherwise.
 */
int send_monitor_generic(Packed_sample &ps, int tmr){
  int error=0;
  unsigned char * buffer = nullptr;
  buffer = (unsigned char *) calloc(MAX_PACKET_SIZE, sizeof(char));
  if(buffer==nullptr){
    //std::cerr << "Error creating monitor packet." << endl;
    error = -1;
  }else{
	  int bsent = 0;
	  ps.sample_size = ps.packed_ptr;
    /*Creating the packet and sending it*/
    //int packet_size = create_generic_packet(ps, buffer);

    // 3 tries to connect on each server
    for (int i = 0; i < 9; i++) {
      struct sockaddr_in socketsend;
      if (i<3)
        socketsend = server_addr;
      else if (i >= 3 && i<6)
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

/**
 * Send monitoring packet trough the socket.
 * @param ps Structure containing the information
 * @param tmr if TMR is enabled
 * @return 0 if no error occurred, -1 otherwise.
 */
int send_monitor_packet(Packed_sample &ps, int tmr){
	// Allocate memory for the packet using std::unique_ptr
	auto buffer = std::make_unique<unsigned char[]>(MAX_PACKET_SIZE);

	if (!buffer) {
		std::cerr << "Error creating monitor packet." << std::endl;
		return -1;
	}

	int error = 0;

	// Calculate sample size and create the packet
	ps.calculate_sample_size();
	int packet_size = create_sample_packet(ps, buffer.get());
	if (packet_size <= 0) {
		std::cerr << "Error creating sample packet." << std::endl;
		return -1;
	}

	// Array of servers for TMR (Triple Modular Redundancy)
	const std::array<std::pair<int, sockaddr_in*>, 3> servers = {{
		{sd, &server_addr},
		{sd, &server_bk_1},
		{sd, &server_bk_2}
	}};

	// Attempt to send the packet up to 3 tries per server (9 total attempts)
	for (int i = 0; i < 9; ++i) {
		const auto& [sock_fd, server] = servers[i / 3]; // Use integer division to determine the server
		int bsent = sendn(sock_fd, buffer.get(), packet_size, server);

		if (bsent != packet_size) {
			std::cerr << "Error sending monitoring packet." << std::endl;
			error = -1;
		} else {
			error = 0;
			if (tmr != 1) {
				break; // Exit early if not using TMR
			}
		}
	}

	return error;
}

/**
	 Send configuration packet trough the socket.
	 @param hw_conf	Structure containing the information about th hardware configuration of the system.
 	 @return 0 if no error occurred, -1 otherwise.
*/
int send_conf_packet(const Hw_conf* hw_conf) {

	// Calculate the total size of the packet
	const size_t packet_size = CONF_PACKET_SIZE + hw_conf->hostname.length() + 1;
	// Allocate memory for the configuration packet using std::unique_ptr
	auto conf_packet = std::make_unique<char[]>(packet_size);

    if (!conf_packet) {
        std::cerr << "Error creating configuration packet.\n" ;
        return -1;
    }

	// Create the configuration packet
	int size = create_conf_packet(*hw_conf, conf_packet.get());
	if (size <= 0) {
		std::cerr << "Error while creating the configuration packet.\n";
		return -1;
	}

	// Send the packet to the main server
	int bsent = sendn(sd, conf_packet.get(), packet_size, &server_addr);
	if (bsent != static_cast<int>(packet_size)) {
		std::cerr << "Error sending configuration packet to the main server." << std::endl;
		return -1;
	}

	// Send the packet to the backup servers (TMR - Triple Modular Redundancy)
	for (const auto& [sd_backup, server_backup] : {std::pair{sd1, &server_bk_1},
																		std::pair{sd2, &server_bk_2}}) {
		bsent = sendn(sd_backup, conf_packet.get(), packet_size, server_backup);
		if (bsent != static_cast<int>(packet_size)) {
			std::cerr << "Error sending configuration packet to a backup server." << std::endl;
			return -1;
		}
	}

    return 0;
}

/**
	Initialize socket for communication.
	@param server String containing the Ip address to teh server towars the client will 
	communicate. 
	@param port String with the number of the port in which the server will be listening.
	@return Erroe in case the socket was not correctly created. 
*/
int init_socket(const std::string& server, int port){
	struct hostent *hp=nullptr;
	int error =0;

	if((sd = socket(AF_INET, SOCK_DGRAM, 0))< 0){
		std::cerr << " Could not connect to master. " << std::endl;
		exit(0);
	}
	hp = gethostbyname (server.c_str());
     
    //setting up sockaddr_in to be able to connect using it
    bzero((char *)&server_addr, sizeof(server_addr));
    if(hp != nullptr){
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
        std::cerr << "Error connecting to the server...\n";
        error = -1;
    }   
	return error;
}

/**

	Initialize socket for communication when master server fails.
	@param server String containing the Ip address to teh server towars the client will
	communicate.
	@param port String with the number of the port in which the server will be listening.
	@return Erroe in case the socket was not correctly created.
*/
int init_socket_backup1(const std::string& server, int port){
    struct hostent *hp=nullptr;
    int error =0;

    if((sd1 = socket(AF_INET, SOCK_DGRAM, 0))< 0){
        std::cerr << " Could not connect to master. " << std::endl;
        exit(0);
    }
    hp = gethostbyname (server.c_str());

    //setting up sockaddr_in to be able to connect using it
    bzero((char *)&server_bk_1, sizeof(server_bk_1));
    if(hp != nullptr){
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
        std::cerr << "Error connecting to the server...\n";
        error = -1;
    }
    return error;
}

/**
	Initialize socket for communication when master server and backup server fails. .
	@param server String containing the Ip address to teh server towards the client will
	communicate.
	@param port String with the number of the port in which the server will be listening.
	@return error in case the socket was not correctly created.
*/
int init_socket_backup2(const std::string& server, int port){
    struct hostent *hp=nullptr;
    int error =0;

    if((sd2 = socket(AF_INET, SOCK_DGRAM, 0))< 0){
        std::cerr << " Could not connect to master. " << std::endl;
        exit(0);
    }
    hp = gethostbyname (server.c_str());

    //setting up sockaddr_in to be able to connect using it
    bzero((char *)&server_bk_2, sizeof(server_bk_2));
    if(hp != nullptr){
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
        std::cerr << "Error connecting to the server...\n";
        error = -1;
    }
    return error;
}

/**
 * This function executes a command in linux returning the output.
 * @param cmd
 * @return
 */
std::string execCommand(const char * cmd){
	std::string res;
	std::array<char,128> buffer{};

	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe){
		throw std::runtime_error("popen() failed");
	}

	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr){
		res += buffer.data();
	}

	return res;
}

int close_socket(){
	int error=0;
	error=close(sd);
	if (error ==-1){
		cerr << "Error happened when closing socket."<< endl;
	}
	return error;	
}


void SendDataToInflux(string hostname, int xmitdata, int xmitwait, std::string db_addr) {
	// ************* INFLUX INIT *******
	//std::string url = "127.0.0.1"; // URL --> changed to db_addr from conf_file
	int influxport = 8086;
	std::string token = "ZpJf7k3DPgVeWlK3acd3GTSM8YE75JLpelxhS_J-YKqoNoHrtAhH3WMsux438vulM_XZ7BIPjH9OfqMU1eERAA==";
	std::string org = "uc3m"; // Org
	std::string bucket = "test"; // bucket

	// Data to influx
	auto currtime = std::chrono::high_resolution_clock::now();
	auto dur = currtime.time_since_epoch();
	unsigned long long nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
	influxdb_cpp::server_info si(db_addr, influxport, org, token, bucket);
	// tokenize log and get data --> indexes 5 and 2
	int cpu = std::stoi(split(get_log_line(), ' ')[5]);
	int mem = std::stoi(split(get_log_line(), ' ')[2]);

	influxdb_cpp::builder()
		.meas(hostname)
		.field("cpu", cpu)
		.field("mem", mem)
		.field("xmitdata", xmitdata)
		.field("xmitwait", xmitwait)
		.timestamp(nanoseconds)
		.post_http(si);

	string resp;
	string query(
		"from(bucket: \\\"" + bucket + "\\\")|> range(start: -1h)|>filter(fn: (r)=>r[\\\"_measurement\\\"] == \\\""
		+ hostname + "\\\")|>filter(fn: (r) => r[\\\"_field\\\"] == \\\"xmitwait\\\")");
	influxdb_cpp::flux_query(resp, query, si);
	//** Data sent
}
