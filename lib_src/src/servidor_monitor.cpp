#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <algorithm>
#include <map>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "servidor_monitor.hpp"
#include "server_data.hpp"
#include "network_data.hpp"
#include "Packed_sample.hpp"
#include "common.hpp"


#define NUM_ACCEPTED_CON 10
//#define BUFFER_SIZE 512


using namespace std;

pthread_mutex_t mut_thread;
pthread_cond_t cond_thread;

pthread_cond_t empty_cond;
pthread_mutex_t proc_mut;
int busy = 0; //if 0 false, if 1 true

/*Socket for listening in server*/
int sd_listen=0;
int sd_client_listen=0;
struct sockaddr_in si_me, si_other, si_master, si_bk1, si_bk2;

/*Socket for flexmpi controller*/
int sd_flex_listen=0;
struct sockaddr_in si_me_flex, si_other_flex;

/*socket descriptor for master and backup servers*/
int socket_desc_master = 0;
int socket_desc_bk1 = 0;
int socket_desc_bk2 = 0;

//vector: store all messages and, after full or timeout, send all to master
//std::vector<unsigned char *> _queue;
std::vector<struct handle_args> _global;
std::vector<struct handle_args> _queue_messages; // ldm or lda to lds
std::vector<struct handle_args> _generic_queue; //ldm to lda
std::vector<struct handle_args> _server_queue; //lda generic to lds
std::vector<struct handle_args> _server_concat_queue; //lda concat to lds
std::vector<unsigned long> _generic_aggregation;
std::vector<string> _generic_keys;
bool is_generic_use = false;
std::map<string, unsigned long> map_lda;
unsigned char *packed_concat = nullptr;

/*variables for hot-spots*/
int hotspot_enable = 0;
int mem_hot  = 75;
int cache_hot = 75;
int net_hot = 75;
int busy_hot = 0;
std::mutex mutex_hot;
int tmr = 0;


/* variables for aggregators*/
std::vector<std::string> ips_agg;
std::vector<int> agg_metrics_agg = {0, 0, 0, 0, 0};
int registered_nodes_agg = 0;


/*    COMMUNICATION THREAD FUNCTIONS     */

/**
 * Auxiliar methods, forward declarations
 * @param local_inf
 */
void manageRequestUDP(struct handle_args *local_inf){
	Hw_conf hw_conf;

	//recvn function either receives n bytes by retrying recv or detects error (-1) or close/shutdown of peer (0)
	//receive request type (a char) or detect closed socket

	/********* obtaining request type (1 byte) ***********/
	unsigned char type = local_inf->buffer[BTYPE_POS];

	if (type == 'v') {
		local_inf->socket = sd_flex_listen;
		local_inf->client_addr = &si_other_flex;
		//DISABLED:manage_query_packet(&(local_inf->buffer[0]), si_other_flex, sd_flex_listen);
	}else if(type == 0){
		int error = 0;
		//Configuration request

		/****** receiving request header information ***********/
		/*OBtain the hardware configuration from client*/
		obtain_conf_from_packet(&local_inf->buffer[BTYPE_POS+1], &hw_conf,local_inf->client_IP);
		
		error = insert_hw_conf(hw_conf);
		if(error == -1){
			cerr << "Error saving hardware configuration."<< endl;
		}

		/********************************************************/
	} else if (type < MAX_SAMPLES){ //Monitoring message
		//cout << "Llamando a manage_monitoring_packet" << endl;
		manage_monitoring_packet (&(local_inf->buffer[BTYPE_POS]), local_inf->size, local_inf->client_IP);
	}else { //Client query
		local_inf->socket = sd_client_listen;
		local_inf->client_addr = &si_other;
		//DISABLED:manage_query_packet(&(local_inf->buffer[0]), si_other, sd_client_listen);
		/*#ifdef DAEMON_SERVER_DEBUG
        cerr << clientId << "Request type unknown: " << type << ", closing connection." << endl;
    #endif*/
	}
	//returns to accept another request
}


/**
 * Thread function. It manages packages from Daemons and Clients
 * @param global
 * @return
 */
