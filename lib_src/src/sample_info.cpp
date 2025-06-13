#include <array>
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
void pack_sample(const std::string& sample,
				std::array<unsigned char, 1024>& packed_buffer,
				int & samples_packed, int & n_samples, int & sample_pt,
				int & packed_bytes, const int n_devices, const int n_interfaces){

    std::vector<std::string> svec = parse_log(sample);
    int sample_concat = 0;

    if(samples_packed == n_samples){
        //memset(&packed_buffer,0, 1024);
    	packed_buffer.fill(0);
        sample_pt = 0;
        packed_bytes = 0;
        samples_packed = 0;
    }

	sample_concat = (packed_bytes == 0) ? 0 : 4;

	for (int isample = sample_concat; isample < static_cast<int>(svec.size());) {
		if (sample_pt == 4) {
			packed_buffer[sample_pt++] = static_cast<unsigned char>(n_samples);
		} else if (sample_pt == 9) {
			packed_buffer[sample_pt++] = static_cast<unsigned char>(n_devices);
		} else if (sample_pt == (9 + (2 * n_devices) + 1)) {
			packed_buffer[sample_pt++] = static_cast<unsigned char>(n_interfaces);
		} else {
			packed_buffer[sample_pt++] = static_cast<unsigned char>(std::stoi(svec[isample++]));
		}
	}

	packed_bytes = sample_pt;
	samples_packed++;
}

