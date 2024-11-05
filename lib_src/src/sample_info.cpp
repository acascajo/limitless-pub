#include <vector>
#include <string>
#include <iostream>
#include "daemon_monitor.hpp"
#include "common.hpp"

using namespace std;


/*
****************************************************************************************************************
* Splits the log using dots, spaces and tab characters.
* IN:
* @string	String representing the log content.
* RETURNS:
* Vector of strings with the splitted contents.
****************************************************************************************************************
*/
std::vector <std::string> split_log(std::string str){
    	if(str.length()==0){
        		std::vector<std::string> internal;
    	}

    	std::vector<std::string> internal;
        	std::stringstream ss(str); // Turn the string into a stream.
        	std::string tok;
        	while(getline(ss, tok, '.')) {
            		std::stringstream ss2(tok);
		std::string tok2;
            		while(getline(ss2,tok2,' ')){

                		std::stringstream ss3(tok2);
                		std::string tok3;
                		while(getline(ss3,tok3,'\t')){
                    			if(tok != ""){
                        				internal.push_back(tok3);
                    			}
                		}


		}
        	}
        	return internal;
}

/*
****************************************************************************************************************
* Parse sample to include on the buffer
* IN:
* @sample	String including sample to be parsed.
* RETURNS:
* Vector of strings including the parsed contents.
****************************************************************************************************************
*/
std::vector <std::string> parse_log(std::string sample){
 	std::vector<std::string> log_vector;

        	std::stringstream sdouble;
        	std::string sinsert = "";

        	/* Getting the ip of the sample */
        	std::vector<std::string> sample_split = split(sample, ' ');
        	std::string ip_ = sample_split[0];



        	log_vector = split_log(ip_);

        	for(int i = 1; i < sample_split.size(); i++ ){

            		sdouble.str("");

            		sdouble << stoi(sample_split[i]);


            		log_vector.push_back(sdouble.str());
        	}

        	return log_vector;

}



/*
****************************************************************************************************************
* Pack sample into buffer to be sent.
* IN:
* @sample String to be packed in the buffer.
****************************************************************************************************************
*/
void pack_sample(std::string sample, unsigned char packed_buffer[1024], int & samples_packed, int & n_samples, int & sample_pt, int & packed_bytes, const int n_devices, const int n_interfaces){

    	std::vector<std::string> svec = parse_log(sample);
    	int sample_concat = 0;


    	if(samples_packed == n_samples){
    		std::cout << "hay que enviar el paquete" << std::endl;

        		memset(&(packed_buffer),0, 1024);

        		std::cout << "memset" << std::endl;
        		sample_pt = 0;
        		packed_bytes = 0;
        		samples_packed = 0;
    	}


    	if(packed_bytes == 0){
        		sample_pt = 0;
        		sample_concat = 0;
    	}else{
        		sample_concat = 4;
    	}



    	std::cout << "antes del for" << std::endl;

    	for(int isample = sample_concat; isample < svec.size();){
    		std::cout << "loop" << std::endl;
        		if(sample_pt == 4){
                            	//cout << "numero de samples: *" << n_samples << "*" << endl;
            			packed_buffer[sample_pt] = (unsigned char) n_samples;
            			sample_pt++;
		}else if(sample_pt == 9){
                            	//cout << "numero de dispositivos: *" << n_devices << "*" << endl;
            			packed_buffer[sample_pt] = (unsigned char) n_devices;
            			sample_pt++;
        		}else if(sample_pt == (9+(2*n_devices)+1)){
                            	//cout << "numero de interfaces: *" << n_interfaces << "*" << endl;
            			packed_buffer[sample_pt] = (unsigned char) n_interfaces;
            			sample_pt++;
        		}else{
            			packed_buffer[sample_pt] = (unsigned char)stoi(svec[isample]);
            			sample_pt++;
            			isample++;
        		}

    	}


	packed_bytes = sample_pt;

    	samples_packed++;

}