void *run(void *global) {
	struct handle_args local_inf;

	//auto start = std::chrono::high_resolution_clock::now();

	//This mutext and the next things commented are for launches without worker threads (when we launched global threads)
	//pthread_mutex_lock(&mut_thread);
	/*Allocate temporal buffer with size of request + size of IP*/

	struct handle_args *global_inf = (struct handle_args *) global;
	std::vector<unsigned char> local_buffer(global_inf->buffer.size() + IP_SIZE);//(global_inf->size + IP_SIZE);
	std::memcpy(local_inf.client_IP, global_inf->client_IP, sizeof(local_inf.client_IP));
	local_inf.size = global_inf->size;

	//std::memcpy(local_buffer.data(), global_inf->buffer, local_inf.size);
	//local_inf.buffer = local_buffer.data();  // Asignamos el buffer al campo de la estructura
	local_inf.buffer = global_inf->buffer;

	busy = 0;
	//pthread_cond_signal(&cond_thread);
	//pthread_mutex_unlock(&mut_thread);
	//Process request

	manageRequestUDP(&local_inf);

	//Evaluates the overhead of the packet processing
	/*auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << "Processig time: " << duration.count() << std::endl;*/

	return nullptr;
}

/**
 * Thread function. It processes generic packets from TBON LDAs.
 * @param global
 * @return
 */
void *run_generic(void *global){
	struct handle_args local_inf;

	//auto start = std::chrono::high_resolution_clock::now();

	//This mutext and the next things commented are for launches without worker threads (when we launched global threads)
	//pthread_mutex_lock(&mut_thread);
	/*Allocate temporal buffer with size of request + size of IP*/

	struct handle_args *global_inf = (struct handle_args *) global;
	std::vector<unsigned char> local_buffer(global_inf->buffer.size() + IP_SIZE); //(global_inf->size + IP_SIZE);
	std::memcpy(local_inf.client_IP, global_inf->client_IP, sizeof(local_inf.client_IP));
	local_inf.size = global_inf->size;

	//std::memcpy(local_buffer.data(), global_inf->buffer, local_inf.size);
	//local_inf.buffer = local_buffer.data();  // Asignamos el buffer al campo de la estructura
	local_inf.buffer = global_inf->buffer;

	Hw_conf hw_conf;

	/********* obtaining request type (1 byte) ***********/
	unsigned char type = local_inf.buffer[BTYPE_POS];

	if(type == 0){
		int error = 0;
		//Configuration request
		/****** receiving request header information ***********/

		obtain_conf_from_packet(&local_inf.buffer[BTYPE_POS+1], &hw_conf,local_inf.client_IP);

		error = insert_hw_conf(hw_conf);
		if(error == -1){
			cerr << "Error saving hardware configuration."<< endl;
		}

	} else {// tbon message
		manage_generic_packet(&local_inf);
	}

	//pthread_cond_signal(&cond_thread);
	//pthread_mutex_unlock(&mut_thread);
	//Process request

	//Evaluates the overhead of the packet processing
	/*auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
	std::cout << "Processig time: " << duration.count() << std::endl;*/

	return nullptr;
}


/**
* Initialize the server parameters, binding socket to available port.
* IN:
* @port	POrt in which the server will be binded
* RETURNS Socket descriptor or -1 in case of error.
*/
int init_server(int port) {
    int error = 0;
    struct sockaddr_in server_addr;
    int val;

    /* Inicializar el servidor */
    //creating socket and setting its options
    sd_listen = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sd_listen <= 0) {
        cerr << "An error ocurred while opening socket." << endl;
        error = -1;
        return error;
    }
    val = 1;
    setsockopt(sd_listen, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));

    //initializing sockaddr_in used to store server-side socket
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    struct timeval timing;
    timing.tv_sec = 0;
    timing.tv_usec = 250000;
    setsockopt(sd_listen, SOL_SOCKET, SO_RCVTIMEO, &timing, sizeof(timing));

    //binding
    error = bind(sd_listen, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (error == -1) {
        cerr << "Error binding port. Possibly already in use." << endl;
        error = -1;
        return error;
    }
    cout << "Init server " << inet_ntoa(server_addr.sin_addr) << ":" << port << endl;

    pthread_mutex_init(&proc_mut, NULL);
    pthread_cond_init(&empty_cond, NULL);

    return error;
}

