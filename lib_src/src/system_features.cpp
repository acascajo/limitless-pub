#include <algorithm>
#include <vector>
#include <dirent.h>
#include <iostream>
#include <string>
#include <sstream>
#include <arpa/inet.h>
#include "system_features.hpp"

using namespace std;

//#include "../include/temp_info.hpp"
//#include "../src/temp_info.cpp"
std::string log_line_s, header_s;


void print_hw_conf(Hw_conf* hw_conf){

	cout << "*****HW Configuration Received*****" <<endl;
	cout << "*****IP: " << hw_conf->ip_addr_s << "*****" << endl;
	cout << "*****Number of CPUs: " << hw_conf->n_cpu << "*****" << endl;
	cout << "*****Number of cores: " << hw_conf->n_cores << "*****" << endl;
	cout << "*****Total memory: " << hw_conf->mem_total << "GB *****" << endl;
	cout << "*****Number of IO devices: " << hw_conf->n_devices_io << "*****" << endl;
	cout << "*****Number of Network Interfaces: " << hw_conf->n_interfaces << "*****" << endl;
    	cout << "*****Number of CPU for Temp: " << hw_conf->n_core_temps<< "*****" << endl;
	
	//cout << "*****Modo bitmap: " << hw_conf->modo_bitmap << "*****" << endl; 
}


/**
* Logs hardware info to a buffer in binary.
    @param[in] hw_conf Hardware configuration to be logged.
    @param[in, out] buffer Buffer in which the binary information will be logged. Must be already allocated.

*/
int log_hw_conf(Hw_conf* hw_conf, unsigned char * buffer){

  	int counter = 0;
    	int error = 0;
    	unsigned int IP_add_bin = 0; //IP in decimal

    	//Convert string to char
    	const char * c = hw_conf->ip_addr_s.c_str();

    	//Convert string ip address to binary
	error = inet_pton(AF_INET, c, (void * ) &IP_add_bin);
    #ifdef DAEMON_SERVER_DEBUG
        cout << "IP int: " << IP_add_bin << endl;
    #endif
        if (error != 1){
            cerr << "An error occured while trying to get the IP."<< endl;
            IP_add_bin = ntohl(IP_add_bin);
            return EGEN; 
        }else{
            memcpy((void *)&(buffer[counter]), (void *) &IP_add_bin, sizeof(IP_add_bin));
            counter += sizeof(IP_add_bin);
            buffer[counter]= hw_conf->n_cpu; //Mayber substitute for a memmcoy for generality
            counter += sizeof(unsigned char);
            buffer[counter]= hw_conf->n_cores; //Mayber substitute for a memmcoy for generality
            counter += sizeof(unsigned char);
            buffer[counter]= hw_conf->mem_total; //Mayber substitute for a memmcoy for generality
            counter += sizeof(unsigned char);
            buffer[counter]= hw_conf->n_devices_io ; //Mayber substitute for a memmcoy for generality but system call
            counter += sizeof(unsigned char);
            buffer[counter]= hw_conf->n_interfaces ; //Mayber substitute for a memmcoy for generality but system call
            counter += sizeof(unsigned char);
	    buffer[counter] = hw_conf->hostname.length();
	    counter+=sizeof(unsigned char);
            memcpy((void *)&(buffer[counter]), hw_conf->hostname.c_str(), hw_conf->hostname.length());
            counter += hw_conf->hostname.length();
        }
        return counter;
}

/*
 ***************************************************************************
* Append info (in double precission) to log.
* IN:
* @data     Data to be appended to the log.
* @precis       Decimal precision for the next floating-point value inserted.
 ***************************************************************************
*/
void log_concat(double data, short precis){
        string rd = "";
        stringstream ss;

        ss.precision(precis);
        ss << data;
        ss >>rd;

        log_line_s.append(rd);
        log_line_s.append(" ");
}

/*
 ***************************************************************************
* Clear log.
 ***************************************************************************
*/
void log_clear(){
        log_line_s.resize(0);
}

/*
 ***************************************************************************
* Append infor about interfaces to log header.
***************************************************************************
*/
void log_concat_interfaces(Hw_conf hw_features){
        int i = 0;
        string rd = "";
        stringstream ss;

        for(; i < hw_features.net_interfaces.size(); i++){

            header_s.append(hw_features.net_interfaces[i].net_name);
            header_s.append("{Gb/s NetUsage(%)} ");

        }
}

void log_append(string toappend){
        log_line_s.append(toappend);
        log_line_s.append(" ");
}

string get_log_line(){
    return log_line_s;
}

void header_append(string toappend){
    header_s.append(toappend);
}

string get_header_line(){
    return header_s;
}


void log_concat_coretemps(vector<Temp_features> temp_features){
        int i = 0;
        string rd = "";
        stringstream ss;

        for(; i < temp_features.size(); i++){
	    header_s.append(" coretemp");
	    header_s.append(to_string(i));
	    header_s.append("{currentTemp(Cº) Temp(%)}");

        }
}
