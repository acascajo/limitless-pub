
#include <iostream>
#include <unordered_map>
#include <string>
#include <system_features.hpp>
#include <string.h>
#include <mutex>
#include <network_data.hpp>
#include <library.h>
//#include "connector.h"
#include "servidor_monitor.hpp"


std::unordered_map<std::string, Hw_conf> conf_table; 

std::mutex server_data_lock;

/*ALBERTO*/
/**
 * Obtain the hw configuration from a clientIP and stores its values into the database
 * @param hw_conf
 * @return 0 if data insert is OK, -1 in case of error.
 */
int save_config_into_database(Hw_conf hw_conf){

    /*Send to ES*/
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "%Y/%m/%d %T", timeinfo);
    puts(buffer);
    char *timestamp = (char *) malloc(19);
    strcpy(timestamp, buffer);
    char *ip = new char[hw_conf.ip_addr_s.length() + 1];
    strcpy(ip, hw_conf.ip_addr_s.c_str());
    //curl_es_conf(getElasticSearchAdd(),hw_conf.n_cpu, hw_conf.n_cores, hw_conf.mem_total, 0, hw_conf.n_devices_io, hw_conf.n_interfaces,ip, timestamp);
    free(timestamp);
    //free(ip);
	//QUITADO PARA PRUEBA LIB int error = db_insert_CONF(hw_conf.ip_addr_s.c_str(), hw_conf.n_cpu, hw_conf.n_cores, hw_conf.mem_total, 0, hw_conf.n_devices_io, hw_conf.n_interfaces);
	//error = db_insert_CONF("127.0.0.2", 2, 5, hw_conf.mem_total, hw_conf.n_gpu, hw_conf.n_devices_io, hw_conf.n_interfaces);
	//error = db_insert_CONF("127.0.0.3", 2, 10, hw_conf.mem_total, hw_conf.n_gpu, hw_conf.n_devices_io, hw_conf.n_interfaces);
	//error = db_insert_CONF("127.0.0.321", hw_conf.n_cpu, hw_conf.n_cores, hw_conf.mem_total, hw_conf.n_gpu, hw_conf.n_devices_io, hw_conf.n_interfaces);

	/*db_query_all_Conf();*/
	/*if (error == -1){
		return error;
	}*/

	return 0;//error;
}


int log_info_server_data(Hw_conf hw_conf) {
    int error = 0;

    unsigned char *buffer = (unsigned char *) calloc(
            (CONF_PACKET_SIZE + (hw_conf.hostname.length() + 1)) + sizeof(unsigned int),
            sizeof(unsigned char *));
    std::ofstream log_file_hardware(HW_LOG_FILE, std::ios_base::out | std::ios_base::app);
    //Puts info into buffer and returns the number of bytes.
    error = log_hw_conf(&hw_conf, buffer);
    if (error < 1) {
        cerr << "Error logging hardware info to file" << endl;
    } else {
        log_file_hardware.write((const char *) buffer, error);
    }

    //save to database
    /*error += save_config_into_database(hw_conf);
    error = (error < 0) ? -1 : 0;*/

    free(buffer);

    return error;
}

/**
    Obtain the hardware configuration from a clientIP that  is already registered.
    @param[in] clientIP     String containing the IP of the client.
    @param[out] hw_conf     Pointer to a Hw_conf structure that will contain the result of the search.
    @returns 0 in case everything went ok, -1 in case of error or key was not found

*/


int obtain_hw_conf(const std::string & clientIP, Hw_conf * hw_conf){
    	int error = 0;
    	//It locks the acces to shared data. TODO: To improve to shared reader only one writer scheme
   	    std::unique_lock<std::mutex> lock(server_data_lock);
    	std::unordered_map<std::string,Hw_conf>::const_iterator element = conf_table.find(clientIP);

    	if(element == conf_table.end()){
        	//key was not found, the configuration does not exist
        	error = -1;

    	}else{
        	*hw_conf = element->second;
    	}

    	return error; 
}

int insert_hw_conf( Hw_conf hw_conf){
    	int error =0;
    	std::string IP_client (hw_conf.ip_addr_s);
    	//It locks the acces to shared data. TODO: To improve to shared reader only one writer scheme
     	std::unique_lock<std::mutex> lock(server_data_lock);
    	conf_table[IP_client] = hw_conf;
    	//conf_table.insert(std::make_pair<std::string,Hw_conf> (IP_client,hw_conf));
    	log_info_server_data(hw_conf);
    	return error;
}

int recovery_insert_hw_conf( Hw_conf hw_conf){
    int error =0;
    std::string IP_client (hw_conf.ip_addr_s);
    //It locks the acces to shared data. TODO: To improve to shared reader only one writer scheme
    std::unique_lock<std::mutex> lock(server_data_lock);
    conf_table[IP_client] = hw_conf;

    return error;
}