/**
	Initialize socket for communication.
	@param server String containing the Ip address to teh server towars the client will
	communicate.
	@param port String with the number of the port in which the server will be listening.
	@return Error in case the socket was not correctly created.
*/
int initialize_master_socket(char *server, int port){
	int error = 0;
    struct hostent *hp=NULL;

    //_queue = (unsigned char *)calloc(10 * MAX_PACKET_SIZE, sizeof(unsigned char));

	if ( (socket_desc_master=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		cerr << " Could not connect to master. " << endl;
		exit(0);
	}

	//timeout if packet lost or communation error
	/*struct timeval timing;
    timing.tv_sec = 0;
    timing.tv_usec= 250000;
    setsockopt(socket_desc_client, SOL_SOCKET, SO_RCVTIMEO, &timing, sizeof(timing));*/

    hp = gethostbyname (server);
	bzero((char *)&si_master, sizeof(si_master));
    if(hp != NULL){
        memcpy(&(si_master.sin_addr), hp->h_addr, sizeof(hp->h_length));
    }else{
        std::cerr << "Can not determine the address." << std::endl;
        error = -1;
        return error;
    }

	//memset((char *) &si_master, 0, sizeof(si_master));
	si_master.sin_family = AF_INET;
	si_master.sin_port = htons(port);

	/*if (inet_aton(server , &si_master.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		//exit(1);
	}*/
    if(connect(socket_desc_master,(struct sockaddr *) &si_master, sizeof(si_master)) == -1){
        cerr << "Error connecting to the server...\n";
        error = -1;
    }

    return error;
}


/**
 * Method to manage an UDP server. It contains one listener reserved to the Daemon requests.
 * @return socket descriptor or -1 if error.
 */
int manage_server_udp() {
	struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
	handle_args ha_thread;
    pthread_attr_t attr;

    // Inicializar atributos del hilo y configurarlo como detached
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_mutex_init(&mut_thread, nullptr);
    pthread_cond_init(&cond_thread, nullptr);

    while (true) {
        std::vector<unsigned char> tmp_buffer(MAX_PACKET_SIZE);

        ssize_t size_packet = recvfrom(sd_listen, tmp_buffer.data(), tmp_buffer.size(), 0, (struct sockaddr*)&client_addr, &client_addr_size);


    	if (size_packet == -1) {
            //std::cerr << "Error receiving packet." << std::endl;
        } else {
		    char client_ad[INET_ADDRSTRLEN];
		    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ad, sizeof(client_ad)); // Convertir IP a string
			//char * aux = reinterpret_cast<char*>(tmp_buffer.data());
        	//memcpy(ha_thread.buffer, aux, sizeof(aux));

        	//Esto copia directamente el vector
        	ha_thread.buffer = tmp_buffer;
        	//ha_thread.buffer = reinterpret_cast<unsigned char*>(tmp_buffer.data());
        	//std::copy(tmp_buffer.begin(), tmp_buffer.end(), ha_thread.buffer);

        	strcpy(ha_thread.client_IP, client_ad);
            ha_thread.size = size_packet;
            ha_thread.socket = sd_listen;
            ha_thread.client_addr = &client_addr;

        	//Print for debug
        	for (int j = 0; j < ha_thread.size; j++)
				std::cout << (int) ha_thread.buffer[j] << " ";
			  std::cout << std::endl;

            if (ha_thread.buffer[0] == static_cast<unsigned char>('c')) {
                // Paquete concatenado
                std::cout << "Concatenated packet" << std::endl;
                _server_concat_queue.push_back(ha_thread); // Se agrega a la cola de paquetes concatenados
            } else if (ha_thread.buffer[4] == static_cast<unsigned char>('g')) {
                // Paquete genérico
                std::cout << "Generic packet" << std::endl;
                _server_queue.push_back(ha_thread);  // Se agrega a la cola de paquetes genéricos
                pthread_cond_signal(&empty_cond);
                is_generic_use = true;
            } else {
                // Paquete de monitorización
                std::cout << "Mon packet" << std::endl;
                _queue_messages.push_back(ha_thread); // Se agrega a la cola de mensajes de monitorización
                pthread_cond_signal(&empty_cond);
            }
        }
    }

    return 0;
}

