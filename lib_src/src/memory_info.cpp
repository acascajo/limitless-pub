#ifndef MEMORY_INFO
#define MEMORY_INFO

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "daemon_monitor.hpp"
#include "system_features.hpp"
#include "common.hpp"

using namespace std;
/*
****************************************************************************************************************
* Reads memory status and stores info.
****************************************************************************************************************
*/
int read_memory_stats(/*int & mem_total*/){
                
	std::string line_ = "";
    	std::stringstream smt, smf;
    	std::string mem_path("/proc/meminfo");
               
    	double musage_i = 0.0;

    	double memtotal = 0.0;

    	double mt = 0.0, mf = 0.0;

    	std::ifstream myfile(mem_path, std::ifstream::in);

    	if(myfile.good()){

        	getline(myfile,line_);
                
        	std::vector<string> memt = split(line_, ' ');

        	smt << memt[1];
        	smt >> memtotal;

        	smt.clear();

        	smt << memt[1];
        	smt >> mt;
                                
        	memtotal = memtotal / 1000000;

        	log_concat(memtotal, 3);

        	getline(myfile,line_);
        
        	std::vector<string> memf = split(line_, ' ');
        	myfile.close();

        	/* Calculo de la memoria ocupada */
        	smf << memf[1];
        	smf >> mf;

        	musage_i = mt ? (mt - mf) / mt : 0.0;
        	musage_i = musage_i * 100;

        	log_concat(musage_i, 3);

    	}else{

      		return EFILE;

    	}

    	return EOK;
}

/*

*/
int get_mem_total(int & memtotal){
    	string line_;
    	ifstream myfile("/proc/meminfo");

    	if(myfile.good()){
      		getline(myfile,line_);

      		std::vector<string> memt = split(line_, ' ');
      		stringstream smt, smf;
                        
      		smt << memt[1];
      		smt >> memtotal;

      		memtotal = memtotal / 1000000;
    
    	}else{

      		return EFILE;
    
    	}

    	myfile.close();
    
    	return EOK;
}

#endif
