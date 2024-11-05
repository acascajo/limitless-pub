#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include "system_features.hpp"
#include "common.hpp"
#include "net_info.hpp"
#include <string>

using namespace std;


/*
****************************************************************************************************************
* Reads number of network interfaces.
****************************************************************************************************************
*/
int read_n_net_interface(std::vector<Net_dev> & net_interfaces, int & n_interfaces){


    	#ifdef DEBUG_DAEMON
        	cout << "dentro del read_n_net interfaces" << endl;
    	#endif

    	/* TODO: global variable on system_features */
    	string net_directory = "/sys/class/net";

    	std::vector<std::string> net_names;

    	/* Getting the name of the interfaces */
    	net_names = read_directory(net_directory);

    	/* TODO: Comprobar si tiene velocidad esa interfaz y en caso de que no tenga se elimina */
    	string line_n_speed,line_n_carrier, path("/sys/class/net/"), path_speed, path_carrier;
    	std::stringstream ss,ssc;
    	int n_speed = 0;
    	int n_carrier = 0;

    	unsigned long long aux = 0;

    	double total_bits = 0.0;
    	unsigned long long speed_bits = 0;

    	int net_size = net_names.size();
    
    	#ifdef DEBUG_DAEMON
        	cout << "numero de interfaces es: "<< net_size<<"--------------------------" << endl;
    	#endif

    	std::string aux_net_name = "";

   	for(int i = 0; i < net_size; i++){
        
        	path_speed.clear();
        	path_carrier.clear();

        	/* Getting path to speed an carrier for each interface */
        	path_speed.append(path);
        	path_carrier.append(path);
        	path_speed.append(net_names[i]);
        	path_carrier.append(net_names[i]);

        	path_speed.append("/speed");
        	path_carrier.append("/carrier");

        	#ifdef DEBUG_DAEMON
            		cout << "se va a abrir el archivo "<<path_speed << endl;
        	#endif


        	std::ifstream n_speed_file(path_speed);
        	std::ifstream n_carrier_file(path_carrier);
        	if(n_speed_file.good() && n_carrier_file.good()){
            		if(getline(n_carrier_file,line_n_carrier)){
                		ssc.clear();
                		ssc << line_n_carrier;
                		ssc >> n_carrier;
                    
                		if(n_carrier == 1){ //The current physical link state is up
                    			if(getline(n_speed_file,line_n_speed)){

                        			#ifdef DEBUG_DAEMON
                            				cout << "Interfaz: "<< net_names[i] << " con velocidad" << endl;
                        			#endif


                        			Net_dev netdev;
                        			netdev.net_name = string(net_names[i]);

                        			ss << line_n_speed;
                        			ss >> netdev.speed;

                        			if(netdev.speed >= 0){  /* If speed is equals or greater than 0, the net is included */
                            
                            				net_interfaces.push_back(netdev);
                        
                        			}else{  /* If speed is lower than 0, the net is erase from the list and not included */
                        
                            				net_names.erase(net_names.begin()+i);
                            				i--;
                    
                            				if(net_size != net_names.size()){
                                				net_size = net_names.size();
                            				}   

                        			}                            

                        			ss.clear();
                        
                        
                    			}else{
                        			net_names.erase(net_names.begin()+i);
                        			i--;
                    
                        			if(net_size != net_names.size()){
                            				net_size = net_names.size();
                        			}
                    			}


                		}else{ //The current physical link state is down
                
                    			net_names.erase(net_names.begin()+i);
                    			i--;
            
                    			if(net_size != net_names.size()){
                        			net_size = net_names.size();
                    			}
                		}

       			}else{ //The carrier file is empty

                		net_names.erase(net_names.begin()+i);
                		i--;

                		if(net_size != net_names.size()){
                    			net_size = net_names.size();
                		}	
            		}    
        	}else{
            		return -2;
        	}
        

        	n_speed_file.close();
        	n_carrier_file.close();

	}

    	n_interfaces = net_interfaces.size();

    	#ifdef DEBUG_DAEMON
        	cout << "n_interfaces es: "<<n_interfaces << endl;
    	#endif
 	
	return 0;
            
}

