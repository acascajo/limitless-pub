#ifndef POWER_CPU_INFO
#define POWER_CPU_INFO

#include "system_features.hpp"
#include "common.hpp"
#include "dir_path.hpp"
#include "power_cpu_info.hpp"
#include <cmath>

int get_power_path(vector<Power_cpu> & pwcpu, int & path_error, int n_cpu){

	string common_pwcpu_path = "/sys/devices/virtual/powercap/intel-rapl";
	string pwcpu_path = "";
	std::stringstream ss;

	std::vector<std::string> ptc = read_directory(common_pwcpu_path);	

	if(ptc.empty()){
	
		for(int i = 0; i < n_cpu; i++){
			header_append(" ");
			header_append(to_string(i));
			header_append("PWUsage(Joules)");
		}

		path_error = -1;
		return EDIR;
	}

	std::vector <std::string> result;

	for(int i = 0; i < ptc.size(); i++){

		if(ptc[i].compare(0,10,"intel-rapl") == 0){

			pwcpu_path.clear();
	
			pwcpu_path.append(common_pwcpu_path);

			pwcpu_path.append("/");

			pwcpu_path.append(ptc[i]);

			Power_cpu pcpu;
			pcpu.power_path = pwcpu_path;

			pwcpu.push_back(pcpu);

		}

	}

	string ultimate_path = "";

	bool encontrada = false;

	for(int i = 0; i < pwcpu.size(); i++){
		encontrada = false;

		ultimate_path.clear();

		ultimate_path = pwcpu[i].power_path;

		std::vector<std::string> ppath = read_directory(pwcpu[i].power_path);


		Dir_path a (ultimate_path, ppath);

		std::vector<std::string> npower;

		string dir_aux = "";

		std::vector<Dir_path> directories;
		directories.push_back(a);

		while(!directories.empty() && (!encontrada)){

		
			/* Copiamos la carpeta a una variable temporal y la eliminamos */
			Dir_path dcurrent(directories.back().path, directories.back().dfiles);
			directories.pop_back();


			while(!dcurrent.dfiles.empty()){

				if(dcurrent.dfiles.back().compare("energy_uj") == 0){
					#ifdef DEBUG_PW_CPU
						cout << "ruta final de las temperaturas es: "<< dcurrent.path << endl;
					#endif
					string path_aux = "";
					path_aux.append(dcurrent.path);
					path_aux.append("/");
					path_aux.append(dcurrent.dfiles.back());
		
//					pwcpu[i].energy_path = dcurrent.dfiles.back();
					pwcpu[i].energy_path = path_aux;
					dcurrent.dfiles.pop_back();
//					dcurrent.dfiles.back();

					encontrada = true;
				}else{
					dcurrent.dfiles.pop_back();
				}
			}
		}
		

	}
		
//	cout <<"el numero de cpu con enegia es: "<< pwcpu.size() << endl;

	for(int i = 0; i < pwcpu.size(); i++){
		header_append(" ");
		header_append(to_string(i));
		header_append("PWUsage(Joules)");
	}

		

	return EOK;
}

int get_power(std::vector<Power_cpu> & pwcpu, int path_error, int n_cpu){

	std::stringstream ss;
	string line_pwcpu = "";

	unsigned long long value_diff = 0;
//	unsigned long long value_prev = 0;
	unsigned long long value_now = 0;
	double value_div = 0.0;
	
	if(path_error == -1){
		for(int i = 0; i < n_cpu; i++){
			log_concat(0,2);
		}
	}else{
		for(int i = 0; i < pwcpu.size(); i++){
			std::ifstream pwcpu_file(pwcpu[i].energy_path);

            		#ifdef DEBUG_PW_CPU
                		cout << "file es " << pwcpu[i].energy_path << endl;
            		#endif

            		if(pwcpu_file.good()){
                		getline(pwcpu_file,line_pwcpu);
                		ss.clear();
               	 		ss << line_pwcpu;
                		ss >> value_now;
			
			

				//IF both values are the same then the value division must not be done
				value_diff =(unsigned long long)value_now - pwcpu[i].energy_value;
				
				if(value_now < pwcpu[i].energy_value){
				//Strange behaviour when resetting counter on the file, to avoid 0s 
				//we take the current value as unique energy
					value_div = pwcpu[i].diff_energy_value;
				}else{
					value_div =(double) (value_diff / 1000000.0);	
				}
				pwcpu[i].energy_value = value_now;
				pwcpu[i].diff_energy_value = value_div;
				log_concat(std::round(value_div),2);
			}
	
		}
	}



	return EOK;
}

#endif