/**
 * Sets the TMR mode
 * @param val
 */
void setTMRvalue(int val){
    tmr = val;
}

/***********  NODE AGGREGATOR FUNCTIONS  **********/

/**
 * Method to manage an UDP server. It contains two listeners, one
 * reserved to the Daemon requests and other reserved to the Client requests.
 * @return socket descriptor or -1 if error.
 */
int manage_intermediate_server_udp() {
	struct sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof(client_addr);
	//unsigned char *tmp_buffer;

	struct handle_args ha_thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_mutex_init(&mut_thread, nullptr);
	pthread_cond_init(&cond_thread, nullptr);

	std::vector<unsigned char> tmp_buffer(MAX_PACKET_SIZE);

	while (true) {
		ssize_t size_packet = recvfrom(sd_listen, tmp_buffer.data(), tmp_buffer.size(), 0, (struct sockaddr*)&client_addr, &client_addr_size);

		/*Receive packet of max length.*/
		if (size_packet == -1) {
			//cerr << "Error receiving packet. " << endl;
		} else {
			char client_ad[16];
			//get ip address in a string
			sprintf(client_ad, inet_ntoa(client_addr.sin_addr), sizeof(client_ad));

			//information to pass to the thread (temporal buffer address, client address and port in a req_inf structure that stores this information)
			//ha_thread.buffer = reinterpret_cast<unsigned char*>(tmp_buffer.data());
			ha_thread.buffer = tmp_buffer;
			strcpy(ha_thread.client_IP, client_ad);
			ha_thread.size = size_packet;
			ha_thread.socket = sd_listen;
			ha_thread.client_addr = &client_addr;

			if (ha_thread.buffer[0] == 0) { //Conf packet automatically send
				/*Hw_conf hwconf;
				obtain_conf_from_packet(&ha_thread.buffer[BTYPE_POS+1], &hwconf,ha_thread.client_IP);
				insert_hw_conf(hwconf);
				sendConfNow(ha_thread);*/
			} else {
				if (ha_thread.buffer[12] == 'g') {
					//generic packet
					_generic_queue.push_back(ha_thread);
				} else {
					//Print for debug
					/*for (int j = 0; j < ha_thread.size; j++)
					std::cout << (int) ha_thread.buffer[j] << " ";
					std::cout << std::endl;*/
					_global.push_back(ha_thread);
				}
			}

			//We need to aggregate data instead of rettransmiting the packages.
			/*if (_global.size() == 10) {
				sendAllMessages();
			}*/
		}
	}

	return 0;
}


/**
 * Thread to process the messages and aggregate the data and send.
 */
void * intermediate_processor_agg() {
	std::thread th(packetAggregation);
	th.detach();
	return nullptr;
}

/**
 * Send config to master without waiting.
 * @param ha
 */
void sendConfNow(struct handle_args ha){//unsigned char * mes){
    int slen = sizeof(si_master);
    //add ip to mes
    std::vector<std::string> ip = split(ha.client_IP, '.');
    for (int i = 0; i < ip.size(); i++) {
        ha.buffer[ha.size] = (unsigned char) stoi(ip[i]);
        ha.size++;
    }

    if (sendto(socket_desc_master, ha.buffer.data(), /*MAX_PACKET_SIZE*/ha.size , 0 , (struct sockaddr *) &si_master, slen)==-1)
    {
        std::cerr << " Error sending package. " << std::endl;
    }

    //TMR with other two servers.
    if(socket_desc_bk1 != 0)
        if (sendto(socket_desc_bk1, ha.buffer.data(), /*MAX_PACKET_SIZE*/ha.size , 0 , (struct sockaddr *) &si_bk1, slen)==-1)
        {
            std::cerr << " Error sending package. " << std::endl;
        }

    if(socket_desc_bk2 != 0)
        if (sendto(socket_desc_bk2, ha.buffer.data(), /*MAX_PACKET_SIZE*/ha.size , 0 , (struct sockaddr *) &si_bk2, slen)==-1)
        {
            std::cerr << " Error sending package. " << std::endl;
        }
}