/*
****************************************************************************************************************
* Reads network status and stores info.
****************************************************************************************************************
*/
void read_net_stats(std::vector<Net_dev> & net_interfaces) {

	string line_n_speed, line_rx_bytes, line_tx_bytes, path("/sys/class/net/"), path_speed, path_rx, path_tx;

	std::stringstream ss, rr, tt;
	ss.precision(4);
	rr.precision(4);
	tt.precision(4);
	unsigned long long n_speed = 0;

	unsigned long long rx_bytes = 0;
	unsigned long long tx_bytes = 0;
	unsigned long long aux = 0;

	unsigned long long diff_rx = 0.0;
	unsigned long long diff_tx = 0.0;

	unsigned long rx_bits = 0;
	unsigned long tx_bits = 0;
	unsigned long long total_bits = 0.0;
	unsigned long long speed_bits = 0;
	unsigned long long speed_bytes = 0;
	unsigned long long total_bytes = 0;
	double percent_net_usage = 0.0;

	for (int i = 0; i < net_interfaces.size(); i++) {
		//Interchange with previous one to have both
		net_interfaces[i].stats[1] = net_interfaces[i].stats[0];

		ss.clear();
		rr.clear();
		tt.clear();

		path_speed.clear();
		path_rx.clear();
		path_tx.clear();


		path_speed.append(path);
		path_speed.append(net_interfaces[i].net_name);

		path_rx.append(path_speed);
		path_tx.append(path_speed);

		path_rx.append("/statistics/rx_bytes");
		path_tx.append("/statistics/tx_bytes");

		std::ifstream n_speed_file(path_speed);

		std::ifstream rx_bytes_file(path_rx);
		std::ifstream tx_bytes_file(path_tx);


		getline(rx_bytes_file, line_rx_bytes);

		rr << line_rx_bytes;
		rr >> rx_bytes;


		net_interfaces[i].stats[0].rx_bytes = rx_bytes;


		getline(tx_bytes_file, line_tx_bytes);

		tt << line_tx_bytes;
		tt >> tx_bytes;


		net_interfaces[i].stats[0].tx_bytes = tx_bytes;


#ifdef DEBUG_DAEMON
		cout <<"bytes antes: "<<net_interfaces[i].stats[0].rx_bytes <<" bytes ahora: "<< net_interfaces[i].stats[1].rx_bytes << endl;
#endif


		diff_rx = net_interfaces[i].stats[0].rx_bytes - net_interfaces[i].stats[1].rx_bytes;

		diff_tx = net_interfaces[i].stats[0].tx_bytes - net_interfaces[i].stats[1].tx_bytes;


		total_bits = (diff_rx + diff_tx) * 8;
		total_bytes = total_bits / 8;

		speed_bits = (unsigned long long) net_interfaces[i].speed * (unsigned long long) 1000000;
		speed_bytes = speed_bits / 8;

#ifdef DEBUG_DAEMON
        cout << "total_bytes es: "<< total_bytes << ", " << net_interfaces[i].speed << endl;
        cout << "speed_bytes es: "<< speed_bytes << endl;
#endif

		/* Net speed on Gb/s */
		log_concat(net_interfaces[i].speed / 1000, 3);

#ifdef DEBUG_DAEMON
		cout <<"total de bits es: "<< total_bits<<" speed_bits es: "<< speed_bits << endl;
#endif

		//For full duplex comms different computation is needed

		//percent_net_usage = speed_bits ? (double) (total_bits / speed_bits) * 100 : 0;
		if (diff_rx > diff_tx) {
			percent_net_usage = speed_bits ? (double) ((double) diff_rx / (double) speed_bits) * 800 : 0;
		} else {
			percent_net_usage = speed_bits ? (double) ((double) diff_tx / (double) speed_bits) * 800 : 0;
		}


		if (percent_net_usage > 0.0 && percent_net_usage < 0.1) {
			percent_net_usage = 0.1;
		}

#ifdef DEBUG_DAEMON
        cout << "velocidad de la interfaz es: "<< n_speed << endl;
        cout << "percent net usage es: "<< percent_net_usage<< endl;
#endif

		log_concat(percent_net_usage, 3);

		n_speed_file.close();
		rx_bytes_file.close();
		tx_bytes_file.close();

	}
}


/*
 ***************************************************************************
* Get address of current host.
***************************************************************************
*/
void get_addr(string & my_addr_s, string & hostname_s){
    	string direccion;
    	char myhostname[64];
    	struct hostent *lh;
    	struct sockaddr_in host_addr;
    	char * my_addr;

    	gethostname(myhostname, sizeof(myhostname));

	hostname_s = string(myhostname);

	cout << hostname_s << endl; 	
	#ifdef DEBUG_DAEMON
		cout <<"hostname obtenido" << endl;
        #endif	
	

 	bzero((char *)&host_addr, sizeof(host_addr));

    	lh = gethostbyname2(myhostname , AF_INET);
	//cout << "get host by name 2"<< endl;
    	memcpy(&(host_addr.sin_addr),lh->h_addr_list[0], lh->h_length);
	//cout << "memcpy hecho"<< endl;
    	if (lh){
        	my_addr = inet_ntoa(host_addr.sin_addr);
        	my_addr_s = my_addr;
    	}

    	else{
        	herror("gethostbyname");
    	}
	//cout <<"fuera de get_addr" << endl;

}
