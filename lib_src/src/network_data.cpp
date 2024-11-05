#ifndef NETWORK_DATA
#define NETWORK_DATA

#include <arpa/inet.h>
#include "Packed_sample.hpp"
#include "network_data.hpp"
#include "server_data.hpp"
#include <netinet/in.h>
#include <cstring>
#include <string>
#include <iostream>
#include <mutex>
#include <library.h>
#include <netdb.h>
#include <cmath>
#include <thread>
#include <map>
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <algorithm>
#include <typeinfo>


using namespace std;
using namespace prometheus;


std::mutex logging_lock;
bool startExposer = false;

/*David wants this to show inormation about some tests*/
int displaySample=0;
std::string displayedMes="";

/******* Prometheus data: index to store node metrics ********************/
std::map<std::string, std::vector<int>> counter_metrics;
std::vector<std::string> ips;
std::vector<int> agg_metrics = {0, 0, 0, 0, 0};
int registered_nodes = 0;
int reset = 0;
/********** Generic aggregation in LDS  **********************************/
std::vector<string> _generic_keys_server;
std::vector<unsigned long> _generic_aggregation_server;
std::vector<string> ips_server;
std::map<string, unsigned long> map_server;
/*****************************************************************/


time_t obtain_time_from_packet(unsigned char * buffer){
	int size = sizeof(time_t);
	time_t time_sample = 0;
	for(int i= 1; i <= size; i++){
		time_t tmp = (time_t) buffer[i-1];
		time_sample = (tmp << ((size-i)*8)) | time_sample ;
	}

	return time_sample;
}

