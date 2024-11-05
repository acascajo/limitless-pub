#ifndef TEMP_INFO
#define TEMP_INFO

#include "system_features.hpp"
#include "common.hpp"
#include "temp_info.hpp"
#include <sys/stat.h>
#include "dir_path.hpp"
#include <string.h>
#include <iostream>
#include "daemon_monitor.hpp"


int get_temp_path(vector<Temp_features> & temp_features, int & n_core_temps){


 	#ifdef DEBUG_TEMP
        	cout << "dentro del get temp_path" << endl;
    	#endif

	string common_path = "/sys/bus/platform/devices";
	string core_path= "";
    	double max_local = 0.0;
    	string line_temp_max = "";
    	std::stringstream ss;

	struct stat sb;

	std::vector <std::string> ptc = read_directory(common_path);

    	if(ptc.empty()){
        	return EDIR;
    	}

	std::vector <std::string> result;

	for(int i = 0; i < ptc.size(); i++){
        	if((ptc[i].compare(0,9,"coretemp.")==0)){

            		//Calculamos la ruta del coretemp.*
            		core_path.clear();

            		core_path.append(common_path);

            		core_path.append("/");

            		core_path.append(ptc[i]);

            		Temp_features temp_f;
            		temp_f.temp_path = core_path;

            		temp_features.push_back(temp_f);

        	}
	}

    	// Buscar ruta hacia los temp*_input
    	string ultimate_path = "";

    	bool encontrada = false;

    	for(int i = 0; i < temp_features.size(); i++){
        	encontrada = false;
        	ultimate_path.clear();
        	ultimate_path = temp_features[i].temp_path;

        	std::vector<std::string> cpath = read_directory(temp_features[i].temp_path);

        	Dir_path a (ultimate_path, cpath);

        	std::vector<std::string> nfiles;
        	std::vector<std::string> daux;

        	string dir_aux = "";

        	string current = "";
        	string files_input_s = "";
        	string files_max_s ="";

        	max_local = 0.0;

        	std::vector<Dir_path> directories;
        	directories.push_back(a);

        	while(!directories.empty() && (!encontrada)){

            		/* Copiamos la carpeta a una variable temporal y la eliminamos */
            		Dir_path dcurrent(directories.back().path, directories.back().dfiles);
            		directories.pop_back();

            		while(!dcurrent.dfiles.empty()){

                		//Si encuentra los ficheros input
                		if(dcurrent.dfiles.back().find("_input",0)!=string::npos || dcurrent.dfiles.back().find("_crit",0)!=string::npos){

                    			#ifdef DEBUG_TEMP
                        			std::cout << "ruta final es: "<< dcurrent.path << endl;
                    			#endif


                    			//Guardar todos los ficheros en los vectores
                    			while(!dcurrent.dfiles.empty()){

                        			if(dcurrent.dfiles.back().find("_input",0) != string::npos){
                            				files_input_s.clear();
                            				files_input_s.append(dcurrent.path);
                            				files_input_s.append("/");
                            				files_input_s.append(dcurrent.dfiles.back());

                            				#ifdef DEBUG_TEMP
                                				cout << "se introduce fichero: " << files_input_s << endl;
                            				#endif

                            				temp_features[i].files_input.push_back(files_input_s);
                            				dcurrent.dfiles.pop_back();

                        			}else if(dcurrent.dfiles.back().find("_crit",0)!=string::npos){

                            				files_max_s.clear();
                            				files_max_s.append(dcurrent.path);
                            				files_max_s.append("/");
                            				files_max_s.append(dcurrent.dfiles.back());

                            				std::ifstream temp_sample_file(files_max_s);

                            				if(temp_sample_file.good()){

                                				getline(temp_sample_file,line_temp_max);

                                				ss.clear();
                                				ss << line_temp_max;
                                				ss >> max_local;
                                				max_local = max_local / 1000;

                                				if(max_local > temp_features[i].max_temp){

                                    					temp_features[i].max_temp = max_local;

                                    					#ifdef DEBUG_TEMP
                                        					cout << "max_temp: "<< temp_features[i].max_temp << endl;
                                    					#endif

                                				}

                                				temp_sample_file.close();
                
                                				#ifdef DEBUG_TEMP
                                    					cout << "se introduce fichero: " << files_max_s << endl;
                                				#endif

                                				temp_features[i].files_max.push_back(files_max_s);
                                				dcurrent.dfiles.pop_back();
                                
                            				}else{
                                				return EFILE;
                            				}		
                            
                    
                        			}else{
                                
                            				dcurrent.dfiles.pop_back();
                        			}
                    		}
                
                    		#ifdef DEBUG_TEMP
                        		cout << "size de max: "<< temp_features[i].files_max.size() << endl;
                        
                        		for(int j = 0; j < temp_features[i].files_max.size(); j++){
                            			cout << "ficheros de max_temp: " <<  temp_features[i].files_max[j] << endl;
                        		}

                        		for(int j = 0; j < temp_features[i].files_input.size(); j++){
                            			cout << "ficheros de max_temp: "<< temp_features[i].files_input[j] << endl;
                        		}

                    		#endif

                    		encontrada = true;

                	}else if(dcurrent.dfiles.back().compare("driver") == 0){
                    		dcurrent.dfiles.pop_back();
                	}else{

        			dir_aux.clear();
        			dir_aux.append(dcurrent.path);
        			dir_aux.append("/");
        			dir_aux.append(dcurrent.dfiles.back());

        			nfiles = read_directory(dir_aux);

        			/* Si no esta vacio es una carpeta */
        			if(!nfiles.empty()){

        				Dir_path daux(dir_aux,nfiles);
        				dcurrent.dfiles.pop_back();
        				directories.push_back(daux);

        			}else{ /* Es un archivo pero no nos vale */
        				dcurrent.dfiles.pop_back();
        			}
        		}
        	}

                n_core_temps = temp_features.size();
        }
}

	//log_concat_coretemps(temp_features);
        return EOK;

}