/**
 * Send all messages stored in _queue
 * TODO: Review this functionality because it generates SIGSEV (not in debug mode)
 */
void packetAggregation() {
	std::string ip, hostname;
	get_addr(ip, hostname);

	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(5)); //sleep one time interva
		std::vector<unsigned char> packed_concat(32768); 
		int concat_ptr = 0;

		/*This code is for Generic TBON*/
		if (_generic_queue.size() != 0){
			//Deserialize and aggregate (count number of metrics to create the vector
			//Read data from byte 12 = 'g'
			std::vector<struct handle_args> aux(_generic_queue); //aux(_generic_queue.begin(), _generic_queue.end());
			_generic_queue.clear();
			std::vector <unsigned long> values;
			std::vector <string> keys;

			for(auto& packet : aux){
				Hw_conf hw_conf;
				obtain_hw_conf(packet.client_IP, &hw_conf);
				unsigned char *pointer_buff = &packet.buffer[BTYPE_POS];
				
				
				/* Concatenate packet into a buffer: ip and data (remove conf) */
				if(packet.buffer[0] != 'c') {
					packed_concat[concat_ptr] = 'c'; // concat byte
					//add ip
					// Agregar IP
					Packed_sample sp;
					sp.ip_addr_s = ip;
					auto ip_v = sp.parse_log(sp.ip_addr_s);

					for (const auto& part : ip_v) {
						packed_concat[concat_ptr++] = static_cast<unsigned char>(std::stoi(part));
					}
					
					memcpy(&packed_concat[concat_ptr], &packet.buffer[13], packet.size - 12); //substract the 12 init bytes
					concat_ptr += packet.size - 12;
				} else {
					// packet is already a concatenated packet (lda --> lda)
					// in this case, concatenate directly without adding 'c' at the beginning
					memcpy(&packed_concat[concat_ptr], &packet.buffer[0], packet.size); //concatenated packets do not have config bytes
					concat_ptr += packet.size;
				}

				/*Obtain packet type*/
				int counter = 12;
				counter += 1;
				//get counters
				while (counter < packet.size && (counter + 68) < packet.size) { // 64 bytes clave + 4 bytes valor
					std::string key(reinterpret_cast<char*>(&pointer_buff[counter]), 64);
					keys.push_back(key);
					counter += 64;

					unsigned long val = (static_cast<uint64_t>(pointer_buff[counter]) << 56) |
										(static_cast<uint64_t>(pointer_buff[counter+1]) << 48) |
										(static_cast<uint64_t>(pointer_buff[counter+2]) << 40) |
										(static_cast<uint64_t>(pointer_buff[counter+3]) << 32) |
										(static_cast<uint64_t>(pointer_buff[counter+4]) << 24) |
										(static_cast<uint64_t>(pointer_buff[counter+5]) << 16) |
										(static_cast<uint64_t>(pointer_buff[counter+6]) << 8) |
										static_cast<uint64_t>(pointer_buff[counter+7]);

					values.push_back(val);
					counter += 8;
				}

				/*update index of ips --> not for generic aggregations (because I do not know if the user wants the avg) */
				/*std::string incoming_ip = "";
				for (int i = 0; i < sizeof(packet.client_IP); i++) {
				  if (packet.client_IP[i] == '\0') break;
				  incoming_ip = incoming_ip + packet.client_IP[i];
				}
				if (std::find(ips_agg.begin(), ips_agg.end(), incoming_ip) == ips_agg.end()) {
				  ips_agg.push_back(incoming_ip);
				  registered_nodes_agg++;
				}*/

				/*Aggregate data -- fields have the same order*/
				/*for (int i = 0; i < keys.size(); i++) {
					auto aux = map_lda.find(keys[i]);
					if(aux != map_lda.end())
					{
						// Aggregate
						map_lda[keys[i]] += values[i];
					} else {
						// insert
						map_lda[keys[i]] = values[i];
					}
				}*/
				for (size_t i = 0; i < keys.size(); ++i) {
					map_lda[keys[i]] += values[i];
				}
			}

			//Clear data
			aux.clear();
			ips_agg.clear();
			registered_nodes_agg = 0;

			auto ps = std::make_unique<Packed_sample>();
			for (const auto& [key, value] : map_lda) {
				_generic_keys.push_back(key);
				_generic_aggregation.push_back(value);
			}
			ps->Aggregation_sample_generic(ip, _generic_aggregation, _generic_keys);

			int slen = sizeof(si_other);
			if (sendto(socket_desc_master, ps->packed_buffer, ps->sample_size, 0, (struct sockaddr *) &si_master,
			         slen) == -1) {
				cerr << " Error sending agg package. " << endl;
			}

			//todo: send the concatenation to another socket?
			if (sendto(socket_desc_master, reinterpret_cast<char*>(packed_concat.data()), concat_ptr+1, 0,
						(struct sockaddr *) &si_master, slen) == -1) {
				cerr << " Error sending concat package. " << endl;
			}

			//Print for debug
			/*for (int i = 0; i < ps->sample_size; i++)
			cout << (unsigned char)ps->packed_buffer[i] << " ";
			cout << endl;
			for (int i = 0; i < concat_ptr; i++)
			cout << (unsigned char)packed_concat[i] << " ";
			cout << endl;*/

			/*Reset aggregations after sending data*/
			_generic_aggregation.clear();
			_generic_keys.clear();
			map_lda.clear();
			concat_ptr = 0;
		}
		else {
			/*This code is for ADMIRE*/
			if (_global.size() != 0) {
				std::vector<struct handle_args> aux(std::move(_global));
				_global.clear();

				// read each packet to aggregate its metrics
				for (const auto& packet : aux) {
					/* Aggregate metrics per application:
					* 1: Unpacket data
					* 2: read app name
					* 3: Insert into index if not registered previously, aggregate data otherwise
					* 4: Generate new packet with the aggregated data (send avg)
					*/

					/* Overwrite ip - Not in this version
					std::vector<std::string> ip = split(i.client_IP, '.');
					for (int j = 0; j < ip.size(); j++) {
					i.buffer[i.size] = (unsigned char) stoi(ip[j]);
					i.size++;
					}*/

					/* Packet disassembly and aggregate metrics and generate new packet */
					Hw_conf hw_conf;
					obtain_hw_conf(packet.client_IP, &hw_conf);
					const unsigned char *pointer_buff = packet.buffer.data();//&packet.buffer[BTYPE_POS];
					Packed_sample sp; //(hw_conf, 5000, 1, 50);
					int counter = 14;

					/*Obtain memory usage*/
					int mem_usage_perc = (int) pointer_buff[counter];
					counter += SIZE_PACKED_PERCENTAGE;

					/*Obtain CPU_idle*/
					int CPUidle_perc = (int) pointer_buff[counter];
					counter += SIZE_PACKED_PERCENTAGE;

					/*Vectors for the I/O and network devices */
					vector<int> io_devices_w_perc(hw_conf.n_devices_io);
					vector<int> io_devices_io_perc(hw_conf.n_devices_io);
					vector<int> net_devices_speed(hw_conf.n_interfaces);
					vector<int> net_devices_usage_perc(hw_conf.n_interfaces);

					/*For each device, obtain w(%) and TIO(%)*/
					for (int i = 0; i < hw_conf.n_devices_io; ++i) {
						io_devices_w_perc[i] = static_cast<int>(pointer_buff[counter]);
						counter += SIZE_PACKED_PERCENTAGE;
						io_devices_io_perc[i] = static_cast<int>(pointer_buff[counter]);
						counter += SIZE_PACKED_PERCENTAGE;
					}

					/*For each network device, obtain speed and network usage*/
					for (int i = 0; i < hw_conf.n_interfaces; ++i) {
						net_devices_speed[i] = static_cast<int>(pointer_buff[counter]);
						counter += SIZE_PACKED_PERCENTAGE;
						net_devices_usage_perc[i] = static_cast<int>(pointer_buff[counter]);
						counter += SIZE_PACKED_PERCENTAGE;
					}

					/*Cache data*/
					/*int cache_ratio = static_cast<int>(pointer_buff[counter]);
					counter += SIZE_PACKED_PERCENTAGE;
					int cpu_stalled = static_cast<int>(pointer_buff[counter]);
					counter += SIZE_PACKED_PERCENTAGE;*/

					/* JOBNAME FOR PROMETHEUS*/
					std::string jobname(8, '\0');  // Inicializa una cadena de longitud 8 con caracteres nulos
					for (int i = 0; i < 8; ++i) {
						jobname[i] = static_cast<char>(packet.buffer[counter]);
						++counter;
					}

					/* Prometheus counters aggregated in this LDA*/
					std::vector<int> counters = {CPUidle_perc, mem_usage_perc};
					// DISABLED BECAUSE OF THE SYSTEM --> todo: check this.
					int io_time = 0; //*std::max_element(io_devices_io_perc.begin(), io_devices_io_perc.end());
					int io_w = 0; //*std::max_element(io_devices_w_perc.begin(), io_devices_w_perc.end());
					int com_time = 0; //*std::max_element(net_devices_usage_perc.begin(), net_devices_usage_perc.end());
					counters.push_back(io_time);
					counters.push_back(io_w);
					counters.push_back(com_time);
					
					/* Aggregation in LDA end */

					/*update index of ips*/
					std::string incoming_ip = "";
					for (int i = 0; i < sizeof(packet.client_IP); i++) {
						if (packet.client_IP[i] == '\0') break;
						incoming_ip = incoming_ip + packet.client_IP[i];
					}

					if (std::find(ips_agg.begin(), ips_agg.end(), incoming_ip) == ips_agg.end()) {
						ips_agg.push_back(incoming_ip);
						registered_nodes_agg++;
					}

					/*Aggregate directly*/
					agg_metrics_agg[0] += CPUidle_perc;
					agg_metrics_agg[1] += mem_usage_perc;
					agg_metrics_agg[2] += io_time;
					agg_metrics_agg[3] += io_w;
					agg_metrics_agg[4] += com_time;

					// Print for debug
					auto time = std::chrono::system_clock::now();
					std::time_t now = std::chrono::system_clock::to_time_t(time);
					cout << packet.client_IP << " " << jobname << " " << std::ctime(&now) << " CPU: " << CPUidle_perc << " Mem: "
					   << mem_usage_perc << " IO(t): " << io_time << " IO(w): " << io_w
					   << " Comm: " << com_time << endl;
				}

				/*compute avgs*/
				for (auto& metric : agg_metrics_agg) {
					metric /= registered_nodes_agg;
				}

				/* reset agg structs */
				aux.clear();
				ips_agg.clear();
				registered_nodes_agg = 0;

				/* Create new packet and send it */
				Packed_sample *ps = new Packed_sample();
				ps->Aggregation_sample(ip, agg_metrics_agg[1], agg_metrics_agg[0], agg_metrics_agg[2], agg_metrics_agg[3],
				                       agg_metrics_agg[4]);
				ps->packed_ptr++;

				int slen = sizeof(si_other);
				if (sendto(socket_desc_master, ps->packed_buffer, ps->sample_size + 1, 0, (struct sockaddr *) &si_master,
				           slen) == -1) {
					cerr << " Error sending package. " << endl;
				}

				/*Reset aggregations after sending data*/
				std::fill(agg_metrics_agg.begin(), agg_metrics_agg.end(), 0);
			}
		}
	}
}



