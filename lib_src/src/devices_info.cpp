#ifndef DEVICES_INFO
#define DEVICES_INFO

#include "system_features.hpp"
#include "devices_info.hpp"
#include "daemon_monitor.hpp"
#include <string>
#include <chrono>
#include <vector>
/*
****************************************************************************************************************
*   Read the number of devices
*
*   IN:
*   OUT:
*   RETURNS:
*****************************************************************************************************************
*/
 int read_n_devices(std::vector<IO_dev> & s_dv, int & n_devices){

    string line;
    std::ifstream myfile("/proc/diskstats");

    char dev_name[MAX_NAME_LEN];
    string dev_name_s = "";

    char *dm_name;

    int i;

    unsigned int major, minor;

    if(myfile.good()){

        while(getline(myfile,line)){

            sscanf(line.c_str(), "%u %u %s", &major, &minor, dev_name);

            dev_name_s = dev_name;

            if((dev_name_s.compare("sda") == 0) || (dev_name_s.compare("sdb")==0) || (dev_name_s.compare("sdc")==0) ){
            
                n_devices++;
                header_append(" ");
                header_append(dev_name_s);
                header_append("{w(%) tIO(%)} ");
                IO_dev iodev;
                iodev.dev_name = dev_name_s;
                s_dv.push_back(iodev);
        
            }

        }

    }else{
        return EFILE;
    }

    myfile.close();
    return EOK;
}


/*
****************************************************************************************************************
*    Function that its called each loop to read devices stats
****************************************************************************************************************
*/
void read_devices_stats(std::vector<IO_dev> & s_dv, unsigned int tinterval){

	char dev_name[MAX_NAME_LEN];
    	char *dm_name;
            	
    	int i = 0, nod = 0; //nod = number of device
    	unsigned long rd_diff,wr_diff;

    	char *ioc_dname;
    	unsigned int major = 0, minor = 0;
    	unsigned long aux = 0;

    	using clk = std::chrono::high_resolution_clock;

    	double tw_percent = 0.0, tr_percent = 0.0;
    	double diff_tr = 0.0, diff_tw = 0.0;
    	double tio_percent = 0.0;
        unsigned long long diff_tio =0;

    	string line;
            	

                

    	for(int i = 0;i < s_dv.size();i++){
        
        	ifstream myfile("/proc/diskstats");
        

        	if(myfile.good()){
            		while(getline(myfile,line)){

                	sscanf(line.c_str(), "%u %u %s %lu %lu %lu %lu %lu %lu %lu %u %u %u %u",
                            &major, &minor, dev_name,
                            &s_dv[i].stats[0].rd_ios, &s_dv[i].stats[0].rd_merges, &s_dv[i].stats[0].rd_sec, &s_dv[i].stats[0].rd_ticks,
                            &s_dv[i].stats[0].wr_ios, &s_dv[i].stats[0].wr_merges, &s_dv[i].stats[0].wr_sec, &s_dv[i].stats[0].wr_ticks, 
                            &s_dv[i].stats[0].ios_pgr, &s_dv[i].stats[0].tot_ticks, &s_dv[i].stats[0].rq_ticks);

                		if(s_dv[i].dev_name.compare(dev_name) == 0){
                    
                    		#ifdef DEBUG_DAEMON
		                        cout << "se ha encontrado un dispositivo " << dev_name << endl;
                		#endif

                    /* Total time spent*/
                    
                    diff_tio= s_dv[i].stats[0].tot_ticks - s_dv[i].stats[1].tot_ticks;

                    #ifdef DEBUG_DAEMON
                        cout << "calculando el diff_tio" << endl;
                    #endif

                    tr_percent = diff_tio ? (diff_tr / diff_tio) * 100 : 0.0;
            

                    #ifdef DEBUG_DAEMON
                        cout << "diff_tio calculado"<< endl;
                    #endif



                    #ifdef DEBUG_DAEMON
                        cout << "calculando el diff_tw" << endl;
                    #endif
                    
                    rd_diff = s_dv[i].stats[0].rd_sec-s_dv[i].stats[1].rd_sec;
                    wr_diff = s_dv[i].stats[0].wr_sec -s_dv[i].stats[1].wr_sec;
                    //OBtained from systats
                    tw_percent = (rd_diff+wr_diff) ? ((double)wr_diff) /((double)rd_diff+wr_diff) *100: 0.0;
                   
                    if(tw_percent < 0){
                        tw_percent = 0.0;
                    }
                
                    #ifdef DEBUG_DAEMON
                        cout << "diff_tw y tw percent calculados" << endl;
                    #endif

                    log_concat(tw_percent, 3);

                    tio_percent = tinterval ? ((double)(diff_tio) / (double)tinterval) * 100 : 0;


                    if((tio_percent > 0.0) && (tio_percent < 0.1)){
                
                        tio_percent = 0.1;

                    }else if(tio_percent > 100){
                
                        tio_percent = 100;

                    }

                    #ifdef DEBUG_DAEMON
                        cout << "tio percent caldulado" << endl;
                    #endif

                    log_concat(tio_percent, 3);

                    s_dv[i].stats[1] = s_dv[i].stats[0];
                }
            }

        }
        

        myfile.close();
    }            	
        	
}

/*
****************************************************************************************************************
* Update values of a devices with new values
* IN:
* @rd_ios   Current value number of read operations.
* @rd_sec   Current value seconds in read operations.
* @rd_ticks Current value number of ticks for read operations.
* @wr_ios   Current valie number of write operations.
* @wr_sec   Current valie seconds in write operations.
* @wr_ticks Current value number of ticks for write operations.
* @ios_pqr
* @tot_ticks    Current value number of total ticks.
* @rq_ticks
****************************************************************************************************************
*/
void update_values(struct devices_stats * s_dv[2], unsigned long rd_ios,
        unsigned long rd_merges,
        unsigned long rd_sec,
        unsigned long rd_ticks,
        unsigned long wr_ios,
        unsigned long wr_merges,
        unsigned long wr_sec,
        unsigned int wr_ticks,
        unsigned int ios_pgr,
        unsigned int tot_ticks,
        unsigned int rq_ticks){


            unsigned long aux = 0;
            unsigned int aux_i = 0;

            aux = s_dv[0][0].rd_ios;
            s_dv[0][0].rd_ios = rd_ios;
            s_dv[0][1].rd_ios = aux;

            aux = 0;

            aux = s_dv[0][0].rd_merges;
            s_dv[0][0].rd_merges = rd_merges;
            s_dv[0][1].rd_merges = aux;

            aux = 0;

            aux = s_dv[0][0].rd_sec;
            s_dv[0][0].rd_sec = rd_sec;
            s_dv[0][1].rd_sec = aux;

            aux = 0;

            aux = s_dv[0][0].rd_ticks;
            s_dv[0][0].rd_ticks = rd_ticks;
            s_dv[0][1].rd_ticks = aux;

            aux = 0;

            aux = s_dv[0][0].wr_ios;
            s_dv[0][0].wr_ios = wr_ios;
            s_dv[0][1].wr_ios = aux;

            aux = 0;

            aux = s_dv[0][0].wr_merges;
            s_dv[0][0].wr_merges = wr_merges;
            s_dv[0][1].wr_merges = aux;

            aux = 0;

            aux = s_dv[0][0].wr_sec;
            s_dv[0][0].wr_sec = wr_sec;
            s_dv[0][1].wr_sec = aux;

            aux_i = 0;

            aux_i = s_dv[0][0].wr_ticks;
            s_dv[0][0].wr_ticks = wr_ticks;
            s_dv[0][1].wr_ticks = aux_i;
}



#endif