int get_temperature(std::vector<Temp_features> & temp_features){

    	double temp_sample = 0.0;

	std::string line_temp_sample, temp_max_line, path_speed, temp_input_read="", saux="", sevaluate="";

	std::vector<std::string> vaux_files;

    	std::stringstream ss,sauxs;

    	double temp_percent = 0.0;

    	int aux = 0, max = 0;
   	ss.precision(4);

    	for(int i = 0; i < temp_features.size(); i++){

        	temp_percent = 0.0;
        	aux = 0;
        	max = 0;

        	for(int j = 0; j < temp_features[i].files_input.size(); j++){

            		std::ifstream temp_sample_file(temp_features[i].files_input[j]);

            		#ifdef DEBUG_TEMP
                		cout << "file es " << temp_features[i].files_input[j] << endl;
            		#endif
            
            		if(temp_sample_file.good()){
                		getline(temp_sample_file,line_temp_sample);
                		ss.clear();
                		ss << line_temp_sample;
                		ss >> aux;
                		aux= aux/1000;

                		#ifdef DEBUG_TEMP
                    			cout << "aux es: " << aux << endl;
                		#endif

                		if(aux > temp_sample){
                    			temp_sample = aux;
                		}
            
                		temp_sample_file.close();
                		temp_input_read="";
            		}else{
                		return EFILE;
            		}

        	}

        	if(temp_features[i].max_temp == 0.0){
        
            		temp_percent = 0.0;
        
        	}else{
        
            		temp_percent = temp_features[i].max_temp ? (temp_sample / temp_features[i].max_temp) * 100 : 0.0;
        
        	}	

        	if(temp_sample > 120){

            		temp_sample = 120;

        	}else if(temp_sample < -20){

            		temp_sample = -20;
        
        	}

        	#ifdef DEBUG_TEMP
            
            		std::cout << "Se concatena el valor: "<< temp_sample << endl;
            		std::cout << "Se concatena el de porcentaje: "<< temp_percent << endl;

        	#endif

        	log_concat(temp_sample,4);
        	log_concat(temp_percent,4);
    	}
    	return EOK;
}

#endif