/*******+*   QUEUE MESSAGES FUNCTIONS    *********/

/**
 * Each processor thread gets data from the buffer and process it.
 */
void worker_function(){
    //get the mutex to get data from buffer. If it is empty, we have to wait.
    while(true) {
        pthread_mutex_lock(&proc_mut);
        while (queue_empty() == 1) {
            pthread_cond_wait(&empty_cond, &proc_mut);
        }

        if(!is_generic_use) {
			auto i = _queue_messages[0];
			_queue_messages.erase(_queue_messages.begin());
			pthread_mutex_unlock(&proc_mut);
			run(&i);
        } else {
			// generic
			auto i = _server_queue[0];
			_server_queue.erase(_server_queue.begin());
			pthread_mutex_unlock(&proc_mut);
			run_generic(&i);
        }
    }
}

/**
 * Get messages from queue and process them.
 * V2: create only de max number of threads, taking into account that server uses 4 by default.
 */
void * processor() {
    int nthreads = 1;
    int max_concurrency = std::thread::hardware_concurrency(); //gets the max concurrency.
    std::cout << "Max concurrency detected: " << max_concurrency << std::endl;
    if (max_concurrency > 2 && max_concurrency < 10) nthreads = max_concurrency -3;
    else if (max_concurrency >= 10) nthreads = 8;

    //V2: create threads that automatically process the information in the buffer.
    std::vector<std::thread> workers(nthreads);
    for (int i = 0; i < nthreads; i++){
    	cout << "Processor worker " << i << " deployed." << endl;
        std::thread th(worker_function);
        th.detach();
    }
	return nullptr;
}

