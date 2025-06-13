#ifndef NETWORK_DATA
#define NETWORK_DATA

#include "Packed_sample.hpp"
#include "network_data.hpp"
#include "server_data.hpp"
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <map>
#include <algorithm>
#include <typeinfo>

using namespace std;

std::mutex logging_lock;
bool startExposer = false;

/*David wants this to show information about some tests*/
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

	// This is for another version
	/*int cache_ratio = (int) buffer[counter];
	counter += SIZE_PACKED_PERCENTAGE;
	int cpu_stalled = (int) buffer[counter];
	counter += SIZE_PACKED_PERCENTAGE;*/

	auto time = std::chrono::system_clock::now();
	std::time_t now = std::chrono::system_clock::to_time_t(time);
	cout << displayedMes << endl;
	cout << "Time: " << std::ctime(&now) << endl;
    cout << "Mem usage: " << mem_usage_perc << "%" << endl;
    cout << "CPU Busy : " << CPUidle_perc << "%" << endl;


    /*ofstream myfile;
    myfile.open("/tmp/log_local.txt", ios::out | ios::app);
    myfile << ip << " " << std::ctime(&now) << " " << mem_usage_perc << " "
    << CPUidle_perc << " " << io_devices_io_perc << " " << io_devices_w_perc
    << " " << net_devices_speed << " " << net_devices_usage_perc << "\n";*/
    /*myfile << "*********************\n";
    myfile << "IP: " << ip << endl;
    myfile << "TIME: " << std::ctime(&now) << endl;
    myfile << "Mem usage: " << mem_usage_perc << "%" << endl;
    myfile << "CPU Busy : " << CPUidle_perc << "%" << endl;
    myfile << "Energy usage: " << pw_cpu << "Joules" << endl;
    //myfile << "Net devices speed : " << net_devices_speed << "Gb/s" << endl;
    //myfile << "Net devices usage" << net_devices_usage_perc << "%" << endl;
    myfile.close();*/

	//old
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

		//pointer_buff += sizeof(unsigned char);
		pointer_buff += 1;
		//counter +=sizeof(unsigned char);
		counter += 1;
		puntero++;

		//Bitmap
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

		//Obtain interval to create packedsample
		time_interval = convert_char_to_int(&buffer[BTYPE_POS+1]);
		Packed_sample sp(hw_conf, time_interval,n_samples, 50);
		//Include interval of time
		info_size = obtain_sample_from_packet(buffer, sp);
		
	}
	//cout << "SIze of packed to be logged " << info_size << endl;
	//ALBERTO: LOG commented to reduce overhead
	log_monitoring_packet(&buffer[BTYPE_POS], info_size, clientIP);

	//sp = Packed_sample;

	return error;
}

/**
 * Processes generic tbon packets.
 * Each process should aggregate the data into a global struct for the prometheus exposer
 */
int manage_generic_packet (struct handle_args *info) {
	int counter = 12; // last try 4
    vector<std::string> keys;
    vector<unsigned long> values;

    // Get counters
    while (counter + 72 <= info->size) { // 64 bytes per key + 8 bytes per value
        // get key 64 bytes
        string key(reinterpret_cast<const char*>(&info->buffer[counter]), 64);
        keys.push_back(key);
        counter += 64;

        // get value 8 bytes (unsigned long)
        unsigned long value = 0;
        for (int i = 0; i < 8; ++i) {
            value = (value << 8) | info->buffer[counter + i];
        }
        values.push_back(value);
        counter += 8;
    }

	// Update index of IPs
    string incoming_ip;
    for (int i = 0; i < sizeof(info->client_IP); ++i) {
        if (info->client_IP[i] == '\0') break;
        incoming_ip += info->client_IP[i];
    }

    if (find(ips_server.begin(), ips_server.end(), incoming_ip) == ips_server.end()) {
        ips_server.push_back(incoming_ip);
    }

	/*Aggregate data -- fields have the same order*/
    for (size_t i = 0; i < keys.size(); ++i) {
        map_server[keys[i]] += values[i]; // If no exists, it is initialised to 0 and incremented
    }

    // reset
    _generic_aggregation_server.clear();
    _generic_keys_server.clear();

    // Initialise exporter once we hace the counters
    startExposer = true;

    return 0;
	
	/*unsigned char *pointer_buff = &info->buffer[BTYPE_POS];
	unsigned long val = (uint8_t(pointer_buff[counter]) << 56) | (uint8_t(pointer_buff[counter+1]) << 48) | (uint8_t(pointer_buff[counter+2]) << 40) |
						(uint8_t(pointer_buff[counter+3]) << 32) | (uint8_t(pointer_buff[counter+4]) << 24) | (uint8_t(pointer_buff[counter+5]) << 16) |
						(uint8_t(pointer_buff[counter+6]) << 8) | uint8_t(pointer_buff[counter+7]);
	values.push_back(val);
	counter += 8;*/
}


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


#endif