/* Function to print the raw packed sample on cout */
int print_raw_packet_sample(unsigned char * buffer, int size, int n_io_devices, int n_net_devices, int n_core, std::string ip) {

	int counter = 4;
	//int counter = 0;
	int i = 0;
	int error = 0;
	/*Obtain memory usage*/
	int mem_usage_perc = (int) buffer[counter];
	vector<int> io_devices_w_perc(n_io_devices);
	vector<int> io_devices_io_perc(n_io_devices);
	vector<int> net_devices_speed(n_net_devices);
	vector<int> net_devices_usage_perc(n_net_devices);
	vector<int> pw_cpu(n_core);

	counter += SIZE_PACKED_PERCENTAGE;

	/*Obtain CPU_idle*/
	int CPUidle_perc = (int) buffer[counter];
	counter += SIZE_PACKED_PERCENTAGE;
	//counter +=  SIZE_PACKED_PERCENTAGE;

	/*For each cpu, obtain power usage in Joules*/
	for (i = 0; i < n_core; ++i) {
		pw_cpu[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
	}

	/*For each device, obtain w(%) and TIO(%)*/
	for (i = 0; i < n_io_devices; ++i) {
		io_devices_w_perc[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
		io_devices_io_perc[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
	}
	/*For each network device, obtain speed and network usage*/
	for (i = 0; i < n_net_devices; ++i) {
		net_devices_speed[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;
		net_devices_usage_perc[i] = (int) buffer[counter];
		counter += SIZE_PACKED_PERCENTAGE;

	}

	int cache_ratio = (int) buffer[counter];
	counter += SIZE_PACKED_PERCENTAGE;

	int cpu_stalled = (int) buffer[counter];
	counter += SIZE_PACKED_PERCENTAGE;

	/* JOBNAME FOR PROMETHEUS*/
	char* in_jobname = (char*)malloc(8);
	for (int i = 0; i<8; i++){
		char c = (int)buffer[counter];
		in_jobname[i] = (char)c;
		//printf("%c", c);
		counter++;
	}
	std::string jobname(in_jobname);

	if (displaySample == 1) {
		/*ofstream myfile;
		myfile.open("/tmp/log_monitor/log_v2.xt", ios::out | ios::app);
		myfile << displayedMes << endl;
		myfile << "Mem usage: " << mem_usage_perc << "%" << endl;
		myfile << "CPU Busy : " << CPUidle_perc << "%" << endl;
		//myfile << "Energy usage: " << pw_cpu << "Joules" << endl;
		//myfile << "Net devices speed : " << net_devices_speed << "Gb/s" << endl;
		myfile << "Net devices usage" << net_devices_usage_perc << "%" << endl;
		//myfile << "Temp CPU: " << temp_core << "Cº" << endl;
		//myfile << "Temp percentage: " << temp_core_perc << "%" << endl;
		myfile << "Cache: " << cache_ratio << "%" << endl;
		myfile << "Cpu stalled: " << cpu_stalled << "%" << endl << endl << endl;
		myfile.close();*/

		/*cout << displayedMes << endl;
        cout << "Mem usage: " << mem_usage_perc << "%" << endl;
        cout << "CPU Busy : " << CPUidle_perc << "%" << endl;
        cout << "Energy usage: " << pw_cpu << "Joules" << endl;
        cout << "Net devices speed : " << net_devices_speed << "Gb/s" << endl;
        cout << "Net devices usage" << net_devices_usage_perc << "%" << endl;
        cout << "Temp CPU: " << temp_core << "Cº" << endl;
        cout << "Temp percentage: " << temp_core_perc << "%" << endl;
        cout << "Cache:" << cache_ratio << "%" << endl;*/

		displaySample = 0;
		displayedMes = "";
	}

	/*cout << "Mem usage: " << mem_usage_perc << "%" << endl;
    cout << "CPU Busy : " << CPUidle_perc << "%" << endl;
    cout << "Energy usage: " << pw_cpu << "Joules" << endl;
    cout << "IO devices W : " << io_devices_w_perc << "%" << endl;
    cout << "IO devices IO : " << io_devices_io_perc << "%" << endl;
    cout << "Net devices speed : " << net_devices_speed << "Gb/s" << endl;
    cout << "Net devices usage" << net_devices_usage_perc << "%" << endl;
    cout << "GPU Mem usage: " << gpu_memUsage << "%" << endl;
    cout << "GPU Usage:" << gpu_Usage << "%" << endl;
    cout << "GPU temperature:" << gpu_temp << " Cº" << endl;
    cout << "GPU power usage:" << gpu_pw << " Watts" << endl;
    cout << "Cache:" << cache_ratio << endl;
    for (int i = 0; i < siblings; i++){
        cout << "vCore " << i << ": " << core_load[i] << "%" << endl;
    }*/

	  auto time = std::chrono::system_clock::now();
	  std::time_t now = std::chrono::system_clock::to_time_t(time);
    /*ofstream myfile;
    myfile.open("/tmp/log_daemon/log_local.txt", ios::out | ios::app);
    myfile << ip << " " << std::ctime(&now) << " " << mem_usage_perc << " " << CPUidle_perc << " " << io_devices_io_perc << " " << io_devices_w_perc
    << " " << net_devices_speed << " " << net_devices_usage_perc << endl;*/
    /*myfile << "*********************" << endl;
    myfile << "IP: " << ip << endl;
    myfile << "TIME: " << std::ctime(&now) << endl;
    myfile << "Mem usage: " << mem_usage_perc << "%" << endl;
    myfile << "CPU Busy : " << CPUidle_perc << "%" << endl;
    myfile << "Energy usage: " << pw_cpu << "Joules" << endl;
    //myfile << "Net devices speed : " << net_devices_speed << "Gb/s" << endl;
    //myfile << "Net devices usage" << net_devices_usage_perc << "%" << endl;
    myfile << "Temp CPU: " << temp_core << "Cº" << endl;
    myfile << "Temp percentage: " << temp_core_perc << "%" << endl;
    myfile.close();*/

    /************* Prometheus integration ****************/
    //<cpu%> <mem%> <n_io> <io time> <io_w> <n_net> <net_speed> <net_%>
    std::vector<int> counters;
    counters.push_back(CPUidle_perc);
    counters.push_back(mem_usage_perc);
    //counters.push_back(n_io_devices);
    int io_time = 0;
    for(i = 0; i < n_io_devices; i++) {
      io_time = (io_devices_io_perc[i] > io_time) ? io_devices_io_perc[i] : io_time;
    }
    counters.push_back(io_time);
    int io_w = 0;
    for(i = 0; i < n_io_devices; i++) {
      io_w = (io_devices_w_perc[i] > io_w) ? io_devices_w_perc[i] : io_w;
    }
    counters.push_back(io_w);
    //counters.push_back(n_net_devices);
    //for(i = 0; i < n_net_devices; i++)
    //  counters.push_back(net_devices_speed[i]);
    int com_time = 0;
    for(i = 0; i < n_net_devices; i++) {
      com_time = (net_devices_usage_perc[i] > com_time) ? net_devices_usage_perc[i] : com_time;
    }
    counters.push_back(com_time);

    /************* compute the aggregation instead of manage an index *************/
    /*std::map<std::string, std::vector<int>>::iterator it = counter_metrics.find(ip);
    if (it == counter_metrics.end()) {
      //createCounterFamilyForNode(ip, n_io_devices, n_net_devices);
      counter_metrics.insert(std::pair<std::string, std::vector<int>>(ip, counters));
      registered_nodes ++;
      //it->second = counters;
    } else{
      it->second.clear();
      it->second = counters;
    }*/

    if ( std::find(ips.begin(), ips.end(), ip) == ips.end() ) {
      ips.push_back(ip);
      registered_nodes++;
    }

    if(reset == 1){
      reset = 0;
      agg_metrics = {0, 0, 0, 0, 0};
    }
    /*Aggregate directly*/
    agg_metrics[0] += CPUidle_perc;
    agg_metrics[1] += mem_usage_perc;
    agg_metrics[2] += io_time;
    agg_metrics[3] += io_w;
    agg_metrics[4] += com_time;

    // Print for debug
    cout << ip << " " << jobname << " " << std::ctime(&now) << " " << CPUidle_perc << " " << mem_usage_perc << " " << io_time << " " << io_w
        << " " << com_time << endl;

    /***************************************************/


	/*if (dbSaveCounter == 10){
        backupDb("/tmp/db_bak");
        dbSaveCounter = 0;
	}
	dbSaveCounter++;*/

	return error;

}


/**
	Function to obtain the configuration information from a received raw packet.
	@param[in] conf_packed: char array that is a raw packet in which configuration information has been sent
	@param[in,out] hw_conf: hw_conf object build from conf_packed packet
	@param[in]: client_addr:

*/
int obtain_conf_from_packet(unsigned char * conf_packed, Hw_conf* hw_conf, const std::string client_addr){
	int error = 0;
	int counter = 0;
	unsigned short int tmp =0;
	int hostname_size = 0;
	//cout << "obtain conf from packet ///////////////////////////////////////////////////" << endl;
	hw_conf->ip_addr_s = client_addr;

	hw_conf->n_cpu = (int)conf_packed[counter];
  	counter++;

	hw_conf->n_cores = (int)conf_packed[counter];
	counter++;

	convert_char_to_short_int(&conf_packed[counter],&tmp);
	hw_conf->mem_total = (unsigned short int) tmp;
	counter+=sizeof(short int);

	hw_conf->n_devices_io=(int)conf_packed[counter];
	counter++;

	hw_conf->n_interfaces= (int)conf_packed[counter];
	counter ++;

	hw_conf->n_core_temps= hw_conf->n_cpu;


	hostname_size = conf_packed[counter];

	//cout << "****************************************** size hostname: "<< hostname_size << endl;

	counter++;
	for(int i=0; i < hostname_size; i++){
		stringstream ss;
		ss << (char) conf_packed[counter];
//		char aux = (char) conf_packed[counter];
		counter++;
		hw_conf->hostname.append(ss.str());
//		ss << conf_packed[counter];
//		hw_conf->hostname << (char)conf_packed[counter];
//		ss >> hostname;
//		counter++;
//		hw_conf->hostname.append(ss);
	}
	
	/* Obtener modo bitmap */
	//hw_conf->modo_bitmap = conf_packed[counter];
	//counter++;
	//printf("En el servidor se obtiene el modo de bitmap de: %d\n", hw_conf->modo_bitmap);

	/*std::string new_ip;
	for (int i = 0; i < 4; i++){
		new_ip = new_ip + std::to_string((int)conf_packed[counter+i]);
		if (i < 3)
            new_ip = new_ip + ".";
	}
	hw_conf->ip_addr_s = new_ip;*/

	#ifdef DEBUG_DAEMON
		/*for(int i = 0; i < (counter+1); i++ ){
			printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++Posicion del paquete recibido %d contiene %d \n",i,conf_packed[i]);
		}*/
	#endif
	
	return error;
}

/**
	Obtain a sample packed with the samples obtained in the raw packet.
	@param[in] buffer Raw packet containing the monitored samples.
	@param[out]	mon_sample Packed including the data from the raw buffer.
	@returns 0 if no error occurred and -1 if something happened while obtaining the information of the packet. 
*/
int obtain_sample_from_packet(unsigned char * buffer, Packed_sample &mon_sample){
	int error = 0;
	int counter = 0;
	int i =0;

	unsigned char *pointer_buff = NULL;
	//Aboid type of packet
	pointer_buff = &buffer[BTYPE_POS+1];

	
	unsigned char puntero = 1;

	/************************* Dependiendo del modo de bitmap habra un contenido u otro **************************************/
	int modo_bitmap = buffer[puntero];
	if(modo_bitmap == 0){ //Solo sample
		puntero++;
		pointer_buff += sizeof(unsigned char);
		counter +=sizeof(unsigned char);
		//cout << "Bitmap mode:  0\n";

	}else if(modo_bitmap == 1){ //Solo bitmap

		pointer_buff += sizeof(unsigned char);
		counter +=sizeof(unsigned char);
		puntero++;
#ifdef DAEMON_SERVER_DEBUG
		//printf("modo del bitmap es 1\n");
#endif
		int bytes_bitmap = buffer[puntero];
		counter += bytes_bitmap+1;
		//return counter;
		/* Solo se va a recibir el bitmap */		

	}else if(modo_bitmap == 2){ // Bitmap y sample
		
		//pointer_buff += sizeof(unsigned char);
		pointer_buff += 1;
		//counter +=sizeof(unsigned char);
		counter += 1;
		puntero++;
		//Numero de bytes del bitmap
		int bytes_bitmap = buffer[puntero];
#ifdef DAEMON_SERVER_DEBUG
		//printf("numero de bytes del bitmap %d\n", bytes_bitmap);
#endif
		//pointer_buff += sizeof(unsigned char);
		pointer_buff += 1;
		//counter +=sizeof(unsigned char);
		counter += 1;
		puntero++;

		//Bitmap
#ifdef DAEMON_SERVER_DEBUG
		//printf("El bitmap es:\n");
		for(int i = 0; i < bytes_bitmap; i++){
			//printf("bitmap[%d]: %d\n",i,buffer[puntero+i]);
		}
#endif
		//pointer_buff += sizeof(unsigned char) * bytes_bitmap;
		pointer_buff += bytes_bitmap;
		//counter += sizeof(unsigned char) * bytes_bitmap;
		counter += bytes_bitmap;
		puntero+=(unsigned char)bytes_bitmap;

		//printf("modo del bitmap es 2\n");
		
	}


	//Obtaining information of time interval 
	//pointer_buff += sizeof(unsigned int);
	//counter +=sizeof(unsigned int);
	
	time_t time_sample = obtain_time_from_packet(pointer_buff);
	
	 char buff[20];
	 strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&time_sample));
	// if(displaySample == 1)
            	//cout << "Buff: " << buff << endl;
	 //std::ofstream os;
	 //os.open("/home/alberto/Escritorio/net_log.txt", std::ios::out | std::ios::app);
	 //os << buff << ";1" << std::endl;
	 //os.close();
	
	pointer_buff += sizeof(time_t);
	counter +=sizeof(time_t);

	//Parsing packed: mon sample should already have harware conf and number of samples
	for(i = 0; i < mon_sample.n_samples; ++i){ 

	 	print_raw_packet_sample(pointer_buff, mon_sample.sample_size, mon_sample.n_devices_io, mon_sample.n_interfaces, mon_sample.n_cpu, mon_sample.ip_addr_s);
		//cout << "Sample size: " << mon_sample.sample_size << endl;

		//Send data to DB and transform it in json
		//error = import_mon_packet_to_database(pointer_buff, mon_sample.n_devices_io, mon_sample.n_interfaces, mon_sample.n_cpu, mon_sample.ip_addr_s, time_sample);

		/*Point to the next sample in the packet*/
		pointer_buff+=mon_sample.sample_size;
		counter+=mon_sample.sample_size;
	}
//	counter = counter + puntero;
	return counter;
}


/**
 * Log monitoring packet into file.
 * @param[in] packet Packet to be logged (received as received from network buffer)
 * @param[in] size Size of the packet (not counting on number of packets)
 * @param[in] clientIP Ip from which the packet was received.
*/
void log_monitoring_packet(unsigned char *packet, ssize_t size, const std::string &clientIP){
	//4 bytes for ipV4 address
	unsigned int IP_add_bin = 0;
	int error = 0;
	const char * c = clientIP.c_str();
	//Locking in case there are several threads working
	std::unique_lock<std::mutex> lock(logging_lock);
	std::ofstream log_file(MON_LOG_FILE, std::ios_base::out | std::ios_base::app );
	
	//Convert string ip address to binary
	error = inet_pton(AF_INET, c, (void * ) &IP_add_bin);
#ifdef DAEMON_SERVER_DEBUG
	//cout << "IP int: " << IP_add_bin << endl;
#endif
	if (error != 1){
		cerr << "An error occured while trying to get the IP."<< endl;
		IP_add_bin = ntohl(IP_add_bin);
	}else{

		log_file.write((const char *) &IP_add_bin, sizeof(unsigned int));
		//Write size of the packet with teh samples and teh hour plus the number of packets (type of packet) max 127
		log_file.write((const char *) packet, size+sizeof(char));
    	log_file << std::endl;
		//File closed by destrcutor
	}
}

/**
 * Processes information provided by Daemons
 * @param buffer
 * @param size
 * @param clientIP
 * @return
 */
int manage_monitoring_packet (unsigned char * buffer, ssize_t size,const std::string &clientIP){
	int error = 0;
	Hw_conf hw_conf;
	int n_samples= 0;
	error = obtain_hw_conf(clientIP, &hw_conf);
	int info_size = 0;
	unsigned int time_interval= 0;
	if(error ==-1){
		//cerr <<  clientIP << "not found..."<< endl;
    }else{
		//print_hw_conf(&hw_conf);
		n_samples = (int) buffer[BTYPE_POS];
		
		#ifdef DAEMON_SERVER_DEBUG
			//cout << "Number of samples to be unpackaged " << n_samples << endl;
			//cout << "size of request" << info_size << endl;
		#endif
		
		//Obtain interval to create packedsample
		time_interval = convert_char_to_int(&buffer[BTYPE_POS+1]);
		Packed_sample sp(hw_conf, time_interval,n_samples, 50);
		//Include interval of time
		info_size = obtain_sample_from_packet(buffer, sp);
		
	}
	//cout << "SIze of packed to be logged " << info_size << endl;
	//ALBERTO: LOG commented to reduce overhead
	//log_monitoring_packet(&buffer[BTYPE_POS], info_size, clientIP);

	//sp = Packed_sample;

	return error;
}

/**
 * Processes generic tbon packets.
 * Each process should aggregate the data into a global struct for the prometheus exposer
 */
int manage_generic_packet (struct handle_args *info) {
  std::vector<unsigned long> values;
  std::vector<string> keys;
  //Hw_conf hw_conf;
  //int error = obtain_hw_conf(info->client_IP, &hw_conf);
  unsigned char *pointer_buff = &info->buffer[BTYPE_POS];
  //Packed_sample sp(hw_conf, 5000, 1, 50);
  int counter = 4;//12;
  /*Obtain packet type*/
  char type = (char) pointer_buff[counter];
  counter += 1;
  //get counters
  for (; counter < info->size &&
                (counter + (64 + 8)) <= info->size; ) { // 8 bytes per counter add 64bytes per key
    char *key = (char *) calloc(64, 1);
    memcpy(key, &pointer_buff[counter], 64);
    string k(key);
    keys.push_back(k);
    counter += 64;
    unsigned long val = (uint8_t(pointer_buff[counter]) << 56) | (uint8_t(pointer_buff[counter+1]) << 48) | (uint8_t(pointer_buff[counter+2]) << 40) |
                        (uint8_t(pointer_buff[counter+3]) << 32) | (uint8_t(pointer_buff[counter+4]) << 24) | (uint8_t(pointer_buff[counter+5]) << 16) |
                        (uint8_t(pointer_buff[counter+6]) << 8) | uint8_t(pointer_buff[counter+7]);
    values.push_back(val);
    counter += 8;
    free(key);
  }

  /*update index of ips */
  std::string incoming_ip = "";
  for (int i = 0; i < sizeof(info->client_IP); i++) {
    if (info->client_IP[i] == '\0') break;
    incoming_ip = incoming_ip + info->client_IP[i];
  }
  if (std::find(ips_server.begin(), ips_server.end(), incoming_ip) == ips_server.end()) {
    ips_server.push_back(incoming_ip);
  }

  /*Aggregate data -- fields have the same order*/
  for (int i = 0; i < keys.size(); i++) {
    auto aux = map_server.find(keys[i]);
    if(aux != map_server.end())
    {
      // Aggregate
      map_server[keys[i]] += values[i];
    } else {
      // insert
      map_server[keys[i]] = values[i];
    }
  }

  /*Reset aggregations after sending data*/
  _generic_aggregation_server.clear();
  _generic_keys_server.clear();

  //Initiate exposer when we already have counter data.
  startExposer = true;

  return 0;
}

/**
 * Based on the information provided by user (n_processes and profile), it returns the best nodes to run an application
 * @param buffer
 * @param si_other
 * @param socket
 * @return "0" if there isn't info about nodes. "node_name:free_cores"
 */
void * manage_allocate_packet(unsigned char * buffer, struct sockaddr_in si_other, int socket) {
    int np = 2, profile = 1, counter = 0, pointer_final = 0, pairs = 0, index_cadena = 0, no_points = 0;
    char *hostname = (char *) calloc(NI_MAXHOST, sizeof(char));
    char *cadenaFinal = (char *) calloc((np + 4) * (NI_MAXHOST + 3), sizeof(char));
    char *token;
    int ncores = 0, cpu_usage = 0;

    int query = 0;
    char *b = (char *) buffer;
    token = strtok_r(b, ":", &b);
    while (token != NULL) {
        if (strcmp(token, "X") == 0) {
            /*printf("Display next sample ");
            token = strtok_r(b, ":", &b);//token = strtok(NULL, ":");
            printf("with message: %s\n", token);
            query=1;*/
            query = 2;
        } else {
            printf("Query allocate detected\n");
            np = strtol(token, NULL, 10);
            if (np <= 0) {
                /*query = 1;
                token = strtok_r(b, ":", &b);
                char err[] = "ERROR TOK";
                if (token == NULL)
                    token = err;*/
                query = 2;
            } else {
                token = strtok_r(b, ":", &b);//token = strtok(NULL, ":");
                profile = strtol(token, NULL, 10);
                if (profile > 0) {
                    query = 0;
                } else {
                    //query=1;
                    query = 2;
                }
            }
        }
        break;
    }

    if (query == 1) {
        //display information about current state
        displaySample = 1;
        displayedMes = token;

        free(cadenaFinal);
        free(hostname);
    } else if (query == 0) {
        displaySample = 1;   // TO CHECK TEST AND VALIDATION
        /*char *newBuf = curl_es_get_allocation(getElasticSearchAdd(), np, profile);//db_query_allocate(np, profile);
        //std::cout << "Allocation: " << newBuf << std::endl;
        if (newBuf != NULL) {
            char *auxBuff = (char *) calloc(strlen(newBuf), sizeof(char));
            strcpy(auxBuff, newBuf);
            if (newBuf != NULL) { //crear paquete
                int index = 0;
                char *cadena = (char *) calloc(NI_MAXHOST + 3, sizeof(char));
                for (int i = 0; i < strlen(newBuf) && counter < np; i++) {
                    if (i != 0 && pairs != 1 && no_points == 0) {
                        cadena[index_cadena] = ':';
                        index_cadena++;
                    }
                    no_points = 0;

                    int size;
                    //unsigned int size = (unsigned int) newBuf[index];
                    char *pch = strchr(auxBuff, ':');
                    size = pch - auxBuff;
                    char *merge = (char *) calloc(strlen(pch), sizeof(char));
                    strcpy(merge, pch);
                    memset(auxBuff, 0, sizeof(auxBuff));
                    strcpy(auxBuff, &merge[1]);
                    free(merge);
                    //index++;

                    //to manage SIGSEV exceptions
                    if ((index + size) > strlen(newBuf))
                        break;

                    char aux[size + 1];
                    memcpy(aux, &newBuf[index], size);
                    aux[size] = '\0';
                    index += size + 1;
                    char *end;
                    char *campo = static_cast<char *>(malloc(sizeof(char) * size));
                    memset(campo, 0, size);
                    strcpy(campo, aux);
                    if (pairs == 0) {
                        int err = obtainHostNameByIP(campo, hostname);
                        if (err == 0) {
                            strcpy(&cadena[index_cadena], hostname);
                            index_cadena += strlen(hostname);
                        } else {
                            strcpy(&cadena[index_cadena], campo);
                            index_cadena += strlen(campo);
                        }
                        i = index;
                        pairs++;
                    } else if (pairs == 1) { //cpu usage
                        ncores = strtol(campo, NULL, 10);
                        i = index;
                        pairs++;
                    } else { //num cores
                        cpu_usage = strtol(campo, NULL, 10);
                        i = index;
                        pairs++;
                    }

                    //add to cadena
                    if (pairs == 3) {
                        pairs = 0;
                        int c = ncores - ceil(cpu_usage * ncores / 100.0);
                        if (c < 0) c = 0;
                        counter += c; //send nodes until counter>=np --> min number of nodes in which the app could run
                        if (c != 0) {
                            if (counter > np) //return same free cores than np requested.
                                c -= counter - np;

                            std::string c_s = std::to_string(c);
                            strcpy(&cadena[index_cadena], c_s.c_str());
                            index_cadena += strlen(c_s.c_str());
                            i = index;
                            strcat(cadenaFinal, cadena);
                            free(cadena);
                            cadena = (char *) calloc(NI_MAXHOST + 3, sizeof(char));
                            index_cadena = 0;
                        } else {
                            no_points = 1;
                            free(cadena);
                            cadena = (char *) calloc(NI_MAXHOST + 3, sizeof(char));
                            index_cadena = 0;
                        }
                    }
                    free(campo);
                }
                if (strlen(cadenaFinal) == 0) strcpy(cadenaFinal, "NULL");
                free(cadena);
            } else {
                strcpy(cadenaFinal, "NULL");
            }


            size_t slen = sizeof(si_other);
            //std::cout << cadenaFinal << std::endl;
            int res = sendto(socket, cadenaFinal, strlen((const char *) cadenaFinal), 0, (struct sockaddr *) &si_other,
                             slen);
            if (res < 0)
                printf("Error sending packake\n");
            free(auxBuff);
        }*/

        /*free(cadenaFinal); done after
        free(hostname);*/
    }
    free(cadenaFinal);
    free(hostname);
}

/**
 * Do a simple query based on the option received
 * @param buffer Buffer that contains the option.
 * @param clientIP IP client
 */
/*void * manage_query_packet(unsigned char * buffer, struct sockaddr_in si_other, int socket){
    if (buffer[0] == QUERY_CONFIGURATION){
        unsigned char * newBuf = (unsigned char *)db_query_all_Conf();
        size_t slen = sizeof(si_other);
        int res = sendto(socket, newBuf, strlen((const char *)newBuf), 0, (struct sockaddr *)&si_other, slen);
        if(res < 0)
            printf("Error sending packake\n");

    } else if (buffer[0] == QUERY_STATE_NOW){
        unsigned char * newBuf = (unsigned char *)db_query_all_Now();
        size_t slen = sizeof(si_other);
        int res = sendto(socket, newBuf, strlen((const char *)newBuf), 0, (struct sockaddr *)&si_other, slen);
        if(res < 0)
            printf("Error sending packake\n");

    } else if (buffer[0] == QUERY_AVG){
        unsigned char * newBuf = (unsigned char *)db_query_all_Summary();
        size_t slen = sizeof(si_other);
        int res = sendto(socket, newBuf, strlen((const char *)newBuf), 0, (struct sockaddr *)&si_other, slen);
        if(res < 0)
            printf("Error sending packake\n");
    }/*else if (buffer[0] == QUERY_ONE_NOW){
		char * ip = (char*)malloc(sizeof(char)*15);
		strcpy(ip, (char*)&buffer[2]);
		unsigned char * newBuf = (unsigned char *)db_query_spec_CR(ip, -1, -1, -1, -1);
		free(ip);
		size_t slen = sizeof(si_other);
		if(newBuf != NULL) {
			int res = sendto(socket, newBuf, strlen((const char *) newBuf), 0, (struct sockaddr *) &si_other, slen);
			if (res < 0)
				printf("Error sending packake\n");
		} else {
			//to unlock the client
			int res = sendto(socket, &buffer[0], 1, 0, (struct sockaddr *) &si_other, slen);
		}
	}*/
//}

/**
 * it obtain node-name based on its ip (nslookup)
 * @param ip
 * @param host
 * @return
 */
int obtainHostNameByIP(char const * ip, char * host) {

    if (strcmp(ip, "10.0.40.15") == 0) {
        strcpy(host, "compute-11-2");
        return 0;
    } else if (strcmp(ip, "10.0.40.18") == 0) {
        strcpy(host, "compute-11-4");
        return 0;
    } else if (strcmp(ip, "10.0.40.19") == 0) {
        strcpy(host, "compute-11-5");
        return 0;
    } else if (strcmp(ip, "10.0.40.12") == 0) {
        strcpy(host, "compute-11-6");
        return 0;
    } else if (strcmp(ip, "10.0.40.20") == 0) {
        strcpy(host, "compute-11-7");
        return 0;
    } else if (strcmp(ip, "10.0.40.13") == 0) {
        strcpy(host, "compute-11-8");
        return 0;
    } else if (strcmp(ip, "163.117.148.24") == 0) {
        strcpy(host, "arpia.arcos.inf.uc3m.es");
        return 0;
    }else if (strcmp(ip, "127.0.0.1") == 0) {
        strcpy(host, "localhost");
        return 0;
    }
    return 1;

    /*struct sockaddr_in *sa = (sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    socklen_t len;
    char hbuf[NI_MAXHOST];

    memset(&sa->sin_zero, 0, sizeof(sa->sin_zero));

    sa->sin_family = AF_INET;
    sa->sin_addr.s_addr = inet_addr(ip);
    len = sizeof(struct sockaddr_in);

    if (getnameinfo((struct sockaddr *) sa, sizeof(sa), hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD)) {
        ///printf("Can't find host\n");
        return 1;
    } else {
        char *newhbuf = (char *) calloc(strlen(hbuf), 1);// - 5, 1);
        memcpy(newhbuf, hbuf, strlen(hbuf));// - 5);
        //printf("host=%s\n", newhbuf);
        strcpy(host, newhbuf);
        free(newhbuf);
        return 0;
    }*/
}

/*
void createCounterFamilyForNode(std::string nodename, int nio, int nnet){
  std::replace(nodename.begin(), nodename.end(), '.', '_');
  auto& perf_counter = BuildCounter()
    .Name("ip:" + nodename)
    .Help("Node performance counters.")
    .Register(*registry);

  // add and remember dimensional data, incrementing those is very cheap
  auto& cpu_counter =
    perf_counter.Add({{"Counter", nodename}, {"cpu", "%"}});

  auto& mem_counter =
    perf_counter.Add({{"Counter", nodename}, {"mem", "%"}});

  for(int i = 0; i < nio; i++) {
    auto &io_counter =
      perf_counter.Add({{"Counter", nodename},
                        {"io",      "%"}});
  }
  for(int i = 0; i < nnet; i++) {
    auto &net_counter =
      perf_counter.Add({{"Counter", nodename},
                        {"net",     "%"}});
  }

  // ask the exposer to scrape the registry on incoming HTTP requests
  exposer.RegisterCollectable(registry);

  // insert into map
  //family_counters.insert(std::pair<std::string, prometheus::Family<prometheus::Counter>>("ip:" + nodename, perf_counter));

}*/


/**
 * Computes the average load of the cluster
 * @return
 */
std::vector<int> computeMeans(){
  /* This is for map of ips and counters
  int cpu = 0;
  int mem = 0;
  int io_t = 0;
  int io_w = 0;
  int com = 0;
  int type = 0;
  for (auto& family : counter_metrics){
    auto vals = family.second;
    for (int val : vals){
      switch (type){
        case 0: cpu += val; type++; break;
        case 1: mem += val; type++; break;
        case 2: io_t += val; type++; break;
        case 3: io_w += val; type++; break;
        case 4: com += val; type++; break;
        default: type = 0; break;
      }
    }
  }

  std::vector<int> res = {cpu/registered_nodes, mem/registered_nodes, io_t/registered_nodes, io_w/registered_nodes, com/registered_nodes};*/


  std::vector<int> res = {agg_metrics[0]/registered_nodes, agg_metrics[1]/registered_nodes, agg_metrics[2]/registered_nodes, agg_metrics[3]/registered_nodes, agg_metrics[4]/registered_nodes};
  agg_metrics.clear();
  //reset once the next packet has arrived.
  //agg_metrics = {0, 0, 0, 0, 0};
  reset=1;
  return res;
}


/**
 * Initiates HTTP server for prometheus queries
 */
void managePrometheusServer(){
  using namespace prometheus;

  //create an http server running on port 8080
  Exposer exposer{"127.0.0.1:9092"};

  // create a metrics registry
  auto registry = std::make_shared<Registry>();


  // add a new counter family to the registry (families combine values with the
  // same name, but distinct label dimensions)
  auto& perf_counter = BuildGauge()//BuildCounter()
    .Name("cluster_performance")
    .Help("Node-level performance counters.")
    .Register(*registry);

  // add and remember dimensional data, incrementing those is very cheap
  auto& cpu_counter =
    perf_counter.Add({{"node", "performance"}, {"cpu", "%"}});
  auto& mem_counter =
    perf_counter.Add({{"node", "performance"}, {"mem", "%"}});
  auto& io_t_counter =
    perf_counter.Add({{"node", "performance"}, {"io_time", "%"}});
  auto& io_w_counter =
    perf_counter.Add({{"node", "performance"}, {"io_writes", "%"}});
  auto& net_counter =
    perf_counter.Add({{"node", "performance"}, {"comm", "%"}});

  /*cpu_counter.Increment(0);
  mem_counter.Increment(0);
  io_t_counter.Increment(0);
  io_w_counter.Increment(0);
  net_counter.Increment(0);*/

  // add a counter whose dimensional data is not known at compile time
  // nevertheless dimensional values should only occur in low cardinality:
  // https://prometheus.io/docs/practices/naming/#labels
  /*auto& http_requests_counter = BuildCounter()
    .Name("http_requests_total")
    .Help("Number of HTTP requests")
    .Register(*registry);*/

  // ask the exposer to scrape the registry on incoming HTTP requests
  exposer.RegisterCollectable(registry);



  while(1) {
    std::this_thread::sleep_for(std::chrono::seconds(5));

    //const auto random_value = std::rand();
    /*if (random_value & 1) cpu_counter.Increment();
    if (random_value & 2) mem_counter.Increment();
    if (random_value & 4) io_counter.Increment();
    if (random_value & 8) net_counter.Increment();*/
    /*const std::array<std::string, 4> methods = {"GET", "PUT", "POST", "HEAD"};
    //auto method = methods.at(random_value % methods.size());
    // dynamically calling Family<T>.Add() works but is slow and should be
    // avoided
    //http_requests_counter.Add({{"method", method}}).Increment();*/

    /*read map and increment*/
    if (registered_nodes != 0) {
      std::vector<int> current = computeMeans();
      /*cpu_counter.Increment(
        (cpu_counter.Value() >= current[0]) ? current[0] - cpu_counter.Value() : cpu_counter.Value() - current[0]);
      mem_counter.Increment(
        (mem_counter.Value() > current[1]) ? current[1] - mem_counter.Value() : mem_counter.Value() - current[1]);
      io_t_counter.Increment(
        (io_t_counter.Value() > current[2]) ? current[2] - io_t_counter.Value() : io_t_counter.Value() - current[2]);
      io_w_counter.Increment(
        (io_w_counter.Value() > current[2]) ? current[2] - io_w_counter.Value() : io_w_counter.Value() - current[3]);
      net_counter.Increment(
        (net_counter.Value() > current[3]) ? current[3] - net_counter.Value() : net_counter.Value() - current[4]);*/
      cpu_counter.Set(current[0]);
      mem_counter.Set(current[1]);
      io_t_counter.Set(current[2]);
      io_w_counter.Set(current[3]);
      net_counter.Set(current[4]);

      //cout << cpu_counter.Value() << " " << mem_counter.Value() << " " << io_t_counter.Value() << " " << io_w_counter.Value() << " " << net_counter.Value() << endl;
    }
  }
}


/**
 * Initiates HTTP server for prometheus queries
 */
void managePrometheusServerGeneric() {
  using namespace prometheus;

  //Let the system start and collect some performance metrics to include the labels.
  //std::this_thread::sleep_for(std::chrono::seconds(20));
  while (!startExposer) {}


  //create an http server running on port 8080
  Exposer exposer{"127.0.0.1:9093"};

  // create a metrics registry
  auto registry = std::make_shared<Registry>();

  // add a new counter family to the registry (families combine values with the
  // same name, but distinct label dimensions)
  auto &counter_family = BuildCounter()
    .Name("generic_family_counter")
      //.Labels({{"label", "value"}}) --> done it two lines after with all labels
    .Register(*registry);

  //create labels to initialize the counter family --> keys from packets are labels?
  std::map<string, string> aux_map;
  /*aux_map["key"] = "value"; OP1
  //for( auto kv : map_server){ OP2
  //  aux_map["generic_"+kv.first] = "value"; //don't know if makes sense
  //}
  //const prometheus::Labels lbs (aux_map);

  // add and remember dimensional data, incrementing those is very cheap
  auto& gen_counter = counter_family.Add(lbs);
  aux_map.clear();*/

  //auto& cpu_counter =
  //  counter_family.Add({{"CPU", "%"}});

  // ask the exposer to scrape the registry on incoming HTTP requests
  exposer.RegisterCollectable(registry);

  //Vector of counters --> it allows updates with set()
  std::vector<prometheus::Counter> counterVec(map_server.size());

  while (1) {
    std::this_thread::sleep_for(std::chrono::seconds(5));

    //todo: try to update counters instead of creating them each iteration.
    for (auto kv : map_server) {
      aux_map.clear();
      aux_map[kv.first] = "value";
      const prometheus::Labels genlabels(aux_map);
      auto &gen_counter = counter_family.Add(genlabels);
      gen_counter.Increment(kv.second);
    }
    exposer.RegisterCollectable(registry);
  }
}

#endif