/**
 * Return 0 if there are elements in queue.
 * @return
 */
int queue_empty(){
    if(is_generic_use)
      return (_server_queue.size() > 0) ? 0 : 1;
    else
      return (_queue_messages.size() > 0) ? 0 : 1;
}


/*********   HOTSPOTS FUNCTIONS  ********/
/**
 * If in any sample, one of the measures are grater than these, a hot-spot notification will send.
 * @param mem
 * @param cache
 * @param net
 */
void setHotspotsValueNotification(int mem, int cache, int net){
	mem_hot = mem;
	cache_hot = cache;
	net_hot = net;
}

/**
 * Get value hot-spot range to mem usage.
 * @return
 */
int getMemHotValue(){
	return mem_hot;
}

/**
 * Get value hot-spot range to cache usage.
 * @return
 */
int getCacheHotValue(){
	return cache_hot;
}

/**
 * Get value hot-spot range to network usage.
 * @return
 */
int getNetHotValue(){
	return net_hot;
}

/**
 * If  server detects hotspot (cpu, mem, io), send this information to flex_client.
 * @param ip
 * @param profile
 */
void hotspotNotification(std::string ip, int profile, char* mes){
    if (hotspot_enable != 0) {
        if (mutex_hot.try_lock()) {
            if (busy_hot == 0) {
                busy_hot = 1;
                std::vector<char> hostname(NI_MAXHOST, '\0');
                int err = obtainHostNameByIP(ip.c_str(), hostname.data());

				std::string out_message;

				std::string aux = (profile == CPU_HOT) ? std::to_string(CPU_HOT) :
								(profile == MEM_HOT) ? std::to_string(MEM_HOT) :
								(profile == IO_HOT) ? std::to_string(IO_HOT) :
								(profile == NET_HOT) ? std::to_string(NET_HOT) : std::to_string(CACHE_HOT);

				if (err == 0) {
					out_message = std::string(hostname.data()) + ":" + aux + ":" + mes;
				} else {
					out_message = ip + ":" + aux + ":" + mes;
				}

				size_t slen = sizeof(si_other_flex);
                ssize_t res = sendto(sd_flex_listen, out_message.c_str(), out_message.size(), 0,
                                     (struct sockaddr*)&si_other_flex, slen);
                if (res < 0) {
                    std::cerr << "Error sending package\n";
                }

                // Register log
                std::ofstream myfile;
                myfile.open("/tmp/log_monitor/log_v2.xt", std::ios::out | std::ios::app);
                if (myfile.is_open()) {
                    myfile << "Hot-spot sent: " << (err == 0 ? hostname.data() : ip) << ":" << mes << std::endl;
                    myfile.close();
                }

                busy_hot = 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            mutex_hot.unlock();
        } else {
            // Si el socket está ocupado, registrar en el log
            ofstream myfile;
            myfile.open("/tmp/log_monitor/log_v2.xt", std::ios::out | std::ios::app);
            if (myfile.is_open()) {
                myfile << "Hot-spot NOT sent (socket busy): " << ip << ":" << mes << endl;
                myfile.close();
            }
        }
    }
}

